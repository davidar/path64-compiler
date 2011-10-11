# Restore the real executable output path.  We want to put our executable
# there.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GCC_EXECUTABLE_OUTPUT_PATH})

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

#-----------------------------------------------------------------------------
# C++ Compiler Executable

if(CMAKE_SYSTEM MATCHES "HP-UX")
	if(CMAKE_C_COMPILER MATCHES "(^cc$|/cc$)")
		set_source_files_properties(../c-opts.c
			PROPERTIES
			COMPILE_FLAGS
			"-Wp,-H,20000")
	endif()
endif()

#-----------------------------------------------------------------------------
# Special flags for some source files.

if(CYGWIN)
	set_source_files_properties(../cppdefault.c
		PROPERTIES
		COMPILE_FLAGS
		"-DTOOL_INCLUDE_DIR=/usr/include")
endif()

include_directories(BEFORE ${CMAKE_CURRENT_BINARY_DIR})
configure_file(${GCC_SOURCE_DIR}/gcc/path64_prefix.h.in
	${GCC_BINARY_DIR}/gcc/cp/path64_prefix.h
	@ONLY
	IMMEDIATE)

path64_set_multiarch_sources(cc1plus42_sources
	COMMON
	${cxx_target_srcs}
	../attribs.c
	../c-common.c
	../c-cppbuiltin.c
	../c-dump.c
	../c-format.c
	../c-gimplify.c
	../c-incpath.c
	../c-lex.c
	../c-omp.c
	../c-opts.c
	../c-pch.c
	../c-ppoutput.c
	../c-pragma.c
	../c-pretty-print.c
	../c-semantics.c
	../cppdefault.c
	../dummy-checksum.c
	../main.c
	../prefix.c
	../stub-objc.c
	../tree-inline.c
	../tree-mudflap.c
	call.c
	class.c
	cp-gimplify.c
	cp-lang.c
	cp-objcp-common.c
	cvt.c
	cxx-pretty-print.c
	decl.c
	decl2.c
	dump.c
	error.c
	except.c
	expr.c
	friend.c
	init.c
	lex.c
	mangle.c
	method.c
	name-lookup.c
	optimize.c
	parser.c
	pt.c
	ptree.c
	repo.c
	rtti.c
	search.c
	semantics.c
	tree.c
	typeck.c
	typeck2.c)	# source file count: 53


path64_add_multiarch_executable(cc1plus42 cc1plus42_sources)
path64_multiarch_link_multiarch_libraries(cc1plus42 backend)
path64_multiarch_link_libraries(cc1plus42 cpp decnumber iberty)
path64_multiarch_link_multiarch_libraries(cc1plus42 spin)
path64_install_multiarch(cc1plus42)
path64_add_dependencies_from_multiarch(compiler-stage cc1plus42)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	path64_multiarch_link_libraries(cc1plus42 winlib)
endif()


# Use the "-lx" option to disable map file generation on Borland.
# This avoids the "Fatal: Access violation.  Link terminated." error.
if(BORLAND)
	path64_set_multiarch_property(cc1plus42 LINK_FLAGS "-lx")
endif()

