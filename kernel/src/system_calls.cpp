/* --------------------------------------------------------------------
 *  system_calls.cpp  –  unified handler
 * ------------------------------------------------------------------ */
#include "system_call.h"

#include "cores.h"
#include "event_loop.h"
#include "printf.h"
#include "process.h"

/* indices of general registers in the trap frame ------------------- */
enum { X0=0, X1, X2, X3, X4, X5, X6, X7, X8 /* … */ };

uint64_t system_call_handler(uint16_t esr_imm, uint64_t *regs)
{
    /* Prefer the Linux-style ABI (number in x8).       */
    uint64_t nr = regs[X8];

    /* If user put nothing in x8, fall back to imm16 so
       the tiny ‘hi’ demo (svc #2 / svc #0) still works. */
    if (nr == 0)
        nr = esr_imm;

    switch (nr)
    {
    /* --------------------------------------------------------------
     * 0  –  exit(status)
     * ------------------------------------------------------------ */
    case 0: {
        debug_printf("[kernel] exit(%lu)\n", regs[X0]);

        uint8_t  core = SMP::whichCore();
        activeProcess[core] = nullptr;

        /* never returns */
        event_loop();
        break;                          /* unreachable, keeps compiler happy */
    }

    /* --------------------------------------------------------------
     * 1  –  yield()
     * ------------------------------------------------------------ */
    case 1: {
        debug_printf("[kernel] yield()\n");

        uint8_t  core = SMP::whichCore();
        Process *p    = activeProcess[core];

        p->save_state(regs);
        activeProcess[core] = nullptr;

        __asm__ volatile("dmb sy" ::: "memory");

        schedule_event([p]{ p->run(); });
        event_loop();                   /* never returns */
        break;
    }

    /* --------------------------------------------------------------
     * 2  –  write(fd, buf, len)  (we honour only fd==1)
     * ------------------------------------------------------------ */
    case 2: {
        const char *buf = (const char *)regs[X0];
        uint64_t    len =               regs[X1];

        for (uint64_t i = 0; i < len; ++i)
            printf("%c", buf[i]);

        regs[X0] = len;                 /* return #bytes written */
        return 0;                       /* handled */
    }

    /* -------------------------------------------------------------- */
    default:
        printf("[kernel] unknown syscall %lu\n", nr);
        regs[X0] = (uint64_t)-1;        /* –1  →  ENOSYS for user-space */
        return 0;
    }
}
