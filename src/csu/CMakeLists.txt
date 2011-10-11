

path64_is_target_enabled(res x86_64)
if(res)
	add_subdirectory(elf-x86_64)
endif()

path64_is_target_enabled(res x86_32)
if(res)
	add_subdirectory(elf-i386)
endif()

