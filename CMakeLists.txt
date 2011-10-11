cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)
project(Path64)

enable_testing()

# Disallow in-source build
if("${Path64_SOURCE_DIR}" STREQUAL "${Path64_BINARY_DIR}")
	message(FATAL_ERROR
		"Project must be built out-of-source.  Remove files from the source tree that were created by this attempt:
  rm -rf CMakeFiles CMakeCache.txt
Then create a separate build directory and run CMake there.
  mkdir ../build
  cd ../build
  cmake ../
This approach keeps the source tree pristine.")
endif()

# Search for various include files and functions
include(${Path64_SOURCE_DIR}/cmake/Modules/ConfigureChecks.cmake)

# Force the build_type=debug flags to sane values overriding users choices
set(CMAKE_C_FLAGS_DEBUG
	"-O0 -g -D_DEBUG -DIs_True_On=1"
	CACHE
	STRING
	"Flags used by the compiler during debug builds."
	FORCE)
set(CMAKE_CXX_FLAGS_DEBUG
	"-Wno-deprecated -O0 -g -D_DEBUG -DIs_True_On=1"
	CACHE
	STRING
	"Flags used by the compiler during debug builds."
	FORCE)
set(CMAKE_Fortran_FLAGS_DEBUG
	"-O0 -g3 -D_DEBUG -DIs_True_On=1"
	CACHE
	STRING
	"Flags used by the compiler during debug builds."
	FORCE)

# Using -mno-sse3 flag for release build if PATH64_ENABLE_PSCRUNTIME=ON
if(PATH64_ENABLE_PSCRUNTIME)
	set(SSE_FLAGS "-mno-sse3")
endif()

# Force the build_type=release flags to -O2 -DNDEBUG values overriding default cmake -O3
set(CMAKE_C_FLAGS_RELEASE
	"-O2 -DNDEBUG ${SSE_FLAGS}"
	CACHE
	STRING
	"Flags used by the compiler during release builds."
	FORCE)
set(CMAKE_CXX_FLAGS_RELEASE
	"-Wno-deprecated -O2 -DNDEBUG ${SSE_FLAGS}"
	CACHE
	STRING
	"Flags used by the compiler during release builds."
	FORCE)
set(CMAKE_Fortran_FLAGS_RELEASE
	"-O2 -DNDEBUG ${SSE_FLAGS}"
	CACHE
	STRING
	"Flags used by the compiler during release builds."
	FORCE)


option(PATH64_ENABLE_GNU_FRONTEND "Use GNU C/C++ front-end" ON)
if(NOT WIN32)
	# IPA not yet ported to windows.
	option(PATH64_ENABLE_IPA "Enable IPA" ON)
endif()


# Define user option whether to build the Fortran compiler.
option(PATH64_ENABLE_FORTRAN "Build Fortran compiler" ON)

if(PATH64_ENABLE_FORTRAN)
	if("${CMAKE_Fortran_COMPILER}" STREQUAL "")
		set(CMAKE_Fortran_COMPILER_FORCED TRUE)
		set(CMAKE_Fortran_COMPILER "true")
	endif()
	enable_language(Fortran)
endif()

# Define user option whether to build the profiling libraries
option(PATH64_ENABLE_PROFILING "Build profiling libraries" ON)
if(PATH64_ENABLE_PROFILING)
	message(STATUS "Profiling build enabled")
else()
	message(STATUS "Profiling build disabled")
endif()

# Define user option whether to build OpenMP
# TODO : Threading and other things need to be ported
if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	option(PATH64_ENABLE_OPENMP "Build OpenMP" ON)
endif()

if(PATH64_ENABLE_OPENMP)
	message(STATUS "OpenMP build enabled")
else()
	message(STATUS "OpenMP build disabled")
endif()


# GNU-style hashing needs support of the dynamic linker, so white-list
# potential platforms.
include(CheckCCompilerFlag)
check_c_compiler_flag("-Wl,--hash-style=gnu" SUPPORT_HASH_STYLE_GNU)

if(SUPPORT_HASH_STYLE_GNU)
	set(hash_style_supported TRUE)
else()
	set(hash_style_supported FALSE)
endif()

#-----------------------------------------------------------------------------
# Target architecture stuff.
include(${Path64_SOURCE_DIR}/src/targets.cmake)


# Use -G0 option if building with pathscale on MIPS
if("${CMAKE_C_COMPILER_ID}" STREQUAL "PathScale")
	path64_get_host_arch(arch)
	if("${arch}" STREQUAL "mips")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -G0")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -G0")
		set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -G0")
	endif()
