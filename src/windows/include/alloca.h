
#ifdef __MINGW32__
#include <malloc.h>
#endif

#ifndef alloca
#	define alloca(s) __builtin_alloca(s)
#endif

