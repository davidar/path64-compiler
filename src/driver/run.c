/*
 * Copyright (C) 2007 Pathscale, LLC.  All Rights Reserved.
 */

/*
 * Copyright (C) 2008-2009 Advanced Micro Devices, Inc.  All Rights Reserved.
 */

/*
 *  Copyright (C) 2006, 2007. QLogic Corporation. All Rights Reserved.
 */

/*
 * Copyright 2003, 2004, 2005, 2006 PathScale, Inc.  All Rights Reserved.
 */

/*

  Copyright (C) 2000, 2001 Silicon Graphics, Inc.  All Rights Reserved.

   Path64 is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Path64 is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with Path64; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Special thanks goes to SGI for their continued support to open source

*/

#ifdef __linux
#define _GNU_SOURCE /* For *asprintf */
#endif

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <limits.h>
#if HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <cmplrs/rcodes.h>
#include "run.h"
#include "string_utils.h"
#include "errors.h"
#include "file_names.h"
#include "phases.h"
#include "opt_actions.h"
#include "option_seen.h"
#include "file_utils.h"
#include "driver_defs.h"
#include "option_names.h"
#include "targets.h"

#ifndef WCOREFLAG  
#define WCOREFLAG WCOREFLG //osol compatibility
#endif

boolean show_flag = FALSE;
boolean show_but_not_run = FALSE;
boolean execute_flag = TRUE;
boolean time_flag = FALSE;
boolean prelink_flag = TRUE;
boolean quiet_flag = TRUE;
boolean run_m4 = FALSE;
static boolean ran_twice = FALSE;
static int f90_fe_status = RC_OKAY;
static char *f90_fe_name = NULL; 

static void init_time (void);
static void print_time (char *phase);
static void my_psema(void);
static void my_vsema(void);

extern boolean add_heap_limit;
extern int heap_limit;
extern int hugepage_attr;

#define LOGFILE "/var/log/messages"

static int show_command_only(const char *name, const char *argv[])
{
    int m, n;
	int i;

    if (!show_but_not_run) {
		return 0;
    }
	
	fprintf(stderr, "\"%s\" ", name);

	for (i = 1; argv[i] != NULL; i++)
	    fprintf(stderr, "\"%s\" ", argv[i]);
	fputc('\n', stderr);

	m = strlen(name);
	n = sizeof(FN_EXE("gcc"));

	if (m > n && IS_DIR_SLASH(name[m - n]) && 
		(strcmp(name + m - n + 1, FN_EXE("gcc")) ||
		 strcmp(name + m - n + 1, FN_EXE("g++")))) {
		 return 0;
	}
	return 1;
}


// Executes program with specified name, paramters and input/output.
// Returns zero on success
#ifdef _WIN32
int execute (const char *name,
             const char **argv,
             const char *input,
             const char *output,
             const char **errmsg,
             int *waitstatus)
{
    int errnum;

    if(input != NULL || output != NULL) {
        *errmsg = "execute input/output NYI";
        return -1;
    }

    struct pex_obj *pex = pex_init(0, name, NULL);
    if (pex == NULL) {
        *errmsg = "pex_init failed";
        return -1;
    }

    *errmsg = pex_run(pex, PEX_LAST, name, (char * const *)argv, output, NULL, &errnum);
    if (*errmsg != NULL || !pex_get_status(pex, 1, waitstatus)) {
        if (&errmsg == NULL) {
            *errmsg = "can't get program status";
        }
        pex_free(pex);
        return -1;
    }

    pex_free(pex);
    return 0;
}
#else // !_WIN32
int execute (const char *name,
             const char **argv,
             const char *input,
             const char *output,
             const char **errmsg,
             int *waitstatus) {

    pid_t pid;

    if (input != NULL || output != NULL) {
        *errmsg = "execute with input/output NYI";
        return -1;
    }
   
    pid = fork();
    if (pid == -1) {
        // error
        *errmsg = "fork failed";
        return -1;

    } else if(pid == 0) {
        // child
        execvp(name, (char * const *)argv);

        // should not reach here
        error("execvp failed");
	do_exit(RC_SYSTEM_ERROR);
        return -1;

    } else {
        // parent
        if (wait(waitstatus) != pid) {
            *errmsg = "bad return from wait";
            return -1;
        }

        return 0;
    }
}
#endif // !_WIN32


