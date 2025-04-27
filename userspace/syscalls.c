#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>
#include <stdint.h>

/* ---------------------------------------------------------------------------
 *  Helpers : invoke the kernel’s SVC interface
 * --------------------------------------------------------------------------- */
static inline long __svc(long num, long arg0, long arg1, long arg2,
                         long arg3, long arg4, long arg5) {
  register long x8  asm("x8") = num;
  register long x0  asm("x0") = arg0;
  register long x1  asm("x1") = arg1;
  register long x2  asm("x2") = arg2;
  register long x3  asm("x3") = arg3;
  register long x4  asm("x4") = arg4;
  register long x5  asm("x5") = arg5;
  asm volatile("svc 0" : "+r"(x0) : "r"(x1),"r"(x2),"r"(x3),"r"(x4),"r"(x5),"r"(x8)
                          : "memory");
  return x0;
}

/* ---------------------------------------------------------------------------
 *  _write – maps to kernel syscall 2
 * --------------------------------------------------------------------------- */
int _write(int fd, const void *buf, size_t len) {
  (void)fd;                 /* only stdout for now */
  long ret = __svc(2, (long)buf, (long)len, 0,0,0,0);
  return (int)ret;
}

/* ---------------------------------------------------------------------------
 *  _exit – syscall 0 (will never return)
 * --------------------------------------------------------------------------- */
void _exit(int status) {
  __svc(0, status, 0,0,0,0,0);
  while(1) ;                /* should not get here */
}

/* ---------------------------------------------------------------------------
 *  Minimal (“do nothing”) stubs so Newlib is happy
 * --------------------------------------------------------------------------- */
int _close(int)                    { errno = ENOSYS; return -1; }
int _lseek(int, off_t, int)        { errno = ENOSYS; return -1; }
int _read(int, void *, size_t)     { errno = ENOSYS; return -1; }

extern char end;  // provided by hello.ld
static char *heap = &end;
static char *heap_end;

void *_sbrk(ptrdiff_t incr) {
  if (!heap_end) heap_end = heap;
  char *prev = heap_end;
  heap_end += incr;
  return prev;
}
