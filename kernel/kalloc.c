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

struct run {
  struct run *next;
};
// 接下来我们来实现获取空闲内存数量的函数。可用空间的判断在 kernel/kalloc.c 文件中。
// 这里定义了一个链表，每个链表都指向上一个可用空间，这里的 kmem 就是一个保存最后链表的变量。
struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}
// 这里把从 end (内核后的第一个地址) 到 PHYSTOP (KERNBASE + 128*1024*1024) 之间的物理空间以 PGSIZE 为单位全部初始化为 1 ，
// 然后每次初始化一个 PGSIZE 就把这个页挂在了 kmem.freelist 上，所以 kmem.freelist 永远指向最后一个可用页，那我们只要顺着这个链表往前走，
// 直到 NULL 为止。所以我们就可以在 kernel/kalloc.c 中新增函数 free_mem ，以获取空闲内存数量：
// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
// Return the number of bytes of free memory

uint64
count_freemem(void)
{
  acquire(&kmem.lock); //先锁内存管理结构，防止竞态条件出现

  uint64 mem_bytes = 0;
  struct run *p = kmem.freelist;
  while (p) {
    mem_bytes += PGSIZE;
    p = p->next;
  }

  release(&kmem.lock);
  return mem_bytes;
}