static void my_putenv(const char *name, const char *fmt, ...)
    __attribute__((format (printf, 2, 3)));
    
static void my_putenv(const char *name, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char *rhs, *env;
    int len;

    if (vasprintf(&rhs, fmt, ap) == -1) {
	internal_error("cannot allocate memory");
	do_exit(RC_SYSTEM_ERROR);
    }
	
    if (show_but_not_run)
	fprintf(stderr, "%s=\"%s\" ", name, rhs);

    // This looks like a memory leak, but it's not interesting, because
    // we're either going to call exec() or do_exit() soon after we're done.

    if (asprintf(&env, "%s=%s", name, rhs) == -1) {
	internal_error("cannot allocate memory");
	do_exit(RC_SYSTEM_ERROR);
    }

    putenv(env);
	
    va_end(ap);
}

char *
get_binutils_lib_path(void)
{
	static const char *binutils_library_path = "../lib/";
	char *my_path;
	
	asprintf(&my_path, "%s/%s", get_executable_dir(),
		 binutils_library_path);
	return my_path;
}


void
run_phase (phases_t phase, char *name, string_list_t *args)
{
    const char **argv;
    int argc;
    string_item_t *p;
    char *output = NULL;
    char *input = NULL;
    boolean save_stderr = FALSE;
    int fdin, fdout;
    int waitstatus;
    int termsig;
    int num_maps;
    char *rld_path;
    struct stat stat_buf;
    const char* errmsg;
    int errnum;
    char *my_path;
    char *l_path;
    char *l32_path;
    char *nls_path;
    char *env_path;
    char *root_prefix;

#if defined(BUILD_OS_DARWIN)
    int suppress_compiler_path = (phase == P_gas);
#endif /* defined(BUILD_OS_DARWIN) */
    const boolean uses_message_system = 
            (phase == P_f90_fe || phase == P_cppf90_fe);

    if ((phase == P_be || phase == P_ipl) && add_heap_limit) {
        char str[200];

        sprintf(str, "-OPT:hugepage_heap_limit=%d -OPT:hugepage_attr=%d", 
			    heap_limit, hugepage_attr);
        add_string(args, str);
    }
    
    if (show_flag) {
        /* echo the command */
        fprintf(stderr, "%s ", name);
        print_string_list(stderr, args);
    }

    if (!execute_flag) return;

    if (time_flag) init_time();

    /* copy arg_list to argv format that exec wants */
    for (argc = 1, p = args->head; p != NULL; p = p->next) {
        //bug# 581, bug #932, bug #1049
        if (p->name == NULL || p->name[0] == '\0') {
            continue;
        }
        argc++;
    }
    argv = (const char **)malloc((argc + 1) * sizeof(char *));
    if (argv == NULL) {
        error("not enough memory");
        cleanup();
        do_exit(RC_SYSTEM_ERROR);
    }

    argv[0] = name;
    for (argc = 1, p = args->head; p != NULL; p = p->next) {
        //bug# 581, bug #932
        if (p->name == NULL || p->name[0] == '\0') {
            continue;
        }
        /* don't put redirection in arg list */
        if (strcmp(p->name, "<") == 0) {
            /* has input file */
            input = p->next->name;
            break;
        } else if (strcmp(p->name, ">") == 0) {
            /* has output file */
            output = p->next->name;
            break;
        } else if (strcmp(p->name, ">&") == 0) {
            /* has error output file */
            output = p->next->name;
            save_stderr = TRUE;
            break;
        }
        argv[argc++] = p->name;
    }
    argv[argc] = NULL;

    if (show_command_only(name, argv)) {
        return;
    }

    my_path = get_binutils_lib_path();
    rld_path = get_phase_ld_library_path(phase);
    
    if (rld_path != NULL) {
        asprintf(&my_path, "%s:%s", my_path, rld_path);
    }
    
    l_path = l32_path = my_path;
    
    if (ld_library_path) {
        asprintf(&l_path, "%s:%s", my_path, ld_library_path);
    }

    /* if we want memory stats, we have to wait for
       parent to connect to our /proc */
    if (input != NULL) {
    	if ((fdin = open (input, O_RDONLY)) == -1) {
            error ("cannot open input file %s", input);
            cleanup ();
            do_exit (RC_SYSTEM_ERROR);
            /* NOTREACHED */
    	}
    	dup2 (fdin, fileno(stdin));
    }
    if (output != NULL) {
    	if ((fdout = creat (output, 0666)) == -1) {
            error ("cannot create output file %s", output);
            cleanup ();
            do_exit (RC_SYSTEM_ERROR);
            /* NOTREACHED */
    	}
    	if (save_stderr) {
            dup2 (fdout, fileno(stderr));
    	} else {
            dup2 (fdout, fileno(stdout));
    	}
    } 
    
    my_path = get_binutils_lib_path();
    rld_path = get_phase_ld_library_path (phase);
    
    if (rld_path != 0) {
    	asprintf(&my_path, "%s:%s:%s/%s", my_path, rld_path, rld_path, current_target->abi_name);
    }
    
    l_path = l32_path = my_path;
    
    if (ld_library_path)
    	asprintf(&l_path, "%s:%s", my_path, ld_library_path);
    
    if (ld_libraryn32_path)
    	asprintf(&l32_path, "%s:%s", my_path,
    		 ld_libraryn32_path);

#if defined(BUILD_OS_DARWIN)
    /* Darwin static linker uses LD_LIBRARY_PATH, but dynamic
     * linker uses DYLD_LIBRARY_PATH */
    my_putenv("DYLD_LIBRARY_PATH", "%s", l_path);
#else /* defined(BUILD_OS_DARWIN) */
    my_putenv("LD_LIBRARY_PATH", "%s", l_path);
    my_putenv("LD_LIBRARYN32_PATH", "%s", l32_path);
#endif /* defined(BUILD_OS_DARWIN) */
		
    // Set up NLSPATH, for the Fortran front end.
    
    nls_path = getenv("NLSPATH");
    root_prefix = directory_path(get_executable_dir());
    
    if (nls_path) {
        my_putenv("NLSPATH", "%s:%s%s/%%N.cat", nls_path, root_prefix, LIBPATH);
    } else {
        my_putenv("NLSPATH", "%s%s/%%N.cat", root_prefix, LIBPATH);
    }
    
    if (uses_message_system && getenv("ORIG_CMD_NAME") == NULL)
        my_putenv("ORIG_CMD_NAME", "%s", program_name);
    
    if (phase == P_f90_fe) {
        char *root;
        char *modulepath;
        int len;
        char *new_env;
        char *env_name = "FORTRAN_SYSTEM_MODULES=";
        char *env_val = "/usr/lib/f90modules";
        root = getenv("TOOLROOT");
        if (root != NULL) {
            len = strlen(env_val) + strlen(root) +3 + strlen(env_val);
            new_env = alloca(len);
            sprintf(new_env,"%s/%s:%s",root,env_val,env_val);
            env_val = new_env;
        }
        modulepath = string_copy(getenv("FORTRAN_SYSTEM_MODULES"));
        if (modulepath != NULL) {
            /* Append env_val to FORTRAN_SYSTEM_MODULES */
            if (modulepath[strlen(modulepath)-1] == ':') {
                /* Just append env_val */
                len = strlen(modulepath) + strlen(env_val) + 1;
                new_env = alloca(len);
                sprintf(new_env,"%s%s",modulepath,env_val);
            } else {
                /* append :env_val */
                len = strlen(modulepath) + strlen(env_val) + 2;
                new_env = alloca(len);
                sprintf(new_env,"%s:%s",modulepath,env_val);
            }
            env_val = new_env;
        }
        
        my_putenv ("FORTRAN_SYSTEM_MODULES", "%s", env_val);
    }

    /* need to setenv COMPILER_PATH for collect to find ld */
    if (
#ifdef KEY
        // gcc will invoke the cc1 in the COMPILER_PATH directory,
        // which is not what we want when we invoke gcc for
        // preprocessing.  Bug 10164.

#if defined(BUILD_OS_DARWIN)
        /* Driver uses gcc to run assembler. If we set COMPILER_PATH,
         * gcc uses PSC version of cc1, which doesn't accept one of
         * the options (-mmacosx-version-min=10.5.1) which Apple's
         * gcc driver passes to Apple's cc1. Looks like the
         * "is_matching_phase" test might already be trying to fix
         * this, but it's not succeeding.
         */
        !suppress_compiler_path &&
#endif
        !is_matching_phase(get_phase_mask(P_any_cpp), phase) &&
#endif
        1) {
        my_putenv("COMPILER_PATH", "%s", get_phase_dir(P_collect));
    }

    /* Tell IPA where to find the driver. */
    my_putenv("COMPILER_BIN", "%s/" PSC_NAME_PREFIX "cc-"
              PSC_FULL_VERSION, get_executable_dir());

    if (execute(name, argv, input, output, &errmsg, &waitstatus) == -1) {
        error("execute failed: %s", errmsg);
        cleanup();
        do_exit(RC_SYSTEM_ERROR);
    }

    if (time_flag) print_time(name);
    
    if (WIFSTOPPED(waitstatus)) {
        error("STOPPED signal received from %s", name);
        cleanup();
        do_exit(RC_SYSTEM_ERROR);
        /* NOTREACHED */
    } else if (WIFEXITED(waitstatus)) {
        int status = WEXITSTATUS(waitstatus);
        extern int inline_t;
        boolean internal_err = FALSE;
        boolean user_err = FALSE;
        
        if (phase == P_prof) {
            /* Make sure the .cfb files were created before
               changing the STATUS to OKAY */
            if (prof_file != NULL) {
                if (!(stat(fb_file, &stat_buf) != 0 && errno == ENOENT)) {
                    status = RC_OKAY;
                }
            } else {
                internal_error("No count file was specified for a prof run");
                perror(program_name);
            }
        }
    
        if (phase == P_f90_fe && keep_listing) {
            char *cif_file = construct_given_name(drop_path(source_file), 
                                                  "T", TRUE);
    
            if (!(stat(cif_file, &stat_buf) != 0 && errno == ENOENT)) {
                f90_fe_status = status;
            }
            
            f90_fe_name = string_copy(name);
    
            /* Change the status to OKAY so that we can 
             * execute the lister on the cif_file; we will
             * take appropriate action on this status once 
             * the lister has finished executing. See below.
             */
            status = RC_OKAY;
        }
    
        if (phase == P_lister) {
            if (status == RC_OKAY && f90_fe_status != RC_OKAY) {
    
                /* We had encountered an error in the F90_fe phase
                 * but we ignored it so that we could execute the
                 * lister on the cif file; we need to switch the
                 * status to the status we received from F90_fe
                 * and use the name of the F90_fe_phase, so that
                 * we can issue a correct error message.
                 */
    
                status = f90_fe_status;
                name = string_copy(f90_fe_name);
    
                /* Reset f90_fe_status to OKAY for any further
                 * compilations on other source files.
                 */
    
                f90_fe_status = RC_OKAY;
            }
        }
    
        switch (status) {
        case RC_OKAY:
#ifdef KEY
            // If the command line has explicit inline
            // setting, follow it; else follow the
            // front-end request.  Bug 11325.
            if (inline_t != UNDEFINED) {
                run_inline = inline_t;
                break;
            }

#ifdef PATH64_ENABLE_GNU_FRONTEND
            // bug 10215
            if (is_matching_phase(get_phase_mask(phase), P_wgen)) {
              run_inline = FALSE;
            }
#endif // PATH64_ENABLE_GNU_FRONTEND
            // bug 10215
            if (is_matching_phase(get_phase_mask(phase), P_psclang)) {
              run_inline = FALSE;
            }
            break;
#endif
            if (inline_t == UNDEFINED  && 
                is_matching_phase(get_phase_mask(phase), P_any_fe)) {
#ifdef KEY
                run_inline = FALSE; // bug 11325
#else
                inline_t = FALSE;
#endif
            }
            break;
        
        case RC_NEED_INLINER:
#ifdef KEY      // If the command line has explicit inline
            // setting, follow it; else follow the
            // front-end request.  Bug 11325.
            if (inline_t != UNDEFINED) {
                run_inline = inline_t;
                break;
            }

#ifdef PATH64_ENABLE_GNU_FRONTEND
            // bug 10215
            if (is_matching_phase(get_phase_mask(phase), P_wgen)) {
                run_inline = TRUE;
            }
#endif // PATH64_ENABLE_GNU_FRONTEND
            // bug 10215
            if (is_matching_phase(get_phase_mask(phase), P_psclang)) {
              run_inline = TRUE;
            }
            break;

#endif
            if (inline_t == UNDEFINED && 
                is_matching_phase(get_phase_mask(phase), P_any_fe)) {
#ifdef KEY
                run_inline = TRUE;  // bug 11325
#else
                inline_t = TRUE;
#endif
            }
            /* completed successfully */
            break;
            
        case RC_USER_ERROR:
        case RC_NORECOVER_USER_ERROR:
        case RC_SYSTEM_ERROR:
        case RC_GCC_ERROR:
#ifdef KEY
        case RC_RTL_MISSING_ERROR: /* bug 14054 */
#endif
            user_err = TRUE;
            break;

        case RC_OVERFLOW_ERROR:
            if (!ran_twice && phase == P_be) {
                /* try recompiling with larger limits */
                ran_twice = TRUE;
                add_string(args, "-TENV:long_eh_offsets");
                add_string(args, "-TENV:large_stack");
                run_phase(phase, name, args);
                return;
            }
            internal_err = TRUE;
            break;
#ifdef KEY
        case RC_GCC_INTERNAL_ERROR:
#endif
        case RC_INTERNAL_ERROR:
            internal_err = TRUE;
            break;

        default:
            internal_err = TRUE;
            break;
        } 

        if (internal_err) {
            if (phase == P_ld ||
                phase == P_ldplus ||
#ifdef KEY
                phase == P_gas ||           // bug 4846
                phase == P_f_coco ||        // bug 9058
                phase == P_psclang_cpp ||
                phase == P_psclang ||
                status == RC_GCC_INTERNAL_ERROR ||  //bug 9637
#endif // KEY
#ifdef PATH64_ENABLE_GNU_FRONTEND
                phase == P_spin_cc1 ||
                phase == P_spin_cc1plus ||
                phase == P_gcpp ||
                phase == P_gcpp_plus ||
#endif // PATH64_ENABLE_GNU_FRONTEND
                TRUE) {

                if (phase == P_gas ||
                    status == RC_GCC_INTERNAL_ERROR) {
                	internal_error_occurred = 1;
                }
                log_error("%s returned non-zero status %d", name, status);
                nomsg_error(status);
            } else {
                internal_error("%s returned non-zero status %d", name, status);
            }
        }
        else if (user_err) {
            /* assume phase will print diagnostics */
            if (phase == P_c_gfe || phase == P_cplus_gfe
#ifdef PATH64_ENABLE_GNU_FRONTEND
#ifdef KEY
                || phase == P_wgen
                || phase == P_spin_cc1
                || phase == P_spin_cc1plus
#endif // KEY
#endif // PATH64_ENABLE_GNU_FRONTEND
               ) {
                nomsg_error(RC_INTERNAL_ERROR);
            }
            else if (!show_flag || save_stderr) {
                nomsg_error(RC_USER_ERROR);
            } else {
                error("%s returned non-zero status %d", name, status);
            }
        }
        ran_twice = FALSE;
        return;

    } else if(WIFSIGNALED(waitstatus)) {
        termsig = WTERMSIG(waitstatus);
        switch (termsig) {
#ifdef SIGHUP
        case SIGHUP:
#endif
        case SIGINT:
#ifdef SIGQUIT
        case SIGQUIT:
#endif
#ifdef SIGKILL
        case SIGKILL:
#endif
        case SIGTERM:
            error("%s died due to signal %d", name, termsig);
            break;
        default:
            internal_error("%s died due to signal %d", name, termsig);
            break;
        }

        if(waitstatus & WCOREFLAG) {
            error("core dumped");
        }

#ifdef SIGKILL
        if (termsig == SIGKILL) {
            error("Probably caused by running out of swap space -- check %s", LOGFILE);
        }
#endif

        cleanup();
        do_exit(RC_SYSTEM_ERROR);
    } else {
        /* cannot happen, I think! */
        internal_error("driver exec'ing is confused");
        return;
    }
}

