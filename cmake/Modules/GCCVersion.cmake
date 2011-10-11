execute_process(COMMAND
	"gccx"
	"-dumpversion"
	OUTPUT_VARIABLE
	PSC_GCC_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE)

# FIXME: Something's broken here
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(PSC_GCC_VERSION 3.4.5)
endif()
