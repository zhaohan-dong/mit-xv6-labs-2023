// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run
{
    struct run *next;
};

struct
{
    struct spinlock lock;
    struct run *freelist;
    int refcnt[(PHYSTOP - KERNBASE) / PGSIZE];
} kmem;

void kinit()
{
    printf("kinit: starting initialization\n");
    initlock(&kmem.lock, "kmem");
    char *p;
    for (p = end; p + PGSIZE <= (char *)PHYSTOP; p += PGSIZE)
    {
        kmem.refcnt[(uint64)p / PGSIZE] = 1;
    }

    freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end)
{
    char *p;
    p = (char *)PGROUNDUP((uint64)pa_start);
    for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE)
        kfree(p);
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa)
{
    struct run *r;
    uint64 page_num = (uint64)pa / PGSIZE;

    if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");

    acquire(&kmem.lock);

    if (kmem.refcnt[page_num] > 0)
    {
        kmem.refcnt[page_num]--;
    }

    if (kmem.refcnt[page_num] == 0)
    {
        release(&kmem.lock);
        // Fill with junk to catch dangling refs.
        memset(pa, 1, PGSIZE);

        r = (struct run *)pa;

        acquire(&kmem.lock);
        r->next = kmem.freelist;
        kmem.freelist = r;
    }

    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void)
{
    struct run *r;

    acquire(&kmem.lock);
    r = kmem.freelist;
    if (r)
    {
        kmem.freelist = r->next;
        kmem.refcnt[((uint64)r) / PGSIZE] = 1;
        memset((char *)r, 5, PGSIZE); // fill with junk
    }

    release(&kmem.lock);

    return (void *)r;
}

// Add reference count for this page
void add_ref(void *pa)
{
    uint64 page_num = (uint64)pa / PGSIZE;

    acquire(&kmem.lock);
    kmem.refcnt[page_num]++;
    release(&kmem.lock);
}
