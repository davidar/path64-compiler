project(GCCCONFIG C)

include(${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
include(${CMAKE_ROOT}/Modules/CheckIncludeFile.cmake)
include(${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
include(${CMAKE_ROOT}/Modules/CheckVariableExists.cmake)
include(${CMAKE_ROOT}/Modules/CheckTypeSize.cmake)

# Start with tests needed by PERFORM_C_TEST and PERFORM_C_TEST_RUN.
check_function_exists(getpagesize HAVE_GETPAGESIZE)
check_include_file(inttypes.h HAVE_INTTYPES_H)
check_include_file(memory.h HAVE_MEMORY_H)
check_include_file(stdint.h HAVE_STDINT_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/types.h HAVE_SYS_TYPES_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(unistd.h HAVE_UNISTD_H)
foreach(have
	HAVE_GETPAGESIZE
	HAVE_INTTYPES_H
	HAVE_MEMORY_H
	HAVE_STDINT_H
	HAVE_STDLIB_H
	HAVE_STRINGS_H
	HAVE_STRING_H
	HAVE_SYS_STAT_H
	HAVE_SYS_TYPES_H
	HAVE_SYS_PARAM_H
	HAVE_UNISTD_H)
	if(NOT ${have})
		set(${have} 0)
	endif()
endforeach()

macro(PERFORM_C_TEST var description invert)
	if(${var}_COMPILED MATCHES "^${var}_COMPILED$")
		message(STATUS "${description}")
		try_compile(${var}_COMPILED
			${GCCCONFIG_BINARY_DIR}
			${GCCCONFIG_SOURCE_DIR}/gcc_c_tests.c
			OUTPUT_VARIABLE
			OUTPUT
			COMPILE_DEFINITIONS
			-DTEST_${var}
			-DHAVE_GETPAGESIZE=${HAVE_GETPAGESIZE}
			-DHAVE_INTTYPES_H=${HAVE_INTTYPES_H}
			-DHAVE_MEMORY_H=${HAVE_MEMORY_H}
			-DHAVE_STDINT_H=${HAVE_STDINT_H}
			-DHAVE_STDLIB_H=${HAVE_STDLIB_H}
			-DHAVE_STRINGS_H=${HAVE_STRINGS_H}
			-DHAVE_STRING_H=${HAVE_STRING_H}
			-DHAVE_SYS_STAT_H=${HAVE_SYS_STAT_H}
			-DHAVE_SYS_TYPES_H=${HAVE_SYS_TYPES_H}
			-DHAVE_SYS_PARAM_H=${HAVE_SYS_PARAM_H}
			-DHAVE_UNISTD_H=${HAVE_UNISTD_H}
			-DSTDC_HEADERS=${STDC_HEADERS})
		if(${var}_COMPILED)
			file(APPEND
				${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
				"${description} compiled with the following output:\n${OUTPUT}\n\n")
		else()
			file(APPEND
				${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
				"${description} failed to compile with the following output:\n${OUTPUT}\n\n")
		endif()
		if(${invert} MATCHES INVERT)
			if(${var}_COMPILED)
				message(STATUS "${description} - no")
			else()
				message(STATUS "${description} - yes")
			endif()
		else()
			if(${var}_COMPILED)
				message(STATUS "${description} - yes")
			else()
				message(STATUS "${description} - no")
			endif()
		endif()
	endif()
	if(${invert} MATCHES INVERT)
		if(${var}_COMPILED)
			set(${var} 0)
		else()
			set(${var} 1)
		endif()
	else()
		if(${var}_COMPILED)
			set(${var} 1)
		else()
			set(${var} 0)
		endif()
	endif()
endmacro()

macro(PERFORM_C_TEST_RUN var description zero nonzero invert)
	if(${var}_RESULT MATCHES "^${var}_RESULT$")
		message(STATUS "${description}")
		try_run(${var}_RESULT
			${var}_COMPILED
			${GCCCONFIG_BINARY_DIR}
			${GCCCONFIG_SOURCE_DIR}/gcc_c_tests.c
			OUTPUT_VARIABLE
			OUTPUT
			COMPILE_DEFINITIONS
			-DTEST_${var}
			-DHAVE_GETPAGESIZE=${HAVE_GETPAGESIZE}
			-DHAVE_INTTYPES_H=${HAVE_INTTYPES_H}
			-DHAVE_MEMORY_H=${HAVE_MEMORY_H}
			-DHAVE_STDINT_H=${HAVE_STDINT_H}
			-DHAVE_STDLIB_H=${HAVE_STDLIB_H}
			-DHAVE_STRINGS_H=${HAVE_STRINGS_H}
			-DHAVE_STRING_H=${HAVE_STRING_H}
			-DHAVE_SYS_STAT_H=${HAVE_SYS_STAT_H}
			-DHAVE_SYS_TYPES_H=${HAVE_SYS_TYPES_H}
			-DHAVE_SYS_PARAM_H=${HAVE_SYS_PARAM_H}
			-DHAVE_UNISTD_H=${HAVE_UNISTD_H}
			-DSTDC_HEADERS=${STDC_HEADERS})
		if(${var}_COMPILED)
			file(APPEND
				${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
				"${description} compiled with the following output:\n${OUTPUT}\n\n")
			if(${invert} MATCHES INVERT)
				if(${var}_RESULT)
					message(STATUS "${description} - ${zero}")
				else()
					message(STATUS "${description} - ${nonzero}")
				endif()
			else()
				if(${var}_RESULT)
					message(STATUS "${description} - ${nonzero}")
				else()
					message(STATUS "${description} - ${zero}")
				endif()
			endif()
		else()
			file(APPEND
				${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
				"${description} failed to compile with the following output:\n${OUTPUT}\n\n")
			message(STATUS "${description} - test compilation failed")
		endif()
	endif()
	if(${var}_COMPILED)
		if(${invert} MATCHES INVERT)
			if(${var}_RESULT)
				set(${var} 1)
			else()
				set(${var} 0)
			endif()
		else()
			if(${var}_RESULT)
				set(${var} 0)
			else()
				set(${var} 1)
			endif()
		endif()
	endif()
endmacro()

test_big_endian(HOST_WORDS_BIG_ENDIAN)
if(HOST_WORDS_BIG_ENDIAN)
	set(WORDS_BIGENDIAN 1)
endif()
if(NOT WORDS_BIGENDIAN)
	set(HOST_WORDS_BIG_ENDIAN "")
endif()
perform_c_test(gcc_ac_cv_c_const
	"checking for an ANSI C-conforming const"
	DIRECT)
perform_c_test(HAVE_SYS_WAIT_H
	"checking for sys/wait.h that is POSIX.1 compatible"
	DIRECT)
perform_c_test(TIME_WITH_SYS_TIME
	"checking whether time.h and sys/time.h may both be included"
	DIRECT)
perform_c_test(TM_IN_TIME_H
	"checking whether struct tm is in time.h"
	DIRECT)
perform_c_test(NEED_DECLARATION_ERRNO
	"checking whether errno must be declared"
	INVERT)
perform_c_test(STDC_HEADERS "checking for ANSI C header files" DIRECT)
perform_c_test(HAVE_SYS_RESOURCE_H_WITH_SYS_TIME_H
	"checking for sys/resource.h with sys/time.h"
	DIRECT)
perform_c_test(HAVE_DIRENT_H
	"checking for dirent.h with sys/types.h"
	DIRECT)
perform_c_test(HAVE_LANGINFO_CODESET
	"checking for nl_langinfo and CODESET"
	DIRECT)

check_include_file(alloca.h HAVE_ALLOCA_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(machine/hal_sysinfo.h HAVE_MACHINE/HAL_SYSINFO_H)
check_include_file(malloc.h HAVE_MALLOC_H)
check_include_file(sys/file.h HAVE_SYS_FILE_H)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/pstat.h HAVE_SYS_PSTAT_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/sysctl.h HAVE_SYS_SYSCTL_H)
check_include_file(sys/sysinfo.h HAVE_SYS_SYSINFO_H)
check_include_file(sys/sysmp.h HAVE_SYS_SYSMP_H)
check_include_file(sys/systemcfg.h HAVE_SYS_SYSTEMCFG_H)
check_include_file(sys/table.h HAVE_SYS_TABLE_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(stdio_ext.h HAVE_STDIO_EXT_H)
check_include_file(time.h HAVE_TIME_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(ctype.h HAVE_CTYPE_H)
check_include_file(stdio.h HAVE_STDIO_H)

check_include_file(vfork.h HAVE_VFORK_H)

check_function_exists(_doprnt HAVE__DOPRNT)
check_function_exists(__fsetlocking HAVE___FSETLOCKING)
check_function_exists(asprintf HAVE_ASPRINTF)
check_function_exists(atexit HAVE_ATEXIT)
check_function_exists(basename HAVE_BASENAME)
check_function_exists(bsearch HAVE_BSEARCH)
check_function_exists(calloc HAVE_CALLOC)
check_function_exists(clock HAVE_CLOCK)
check_function_exists(ffs HAVE_FFS)
check_function_exists(getcwd HAVE_GETCWD)
check_function_exists(index HAVE_INDEX)
check_function_exists(insque HAVE_INSQUE)
check_function_exists(memchr HAVE_MEMCHR)
check_function_exists(memmove HAVE_MEMMOVE)
check_function_exists(mempcpy HAVE_MEMPCPY)
check_function_exists(mkstemps HAVE_MKSTEMPS)
check_function_exists(putenv HAVE_PUTENV)
check_function_exists(random HAVE_RANDOM)
check_function_exists(rename HAVE_RENAME)
check_function_exists(rindex HAVE_RINDEX)
check_function_exists(setenv HAVE_SETENV)
check_function_exists(sigsetmask HAVE_SIGSETMASK)
check_function_exists(snprintf HAVE_SNPRINTF)
check_function_exists(stpcpy HAVE_STPCPY)
check_function_exists(stpncpy HAVE_STPNCPY)
check_function_exists(strcasecmp HAVE_STRCASECMP)
check_function_exists(strchr HAVE_STRCHR)
check_function_exists(strdup HAVE_STRDUP)
check_function_exists(strncmp HAVE_STRNCMP)
check_function_exists(strncasecmp HAVE_STRNCASECMP)
check_function_exists(strndup HAVE_STRNDUP)
check_function_exists(strrchr HAVE_STRRCHR)
check_function_exists(strstr HAVE_STRSTR)
check_function_exists(strtod HAVE_STRTOD)
check_function_exists(strtol HAVE_STRTOL)
check_function_exists(strtoul HAVE_STRTOUL)
check_function_exists(strverscmp HAVE_STRVERSCMP)
check_function_exists(tmpnam HAVE_TMPNAM)
check_function_exists(vasprintf HAVE_VASPRINTF)
check_function_exists(vfprintf HAVE_VFPRINTF)
check_function_exists(vprintf HAVE_VPRINTF)
check_function_exists(vsnprintf HAVE_VSNPRINTF)
check_function_exists(vsprintf HAVE_VSPRINTF)
check_function_exists(waitpid HAVE_WAITPID)
check_function_exists(wait3 HAVE_WAIT3)
check_function_exists(wait3 HAVE_WAIT4)
check_function_exists(fork HAVE_FORK)
check_function_exists(vfork HAVE_VFORK)

#-----------------------------------------------------------------------------
# Fake that these functions always exist until CMake bug #1775 is fixed.
#
# Workaround for CMake's existence testing problem with intrinsic functions
# on Windows with cl and /O2. If we don't do this, we get "intrinsic function,
# cannot be defined" errors when building optimized builds with cl on Windows.
# Change back to CHECK_FUNCTION_EXISTS calls when bug is fixed.
#
# CHECK_FUNCTION_EXISTS(memcmp HAVE_MEMCMP)
# CHECK_FUNCTION_EXISTS(memcpy HAVE_MEMCPY)
# CHECK_FUNCTION_EXISTS(memset HAVE_MEMSET)
set(HAVE_MEMCMP
	1
	CACHE
	INTERNAL
	"Always have memcmp when building CableSwig...")
set(HAVE_MEMCPY
	1
	CACHE
	INTERNAL
	"Always have memcpy when building CableSwig...")
set(HAVE_MEMSET
	1
	CACHE
	INTERNAL
	"Always have memset when building CableSwig...")

check_function_exists(canonicalize_file_name
	HAVE_CANONICALIZE_FILE_NAME)
check_function_exists(getrusage HAVE_GETRUSAGE)
check_function_exists(getsysinfo HAVE_GETSYSINFO)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(on_exit HAVE_ON_EXIT)
check_function_exists(psignal HAVE_PSIGNAL)
check_function_exists(pstat_getdynamic HAVE_PSTAT_GETDYNAMIC)
check_function_exists(pstat_getstatic HAVE_PSTAT_GETSTATIC)
check_function_exists(realpath HAVE_REALPATH)
check_function_exists(sbrk HAVE_SBRK)
check_function_exists(strerror HAVE_STRERROR)
check_function_exists(strsignal HAVE_STRSIGNAL)
check_function_exists(sysconf HAVE_SYSCONF)
check_function_exists(sysctl HAVE_SYSCTL)
check_function_exists(sysmp HAVE_SYSMP)
check_function_exists(table HAVE_TABLE)
check_function_exists(times HAVE_TIMES)

perform_c_test(HAVE_SYS_ERRLIST
	"checking for variable sys_errlist"
	DIRECT)
perform_c_test(HAVE_SYS_ERRLIST_IN_STDLIB_H
	"checking for sys_errlist in stdlib.h"
	DIRECT)
if(NOT HAVE_SYS_ERRLIST)
	if(HAVE_SYS_ERRLIST_IN_STDLIB_H)
		set(HAVE_SYS_ERRLIST 1 CACHE INTERNAL "Have sys_errlist in stdlib.h.")
	endif()
endif()

perform_c_test(HAVE_SYS_NERR "checking for variable sys_nerr" DIRECT)
perform_c_test(HAVE_SYS_SIGLIST
	"checking for variable sys_siglist"
	DIRECT)
perform_c_test(HAVE__DOPRNT "checking for variable _doprnt" DIRECT)
perform_c_test(HAVE__SYSTEM_CONFIGURATION
	"checking for variable _system_configuration"
	DIRECT)

perform_c_test(NEED_DECLARATION_CANONICALIZE_FILE_NAME
	"checking whether canonicalize_file_name must be declared"
	INVERT)

perform_c_test(HAVE_UINTPTR_T "checking for uintptr_t" DIRECT)
perform_c_test(HAVE_PID_T "checking for pid_t" DIRECT)

if(NOT HAVE_PID_T)
	set(pid_t int)
endif()

set(GCC_UNIX_TESTS 1)
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	if(NOT CYGWIN)
		set(GCC_UNIX_TESTS 0)
	endif()
endif()

if(GCC_UNIX_TESTS)
	perform_c_test_run(gcc_ac_cv_func_fork
		"checking for fork"
		yes
		no
		DIRECT)
	perform_c_test_run(gcc_ac_cv_func_vfork
		"checking for vfork"
		yes
		no
		DIRECT)
	perform_c_test_run(HAVE_MMAP "checking for working mmap" yes no DIRECT)
else()
	set(gcc_ac_cv_func_fork 0)
	set(gcc_ac_cv_func_vfork 0)
	set(HAVE_MMAP 0)
endif()

perform_c_test(CRAY_STACKSEG_END
	"checking whether alloca needs Cray hooks"
	DIRECT)
perform_c_test_run(STACK_DIRECTION
	"checking stack direction for C alloca"
	-1
	+1
	DIRECT)
if(STACK_DIRECTION_RESULT)
	set(STACK_DIRECTION -1)
else()
	set(STACK_DIRECTION 1)
endif()

# Some platforms (at least Mac) require sys/time.h to be included
# before sys/resource.h.  If including these together works, but
# sys/resource.h fails on its own, pretend we have it.
if(NOT HAVE_SYS_RESOURCE_H)
	if(HAVE_SYS_RESOURCE_H_WITH_SYS_TIME_H)
		set(HAVE_SYS_RESOURCE_H 1)
	endif()
endif()

if(HAVE_GAS_WEAK MATCHES "^HAVE_GAS_WEAK$")
	message(STATUS "checking assembler for .weak")
	try_compile(HAVE_GAS_WEAK
		${GCCCONFIG_BINARY_DIR}/gcc_asm_tests
		${GCCCONFIG_SOURCE_DIR}/gcc_asm_tests
		GAT
		test_weak)
	if(HAVE_GAS_WEAK)
		message(STATUS "checking assembler for .weak - yes")
	else()
		message(STATUS "checking assembler for .weak - no")
	endif()
endif()

if(HAVE_GAS_WEAKREF MATCHES "^HAVE_GAS_WEAKREF$")
	message(STATUS "checking assembler for .weakref")
	try_compile(HAVE_GAS_WEAKREF
		${GCCCONFIG_BINARY_DIR}/gcc_asm_tests
		${GCCCONFIG_SOURCE_DIR}/gcc_asm_tests
		GAT
		test_weakref)
	if(HAVE_GAS_WEAKREF)
		message(STATUS "checking assembler for .weakref - yes")
	else()
		message(STATUS "checking assembler for .weakref - no")
	endif()
endif()

if(NOT gcc_ac_cv_func_vfork)
	set(vfork fork)
endif()

set(inline)
if(gcc_ac_cv_c_const)
	set(const)
else()
	set(const 1)
endif()

if(WORDS_BIGENDIAN)
	set(BYTEORDER 4321)
else()
	set(BYTEORDER 1234)
endif()

perform_c_test(HAVE_UINT64_T "checking for type uint64_t" DIRECT)

# Look for a 64-bit integer type.
if(HAVE_UINT64_T)
	set(UNSIGNED_64BIT_TYPE "uint64_t")
else()
	if(SIZEOF_LONG MATCHES "^8$")
		set(UNSIGNED_64BIT_TYPE "unsigned long")
	else()
		if(SIZEOF_LONG_LONG MATCHES "^8$")
			set(UNSIGNED_64BIT_TYPE "unsigned long long")
		endif()
	endif()
endif()

# Tests not done:
# checking for inline... (cached) inline
# checking for working fork... yes
# checking for working vfork... (cached) yes
# checking for working strncmp... yes
# checking for library containing strerror... none required
# checking for alloca

set(HAVE_WORKING_FORK ${HAVE_FORK})
set(HAVE_WORKING_VFORK ${HAVE_VFORK})
set(HAVE_ALLOCA ${HAVE_ALLOCA_H})

if(TM_IN_TIME_H)
	set(TM_IN_SYS_TIME 0)
else()
	set(TM_IN_SYS_TIME ${HAVE_SYS_TIME_H})
endif()

# gcc tests

if(GCC_UNIX_TESTS)
	perform_c_test_run(GETGROUPS_T
		"checking type of array argument to getgroups"
		"gid_t"
		"int"
		DIRECT)
else()
	set(GETGROUPS_T 0)
endif()
perform_c_test(STRING_WITH_STRINGS
	"checking whether string.h and strings.h may both be included"
	DIRECT)
perform_c_test(_GNU_SOURCE "checking whether C library is GNU" DIRECT)
perform_c_test(HAVE_LC_MESSAGES "checking for LC_MESSAGES" DIRECT)
perform_c_test(HAVE_INITFINI_ARRAY
	"checking for .preinit_array/.init_array/.fini_array support"
	DIRECT)
perform_c_test_run(HAVE_WORKING_MBSTOWCS
	"checking whether mbstowcs works"
	"yes"
	"no"
	DIRECT)
perform_c_test_run(HAVE_PRINTF_PTR
	"checking whether the printf functions support %p"
	"yes"
	"no"
	DIRECT)
perform_c_test(SYS_TYPES_HAS_OFF_T
	"checking whether sys/types.h has off_t"
	DIRECT)
perform_c_test(SYS_TYPES_HAS_SSIZE_T
	"checking whether sys/types.h has ssize_t"
	DIRECT)
if(GETGROUPS_T)
	set(GETGROUPS_T gid_t)
endif()
if(NOT SYS_TYPES_HAS_OFF_T)
	set(off_t long)
endif()
if(NOT SYS_TYPES_HAS_SSIZE_T)
	set(ssize_t int)
endif()

check_include_file(direct.h HAVE_DIRECT_H)
check_include_file(fcntl.h HAVE_FCNTL_H)
check_include_file(langinfo.h HAVE_LANGINFO_H)
check_include_file(ldfcn.h HAVE_LDFCN_H)
check_include_file(limits.h HAVE_LIMITS_H)
check_include_file(locale.h HAVE_LOCALE_H)
check_include_file(malloc.h HAVE_MALLOC_H)
check_include_file(stdbool.h HAVE_STDBOOL_H)
check_include_file(stddef.h HAVE_STDDEF_H)
check_include_file(stdlib.h HAVE_STDLIB_H)
check_include_file(string.h HAVE_STRING_H)
check_include_file(strings.h HAVE_STRINGS_H)
check_include_file(sys/file.h HAVE_SYS_FILE_H)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_file(sys/stat.h HAVE_SYS_STAT_H)
check_include_file(sys/time.h HAVE_SYS_TIME_H)
check_include_file(sys/times.h HAVE_SYS_TIMES_H)
check_include_file(time.h HAVE_TIME_H)
check_include_file(unistd.h HAVE_UNISTD_H)
check_include_file(wchar.h HAVE_WCHAR_H)

check_function_exists(alphasort HAVE_ALPHASORT)
check_function_exists(atoll HAVE_ATOLL)
check_function_exists(atoq HAVE_ATOQ)
check_function_exists(clearerr_unlocked HAVE_CLEARERR_UNLOCKED)
check_function_exists(clock HAVE_CLOCK)
check_function_exists(dup2 HAVE_DUP2)
check_function_exists(feof_unlocked HAVE_FEOF_UNLOCKED)
check_function_exists(ferror_unlocked HAVE_FERROR_UNLOCKED)
check_function_exists(fflush_unlocked HAVE_FFLUSH_UNLOCKED)
check_function_exists(fgetc_unlocked HAVE_FGETC_UNLOCKED)
check_function_exists(fgets_unlocked HAVE_FGETS_UNLOCKED)
check_function_exists(fileno_unlocked HAVE_FILENO_UNLOCKED)
check_function_exists(fprintf_unlocked HAVE_FPRINTF_UNLOCKED)
check_function_exists(fputc_unlocked HAVE_FPUTC_UNLOCKED)
check_function_exists(fputs_unlocked HAVE_FPUTS_UNLOCKED)
check_function_exists(fread_unlocked HAVE_FREAD_UNLOCKED)
check_function_exists(fwrite_unlocked HAVE_FWRITE_UNLOCKED)
check_function_exists(getc_unlocked HAVE_GETC_UNLOCKED)
check_function_exists(getchar_unlocked HAVE_GETCHAR_UNLOCKED)
check_function_exists(getrlimit HAVE_GETRLIMIT)
check_function_exists(getrusage HAVE_GETRUSAGE)
check_function_exists(gettimeofday HAVE_GETTIMEOFDAY)
check_function_exists(kill HAVE_KILL)
check_function_exists(mbstowcs HAVE_MBSTOWCS)
check_function_exists(mincore HAVE_MINCORE)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(nl_langinfo HAVE_NL_LANGINFO)
check_function_exists(putc_unlocked HAVE_PUTC_UNLOCKED)
check_function_exists(putchar_unlocked HAVE_PUTCHAR_UNLOCKED)
check_function_exists(scandir HAVE_SCANDIR)
check_function_exists(setlocale HAVE_SETLOCALE)
check_function_exists(setrlimit HAVE_SETRLIMIT)
check_function_exists(strsignal HAVE_STRSIGNAL)
check_function_exists(sysconf HAVE_SYSCONF)
check_function_exists(times HAVE_TIMES)
check_function_exists(wcswidth HAVE_WCSWIDTH)

check_type_size("void*" SIZEOF_VOID_P)
check_type_size("short" SIZEOF_SHORT)
check_type_size("int" SIZEOF_INT)
check_type_size("long" SIZEOF_LONG)
check_type_size("long long" SIZEOF_LONG_LONG)
if(SIZEOF_LONG_LONG)
	set(HAVE_LONG_LONG 1)
endif()
check_type_size("_Bool" SIZEOF__BOOL)
if(SIZEOF__BOOL)
	set(HAVE__BOOL 1)
endif()

# Skip this.
set(NEED_64BIT_HOST_WIDE_INT 0)
if(NEED_64BIT_HOST_WIDE_INT)
	if(HAVE_LONG_LONG)
		set(HOST_WIDE_INT "long long")
	else()
		set(HOST_WIDE_INT "__int64")
	endif()
else()
	set(HOST_WIDE_INT "long")
endif()

set(ENABLE_ASSERT_CHECKING 1)
set(ENABLE_CHECKING 1)
set(ENABLE_RUNTIME_CHECKING 1)
#SET(ENABLE_TREE_CHECKING 1)
set(ENABLE_GC_CHECKING 1)
set(ENABLE_RTL_FLAG_CHECKING 1)
set(ENABLE_NLS 0)
set(ENABLE_DECIMAL_FLOAT 0)

macro(CHECK_DECL_EXISTS symbol var)
	if("${var}_COMPILED" MATCHES "^${var}_COMPILED$")
		message(STATUS "Looking for declaration of ${symbol}")
		set(CHECK_DECL_EXISTS_SYM ${symbol})
		set(CHECK_DECL_EXISTS_VAR ${var})
		configure_file(${GCCCONFIG_SOURCE_DIR}/auto-host.h.in
			${GCCCONFIG_BINARY_DIR}/gcc_check_decl.h
			@ONLY
			IMMEDIATE)
		configure_file(${GCCCONFIG_SOURCE_DIR}/gcc_check_decl.c.in
			${GCCCONFIG_BINARY_DIR}/gcc_check_decl.c
			@ONLY
			IMMEDIATE)
		try_compile(${var}_COMPILED
			${GCCCONFIG_BINARY_DIR}
			${GCCCONFIG_BINARY_DIR}/gcc_check_decl.c
			OUTPUT_VARIABLE
			OUTPUT
			COMPILE_DEFINITIONS
			-I\"${GCC_SOURCE_DIR}/gcc\"
			-I\"${GCC_SOURCE_DIR}/include\")
		if(${var}_COMPILED)
			file(APPEND
				${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
				"Check for ${symbol} compiled with the following output:\n${OUTPUT}\n\n")
			message(STATUS "Looking for declaration of ${symbol} - found")
		else()
			file(APPEND
				${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
				"Check for ${symbol} failed to compile with the following output:\n${OUTPUT}\n\n")
			message(STATUS "Looking for declaration of ${symbol} - not found")
		endif()
	endif()
	if(${var}_COMPILED)
		set(${var} 1)
	else()
		set(${var} 0)
	endif()
endmacro()

check_decl_exists(ffs HAVE_DECL_FFS)
check_decl_exists(snprintf HAVE_DECL_SNPRINTF)
check_decl_exists(vsnprintf HAVE_DECL_VSNPRINTF)
check_decl_exists(strverscmp HAVE_DECL_STRVERSCMP)

check_decl_exists(getenv HAVE_DECL_GETENV)
check_decl_exists(atol HAVE_DECL_ATOL)
check_decl_exists(sbrk HAVE_DECL_SBRK)
check_decl_exists(abort HAVE_DECL_ABORT)
check_decl_exists(atof HAVE_DECL_ATOF)
check_decl_exists(getcwd HAVE_DECL_GETCWD)
check_decl_exists(getwd HAVE_DECL_GETWD)
check_decl_exists(strsignal HAVE_DECL_STRSIGNAL)
check_decl_exists(clearerr_unlocked HAVE_DECL_CLEARERR_UNLOCKED)
check_decl_exists(feof_unlocked HAVE_DECL_FEOF_UNLOCKED)
check_decl_exists(ferror_unlocked HAVE_DECL_FERROR_UNLOCKED)
check_decl_exists(fflush_unlocked HAVE_DECL_FFLUSH_UNLOCKED)
check_decl_exists(fgetc_unlocked HAVE_DECL_FGETC_UNLOCKED)
check_decl_exists(fgets_unlocked HAVE_DECL_FGETS_UNLOCKED)
check_decl_exists(fileno_unlocked HAVE_DECL_FILENO_UNLOCKED)
check_decl_exists(fprintf_unlocked HAVE_DECL_FPRINTF_UNLOCKED)
check_decl_exists(fputc_unlocked HAVE_DECL_FPUTC_UNLOCKED)
check_decl_exists(fputs_unlocked HAVE_DECL_FPUTS_UNLOCKED)
check_decl_exists(fread_unlocked HAVE_DECL_FREAD_UNLOCKED)
check_decl_exists(fwrite_unlocked HAVE_DECL_FWRITE_UNLOCKED)
check_decl_exists(getc_unlocked HAVE_DECL_GETC_UNLOCKED)
check_decl_exists(getchar_unlocked HAVE_DECL_GETCHAR_UNLOCKED)
check_decl_exists(putc_unlocked HAVE_DECL_PUTC_UNLOCKED)
check_decl_exists(putchar_unlocked HAVE_DECL_PUTCHAR_UNLOCKED)
check_decl_exists(strstr HAVE_DECL_STRSTR)
check_decl_exists(errno HAVE_DECL_ERRNO)
check_decl_exists(asprintf HAVE_DECL_ASPRINTF)
check_decl_exists(vasprintf HAVE_DECL_VASPRINTF)
check_decl_exists(malloc HAVE_DECL_MALLOC)
check_decl_exists(realloc HAVE_DECL_REALLOC)
check_decl_exists(calloc HAVE_DECL_CALLOC)
check_decl_exists(free HAVE_DECL_FREE)
check_decl_exists(basename HAVE_DECL_BASENAME)
check_decl_exists(getopt HAVE_DECL_GETOPT)
check_decl_exists(clock HAVE_DECL_CLOCK)

check_decl_exists(getrlimit HAVE_DECL_GETRLIMIT)
check_decl_exists(setrlimit HAVE_DECL_SETRLIMIT)
check_decl_exists(getrusage HAVE_DECL_GETRUSAGE)
check_decl_exists(ldgetname HAVE_DECL_LDGETNAME)
check_decl_exists(sigaltstack HAVE_DECL_SIGALTSTACK)
check_decl_exists(getpagesize HAVE_DECL_GETPAGESIZE)
check_decl_exists(times HAVE_DECL_TIMES)

#-----------------------------------------------------------------------------
# We fake the following settings because we don't care about the backend.
set(HAVE_MMAP_FILE 0)
set(HAVE_MMAP_DEV_ZERO 0)
set(HAVE_MMAP_ANON 0)
set(HAVE_ICONV 0)
set(HAVE_ICONV_H 0)
set(ICONV_CONST 0)
set(HAVE_OBSTACK 0)
set(HAVE_GAS_BALIGN_AND_P2ALIGN 1)
set(HAVE_GAS_MAX_SKIP_P2ALIGN 1)
set(HAVE_GAS_SUBSECTION_ORDERING 1)
set(HAVE_GAS_HIDDEN 1)
set(HAVE_AS_LEB128 1)
set(HAVE_GAS_SHF_MERGE 1)
set(HAVE_AS_TLS 1)
set(HAVE_GAS_FILDS_FISTS 1)
set(HAVE_AS_GOTOFF_IN_DATA 1)
set(HAVE_AS_GSTABS_DEBUG_FLAG 1)
set(HAVE_AS_DWARF2_DEBUG_LINE 1)
set(HAVE_AS_GDWARF2_DEBUG_FLAG 1)
set(HAVE_LD_RO_RW_SECTION_MIXING 1)
set(HAVE_LD_EH_FRAME_HDR 1)
set(TARGET_LIBC_PROVIDES_SSP 0)
set(HAVE_COMDAT_GROUP 0)
#-----------------------------------------------------------------------------

set(PREFIX_INCLUDE_DIR "${CMAKE_INSTALL_PREFIX}/include")

macro(CHECK_TYPE_EXISTS symbol var)
	if("${var}_COMPILED" MATCHES "^${var}_COMPILED$")
		message(STATUS "Checking for ${symbol}")
		configure_file(${GCCCONFIG_SOURCE_DIR}/auto-host.h.in
			${GCCCONFIG_BINARY_DIR}/gcc_test_auto_host.h
			@ONLY
			IMMEDIATE)
		set(COMPILE_DEFINITIONS "-DTEST_${var} -DHELLO_WORLD")
		set(INCLUDE_DIRECTORIES
			"${GCC_SOURCE_DIR}/gcc"
			"${GCC_SOURCE_DIR}/include"
			"${GCCCONFIG_BINARY_DIR}")
		try_compile(${var}_COMPILED
			${GCCCONFIG_BINARY_DIR}
			${GCCCONFIG_SOURCE_DIR}/gcc_c_tests.c
			OUTPUT_VARIABLE
			OUTPUT
			CMAKE_FLAGS
			"-DCOMPILE_DEFINITIONS:STRING=${COMPILE_DEFINITIONS}"
			"-DINCLUDE_DIRECTORIES:STRING=${INCLUDE_DIRECTORIES}")
		if(${var}_COMPILED)
			file(APPEND
				${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
				"Check for ${symbol} compiled with the following output:\n${OUTPUT}\n\n")
			message(STATUS "Checking for ${symbol} - found")
		else()
			file(APPEND
				${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log
				"Check for ${symbol} failed to compile with the following output:\n${OUTPUT}\n\n")
			message(STATUS "Checking for ${symbol} - not found")
		endif()
	endif()
	if(${var}_COMPILED)
		set(${var} 1)
	else()
		set(${var} 0)
	endif()
endmacro()

check_type_exists("struct tms" HAVE_STRUCT_TMS)
check_type_exists(clock_t HAVE_CLOCK_T)
#CHECK_TYPE_EXISTS(ino_t HAVE_INO_T)
#CHECK_TYPE_EXISTS(dev_t HAVE_DEV_T)

# Find the platform configuration.
if(UNIX)
	set(GCC_USE_PLATFORM_SCRIPT 1)
endif()
set(GCC_EXECUTED_PLATFORM_SCRIPT)
if(GCC_USE_PLATFORM_SCRIPT)
	find_program(GCC_SH sh /bin/sh c:/msys/1.0/bin/sh.exe)
	mark_as_advanced(GCC_SH)
	if(GCC_SH)
		if("${CMAKE_SYSTEM_NAME}" STREQUAL "SunOS")
			# other values don't work on solaris
			set(TG "i386-pc-solaris2.11")
		else()
			set(TG "${PSC_TARGET}")
		endif()
		exec_program(${GCC_SH}
			ARGS
			"\"${GCCCONFIG_SOURCE_DIR}/gcc_platform.sh\""
			"\"${GCCCONFIG_BINARY_DIR}/gcc_platform.cmake\""
			"\"${CMAKE_COMMAND}\""
			"\"${TG}\"")
		set(GCC_EXECUTED_PLATFORM_SCRIPT 1)
	endif()
endif()
if(NOT GCC_EXECUTED_PLATFORM_SCRIPT)
	configure_file(${GCCCONFIG_SOURCE_DIR}/gcc_platform_win32.cmake
		${GCCCONFIG_BINARY_DIR}/gcc_platform.cmake
		@ONLY
		IMMEDIATE)
endif()

# Load the platform configuration.
if(EXISTS "${GCCCONFIG_BINARY_DIR}/gcc_platform.cmake")
	include("${GCCCONFIG_BINARY_DIR}/gcc_platform.cmake")
else()
	message(FATAL_ERROR "Cannot find gcc_platform.cmake.")
endif()

if(extra_modes)
	set(EXTRA_MODES_FILE config/${extra_modes})	# ${GCC_SOURCE_DIR}/gcc/
	set(EXTRA_CC_MODES 1)
endif()

if(NOT HAVE_DIRENT_H)
	configure_file(${GCCCONFIG_SOURCE_DIR}/dirent.h
		${GCC_BINARY_DIR}/libcpp/dirent.h
		@ONLY
		IMMEDIATE)
endif()

configure_file(${GCCCONFIG_SOURCE_DIR}/libiberty_config.h.in
	${GCC_BINARY_DIR}/libiberty/config.h
	@ONLY
	IMMEDIATE)
configure_file(${GCCCONFIG_SOURCE_DIR}/libdecnumber_config.h.in
	${GCC_BINARY_DIR}/libdecnumber/config.h
	@ONLY
	IMMEDIATE)
configure_file(${GCCCONFIG_SOURCE_DIR}/libcpp_config.h.in
	${GCC_BINARY_DIR}/libcpp/config.h
	@ONLY
	IMMEDIATE)
configure_file(${GCCCONFIG_SOURCE_DIR}/auto-host.h.in
	${GCC_BINARY_DIR}/gcc/auto-host.h
	@ONLY
	IMMEDIATE)