endif()


if(NOT PATH64_USE_SYSTEM_COMPILER_FOR_TARGET_LIBS OR "${CMAKE_C_COMPILER_ID}" STREQUAL "PathScale")
	set(BOOTSTRAP_HACK_FLAG "-fbootstrap-hack")
else()
	set(BOOTSTRAP_HACK_FLAG "")
endif()


# Define if we are doing a supported build
option(PATH64_ENABLE_SUPPORTED "Enabling supported build" OFF)
if(PATH64_ENABLE_SUPPORTED)
	message(FATAL_ERROR
		"This source tree is still not officially supported yet")
else()
	message(STATUS
		"Community supported version.  For officially supported version please visit http://www.pathscale.com")
endif()

# Valgrind?
option(PATH64_ENABLE_VALGRIND "Enabling valgrind" OFF)
if(PATH64_ENABLE_VALGRIND)
	message(STATUS "Valgrind enabled")
else()
	message(STATUS "Valgrind disabled")
endif()

# Whatever arbitrary number you'd like in 3 string format eg 3.9.99
if(NOT PSC_FULL_VERSION)
	set(PSC_FULL_VERSION
		"4.0.10"
		CACHE
		STRING
		"Defaulting to version 4.0.10")
endif()

message(STATUS "Version is set to " ${PSC_FULL_VERSION})
string(REGEX
	REPLACE
	"^([0-9.]+).*$"
	"\\1"
	TMPVERSION
	${PSC_FULL_VERSION})
# If it's not in three string format it sucks to be you
string(REGEX
	REPLACE
	"^([0-9]+).+$"
	"\\1"
	PSC_MAJOR_VERSION
	${TMPVERSION})
string(REGEX
	REPLACE
	"^[0-9]+\\.([0-9]+).*$"
	"\\1"
	PSC_MINOR_VERSION
	${TMPVERSION})

if(NOT PSC_TARGET)
	path64_get_host_arch(arch)
	if("${arch}" STREQUAL "x8664")
		set(arch "x86_64")
	elseif("${arch}" STREQUAL "x8632")
		set(arch "i386")
	endif()
	string(TOLOWER "${arch}-unknown-${CMAKE_SYSTEM_NAME}" PSC_TARGET)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set(PSC_TARGET "${PSC_TARGET}-gnu")
	endif()
endif()

# Checking that CRT paths are specified for all targets
foreach(targ ${PATH64_ENABLE_TARGETS})
	if(PSC_CRTBEGIN_PATH_${targ})
		message(STATUS
			"crtbegin/crtend path for ${targ} target: ${PSC_CRTBEGIN_PATH_${targ}}")
	else()
		set(arch "${_PATH64_TARGET_ARCH_${targ}}")
		if("${arch}" STREQUAL "x8664")
			set(PSC_CRTBEGIN_PATH_${targ} "LOCAL")
		else()
			message(FATAL_ERROR
				"crtbegin/crtend path for ${targ} target is not specified. Please specify PSC_CRTBEGIN_PATH_${targ} variable")
		endif()
	endif()
endforeach()

# Do we build with huge pages support
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	option(PATH64_ENABLE_HUGEPAGES "Enabling supported huge pages" OFF)
endif()

if(PATH64_ENABLE_HUGEPAGES)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		message(STATUS "Building with huge pages support.")
	else()
		message(FATAL_ERROR
			"Huge pages not supported on this platform yet.  Please disable this option or contact PathScale for support")
	endif()
else()
	message(STATUS "Not building with huge pages support")
endif()

# Use X86 elf file format for whirl objects
option(X86_WHIRL_OBJECTS
	"Use x86 elf file format for whirl object files (otherwise uses MIPS elf format)"
	ON)
