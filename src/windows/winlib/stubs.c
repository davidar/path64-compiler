/*
 * Copyright (C) 2010. PathScale Inc. All Rights Reserved.
 */

/*

   Path64 is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation version 3

   Path64 is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   You should have received a copy of the GNU General Public License
   along with Path64; see the file COPYING.  If not, write to the Free
   Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

*/

#include <sys/wait.h>
#include <sys/utsname.h>
#include <stdio.h>
#include <stdlib.h>
#include <cmplrs/rcodes.h>
#include <errno.h>
#include <sys/resource.h>
#include <windows.h>

int getrusage(int who, struct rusage *usage)
{
    FILETIME CreateTime, ExitTime;
    FILETIME KernelTime, UserTime;
    ULARGE_INTEGER Value;
    
    if (who != RUSAGE_SELF) {
        fprintf(stderr, "The param=%d is unimplemented\n", who);
        exit(RC_UNIMPLEMENTED_ERROR);
	return -1;
    }

    if (GetProcessTimes(GetCurrentProcess(), &CreateTime, &ExitTime,
    	                                     &KernelTime, &UserTime)) {
    	errno = EINVAL;
    	return -1;
    }

    Value.HighPart = KernelTime.dwHighDateTime;
    Value.LowPart  = KernelTime.dwLowDateTime;

#define CAST_TO(x, v)  (x) = (typeof(x))(v)

    CAST_TO(usage->ru_stime.tv_sec,   Value.QuadPart / 10000000ULL);
    CAST_TO(usage->ru_stime.tv_usec, (Value.QuadPart % 10000000ULL) / 10ULL);

    Value.HighPart = UserTime.dwHighDateTime;
    Value.LowPart  = UserTime.dwLowDateTime;

    CAST_TO(usage->ru_utime.tv_sec,   Value.QuadPart / 10000000ULL);
    CAST_TO(usage->ru_utime.tv_usec, (Value.QuadPart % 10000000ULL) / 10ULL);

    return 0;
}

char *sbrk(int increment)
{
    fprintf(stderr, "sbrk() is unimplemented\n");
    return 0;
}

int kill(pid_t pid, int sig)
{
    fprintf(stderr, "kill() is unimplemented\n");
    exit(RC_UNIMPLEMENTED_ERROR);
    return 0;
}

int fork()
{
    fprintf(stderr, "fork() is unimplemented\n");
    exit(RC_UNIMPLEMENTED_ERROR);
    return -1;
}

int getdomainname(char *name, size_t len)
{
    fprintf(stderr, "getdomainname() is unimplemented\n");
    snprintf(name, len, "fakedomain");
    return 0;
}

#if 0
int* Pre_Optimizer (int a1, int* a2, int* a3, int* a4)
{
    fprintf(stderr, "Pre_Optimizer() is unimplemented\n");
    return 0;
}

char *cplus_demangle(const char * a1, int a2)
{
    fprintf(stderr, "cplus_demangle() is unimplemented\n");
    return 0;
}
#endif

