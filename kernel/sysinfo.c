#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

// Get current system info
// addr is a user virtual address, pointing to a struct sysinfo
int systeminfo(uint64 addr)
{
    struct proc *p = myproc();
    struct sysinfo info;

    info.freemem = freemem();
    info.nproc = nproc();

    if (copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
    {
        return -1;
    }

    return 0;
}