#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stddef.h>
#include <stdint.h>

/* kernel SVC IDs ---------------------------------------------------- */
#define SYS_EXIT   0x00
#define SYS_WRITE  0x08
#define SYS_READ   0x07     /* not used here – stub only */

/* ------------------------------ helpers --------------------------- */
#define SVC(num)  asm volatile("svc %0" :: "I"(num) : "memory")

/* _exit ­– never returns ------------------------------------------ */
void _exit(int status)
{
    register long x0 asm("x0") = status;
    SVC(SYS_EXIT);
    while (1) { }           /* for silence – should not return */
}

/* _write – fd, buf, len  ------------------------------------------- */
int _write(int fd, const void *buf, size_t len)
{
    register long x0 asm("x0") = (long)buf;   /* buffer pointer          */
    register long x1 asm("x1") = (long)len;   /* count                   */
    register long x2 asm("x2") = (long)fd;    /* file descriptor         */

    /* on return: x0 = result, x1 = error code (0 ⇒ success) */
    asm volatile("svc #0x08"
                 : "+r"(x0), "+r"(x1)
                 : "r"(x2)
                 : "memory");

    if (x1 != 0)     /* kernel put an error code in x1 */
    {
        errno = EINVAL;     /* quick-n-dirty mapping   */
        return -1;
    }
    return (int)x0;
}

/* minimal stubs so newlib links ----------------------------------- */
int  _read (int, void*, size_t)          { errno = ENOSYS; return -1; }
int  _close(int)                        { errno = ENOSYS; return -1; }
int  _lseek(int, off_t, int)            { errno = ENOSYS; return -1; }

extern char end;           /* provided by linker */
static char *heap_end;
void *_sbrk(ptrdiff_t incr)
{
    if (!heap_end) heap_end = &end;
    char *prev = heap_end;
    heap_end += incr;
    return prev;
}
