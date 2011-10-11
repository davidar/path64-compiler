macro(GCC_MAKE_CONFIG_H output blocker defines headers cpu)
	set(GCC_CONFIG_BLOCKER "${blocker}")
	set(GCC_CONFIG_CONFIG_H)
	set(GCC_CONFIG_CPU_DEFAULT)
	set(GCC_CONFIG_DEFINES)
	set(GCC_CONFIG_AUTO)
	set(GCC_CONFIG_HEADERS)
	set(GCC_CONFIG_TM_H)

	if("${output}" MATCHES "^config\\.h$")
		set(GCC_CONFIG_CONFIG_H
			"#ifdef GENERATOR_FILE
#error config.h is for the host, not build, machine.
#endif
")
	endif()

	set(GCC_CONFIG_CPU ${cpu})
	if(GCC_CONFIG_CPU)
		set(GCC_CONFIG_CPU_DEFAULT
			"#define TARGET_CPU_DEFAULT ${GCC_CONFIG_CPU}\n")
	endif()
	foreach(d ${defines})
		string(REGEX REPLACE "=.*" "" name "${d}")
		string(REGEX REPLACE "=" " " define "${d}")
		set(GCC_CONFIG_DEFINES
			"${GCC_CONFIG_DEFINES}#ifndef ${name}\n# define ${define}\n#endif\n")
	endforeach()

	set(GCC_CONFIG_HEADERS_LIST ${headers})
	if(GCC_CONFIG_HEADERS_LIST)
		list(GET GCC_CONFIG_HEADERS_LIST 0 FIRST_HEADER)
		if("${FIRST_HEADER}" MATCHES "^auto")
			set(GCC_CONFIG_AUTO "#include \"${FIRST_HEADER}\"\n")
			list(REMOVE_AT GCC_CONFIG_HEADERS_LIST 0)
		endif()
		foreach(f ${GCC_CONFIG_HEADERS_LIST})
			set(GCC_CONFIG_HEADERS "${GCC_CONFIG_HEADERS}# include \"${f}\"\n")
		endforeach()
	endif()

	if("${output}" MATCHES "^tm\\.h$")
		set(GCC_CONFIG_TM_H
			"#if defined IN_GCC && !defined GENERATOR_FILE && !defined USED_FOR_TARGET
# include \"insn-constants.h\"
# include \"insn-flags.h\"
#endif
")
	endif()

	configure_file(${GCCCONFIG_SOURCE_DIR}/gcc_config.h.in
		${GCC_BINARY_DIR}/gcc/${output}
		@ONLY
		IMMEDIATE)
endmacro()
