#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;

  if(proc->threadflag == 1)
      addr = proc->parent->sz;
  else
      addr = proc->sz;

  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int sys_yield(void)
{
    yield();
    return 0;
}

int sys_getlev(void)
{
    return proc->priority;
}

int sys_set_cpu_share(void)
{
    int cpu_share;

    if(argint(0, &cpu_share) < 0)
        return -1;

    return set_cpu_share(cpu_share);
}

int sys_thread_create(void)
{
    thread_t* thread;
    void* (*routine)(void*);
    void* arg;
    int temp;

    if(argint(0, &temp) < 0)
        return -1;

    thread = (thread_t*)temp;

    if(argint(1, &temp) < 0)
        return -1;

    routine = (void*)temp;

    if(argint(2, &temp) < 0)
        return -1;

    arg = (void*)temp;

    return thread_create(thread, routine, arg);
}

int sys_thread_exit(void)
{
    int temp;
    void* retval;
    if(argint(0, &temp) < 0)
        return -1;

    retval = (void*)temp;
    thread_exit(retval);
    return 0;
}

int sys_thread_join(void)
{
    int temp;
    thread_t thread;
    void** retval;
    
    if(argint(0, &temp) < 0)
        return -1;
    
    thread = (thread_t)temp;

    if(argint(1, &temp) < 0)
        return -1;

    retval = (void**)temp;
    
    return thread_join(thread,retval);
}
