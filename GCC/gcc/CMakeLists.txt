#-----------------------------------------------------------------------------
include("${GCCCONFIG_SOURCE_DIR}/gcc_make_config_h.cmake")

include_directories(BEFORE
	${GCC_SOURCE_DIR}/include
	${GCC_SOURCE_DIR}/libcpp/include
	${GCC_SOURCE_DIR}/libdecnumber
	${GCC_SOURCE_DIR}/gcc
	${GCC_SOURCE_DIR}/gcc/config
	${GCC_BINARY_DIR}/gcc
	${GCC_SOURCE_DIR}/libspin)

add_definitions(-DIN_GCC -DHAVE_CONFIG_H)

include_regular_expression("^([^ri]|i[^n]|in[^s]|ins[^n]|insn[^-]|r[^t]|rt[^l]|rtl[^.])")

if(PATH64_ENABLE_CXX)
	add_subdirectory(cp)
endif()

#-----------------------------------------------------------------------------
# Load the platform configuration.
if(EXISTS "${GCCCONFIG_BINARY_DIR}/gcc_platform.cmake")
	include("${GCCCONFIG_BINARY_DIR}/gcc_platform.cmake")
else()
	message(FATAL_ERROR "Cannot find gcc_platform.cmake.")
endif()

# Default the target-machine variables that were not explicitly set.
if(NOT tm_file)
	set(tm_file ${cpu_type}/${cpu_type}.h)
endif()
if(NOT md_file)
	set(md_file ${cpu_type}/${cpu_type}.md)
endif()
if(NOT out_file)
	set(out_file ${cpu_type}/${cpu_type}.c)
endif()

set(build_auto auto-host.h)	# or auto-build.h for cross compiling?

set(tm_file ${tm_file} defaults.h)
set(tm_p_file ${tm_p_file} tm-preds.h)
set(host_xm_file auto-host.h ansidecl.h ${host_xm_file})
set(build_xm_file ${build_auto} ansidecl.h ${build_xm_file})
set(xm_file auto-host.h ansidecl.h ${xm_file})

# Build file and include lists.
foreach(m host_ build_ "")
	set(${m}xm_include_list)
	set(${m}xm_file_list)
	foreach(f ${${m}xm_file})
		if("${f}" MATCHES "^ansidecl\\.h")
			list(APPEND ${m}xm_include_list ${f})
			list(APPEND ${m}xm_file_list ${GCC_SOURCE_DIR}/include/${f})
		elseif("${f}" MATCHES "^auto-(build|host)\\.h$")
			list(APPEND ${m}xm_include_list ${f})
			list(APPEND ${m}xm_file_list ${GCC_BINARY_DIR}/gcc/${f})
		else()
			list(APPEND ${m}xm_include_list config/${f})
			list(APPEND ${m}xm_file_list ${GCC_SOURCE_DIR}/gcc/config/${f})
		endif()
	endforeach()
endforeach()

set(tm_include_list options.h)
set(tm_file_list)
foreach(f ${tm_file})
	if("${f}" MATCHES "^defaults\\.h$")
		list(APPEND tm_include_list ${f})
		list(APPEND tm_file_list ${GCC_SOURCE_DIR}/gcc/${f})
	else()
		list(APPEND tm_include_list config/${f})
		list(APPEND tm_file_list ${GCC_SOURCE_DIR}/gcc/config/${f})
	endif()
endforeach()

set(tm_p_include_list)
set(tm_p_file_list)
foreach(f ${tm_p_file})
	if("${f}" MATCHES "^tm-preds\\.h$")
		list(APPEND tm_p_include_list ${f})
		list(APPEND tm_p_file_list ${GCC_SOURCE_DIR}/gcc/${f})
	else()
		list(APPEND tm_p_include_list config/${f})
		list(APPEND tm_p_file_list ${GCC_SOURCE_DIR}/gcc/config/${f})
	endif()
endforeach()

# Find the platform-specific sources.
macro(FIND_EXTRA_SRCS objs var)
	string(REGEX REPLACE "\\.o" ".c" EXTRA_SRCS "${objs}")
	foreach(f ${EXTRA_SRCS})
		if(EXISTS ${GCC_SOURCE_DIR}/gcc/config/${f})
			set(${var} ${${var}} ${GCC_SOURCE_DIR}/gcc/config/${f})
		else()
			if(EXISTS ${GCC_SOURCE_DIR}/gcc/config/${cpu_type}/${f})
				set(${var} ${${var}} ${GCC_SOURCE_DIR}/gcc/config/${cpu_type}/${f})
			else()
				if(EXISTS ${GCC_SOURCE_DIR}/gcc/${f})
					set(${var} ${${var}} ${GCC_SOURCE_DIR}/gcc/${f})
				else()
					message(SEND_ERROR "Cannot find platform-specific source ${f}.")
				endif()
			endif()
		endif()
	endforeach()
