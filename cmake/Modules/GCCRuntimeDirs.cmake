# Looking for gcc runtime search path
SET(SOURCE_FILE "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/GetGccRuntimeDir/rt.cc")
FILE(WRITE "${SOURCE_FILE}" "#include <iostream>
int main(void)
{
        std::cout << \"\";
        return 0;
}
")

MESSAGE(STATUS "Checking for gcc runtime search path")

if(CMAKE_COMPILER_IS_GNUCXX)
	set(TEST_CXX_COMPILER ${CMAKE_CXX_COMPILER})
else()
	set(TEST_CXX_COMPILER g++)
endif()
try_compile(GXX_RUNTIME_RESULT
      ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}
      ${SOURCE_FILE}
      CMAKE_FLAGS -DCMAKE_EXE_LINKER_FLAGS:STRING=-v -DCMAKE_CXX_COMPILER=${TEST_CXX_COMPILER}
      OUTPUT_VARIABLE GXX_RUNTIME_OUTPUT)
message(STATUS "GXX_RUNTIME_RESULT ${GXX_RUNTIME_RESULT}")
message(STATUS "GXX_RUNTIME_OUTPUT ${GXX_RUNTIME_OUTPUT}")
if (GXX_RUNTIME_OUTPUT AND GXX_RUNTIME_RESULT)
	set(GCCRTDATA "${GXX_RUNTIME_OUTPUT}")
    STRING(REGEX MATCHALL "collect2.*$"
		    COLLECT2_COMMAND "${GCCRTDATA}")
    FILE(APPEND ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeOutput.log
	"collect2 command:\n"
	"${COLLECT2_COMMAND}\n")

    STRING(REGEX MATCHALL "-L\([^ ]*\)" LIBS "${COLLECT2_COMMAND}")

    FOREACH(libdir ${LIBS})
	STRING(REPLACE "-L" "" stripped_dir ${libdir})
	get_filename_component(stripped_dir "${stripped_dir}" ABSOLUTE)
	list(APPEND stripped_dirs_list ${stripped_dir})
    ENDFOREACH()
    
    # Since we keep the last directory we find the file in, reverse the
    # order of the search path.
    list(REVERSE stripped_dirs_list)

    FOREACH(stripped_dir ${stripped_dirs_list})
	IF(EXISTS "${stripped_dir}/libsupc++.a")
	    SET(CONFIGURED_LIBSUPCXX_DIR ${stripped_dir})
	ENDIF()
	IF(EXISTS "${stripped_dir}/libstdc++.a")
	    SET(CONFIGURED_LIBSTDCXX_DIR ${stripped_dir})
	ENDIF()
	IF(EXISTS "${stripped_dir}/libgcc.a")
	    SET(CONFIGURED_LIBGCC_DIR ${stripped_dir})
	ENDIF()
	IF(EXISTS "${stripped_dir}/libgcc_eh.a")
	    SET(CONFIGURED_LIBGCC_EH_DIR ${stripped_dir})
	ENDIF()
	IF(EXISTS "${stripped_dir}/libgcc_s.so")
	    SET(CONFIGURED_LIBGCC_S_DIR ${stripped_dir})
	ENDIF()
    ENDFOREACH()
endif()