/*
 * Handler () is used for catching signals.
 */
void
handler (int sig)
{
	error("signal %s caught, stop processing", strsignal(sig));
	cleanup ();
	do_exit (RC_SYSTEM_ERROR);
}

/* set signal handler */
void
catch_signals (void)
{
    /* modelled after Handle_Signals in common/util/errors.c */
#ifdef SIGHUP
    if (signal (SIGHUP, SIG_IGN) != SIG_IGN)
        signal (SIGHUP,  handler);
#endif
    if (signal (SIGINT, SIG_IGN) != SIG_IGN)
        signal (SIGINT,  handler);
#ifdef SIGQUIT
    if (signal (SIGQUIT, SIG_IGN) != SIG_IGN)
        signal (SIGQUIT,  handler);
#endif
    if (signal (SIGILL, SIG_IGN) != SIG_IGN)
        signal (SIGILL,  handler);
#ifdef SIGTRAP
    if (signal (SIGTRAP, SIG_IGN) != SIG_IGN)
        signal (SIGTRAP,  handler);
#endif
#ifdef SIGIOT
    if (signal (SIGIOT, SIG_IGN) != SIG_IGN)
        signal (SIGIOT,  handler);
#endif
    if (signal (SIGFPE, SIG_IGN) != SIG_IGN)
        signal (SIGFPE,  handler);
#ifdef SIGBUS
    if (signal (SIGBUS, SIG_IGN) != SIG_IGN)
        signal (SIGBUS,  handler);
#endif
    if (signal (SIGSEGV, SIG_IGN) != SIG_IGN)
        signal (SIGSEGV,  handler);
#ifdef SIGTERM
    if (signal (SIGTERM, SIG_IGN) != SIG_IGN)
        signal (SIGTERM,  handler);
#endif
#ifdef SIGPIPE
    if (signal (SIGPIPE, SIG_IGN) != SIG_IGN)
        signal (SIGPIPE,  handler);
#endif
}

