# Do not inherit library building rules from the parent project.
set(BUILD_SHARED_LIBS 0)

if(APPLE)
	if(CMAKE_COMPILER_IS_GNUCC)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-cpp-precomp")
	endif()
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_ANSI_CFLAGS}")

set(BFD_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/bfd")

add_definitions(-DIPA_LINK
	-DKEY
	-DHAVE_CONFIG_H
	-W
	-Wall
	-Wwrite-strings
	-Wstrict-prototypes
	-Wmissing-prototypes)

add_subdirectory(bfd)
add_subdirectory(ld)
