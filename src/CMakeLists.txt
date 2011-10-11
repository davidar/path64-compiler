project(PATHSCALE)

set(CMAKE_C_FLAGS "-D_LANGUAGE_C ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-D_LANGUAGE_C_PLUS_PLUS ${CMAKE_CXX_FLAGS}")

set(CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES "m;c")

include(includes.cmake)

# Fake target that depends on all be components
add_custom_target(be-all)
add_dependencies(compiler-stage be-all)

#Hack for Window building
if((${CMAKE_SYSTEM_NAME} MATCHES "Windows" ))
else()
	add_subdirectory(csu)
endif()

add_subdirectory(common/com)
add_subdirectory(driver)
add_subdirectory(libcif)
add_subdirectory(libcmplrs)
add_subdirectory(libelf)
add_subdirectory(libelfutil)
add_subdirectory(common/rtkutils)
add_subdirectory(common/targ_info/parser/gensyn)
add_subdirectory(common/targ_info)
add_subdirectory(common/util)
add_subdirectory(be)

if(NOT WIN32)
	# libpscrt not yet ported to Windows
	add_subdirectory(libpscrt)
endif()

if(PATH64_ENABLE_GNU_FRONTEND)
	add_subdirectory(wgen)
endif()

add_subdirectory(ipa)

if(PATH64_ENABLE_IPA)
	add_subdirectory(ipa_link)
endif()

# TODO : Threading and other things need to be ported
if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD" OR ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	if(PATH64_ENABLE_OPENMP)
		add_subdirectory(libopenmp)
	endif()
endif()
add_subdirectory(libfi)

# Check to see if -Wl,-x aka -Wl,--discard-all is supported - it isn't on gold
include(CheckCCompilerFlag)
check_c_compiler_flag("-Wl,-x" LINKER_SUPPORTS_DISCARD_ALL)
if(LINKER_SUPPORTS_DISCARD_ALL)
	set(LINK_FLAG_DISCARD_ALL "-Wl,-x")
else()
	set(LINK_FLAG_DISCARD_ALL)
endif()

# All the math stuff seems to be built from libmpath
# libmv == vector math functions
add_subdirectory(libm)
add_subdirectory(libmpath)

#Hack for Window building
if(NOT WIN32)
	add_subdirectory(instrumentation)
	add_subdirectory(arith)
endif()

# TODO : Can't determine linking method

if(${PATH64_ENABLE_FORTRAN})
	include(libF77/config.cmake)
	add_subdirectory(libpathfortran)
	add_subdirectory(crayf90)
	add_subdirectory(f95_coco)
endif()

add_subdirectory(ir_tools)
add_subdirectory(tools)
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	# This is for huge pages on linux
	add_subdirectory(libhugetlbfs)
endif()
# TODO : Pull in code for superpages on FBSD
# TODO : Pull in integrate with MPSS (Multiple Page Size Support) on Solaris
# http://hub.opensolaris.org/bin/download/Project+muskoka/virtual_memory/opensolarisvm1.pdf
# http://docs.sun.com/app/docs/doc/816-0210/6m6nb7mf1?a=view
# http://cvs.opensolaris.org/source/xref/onnv/onnv-gate/usr/src/uts/sun4/os/startup.c
#

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	add_subdirectory(windows)
endif()