endmacro()
find_extra_srcs("${extra_objs}" extra_srcs)
find_extra_srcs("${c_target_objs}" c_target_srcs)
find_extra_srcs("${cxx_target_objs}" cxx_target_srcs)
find_extra_srcs("${out_host_hook_obj}" host_hook_srcs)

set(MD_FILE ${GCC_SOURCE_DIR}/gcc/config/${md_file})

#-----------------------------------------------------------------------------
# Generate configuration header files.

gcc_make_config_h(bconfig.h
	GCC_BCONFIG_H
	"${build_xm_defines}"
	"${build_xm_include_list}"
	"${target_cpu_default}")
gcc_make_config_h(config.h
	GCC_CONFIG_H
	"${host_xm_defines}"
	"${host_xm_include_list}"
	"${target_cpu_default}")
gcc_make_config_h(tconfig.h
	GCC_TCONFIG_H
	"USED_FOR_TARGET;${xm_defines}"
	"${xm_include_list}"
	"${target_cpu_default}")
gcc_make_config_h(tm_p.h
	GCC_TM_P_H
	""
	"${tm_p_include_list}"
	"${target_cpu_default}")
gcc_make_config_h(tm.h
	GCC_TM_H
	"${tm_defines}"
	"${tm_include_list}"
	"${target_cpu_default}")

#-----------------------------------------------------------------------------
# Rules to generate options.h and options.c

set(ALL_OPT_FILES
	${GCC_SOURCE_DIR}/gcc/c.opt
	${GCC_SOURCE_DIR}/gcc/common.opt)
foreach(f ${extra_options})
	list(APPEND ALL_OPT_FILES ${GCC_SOURCE_DIR}/gcc/config/${f})
endforeach()

# Find an awk tool.
if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows" AND NOT CYGWIN)
	set(AWK_EXECUTABLE "${GCC_SOURCE_DIR}/config_cmake/nawk.exe")
else()
	# Prefer GNU awk at any location over the native awk if possible.
	# The native awk on opensolaris causes opt-gather.awk to sort incorrectly.
	find_program(AWK_EXECUTABLE NAMES gawk)
	find_program(AWK_EXECUTABLE NAMES awk)
	mark_as_advanced(AWK_EXECUTABLE)
endif()

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/optionlist
	COMMAND
	${AWK_EXECUTABLE}
	-f
	${GCC_SOURCE_DIR}/gcc/opt-gather.awk
	${ALL_OPT_FILES}
	>
	optionlist
	DEPENDS
	${GCC_SOURCE_DIR}/gcc/opt-gather.awk
	${ALL_OPT_FILES})

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/options.h
	COMMAND
	${AWK_EXECUTABLE}
	-f
	${GCC_SOURCE_DIR}/gcc/opt-functions.awk
	-f
	${GCC_SOURCE_DIR}/gcc/opth-gen.awk
	<
	optionlist
	>
	options.h
	DEPENDS
	${GCC_SOURCE_DIR}/gcc/opt-functions.awk
	${GCC_SOURCE_DIR}/gcc/opth-gen.awk
	${GCC_BINARY_DIR}/gcc/optionlist)

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/options.c
	COMMAND
	${AWK_EXECUTABLE}
	-f
	${GCC_SOURCE_DIR}/gcc/opt-functions.awk
	-f
	${GCC_SOURCE_DIR}/gcc/optc-gen.awk
	-v
	"header_name=config.h system.h coretypes.h tm.h"
	<
	optionlist
	>
	options.c
	DEPENDS
	${GCC_SOURCE_DIR}/gcc/opt-functions.awk
	${GCC_SOURCE_DIR}/gcc/optc-gen.awk
	${GCC_BINARY_DIR}/gcc/optionlist)

add_custom_target(genoptions
	ALL
	DEPENDS
	${GCC_BINARY_DIR}/gcc/options.h
	${GCC_BINARY_DIR}/gcc/options.c)

#-----------------------------------------------------------------------------
# Rules to generate insn-modes.h, insn-modes.c, and min-insn-modes.c

add_executable(genmodes genmodes.c errors.c)
add_dependencies(genmodes genoptions)
target_link_libraries(genmodes iberty)
set_target_properties(genmodes
	PROPERTIES
	COMPILE_FLAGS
	-DGENERATOR_FILE)

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/insn-modes.h
	COMMAND
	genmodes
	-h
	>
	insn-modes.h
	DEPENDS
	genmodes)

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/insn-modes.c
	COMMAND
	genmodes
	>
	insn-modes.c
	DEPENDS
	genmodes)

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/min-insn-modes.c
	COMMAND
	genmodes
	-m
	>
	min-insn-modes.c
	DEPENDS
	genmodes)