static struct rusage time_start;
static struct timeval time_start_wall;

static void
init_time (void)
{
	getrusage(RUSAGE_SELF, &time_start);
	gettimeofday(&time_start_wall, NULL);
}


static void
print_time (char *phase)
{
	struct timeval diff, time_current_wall;
	struct rusage time_current;
	long long utime, stime, wtime;
	double perc;

	getrusage(RUSAGE_SELF, &time_current);
	gettimeofday(&time_current_wall, NULL);

	timersub(&time_current.ru_utime, &time_start.ru_utime, &diff);
	utime = diff.tv_sec * 100 + diff.tv_usec / 10000;
	timersub(&time_current.ru_stime, &time_start.ru_stime, &diff);
	stime = diff.tv_sec * 100 + diff.tv_usec / 10000;
	timersub(&time_current_wall, &time_start_wall, &diff);
	wtime = diff.tv_sec * 100 + diff.tv_usec / 10000;

	if (wtime == 0)
		perc = 100;
	else
		perc = (utime + stime) * 100.0 / wtime;

	fprintf(stderr, "%s phase time:  %lld.%02lldu "
	    "%lld.%02llds %lld:%02lld.%01lld %.0f%%\n",
	    phase, utime / 100, utime % 100, stime / 100, stime % 100,
	    wtime / 6000, wtime / 100 % 60, wtime % 100, perc);
}