if(X86_WHIRL_OBJECTS)
	message(STATUS "Using x86 elf file format for whirl object files")
	add_definitions(-DX86_WHIRL_OBJECTS)
else()
	message(STATUS "Using MIPS elf file format for whirl object files")
endif()

# Fuse WHIRL and native code in IPA objects
option(FAT_WHIRL_OBJECTS
	" Fuse WHIRL and native code in IPA objects"
	OFF)
if(FAT_WHIRL_OBJECTS)
	message(STATUS "Configuring for fat IPA objects")
	add_definitions(-DFAT_WHIRL_OBJECTS)
else()
	message(STATUS "Configuring for simple WHIRL IPA objects")
endif()


# Are we using the PathScale runtime
if(NOT WIN32)
	# Not yet ported to Windows
	option(PATH64_ENABLE_PSCRUNTIME
		"Using the PathScale c and c++ runtime"
		OFF)
endif()
if(PATH64_ENABLE_PSCRUNTIME)
	add_definitions(-DPATH64_ENABLE_PSCRUNTIME)
	message(STATUS "PathScale runtime enabled")
else()
	message(STATUS "GNU runtime enabled")
endif()


option(PATH64_ENABLE_CXX "Enable C++ support" ON)


# Optional path to libdwarf
if(NOT PATH64_LIBDWARF_PATH)
	set(PATH64_LIBDWARF_PATH NOTFOUND CACHE PATH "Optional path to libdwarf library")
endif()

set(NAMEPREFIX
	"path"
	CACHE
	STRING
	"Default name excutable prefix [path]")
if(NAMEPREFIX)
	message(STATUS "Executables will be prefixed with '${NAMEPREFIX}'")
else()
	message(FATAL_ERROR "The executable name prefix NAMEPREFIX must not be empty.")
endif()


# Set default install prefix (instead of CMake's /usr/local default).
# TODO : How do we fix this so cmake -i shows the correct default prefix?
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX
		"/opt/path64"
		CACHE
		PATH
		"Installation root prefix"
		FORCE)
endif()

# On Linux some things needs to link with -ldl
# but on FBSD they should not.
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(libdl "dl")
else()
	set(libdl "")
endif()


# Selecting macro for enabling int types formats for C++
set(PATH64_STDC_FORMAT_MACROS -D__STDC_FORMAT_MACROS)

option(PATH64_BUILD_32 "Build 32-bit version" OFF)

if(PATH64_BUILD_32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -m32")
	set(CMAKE_EXE_LINKER_FLAGS "-m32")
	set(CMAKE_MODULE_LINKER_FLAGS "-m32")
	set(CMAKE_SHARED_LINKER_FLAGS "-m32")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
	# Forching build for 64-bit on osol
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m64")
	set(CMAKE_Fortran_FLAGS "${CMAKE_Fortran_FLAGS} -m64")
	set(CMAKE_EXE_LINKER_FLAGS "-m64")
	set(CMAKE_MODULE_LINKER_FLAGS "-m64")
	set(CMAKE_SHARED_LINKER_FLAGS "-m64")
endif()


# Tell GCC_XML that we are providing gccxml_cc1plus for it.
set(GCCXML_NATIVE_CC1PLUS 1)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/GCCVersion.cmake)


# Place all targets near the top of the build tree by default.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${Path64_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${Path64_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${Path64_BINARY_DIR}/lib)

file(MAKE_DIRECTORY ${Path64_BINARY_DIR}/lib)
# TODO this doesn't work on windows so can't run from an unmodified build tree (?)
execute_process(COMMAND
	${CMAKE_COMMAND}
	-E
	create_symlink
	.
	${Path64_BINARY_DIR}/lib/${PSC_FULL_VERSION})

set(PATH64_LIB_PATH lib/${PSC_FULL_VERSION})

# Tell GCC where to install relative to the CMAKE_INSTALL_PREFIX.
set(PATH64_GCC_INSTALL_PATH ${PATH64_LIB_PATH})

# If readding support for SGI and other platforms, where the
# dynamic linker allows overwritting weak references in modules
# opened via dlopen or similar means:
# -DUSE_WEAK_REFERENCES
add_definitions(-D_SGI_SOURCE
	-D__GNU_BUG_WORKAROUND
#	-DTARG_X8664
	-DKEY
	-DFE_GNU_4_2_0
	-D_LONGLONG
	-D_MIPSEL
	-DTARG_LINUX)