#-----------------------------------------------------------------------------
# Rules to generate genrtl.h and genrtl.c.

add_executable(gengenrtl gengenrtl.c errors.c)
add_dependencies(gengenrtl genoptions)
target_link_libraries(gengenrtl iberty)
set_target_properties(gengenrtl
	PROPERTIES
	COMPILE_FLAGS
	-DGENERATOR_FILE)

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/genrtl.c
	COMMAND
	gengenrtl
	>
	genrtl.c
	DEPENDS
	gengenrtl)

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/genrtl.h
	COMMAND
	gengenrtl
	-h
	>
	genrtl.h
	DEPENDS
	gengenrtl)

#-----------------------------------------------------------------------------
# Generate gtyp-gen.h.
set(GTFILES_C
	c-lang.c
	c-tree.h
	c-decl.c
	c-common.c
	c-common.h
	c-pragma.c
	c-objc-common.c
	c-parser.c)

set(GTFILES_CXX
	cp/rtti.c
	cp/mangle.c
	cp/name-lookup.h
	cp/name-lookup.c
	cp/cp-tree.h
	cp/decl.h
	cp/call.c
	cp/decl.c
	cp/decl2.c
	cp/pt.c
	cp/repo.c
	cp/semantics.c
	cp/tree.c
	cp/parser.c
	cp/method.c
	cp/typeck2.c
	c-common.c
	c-common.h
	c-lex.c
	c-pragma.c
	cp/class.c
	cp/cp-objcp-common.c)

set(GTFILES
	real.h
	varray.h
	insn-addr.h
	hwint.h
	ipa-reference.h
	output.h
	cselib.h
	basic-block.h
	cgraph.h
	c-common.h
	c-tree.h
	reload.h
	alias.c
	bitmap.c
	cselib.c
	cgraph.c
	ipa-prop.c
	ipa-cp.c
	ipa-inline.c
	dbxout.c
	dwarf2out.c
	dwarf2asm.c
	dojump.c
	tree-profile.c
	emit-rtl.c
	except.c
	explow.c
	expr.c
	function.c
	except.h
	gcse.c
	integrate.c
	lists.c
	optabs.c
	profile.c
	regclass.c
	reg-stack.c
	cfglayout.c
	sdbout.c
	stor-layout.c
	stringpool.c
	tree.c
	varasm.c
	tree-mudflap.c
	tree-flow.h
	c-objc-common.c
	c-common.c
	c-parser.c
	tree-ssanames.c
	tree-eh.c
	tree-ssa-address.c
	tree-phinodes.c
	tree-cfg.c
	tree-dfa.c
	tree-ssa-propagate.c
	tree-iterator.c
	gimplify.c
	tree-chrec.h
	tree-vect-generic.c
	tree-ssa-operands.h
	tree-ssa-operands.c
	tree-profile.c
	tree-nested.c
	ipa-reference.c
	tree-ssa-structalias.h
	tree-ssa-structalias.c
	c-pragma.h
	omp-low.c
	targhooks.c
	cgraphunit.c)	# input.h coretypes.h # these files are listed in order below
# bitmap.h coverage.c rtl.h optabs.h tree.h function.h libfuncs.h

set(GTYP_GEN_LANG_FILES)
set(GTYP_GEN_LANGS_FOR_LANG_FILES)
set(GTYP_GEN_ALL_FILES)
foreach(f
	${GCC_SOURCE_DIR}/gcc/input.h
	${GCC_SOURCE_DIR}/gcc/coretypes.h
	${GCC_SOURCE_DIR}/libcpp/include/line-map.h
	${GCC_SOURCE_DIR}/libcpp/include/cpplib.h
	${GCC_SOURCE_DIR}/libcpp/include/cpp-id-data.h
	${host_xm_file_list}
	${tm_file_list}
	${GCC_BINARY_DIR}/gcc/options.h
	${GCC_SOURCE_DIR}/include/hashtab.h
	${GCC_SOURCE_DIR}/include/splay-tree.h
	${GCC_SOURCE_DIR}/gcc/bitmap.h
	${GCC_SOURCE_DIR}/gcc/coverage.c
	${GCC_SOURCE_DIR}/gcc/rtl.h
	${GCC_SOURCE_DIR}/gcc/optabs.h
	${GCC_SOURCE_DIR}/gcc/tree.h
	${GCC_SOURCE_DIR}/gcc/function.h
	${GCC_SOURCE_DIR}/gcc/libfuncs.h
	${GCC_SOURCE_DIR}/libcpp/include/symtab.h
	${GCC_SOURCE_DIR}/gcc/config/${out_file})
	set(GTYP_GEN_ALL_FILES "${GTYP_GEN_ALL_FILES}\"${f}\",\n")
