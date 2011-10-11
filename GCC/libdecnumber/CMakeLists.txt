configure_file("${GCC_SOURCE_DIR}/ungenerated/common/gstdint.h" "${GCC_BINARY_DIR}/libdecnumber/gstdint.h" COPY_ONLY)
configure_file("${GCC_SOURCE_DIR}/ungenerated/common/gstdint.h" "${GCC_BINARY_DIR}/gcc/gstdint.h" COPY_ONLY)

include_directories(BEFORE
	${GCC_SOURCE_DIR}/include
	${GCC_SOURCE_DIR}/libdecnumber/include
	${GCC_BINARY_DIR}/libdecnumber)
set(decnumber_SRCS
	decContext.c
	decNumber.c
	decUtility.c
	decimal128.c
	decimal32.c
	decimal64.c)	# source file count: 6
add_library(decnumber ${decnumber_SRCS})
#TARGET_LINK_LIBRARIES(decnumber iberty)
