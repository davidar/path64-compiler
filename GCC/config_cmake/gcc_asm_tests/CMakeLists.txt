cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(GAT C)

set(GAT_AS as)

foreach(test weak weakref)
	add_custom_target(test_${test}
		echo
		DEPENDS
		${GAT_BINARY_DIR}/${test}.o)
	add_custom_command(OUTPUT
		${GAT_BINARY_DIR}/${test}.o
		COMMAND
		${GAT_AS}
		ARGS
		-o
		"${GAT_BINARY_DIR}/${test}.o"
		"${GAT_SOURCE_DIR}/${test}.s"
		DEPENDS
		${GAT_SOURCE_DIR}/${test}.s)
endforeach()