endforeach()
foreach(f ${GTFILES})
	set(GTYP_GEN_ALL_FILES
		"${GTYP_GEN_ALL_FILES}\"${GCC_SOURCE_DIR}/gcc/${f}\",\n")
endforeach()
foreach(f ${GTFILES_CXX})
	set(GTYP_GEN_ALL_FILES
		"${GTYP_GEN_ALL_FILES}\"${GCC_SOURCE_DIR}/gcc/${f}\",\n")
	set(GTYP_GEN_LANG_FILES
		"${GTYP_GEN_LANG_FILES}\"${GCC_SOURCE_DIR}/gcc/${f}\",\n")
	set(GTYP_GEN_LANGS_FOR_LANG_FILES
		"${GTYP_GEN_LANGS_FOR_LANG_FILES}\"cp\",\n")
endforeach()
foreach(f ${GTFILES_C})
	set(GTYP_GEN_ALL_FILES
		"${GTYP_GEN_ALL_FILES}\"${GCC_SOURCE_DIR}/gcc/${f}\",\n")
	set(GTYP_GEN_LANG_FILES
		"${GTYP_GEN_LANG_FILES}\"${GCC_SOURCE_DIR}/gcc/${f}\",\n")
	set(GTYP_GEN_LANGS_FOR_LANG_FILES
		"${GTYP_GEN_LANGS_FOR_LANG_FILES}\"c\",\n")
endforeach()
foreach(f ${extra_srcs})
	set(GTYP_GEN_ALL_FILES "${GTYP_GEN_ALL_FILES}\"${f}\",\n")
endforeach()

configure_file(${GCCCONFIG_SOURCE_DIR}/gtyp-gen.h.in
	${GCC_BINARY_DIR}/gcc/gtyp-gen.h
	@ONLY
	IMMEDIATE)

#-----------------------------------------------------------------------------
# Rules to generate gtype-desc.h and gtype-desc.c.

add_executable(gengtype
	gengtype.c
	gengtype-lex.c
	gengtype-yacc.c
	errors.c)
add_dependencies(gengtype genoptions)
target_link_libraries(gengtype iberty)
set_target_properties(gengtype
	PROPERTIES
	COMPILE_FLAGS
	"-DGENERATOR_FILE -DYY_NO_UNISTD_H")

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/gtype-desc.c
	${GCC_BINARY_DIR}/gcc/gtype-desc.h
	COMMAND
	gengtype
	DEPENDS
	gengtype)

#-----------------------------------------------------------------------------
# Rules to generate tree-check.h.

add_executable(gencheck gencheck.c)
add_dependencies(gencheck genoptions)
target_link_libraries(gencheck iberty)
set_target_properties(gencheck
	PROPERTIES
	COMPILE_FLAGS
	-DGENERATOR_FILE)

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/tree-check.h
	COMMAND
	gencheck
	>
	tree-check.h
	DEPENDS
	gencheck)

#-----------------------------------------------------------------------------
# Sources used in several generator executables.

add_library(gen
	rtl.c
	read-rtl.c
	ggc-none.c
	vec.c
	gensupport.c
	print-rtl.c
	errors.c
	${GCC_BINARY_DIR}/gcc/min-insn-modes.c
	${GCC_BINARY_DIR}/gcc/insn-modes.h
	${GCC_BINARY_DIR}/gcc/tree-check.h
	${GCC_BINARY_DIR}/gcc/gtype-desc.h)
add_dependencies(gen genoptions)
target_link_libraries(gen iberty)
set_target_properties(gen PROPERTIES COMPILE_FLAGS -DGENERATOR_FILE)

#-----------------------------------------------------------------------------
# Rules to generate insn-constants.h.

add_executable(genconstants genconstants.c)
target_link_libraries(genconstants gen)
set_target_properties(genconstants
	PROPERTIES
	COMPILE_FLAGS
	-DGENERATOR_FILE)

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/insn-constants.h
	COMMAND
	genconstants
	${MD_FILE}
	>
	insn-constants.h
	DEPENDS
	genconstants
	${MD_FILE})

#-----------------------------------------------------------------------------
# Rules to generate tm-preds.h, tm-constrs.h, and insn-preds.c.

