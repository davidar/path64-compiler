
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
find_library(PSC_WS2_32_LIBRARY ws2_32 libws2_32)
target_link_libraries(winlib ${PSC_WS2_32_LIBRARY})
