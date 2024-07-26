// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define BCACHE_BUCKETS 13

struct bcache_bucket
{
    struct spinlock bucket_lock;
    struct buf buf[NBUF];
};

struct
{
    struct spinlock lock;
    struct bcache_bucket bucket[BCACHE_BUCKETS];
} bcache;

// Get bucket_id given block number
uint bucket_hash(uint blockno) { return blockno % BCACHE_BUCKETS; }

void binit(void)
{
    struct buf *b;

    initlock(&bcache.lock, "bcache");

    for (int bucket_id = 0; bucket_id < BCACHE_BUCKETS; bucket_id++)
    {
        initlock(&bcache.bucket[bucket_id].bucket_lock, "bucket_lock");
        for (b = bcache.bucket[bucket_id].buf;
             b < bcache.bucket[bucket_id].buf + NBUF; b++)
            initsleeplock(&b->lock, "buffer");
    }
}

// Find bucket_index given buf (not index of buf in that bucket though)
uint find_bucket_index(struct buf *b)
{
    for (int bucket_index = 0; bucket_index < BCACHE_BUCKETS; bucket_index++)
    {
        if (b >= bcache.bucket[bucket_index].buf &&
            b < (bcache.bucket[bucket_index].buf + NBUF))
        {
            return bucket_index;
        }
    }
    panic("findbucket");
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf *bget(uint dev, uint blockno)
{
    struct buf *b;
    uint bucket_index = bucket_hash(
        blockno); // Use the has function to get which bucket it is in

    // Acquire only lock on that bucket
    acquire(&bcache.bucket[bucket_index].bucket_lock);

    // Is the block already cached?
    for (int buf_index = 0; buf_index < NBUF; buf_index++)
    {
        b = &bcache.bucket[bucket_index].buf[buf_index];
        if (b->dev == dev && b->blockno == blockno)
        {
            b->refcnt++;
            release(&bcache.bucket[bucket_index].bucket_lock);
            acquiresleep(&b->lock);
            return b;
        }
    }

    // Block not cached, need to find a buffer to evict
    struct buf *dst = 0;
    for (int buf_index = 0; buf_index < NBUF; buf_index++)
    {
        b = &bcache.bucket[bucket_index].buf[buf_index];
        if (b->refcnt == 0)
        {
            dst = b;
            break;
        }
    }

    // If no buffer was found, evict the first buffer regardless of use
    if (!dst)
    {
        dst = &bcache.bucket[bucket_index]
                   .buf[0]; // Simple choice for demonstration
        if (dst->valid)
        {
            bwrite(dst);
        }
    }

    // Update the chosen buffer with new data
    dst->dev = dev;
    dst->blockno = blockno;
    dst->valid = 0;
    dst->refcnt = 1;

    release(&bcache.bucket[bucket_index].bucket_lock);
    acquiresleep(&dst->lock);

    return dst;
}

// Return a locked buf with the contents of the indicated block.
struct buf *bread(uint dev, uint blockno)
{
    struct buf *b;

    b = bget(dev, blockno);
    if (!b->valid)
    {
        virtio_disk_rw(b, 0);
        b->valid = 1;
    }
    return b;
}

// Write b's contents to disk.  Must be locked.
void bwrite(struct buf *b)
{
    if (!holdingsleep(&b->lock))
        panic("bwrite");
    virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void brelse(struct buf *b)
{
    if (!holdingsleep(&b->lock))
        panic("brelse");

    releasesleep(&b->lock);

    uint bucket_index = find_bucket_index(b);

    // acquire(&bcache.lock);
    acquire(&bcache.bucket[bucket_index].bucket_lock);
    b->refcnt--;
    // if (b->refcnt == 0)
    // {
    //     // no one is waiting for it.
    //     b->next->prev = b->prev;
    //     b->prev->next = b->next;
    //     b->next = bcache.head.next;
    //     b->prev = &bcache.head;
    //     bcache.head.next->prev = b;
    //     bcache.head.next = b;
    // }

    release(&bcache.bucket[bucket_index].bucket_lock);
    // release(&bcache.lock);
}

void bpin(struct buf *b)
{
    int bucket_index = find_bucket_index(b);
    // acquire(&bcache.lock);
    acquire(&bcache.bucket[bucket_index].bucket_lock);
    b->refcnt++;
    // release(&bcache.lock);
    release(&bcache.bucket[bucket_index].bucket_lock);
}

void bunpin(struct buf *b)
{
    int bucket_index = find_bucket_index(b);
    // acquire(&bcache.lock);
    acquire(&bcache.bucket[bucket_index].bucket_lock);
    b->refcnt--;
    // release(&bcache.lock);
    release(&bcache.bucket[bucket_index].bucket_lock);
}