add_executable(genpreds genpreds.c)
target_link_libraries(genpreds gen)
set_target_properties(genpreds
	PROPERTIES
	COMPILE_FLAGS
	-DGENERATOR_FILE)

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/tm-preds.h
	COMMAND
	genpreds
	-h
	${MD_FILE}
	>
	tm-preds.h
	DEPENDS
	genpreds
	${MD_FILE})

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/tm-constrs.h
	COMMAND
	genpreds
	-c
	${MD_FILE}
	>
	tm-constrs.h
	DEPENDS
	genpreds
	${MD_FILE})

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/insn-preds.c
	COMMAND
	genpreds
	${MD_FILE}
	>
	insn-preds.c
	DEPENDS
	genpreds
	${MD_FILE})

#-----------------------------------------------------------------------------
# Rules to generate gencondmd.c.

add_executable(genconditions genconditions.c)
target_link_libraries(genconditions gen)
set_target_properties(genconditions
	PROPERTIES
	COMPILE_FLAGS
	-DGENERATOR_FILE)

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/gencondmd.c
	COMMAND
	genconditions
	${MD_FILE}
	>
	gencondmd.c
	DEPENDS
	genconditions
	${MD_FILE})

#-----------------------------------------------------------------------------
# Rules to generate insn-conditions.md.

set(MD_COND ${GCC_BINARY_DIR}/gcc/insn-conditions.md)

add_executable(gencondmd
	${GCC_BINARY_DIR}/gcc/gencondmd.c
	${GCC_BINARY_DIR}/gcc/insn-constants.h
	${GCC_BINARY_DIR}/gcc/tm-preds.h
	${GCC_BINARY_DIR}/gcc/tm-constrs.h)
target_link_libraries(gencondmd gen)
set_target_properties(gencondmd
	PROPERTIES
	COMPILE_FLAGS
	-DGENERATOR_FILE)

add_custom_command(OUTPUT
	${MD_COND}
	COMMAND
	gencondmd
	>
	${MD_COND}
	DEPENDS
	gencondmd)

#-----------------------------------------------------------------------------
# Rules to generate insn-config.h, insn-codes.h, insn-attr.h, and insn-flags.h.

foreach(tgt config codes attr flags)
	add_executable(gen${tgt} gen${tgt}.c)
	target_link_libraries(gen${tgt} gen)
	set_target_properties(gen${tgt}
		PROPERTIES
		COMPILE_FLAGS
		-DGENERATOR_FILE)

	add_custom_command(OUTPUT
		${GCC_BINARY_DIR}/gcc/insn-${tgt}.h
		COMMAND
		gen${tgt}
		${MD_FILE}
		${MD_COND}
		>
		insn-${tgt}.h
		DEPENDS
		gen${tgt}
		${MD_FILE}
		${MD_COND})
endforeach()

#-----------------------------------------------------------------------------
# Rules to generate insn-emit.c, insn-extract.c, insn-opinit.c,
# insn-peep.c, insn-output.c, insn-recog.c, insn-attrtab.c, and
# insn-automata.c.

set(genautomata_EXTRA_LIBS m)
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	if(NOT MINGW)
		set(genautomata_EXTRA_LIBS)
	else()
		find_library(PSC_LIBM_LIBRARY m libm libm.a m.a PATHS ${PSC_GCC_LIBRARY_DIRS} /mingw/lib)
		set(genautomata_EXTRA_LIBS ${PSC_LIBM_LIBRARY})
	endif()
endif()
foreach(tgt emit extract opinit peep output recog attrtab automata)
	add_executable(gen${tgt} gen${tgt}.c)
	target_link_libraries(gen${tgt} gen ${gen${tgt}_EXTRA_LIBS})
	set_target_properties(gen${tgt}
		PROPERTIES
		COMPILE_FLAGS
		-DGENERATOR_FILE)

	add_custom_command(OUTPUT
		${GCC_BINARY_DIR}/gcc/insn-${tgt}.c
		COMMAND
		gen${tgt}
		${MD_FILE}
		${MD_COND}
		>
		insn-${tgt}.c
		DEPENDS
		gen${tgt}
		${MD_FILE}
		${MD_COND})
endforeach()

#-----------------------------------------------------------------------------
# Rules to generate gcov-iov.h.

add_executable(gcov_iov gcov-iov.c)
set_target_properties(gcov_iov
	PROPERTIES
	COMPILE_FLAGS
	-DGENERATOR_FILE)

add_custom_command(OUTPUT
	${GCC_BINARY_DIR}/gcc/gcov-iov.h
	COMMAND
	gcov_iov
	'4.2.1'
	''
	>
	gcov-iov.h
	DEPENDS
	gcov_iov)

#-----------------------------------------------------------------------------
# Special flags for some source files.

set_source_files_properties(toplev.c
	PROPERTIES
	COMPILE_FLAGS
	"-DTARGET_NAME=${target}")

