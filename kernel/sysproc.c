#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64 sys_exit(void)
{
    int n;
    argint(0, &n);
    exit(n);
    return 0; // not reached
}

uint64 sys_getpid(void) { return myproc()->pid; }

uint64 sys_fork(void) { return fork(); }

uint64 sys_wait(void)
{
    uint64 p;
    argaddr(0, &p);
    return wait(p);
}

uint64 sys_sbrk(void)
{
    uint64 addr;
    int n;

    argint(0, &n);
    addr = myproc()->sz;
    if (growproc(n) < 0)
        return -1;
    return addr;
}

uint64 sys_sleep(void)
{
    int n;
    uint ticks0;

    argint(0, &n);
    if (n < 0)
        n = 0;
    acquire(&tickslock);
    ticks0 = ticks;
    while (ticks - ticks0 < n)
    {
        if (killed(myproc()))
        {
            release(&tickslock);
            return -1;
        }
        sleep(&ticks, &tickslock);
    }
    release(&tickslock);
    backtrace();
    return 0;
}

uint64 sys_kill(void)
{
    int pid;

    argint(0, &pid);
    return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64 sys_uptime(void)
{
    uint xticks;

    acquire(&tickslock);
    xticks = ticks;
    release(&tickslock);
    return xticks;
}

uint64 sys_sigalarm(void)
{
    int alarm_interval; // Alarm interval in ticks
    uint64 handler_addr;

    argint(0, &alarm_interval);
    argaddr(1, &handler_addr);

    printf("Called sigalarm - interval: %d %d\n", alarm_interval, handler_addr);

    // Set flags on alarm
    struct proc *p = myproc();

    p->alarm_interval = alarm_interval;
    p->alarm_handler_addr = handler_addr;

    return 0;
}

uint64 sys_sigreturn(void)
{
    struct proc *p = myproc();

    // Restore trapframe
    memmove(p->trapframe, p->saved_trapframe, PGSIZE);
    kfree(p->saved_trapframe);

    p->alarm_ticks_passed = 0;
    p->alarm_handler_running = 0;

    return p->trapframe->a0;
}