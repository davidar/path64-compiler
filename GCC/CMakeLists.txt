project(GCC C)
if(COMMAND CMAKE_POLICY)
	cmake_policy(SET CMP0003 NEW)
endif()

if(Path64_INSTALL_COMPONENT_NAME_RUNTIME_EXECUTABLE)
	set(Path64_INSTALL_COMPONENT_RUNTIME_EXECUTABLE
		COMPONENT
		${Path64_INSTALL_COMPONENT_NAME_RUNTIME_EXECUTABLE})
endif()

# Do not place internal build-related executables in a parent project
# executable path.  Backup the real executable output path so it can
# be restored in gcc/cp for building cc1plus42.
set(GCC_EXECUTABLE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${GCC_BINARY_DIR}/gcc")

# Do not inherit library building rules from the parent project.
set(BUILD_SHARED_LIBS 0)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${GCC_BINARY_DIR}/gcc")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${GCC_BINARY_DIR}/gcc")

if(APPLE)
	if(CMAKE_COMPILER_IS_GNUCC)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-cpp-precomp")
	endif()
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_ANSI_CFLAGS}")

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	string(REGEX REPLACE "/W([0-4])?" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
	set(CMAKE_CXX_WARNING_LEVEL 0)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W0")
else()
	if(CMAKE_C_COMPILER MATCHES "bcc32")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w-")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
	endif()
endif()

# Determine if compiler errors on "redefinition of a 'extern inline' function"
# and if so, find an appropriate flag to change this behavior
include(CheckCSourceCompiles)
function(check_extern_inline_redef _varsuffix _arg)
	if(SUCCESS)
		return()
	endif()
	set(msg "Checking whether 'extern inline' redefinition permitted")
	if(_arg STREQUAL "")
		set(msg "${msg} by default")
	else()
		set(msg "${msg} using flag ${_arg}")
	endif()
	if(NOT EXTERN_INLINE_SUCCESS)
		message(STATUS "${msg}")
	endif()
	string(TOUPPER "PERMITS_EXTERN_INLINE_REDEF_${_varsuffix}" outvar)
	set(CMAKE_REQUIRED_FLAGS "${_arg}")
	check_c_source_compiles("
    extern inline int dummy() { return 0; }
    int dummy() { return 0; }
    int main() {
	    return dummy();
    }"
		${outvar})
	if(${outvar})
		set(msg "${msg} - yes")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${_arg}" PARENT_SCOPE)
		set(SUCCESS yes PARENT_SCOPE)
	else()
		set(msg "${msg} - no")
	endif()

	if(NOT EXTERN_INLINE_SUCCESS)
		message(STATUS "${msg}")
	endif()
endfunction()

set(SUCCESS no)
check_extern_inline_redef(BY_DEFAULT "")
check_extern_inline_redef(WITH_STD_GNU89 -std=gnu89)
check_extern_inline_redef(WITH_FGNU89_INLINE -fgnu89-inline)	# see http://gcc.gnu.org/gcc-4.3/porting_to.html
check_extern_inline_redef(WITH_GNU_INLINE_DEFINE -D__GNUC_GNU_INLINE__)	# see http://gcc.gnu.org/gcc-4.3/porting_to.html

set(EXTERN_INLINE_SUCCESS
	${SUCCESS}
	CACHE
	INTERNAL
	"If we know how to compile redefinitions of extern inlines"
	FORCE)

if(NOT SUCCESS)
	message(STATUS
		"Could not determine the flag for your C compiler "
		"to permit redefinition of 'extern inline' functions. Please file a "
		"bug, being sure to describe your compiler precisely and include the "
		"flag it requires to compile a file with these redefintions.")
endif()

# TODO Unknown what happens if you remove TARG_X8664
add_definitions(-DALMOST_STDC
	-DIs_True_On
	-DHAVE_CONFIG_H
#	-DTARG_X8664
	-DFE_GNU_4_2_0
	-DKEY
	-W
	-Wall
	-Wwrite-strings
	-Wstrict-prototypes
	-Wmissing-prototypes
	-Wmissing-format-attribute)

add_subdirectory(config_cmake)
add_subdirectory(libiberty)
add_subdirectory(libdecnumber)
add_subdirectory(libcpp)
add_subdirectory(libspin)
add_subdirectory(gcc)