#-----------------------------------------------------------------------------
# Compiler backend library.


path64_set_multiarch_sources(backend_sources
	COMMON
	alias.c
	alloc-pool.c
	backend-print-rtl.c
	backend-rtl.c
	backend-vec.c
	bb-reorder.c
	bitmap.c
	bt-load.c
	builtins.c
	caller-save.c
	calls.c
	cfg.c
	cfganal.c
	cfgbuild.c
	cfgcleanup.c
	cfgexpand.c
	cfghooks.c
	cfglayout.c
	cfgloop.c
	cfgloopanal.c
	cfgloopmanip.c
	cfgrtl.c
	cgraph.c
	cgraphunit.c
	combine.c
	conflict.c
	convert.c
	coverage.c
	cse.c
	cselib.c
	dbxout.c
	ddg.c
	debug.c
	df-core.c
	df-problems.c
	df-scan.c
	dfp.c
	diagnostic.c
	dojump.c
	dominance.c
	domwalk.c
	dwarf2asm.c
	dwarf2out.c
	emit-rtl.c
	et-forest.c
	except.c
	explow.c
	expmed.c
	expr.c
	final.c
	flow.c
	fold-const.c
	function.c
	gcse.c
	genrtl.c
	ggc-common.c
	ggc-page.c
	gimple-low.c
	gimplify.c
	global.c
	graph.c
	haifa-sched.c
	hooks.c
	ifcvt.c
	integrate.c
	intl.c
	ipa-cp.c
	ipa-inline.c
	ipa-prop.c
	ipa-pure-const.c
	ipa-reference.c
	ipa-type-escape.c
	ipa-utils.c
	ipa.c
	jump.c
	lambda-code.c
	lambda-mat.c
	lambda-trans.c
	langhooks.c
	lcm.c
	lists.c
	local-alloc.c
	loop-doloop.c
	loop-init.c
	loop-invariant.c
	loop-iv.c
	loop-unroll.c
	loop-unswitch.c
	mode-switching.c
	modulo-sched.c
	omp-low.c
	optabs.c
	opts-common.c
	opts.c
	params.c
	passes.c
	pointer-set.c
	postreload-gcse.c
	postreload.c
	predict.c
	pretty-print.c
	print-tree.c
	profile.c
	real.c
	recog.c
	reg-stack.c
	regclass.c
	regmove.c
	regrename.c
	reload.c
	reload1.c
	reorg.c
	resource.c
	rtl-error.c
	rtl-factoring.c
	rtlanal.c
	rtlhooks.c
	sbitmap.c
	sched-deps.c
	sched-ebb.c
	sched-rgn.c
	sched-vis.c
	sdbout.c
	see.c
	simplify-rtx.c
	sreal.c
	stmt.c
	stor-layout.c
	stringpool.c
	struct-equiv.c
	targhooks.c
	timevar.c
	toplev.c
	tracer.c
	tree-cfg.c
	tree-cfgcleanup.c
	tree-complex.c
	tree-dfa.c
	tree-dump.c
	tree-eh.c
	tree-gimple.c
	tree-if-conv.c
	tree-inline.c
	tree-into-ssa.c
	tree-iterator.c
	tree-loop-linear.c
	tree-nested.c
	tree-nomudflap.c
	tree-nrv.c
	tree-object-size.c
	tree-optimize.c
	tree-outof-ssa.c
	tree-phinodes.c
	tree-pretty-print.c
	tree-profile.c
	tree-sra.c
	tree-ssa-address.c
	tree-ssa-alias.c
	tree-ssa-ccp.c
	tree-ssa-copy.c
	tree-ssa-copyrename.c
	tree-ssa-dce.c
	tree-ssa-dom.c
	tree-ssa-dse.c
	tree-ssa-forwprop.c
	tree-ssa-live.c
	tree-ssa-loop-ch.c
	tree-ssa-loop-im.c
	tree-ssa-loop-ivcanon.c
	tree-ssa-loop-ivopts.c
	tree-ssa-loop-manip.c
	tree-ssa-loop-niter.c
	tree-ssa-loop-prefetch.c
	tree-ssa-loop-unswitch.c
	tree-ssa-loop.c
	tree-ssa-math-opts.c
	tree-ssa-operands.c
	tree-ssa-phiopt.c
	tree-ssa-pre.c
	tree-ssa-propagate.c
	tree-ssa-reassoc.c
	tree-ssa-sink.c
	tree-ssa-structalias.c
	tree-ssa-threadedge.c
	tree-ssa-threadupdate.c
	tree-ssa-uncprop.c
	tree-ssa.c
	tree-ssanames.c
	tree-stdarg.c
	tree-tailcall.c
	tree-vect-analyze.c
	tree-vect-generic.c
	tree-vect-patterns.c
	tree-vect-transform.c
	tree-vectorizer.c
	tree-vn.c
	tree-vrp.c
	tree.c
	value-prof.c
	var-tracking.c
	varasm.c
	varray.c
	version.c
	vmsdbgout.c
	web.c
	xcoffout.c

	${GCC_BINARY_DIR}/gcc/options.c
	${GCC_SOURCE_DIR}/gcc/config/${out_file}
	${extra_srcs}
	${host_hook_srcs}
	${c_target_srcs}
	${cxx_target_srcs}
	${GCC_BINARY_DIR}/gcc/gtype-desc.c
	${GCC_BINARY_DIR}/gcc/insn-attrtab.c
	${GCC_BINARY_DIR}/gcc/insn-emit.c
	${GCC_BINARY_DIR}/gcc/insn-modes.c
	${GCC_BINARY_DIR}/gcc/insn-extract.c
	${GCC_BINARY_DIR}/gcc/insn-opinit.c
	${GCC_BINARY_DIR}/gcc/insn-output.c
	${GCC_BINARY_DIR}/gcc/insn-peep.c
	${GCC_BINARY_DIR}/gcc/insn-recog.c
	${GCC_BINARY_DIR}/gcc/insn-preds.c
	${GCC_BINARY_DIR}/gcc/insn-automata.c
	${GCC_BINARY_DIR}/gcc/insn-flags.h
	${GCC_BINARY_DIR}/gcc/insn-config.h
	${GCC_BINARY_DIR}/gcc/insn-codes.h
	${GCC_BINARY_DIR}/gcc/insn-attr.h
	${GCC_BINARY_DIR}/gcc/genrtl.h
	${GCC_BINARY_DIR}/gcc/gcov-iov.h
	# Seen in CMakeLists.txt: 227 files listed for libbackend.

	# Files that are missing in list above
	double-int.c
	tree-chrec.c
	tree-data-ref.c
	tree-scalar-evolution.c)
