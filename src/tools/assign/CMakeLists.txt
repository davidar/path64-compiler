
set(PATHSCALE_ASSIGN_SRCS "assign.c")

include_directories(../../clibinc)

add_definitions(-D_LITTLE_ENDIAN -U__mips)

if(PATH64_ENABLE_FORTRAN)
	add_executable(assign ${PATHSCALE_ASSIGN_SRCS})
	if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "SunOS")
		set_property(TARGET assign PROPERTY LINK_FLAGS "-static")
	endif()
	target_link_libraries(assign pathfortran-mfef95)

	install(TARGETS
		assign
		RUNTIME
		DESTINATION
		bin
		LIBRARY
		DESTINATION
		${PATH64_LIB_PATH})

endif()
