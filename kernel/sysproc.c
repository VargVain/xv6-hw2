#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
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
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
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

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
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

  argint(0, &pid);
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
sys_getclk(void)
{
  return *(uint64*) CLINT_MTIME;
}

uint64
sys_shmget(void)
{
  return shmget();
}

uint64
sys_shmat(void)
{
  int id;
  uint64 addr;
  argint(0, &id);
  argaddr(1, &addr);
  return shmattach(id, (void *)addr);
}

uint64
sys_shmdat(void)
{
  int id;
  argint(0, &id);
  return shmdeattach(id);
}

uint64
sys_shmqueryperm(void)
{
  int id;
  argint(0, &id);
  return shmqueryperm(id);
}

uint64
sys_shmsetperm(void)
{
  int id, perm;
  argint(0, &id);
  argint(1, &perm);
  return shmsetperm(id, perm);
}