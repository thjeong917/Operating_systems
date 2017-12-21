#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
getppid()
{
    return proc->parent->pid;
}

int sys_getppid()
{
    return getppid();
}