# Define _GNU_SOURCE macro to avoid linking with new versions of glibc on linux
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
	add_definitions(-D_GNU_SOURCE)
endif()

#IF (${BUILD_TARGET_DIR} MATCHES "arm")
#ADD_DEFINITIONS( -DTARG_ST
#                -DTARG_ARM
#                -DARM
#                -DMUMBLE_ARM_BSP
#                -DSUPPORTS_SELECT
#                -DBE_EXPORTED=
#                -DTARGINFO_EXPORTED=
#               )

if(${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
	# --export-dynamic is not supported on osol
	set(EXPORT_DYNAMIC_SUPPORTED "no")
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	# --export-dynamic is not supported on Windows
	set(EXPORT_DYNAMIC_SUPPORTED "no")
else()
	set(EXPORT_DYNAMIC_SUPPORTED "yes")
endif()

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
	if(${CMAKE_GENERATOR} MATCHES "MSYS")
		set(SHELL_EXT sh)
	else()
		set(SHELL_EXT cmd)
	endif()
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(SYMLINK_COMMAND ${CMAKE_COMMAND} -E copy_if_different)
	SET(SYMLINK_COMMAND_FOR_INSTALL "\"${CMAKE_COMMAND}\" -E copy_if_different")
	set(PATH64_AUXILARY_LD_FLAGS '"-FIXME"')
	# ADD_DEFINITIONS(-DPSC_FULL_VERSION="${PSC_FULL_VERSION}")
else()
	set(SHELL_EXT sh)
	set(SYMLINK_COMMAND ${CMAKE_COMMAND} -E create_symlink)
	SET(SYMLINK_COMMAND_FOR_INSTALL "\"${CMAKE_COMMAND}\" -E create_symlink")
endif()

function(symlink_at_install _src _dest)
	install(CODE
			"execute_process(COMMAND ${SYMLINK_COMMAND_FOR_INSTALL} \"${_src}\" \"\$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/${_dest}\")")
endfunction()

# build runtime search paths
include(${Path64_SOURCE_DIR}/cmake/Modules/GCCRuntimeDirs.cmake)

if(PATH64_DEFAULT_ABI)
	add_definitions(-DPATH64_DEFAULT_ABI=${PATH64_DEFAULT_ABI})
endif()

include(${CMAKE_ROOT}/Modules/TestBigEndian.cmake)
test_big_endian(HAVE_BIG_ENDIAN)
if(HAVE_BIG_ENDIAN)
	add_definitions(-DHOST_IS_BIG_ENDIAN=1 -DHOST_IS_LITTLE_ENDIAN=0)
else()
	add_definitions(-DHOST_IS_BIG_ENDIAN=0 -DHOST_IS_LITTLE_ENDIAN=1)
endif()

# Fake targets for compiler stage
add_custom_target(compiler-stage)	# compiler core
add_custom_target(compiler-stage-C)	# C compiler
add_custom_target(compiler-stage-CXX)	# CXX compiler
add_custom_target(compiler-stage-Fortran)	# Fortran compiler
add_custom_target(compiler-stage-libs-C)	# C compiler with libs
add_custom_target(compiler-stage-libs-CXX)	# CXX compiler with libs
add_custom_target(compiler-stage-libs-Fortran)	# Fortran compiler with libs
add_dependencies(compiler-stage-C compiler-stage)
add_dependencies(compiler-stage-CXX compiler-stage-C)
add_dependencies(compiler-stage-Fortran compiler-stage)
add_dependencies(compiler-stage-libs-C compiler-stage-C)
add_dependencies(compiler-stage-libs-CXX
	compiler-stage-CXX
	compiler-stage-libs-C)
add_dependencies(compiler-stage-libs-Fortran compiler-stage-Fortran)

# Build GCC and pathscale.
if(PATH64_ENABLE_GNU_FRONTEND)
	add_subdirectory(GCC)
endif()
add_subdirectory(src)

add_subdirectory(bootstrap)
add_subdirectory(gcc_incl)