path64_add_multiarch_library(backend STATIC backend_sources)


configure_file(${GCC_SOURCE_DIR}/gcc/path64_prefix.h.in
	${GCC_BINARY_DIR}/gcc/path64_prefix.h
	@ONLY
	IMMEDIATE)


path64_set_multiarch_sources(cc142_sources
	COMMON
	c-lang.c
	c-typeck.c
	c-decl.c
	c-parser.c
	c-convert.c
	c-objc-common.c
	stub-objc.c
	attribs.c
	c-errors.c
	c-lex.c
	c-pragma.c
	c-aux-info.c
	c-common.c
	c-opts.c
	c-format.c
	c-semantics.c
	c-incpath.c
	cppdefault.c
	c-ppoutput.c
	c-cppbuiltin.c
	prefix.c
	c-dump.c
	c-pch.c
	c-gimplify.c
	tree-mudflap.c
	c-pretty-print.c
	c-omp.c
	main.c
	dummy-checksum.c)

path64_add_multiarch_executable(cc142 cc142_sources)

path64_multiarch_link_multiarch_libraries(cc142 backend)
path64_multiarch_link_libraries(cc142 cpp decnumber iberty)
path64_multiarch_link_multiarch_libraries(cc142 spin)
path64_install_multiarch(cc142)
path64_add_dependencies_from_multiarch(compiler-stage cc142)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	path64_multiarch_link_libraries(cc142 winlib)
endif()


