
#ifdef __MINGW32__
#include <sys/types.h> /* for pid_t */
#endif
#include_next <fcntl.h>

struct flock {
    short  l_type;
    short  l_whence;
    off_t  l_start;
    off_t  l_len;
    pid_t  l_pid;
};

#define F_WRLCK 1
#define F_SETLKW 2

