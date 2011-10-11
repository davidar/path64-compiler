
#include_next <stdio.h>

#ifndef __MINGW64__
/* mingw w64 runtime conflicts with this, but not mingw32. */

#ifndef __STDIO_PROXY_H_
#define __STDIO_PROXY_H_

#ifdef __cplusplus
extern "C" {
#endif

extern int vasprintf(char**, const char*, __VALIST);
int asprintf(char **strp, const char *fmt, ...);

#define setlinebuf(stream) setvbuf(stream, 0, _IONBF, 0)

#ifdef __cplusplus
}
#endif

#endif /* __STDIO_PROXY_H_ */

#endif