## changed pending verification
#ADD_LIBRARY(backend
#  double-int.c tree-chrec.c tree-scalar-evolution.c tree-data-ref.c
#  tree-cfg.c tree-dfa.c tree-eh.c tree-ssa.c tree-optimize.c
#  tree-gimple.c gimplify.c tree-pretty-print.c tree-into-ssa.c
#  tree-outof-ssa.c tree-ssa-ccp.c tree-vn.c tree-ssa-uncprop.c
#  tree-ssa-dce.c tree-ssa-copy.c tree-nrv.c tree-ssa-copyrename.c
#  tree-ssa-pre.c tree-ssa-live.c tree-ssa-operands.c tree-ssa-alias.c
#  tree-ssa-phiopt.c tree-ssa-forwprop.c tree-nested.c tree-ssa-dse.c
#  tree-ssa-dom.c domwalk.c tree-tailcall.c gimple-low.c tree-iterator.c
#  omp-low.c tree-phinodes.c tree-ssanames.c tree-sra.c tree-complex.c
#  tree-vect-generic.c tree-ssa-loop.c tree-ssa-loop-niter.c
#  tree-ssa-loop-manip.c tree-ssa-threadupdate.c tree-ssa-threadedge.c
#  tree-vectorizer.c tree-vect-analyze.c tree-vect-transform.c
#  tree-vect-patterns.c tree-ssa-loop-prefetch.c tree-ssa-loop-ivcanon.c
#  tree-ssa-propagate.c tree-ssa-address.c tree-ssa-math-opts.c
#  tree-ssa-loop-ivopts.c tree-if-conv.c tree-ssa-loop-unswitch.c alias.c
#  bb-reorder.c bitmap.c builtins.c caller-save.c calls.c cfg.c cfganal.c
#  cfgbuild.c cfgcleanup.c cfglayout.c cfgloop.c cfgloopanal.c
#  cfgloopmanip.c loop-init.c loop-unswitch.c loop-unroll.c cfgrtl.c
#  combine.c conflict.c convert.c coverage.c cse.c cselib.c dbxout.c
#  ddg.c tree-ssa-loop-ch.c loop-invariant.c tree-ssa-loop-im.c debug.c
#  df-core.c df-problems.c df-scan.c dfp.c diagnostic.c dojump.c
#  dominance.c loop-doloop.c dwarf2asm.c dwarf2out.c emit-rtl.c except.c
#  explow.c loop-iv.c expmed.c expr.c final.c flow.c fold-const.c
#  function.c gcse.c genrtl.c ggc-common.c global.c graph.c
#  haifa-sched.c hooks.c ifcvt.c
#  integrate.c intl.c jump.c langhooks.c lcm.c lists.c local-alloc.c
#  mode-switching.c modulo-sched.c optabs.c
#  ${GCC_BINARY_DIR}/gcc/options.c
#  opts.c opts-common.c params.c postreload.c postreload-gcse.c predict.c
#  pointer-set.c backend-print-rtl.c print-tree.c
#  profile.c value-prof.c var-tracking.c real.c recog.c reg-stack.c
#  regclass.c regmove.c regrename.c reload.c reload1.c reorg.c resource.c
#  backend-rtl.c rtlanal.c rtl-error.c sbitmap.c sched-deps.c sched-ebb.c
#  sched-rgn.c sched-vis.c sdbout.c see.c simplify-rtx.c sreal.c stmt.c
#  stor-layout.c stringpool.c struct-equiv.c targhooks.c timevar.c
#  toplev.c tracer.c tree.c tree-dump.c varasm.c varray.c
#  backend-vec.c version.c
#  vmsdbgout.c xcoffout.c alloc-pool.c et-forest.c cfghooks.c bt-load.c
#  pretty-print.c ggc-page.c web.c passes.c tree-profile.c rtlhooks.c
#  cfgexpand.c lambda-mat.c lambda-trans.c lambda-code.c
#  tree-loop-linear.c tree-ssa-sink.c tree-vrp.c tree-stdarg.c
#  tree-cfgcleanup.c tree-ssa-reassoc.c tree-ssa-structalias.c
#  tree-object-size.c rtl-factoring.c
#
#  ${GCC_SOURCE_DIR}/gcc/config/${out_file} ${extra_srcs} ${host_hook_srcs}
#
#  tree-inline.c cgraph.c cgraphunit.c tree-nomudflap.c ipa.c
#  ipa-inline.c ipa-utils.c ipa-reference.c ipa-pure-const.c
#  ipa-type-escape.c ipa-prop.c ipa-cp.c
#
#  ${GCC_BINARY_DIR}/gcc/gtype-desc.c
#  ${GCC_BINARY_DIR}/gcc/insn-attrtab.c
#  ${GCC_BINARY_DIR}/gcc/insn-emit.c
#  ${GCC_BINARY_DIR}/gcc/insn-modes.c
#  ${GCC_BINARY_DIR}/gcc/insn-extract.c
#  ${GCC_BINARY_DIR}/gcc/insn-opinit.c
#  ${GCC_BINARY_DIR}/gcc/insn-output.c
#  ${GCC_BINARY_DIR}/gcc/insn-peep.c
#  ${GCC_BINARY_DIR}/gcc/insn-recog.c
#  ${GCC_BINARY_DIR}/gcc/insn-preds.c
#  ${GCC_BINARY_DIR}/gcc/insn-automata.c
#
#  ${GCC_BINARY_DIR}/gcc/insn-flags.h
#  ${GCC_BINARY_DIR}/gcc/insn-config.h
#  ${GCC_BINARY_DIR}/gcc/insn-codes.h
#  ${GCC_BINARY_DIR}/gcc/insn-attr.h
#  ${GCC_BINARY_DIR}/gcc/genrtl.h
#  ${GCC_BINARY_DIR}/gcc/gcov-iov.h
#  )
