/* Newlib minimal runtime syscall stubs for Cortex-M bare-metal */

#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include "pal.h"

extern char _end; /* Defined by linker script */

int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file; (void)ptr; (void)dir;
    return 0;
}

int _read(int file, char *ptr, int len)
{
    (void)file; (void)ptr; (void)len;
    return 0;
}

int _write(int file, char *ptr, int len)
{
    (void)file;
    for (int i = 0; i < len; i++) {
        if (ptr[i] == '\n') {
            PAL_UART_WriteChar('\r');
        }
        PAL_UART_WriteChar(ptr[i]);
    }
    return len;
}

void *_sbrk(ptrdiff_t incr)
{
    static char *heap_end = NULL;
    char *prev_heap_end;

    if (heap_end == NULL) {
        heap_end = &_end;
    }
    prev_heap_end = heap_end;

    heap_end += incr;
    return (void *)prev_heap_end;
}

int _getpid(void)
{
    return 1;
}

int _kill(int pid, int sig)
{
    (void)pid; (void)sig;
    errno = EINVAL;
    return -1;
}

void _exit(int status)
{
    (void)status;
    while (1) {
        __asm__("BKPT #0");
    }
}
