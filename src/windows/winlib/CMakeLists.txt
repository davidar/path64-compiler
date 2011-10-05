
set(winlib_SRC
	"mmap.c"
	"readlink.c"
	"rlimit.c"
	"stubs.c"
	"symlink.c"
	"timersub.c"
	"times.c"
	"unix2winpath.c"
	"wait.c"
	"win2unixpath.c")

add_definitions(-D_WINLIB_ -DHAVE_CONFIG_H -D__MSVCRT_VERSION__=0x0601)

include_directories(BEFORE .)

add_library(winlib STATIC ${winlib_SRC})
target_link_libraries(winlib ws2_32)

