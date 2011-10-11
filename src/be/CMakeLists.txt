include_directories(${PATHSCALE_BINARY_DIR}/common/targ_info
	${PATHSCALE_BINARY_DIR}/be
	be
	cg
	com
	lno
	opt
	prompf_anl
	region
	vho
	whirl2c
	whirl2f
	purple2
	.

	cg/gra_mon
	cg/swp

	../dfgforise
	../ipa/common
	../ipa/local
	../ipa/main/analyze
	../ipa/main/optimize

	../ipa_link/include

	../common/com
	../common/instrument
	../common/targ_info/access
	../common/util
	../common/stl
	../common/targ_info/parser/inc/

	${PATHSCALE_BINARY_DIR}/../GCC/gcc
	../include)


add_definitions(-funsigned-char ${PATH64_STDC_FORMAT_MACROS})

if(PATH64_ENABLE_VALGRIND)
	add_definitions(-DMONGOOSE_BE -DBACK_END -D__MIPS_AND_IA64_ELF_H)
else()
	add_definitions(-DNO_VALGRIND
		-DMONGOOSE_BE
		-DBACK_END
		-D__MIPS_AND_IA64_ELF_H)
endif()

add_subdirectory(com)
add_subdirectory(be)
add_subdirectory(cg)
add_subdirectory(lno)
add_subdirectory(opt)
add_subdirectory(whirl2c)
add_subdirectory(whirl2f)
add_subdirectory(prompf_anl)
