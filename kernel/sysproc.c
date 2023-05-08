#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_trace(void)
{
  int n;
  if (argint(0, &n) < 0) {
    return -1;
  }
  myproc()->trace_mask = n;
  return 0;
}
// 我们知道该函数其实就是把在内核地址 src 开始的 len 大小的数据拷贝到用户进程 pagetable 的虚地址 dstva 处，
// 所以 sys_sysinfo 函数实现里先用 argaddr 函数读进来我们要保存的在用户态的数据 sysinfo 的指针地址，
// 然后再把从内核里得到的 sysinfo 开始的内容以 sizeof(info) 大小的的数据复制到这个指针上。模仿上面的例子，
// 我们在 kernel/sysproc.c 文件中添加 sys_sysinfo 函数的具体实现如下：
uint64
sys_sysinfo(void)
{
  uint64 p;
  if (argaddr(0, &p) < 0) {
    return -1;
  }

  struct sysinfo sinfo;
  sinfo.freemem = count_freemem();
  sinfo.nproc = count_process();
  
  if (copyout(myproc()->pagetable, p, (char*)&sinfo, sizeof(sinfo)) < 0) {
    return -1;
  }
  return 0;
}
