// ELE3021 Operating System
// 2013011695 
// Jeong, Tae-Hwa
// 2017-05-21 (SUN) 22:00
// Light-weight process implementation

/*
************ Design explanation ***********

There are 3 system calls to create and end threads, 'thread_create()', 'thread_exit()', 'thread_join()';
Each process has a thread id called 'tid', and default is 1. Only thread checks tid, process does not.
Also, for 'retval', each process(thread) will store its return value at 't_retval'.
'thread_create()' creates thread, and copies information of process except stack address. And allocates
stack address of thread itself in same address space of parent process.
'thread_exit()' gets return value of thread, and passes it out.
'thread_join()' wait for the thread specified by the argument to terminate. This cleans up the resources 
allocated to the thread such as a page table, allocated memories and stacks. This gets the return value 
of thread through 'thread_exit()'.

*/


// ELE3021 Operating System
// 2013011695 
// Jeong, Tae-Hwa
// 2017-04-22 (SAT) 15:53
// MLFQ & stride scheduling Implementation

/*
************ Design explanation ***********

Each process has a variable called 'mode'.
If mode is 0, it goes to MLFQ scheduling.
If mode is 1, it goes to stride scheduling.
By declaring 'pass' and 'stride' to each scheduling function, I made each scheduler to be executed with
amount of percentage testcase required.

In MLFQ scheduling, each process has 'priority' and 'ticknum'. Searching through ptable, 
I set 'lowest' so that scheduler can only execute lowest queue.
In each priority queue, ticknum will be reseted so that it can execute for each queue's time quantum.
After checking totaltick exceeds 100 or not, Priority Boost will be executed and every processes' priority
and ticknum will be 0.

In stride scheduling, each process has 'stride' and 'pass'.
Stride is result of (1000/cpu_share). Pass will be the sum of stride in each process.
All I had to do was just compare each process's pass and add one more stride to process who has lower pass.

*/

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

int lowest=100; // lowest priority for MLFQ scheduling
int totaltick=0; // # of tick for priority boost
int tickratio[3]={5, 10, 20}; // time quantum of each priority queue
int cpu_share=0; // percentage of input cpu time
double minpass=0;  // lowest pass for stride scheduling
double totalcpu=0; // to prevent stride scheduling exceeds 80%
int MLFQ_pass=0; // pass for MLFQ scheduling
int stride_pass=0; // pass for stride scheduling
//int totalthread1=0;
//int cpushareflag = 0;

struct {
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
int nexttid = 1;

extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
    initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.

static struct proc*
allocproc(void)
{
    struct proc *p;
    char *sp;
    //int i;

    acquire(&ptable.lock);

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if(p->state == UNUSED)
            goto found;

    release(&ptable.lock);
    return 0;

found:
    p->state = EMBRYO;
    p->pid = nextpid++;
    //p->tid = -1;
    p->tid = 0;
    p->threadflag = 0;

    /*    
    for(i = 0; i < 64; i++){
        p->threadYN[i] = 0;
    }
    */

    release(&ptable.lock);

    // Allocate kernel stack.
    if((p->kstack = kalloc()) == 0){
        p->state = UNUSED;
        return 0;
    }
    sp = p->kstack + KSTACKSIZE;

    // Leave room for trap frame.
    sp -= sizeof *p->tf;
    p->tf = (struct trapframe*)sp;

    // Set up new context to start executing at forkret,
    // which returns to trapret.
    sp -= 4;
    *(uint*)sp = (uint)trapret;

    sp -= sizeof *p->context;
    p->context = (struct context*)sp;
    memset(p->context, 0, sizeof *p->context);
    p->context->eip = (uint)forkret;

    return p;
}

//PAGEBREAK: 32
// Set up first user process.
    void
userinit(void)
{
    struct proc *p;
    extern char _binary_initcode_start[], _binary_initcode_size[];

    p = allocproc();

    initproc = p;
    if((p->pgdir = setupkvm()) == 0)
        panic("userinit: out of memory?");
    inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
    p->sz = PGSIZE;
    memset(p->tf, 0, sizeof(*p->tf));
    p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
    p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
    p->tf->es = p->tf->ds;
    p->tf->ss = p->tf->ds;
    p->tf->eflags = FL_IF;
    p->tf->esp = PGSIZE;
    p->tf->eip = 0;  // beginning of initcode.S

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");

    // this assignment to p->state lets other cores
    // run this process. the acquire forces the above
    // writes to be visible, and the lock is also needed
    // because the assignment might not be atomic.
    acquire(&ptable.lock);

    p->state = RUNNABLE;

    release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
    uint sz;
    
    if(proc->threadflag == 1)
        sz = proc->parent->sz;
    else
        sz = proc->sz;

    //cprintf("HI\n");

    if(n > 0){
        if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
            return -1;
    } else if(n < 0){
        if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
            return -1;
    }
    //cprintf("HELLO\n");

    if(proc->threadflag == 1){
        proc->parent->sz = sz;
    }
    else{
        proc->sz = sz;
    }
    
    switchuvm(proc);
    
    return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
    int
fork(void)
{
    int i, pid;
    struct proc *np;

    // Allocate process.
    if((np = allocproc()) == 0){
        return -1;
    }
/*
    for(;;){
        if(proc->threadflag == 1){
            proc = proc->parent;
            continue;
        }
        else
            break;
    }
*/
    // Copy process state from p.
    if(proc->threadflag == 0){
        if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
            kfree(np->kstack);
            np->kstack = 0;
            np->state = UNUSED;
            return -1;
        }
    }
    else if(proc->threadflag == 1){
        if((np->pgdir = copyuvm2(proc->pgdir, proc->sz)) == 0){
            kfree(np->kstack);
            np->kstack = 0;
            np->state = UNUSED;
            //deallocuvm(np->pgdir, np->threadbase + 2*PGSIZE, np->threadbase);
            return -1;
        }
    }
    np->sz = proc->sz;
    np->parent = proc;
    *np->tf = *proc->tf;
    np->threadflag = 0;

    // Clear %eax so that fork returns 0 in the child.
    np->tf->eax = 0;

    for(i = 0; i < NOFILE; i++)
        if(proc->ofile[i])
            np->ofile[i] = filedup(proc->ofile[i]);
    np->cwd = idup(proc->cwd);

    safestrcpy(np->name, proc->name, sizeof(proc->name));

    pid = np->pid;

    acquire(&ptable.lock);

    np->state = RUNNABLE;

    release(&ptable.lock);

    return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
    struct proc *p;
    int fd;

    if(proc == initproc)
        panic("init exiting");
    
    //cprintf("exit into\n");
    
    if(proc->threadflag == 1){
        for(fd = 0; fd < NOFILE; fd++){
            if(proc->parent->ofile[fd]){
                fileclose(proc->parent->ofile[fd]);
                proc->parent->ofile[fd] = 0;
            }
        }
        begin_op();
        iput(proc->parent->cwd);
        end_op();
        proc->parent->cwd = 0;
    }
    
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->pid == proc->pid && p->threadflag == 1){
            for(fd = 0; fd < NOFILE; fd++){
                if(p->ofile[fd]){
                    fileclose(p->ofile[fd]);
                    p->ofile[fd] = 0;
                }
            }
            begin_op();
            iput(p->cwd);
            end_op();
            p->cwd = 0; 
        }
    }

    // Close all open files.
    for(fd = 0; fd < NOFILE; fd++){
        if(proc->ofile[fd]){
            fileclose(proc->ofile[fd]);
            proc->ofile[fd] = 0;
        }
    }
    begin_op();
    iput(proc->cwd);
    end_op();
    proc->cwd = 0;

    acquire(&ptable.lock);
    
    if(proc->threadflag == 0)
        wakeup1(proc->parent);
    else
        wakeup1(proc->parent->parent);
    
    // if i am thread, make me orphan and
    // also change parent to zombie because
    // proc->parent->parent woke up.
    if(proc->threadflag == 1){
        proc->t_orphan = 1;
        proc->parent->state = ZOMBIE;
    }

    // terminate all abandoned children through ptable.
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        // if caller is thread
        if(proc->threadflag == 1){
            // change other threads(brothers) to orphan
            if(p->threadflag == 1 && p->pid == proc->pid){
                p->t_orphan = 1;
                p->state = ZOMBIE;
            }
        }
        // if caller is process
        if(proc == p->parent){
            // when child is thread
            if(p->threadflag == 1){
                p->t_orphan = 1;
                p->state = ZOMBIE;
            }
            // when child is just process, pass abandoned children
            // to init.
            p->parent = initproc;
            if(p->state == ZOMBIE){
                wakeup1(initproc);
            }
        }
    } 

    //cprintf("went through for loop\n");
    
    // In stride scheduling, initialize total and pass
    // to prevent error when percentage exceeds 80%
    totalcpu -= proc->cpushare; 
    proc->cpushare = 0;
    proc->pass = 0;
    // Jump into the scheduler, never to return.
    proc->state = ZOMBIE;
    sched();
    panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
    struct proc *p;
    int havekids, pid;
    //int fd;
 
    acquire(&ptable.lock);
    for(;;){
        // Scan through table looking for exited children.
        havekids = 0;
        
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->t_orphan == 1 && p->threadflag == 1 && p->state == ZOMBIE){
                pid = p->pid;
                deallocuvm(p->pgdir, p->threadbase + 2*PGSIZE, p->threadbase);
                kfree(p->kstack);
                p->kstack = 0;
                p->pid = 0;
                p->threadflag = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                p->state = UNUSED;
                p->t_orphan = 0;
                //cprintf("orphan dealloc\n");
            }
        }
        //cprintf("orphan ended!\n");
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->parent != proc)
                continue;
            havekids = 1;
           
            if(p->state == ZOMBIE && p->threadflag == 0){
                // Found one.
                pid = p->pid;
                kfree(p->kstack);
                p->kstack = 0;
                freevm(p->pgdir);
                p->pid = 0;
                p->parent = 0;
                p->name[0] = 0;
                p->killed = 0;
                p->state = UNUSED;

                release(&ptable.lock);
                //cprintf("right before return & pid = %d\n",pid);
                return pid;
            }
        }
        
        //cprintf("zombie ended!\n");

        // No point waiting if we don't have any children.
        if(!havekids || proc->killed){
            release(&ptable.lock);
            return -1;
        }
        
        //cprintf("bf slp done\n");
        
        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(proc, &ptable.lock);  //DOC: wait-sleep
        
        //cprintf("sleep done?!\n");
    }
}

// function for MLFQ scheduling
// Lowest queue should executed first.
// This means Queue 0 has highest priority
void 
MLFQ_sched(void)
{
    struct proc* p;

    int MLFQ_stride = (1000/(100-totalcpu));
    MLFQ_pass += MLFQ_stride;

    if(totaltick >= 100){
        // cprintf("boost!\n");
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->state != RUNNABLE)
                continue;
            // cprintf("ticknum : %d, total : %d\n",p->ticknum,totaltick);
            p->priority = 0;
            p->ticknum = 0;
        }
        totaltick = 0;
    }

    lowest=100;

    // search ptable to find lowest queue
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state != RUNNABLE)
            continue;
        if(p->priority < lowest)
            lowest = p->priority;
        //cprintf("lowest : %d\n", lowest);
    }
    
    // execute processes in lowest queue
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state != RUNNABLE || p->mode != 0)
            continue;

        if(p->priority != lowest)
            continue;
        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
        proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&cpu->scheduler, p->context);
        switchkvm();

        // cprintf("pid : %d, lowest : %d, priority : %d, ticknum : %d, total : %d\n",p->pid, lowest, p->priority, p->ticknum, totaltick);

        totaltick++;
        p->ticknum++;
        proc = 0;
        
        // when priority boost is required
        if(totaltick==100)
            break;
        
        // for Queue 0
        if(p->priority == 0 && p->ticknum == tickratio[0]){
            p->priority ++;
            p->ticknum = 0;
            break;
        }
        // for Queue 1
        else if(p->priority == 1 && p->ticknum == tickratio[1]){
            p->priority ++;
            p->ticknum = 0;
            break;
        }
        // for Queue 2
        else if(p->priority == 2 && p->ticknum == tickratio[2]){
            p->ticknum = 0;
            break;
        }
        // make each process executed for its time quantum
        else{
            p--;
            break;
        }

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        //proc = 0;

    }
}

//function for stride scheduling
void 
stride_sched(void)
{
    struct proc* p;
    double minpass;

    int stride_stride = (1000/(totalcpu+1));
    stride_pass += stride_stride;

    minpass = 10000000;
    
    // search ptable to find minpass
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state != RUNNABLE || p->mode != 1)
            continue;
        if(p->pass < minpass)
            minpass = p->pass;
        //cprintf("pid : %d, stride : %d, p->pass : %d, minpass : %d\n", p->pid, p->stride, p->pass, minpass);
    }
    
    // initialize pass of every process when new process is found
    if(minpass == 0){
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
            if(p->state != RUNNABLE || p->mode != 1)
                continue;
            p->pass = 0;
        }
    }

    // execute process who has lower pass amount
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state != RUNNABLE || p->mode != 1)
            continue;
        if(p->pass != minpass)
            continue;

        proc = p;
        switchuvm(p);
        p->state = RUNNING;
        swtch(&cpu->scheduler, p->context);
        switchkvm();

        p->pass = p->pass + p->stride;
        //cprintf("totalcpu : %d\n", totalcpu); 
        proc = 0;
    }
    //cpushareflag = 0;
}


//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
   
    for(;;){
        // Enable interrupts on this processor.
        sti();
        // Loop over process table looking for process to run.
        acquire(&ptable.lock);
        
        // Choose to execute which scheduling by comparing pass
        if(MLFQ_pass <= stride_pass)
            MLFQ_sched();
        else
            stride_sched();

        release(&ptable.lock);
    }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
    void
sched(void)
{
    int intena;

    if(!holding(&ptable.lock))
        panic("sched ptable.lock");
    if(cpu->ncli != 1)
        panic("sched locks");
    if(proc->state == RUNNING)
        panic("sched running");
    if(readeflags()&FL_IF)
        panic("sched interruptible");
    intena = cpu->intena;
    swtch(&proc->context, cpu->scheduler);
    cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
    void
yield(void)
{
    acquire(&ptable.lock);  //DOC: yieldlock
    proc->state = RUNNABLE;
    sched();
    release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
    void
forkret(void)
{
    static int first = 1;
    // Still holding ptable.lock from scheduler.
    release(&ptable.lock);

    if (first) {
        // Some initialization functions must be run in the context
        // of a regular process (e.g., they call sleep), and thus cannot
        // be run from main().
        first = 0;
        iinit(ROOTDEV);
        initlog(ROOTDEV);
    }

    // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
    if(proc == 0)
        panic("sleep");

    if(lk == 0)
        panic("sleep without lk");

    // Must acquire ptable.lock in order to
    // change p->state and then call sched.
    // Once we hold ptable.lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup runs with ptable.lock locked),
    // so it's okay to release lk.
    if(lk != &ptable.lock){  //DOC: sleeplock0
        acquire(&ptable.lock);  //DOC: sleeplock1
        release(lk);
    }

    // Go to sleep.
    proc->chan = chan;
    proc->state = SLEEPING;
    sched();

    // Tidy up.
    proc->chan = 0;

    // Reacquire original lock.
    if(lk != &ptable.lock){  //DOC: sleeplock2
        release(&ptable.lock);
        acquire(lk);
    }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
    static void
wakeup1(void *chan)
{
    struct proc *p;

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if(p->state == SLEEPING && p->chan == chan)
            p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
    void
wakeup(void *chan)
{
    acquire(&ptable.lock);
    wakeup1(chan);
    release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
    int
kill(int pid)
{
    struct proc *p;

    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->pid == pid){
            p->killed = 1;
            // Wake process from sleep if necessary.
            if(p->state == SLEEPING)
                p->state = RUNNABLE;
            release(&ptable.lock);
            return 0;
        }
    }
    release(&ptable.lock);
    return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
    void
procdump(void)
{
    static char *states[] = {
        [UNUSED]    "unused",
        [EMBRYO]    "embryo",
        [SLEEPING]  "sleep ",
        [RUNNABLE]  "runble",
        [RUNNING]   "run   ",
        [ZOMBIE]    "zombie"
    };
    int i;
    struct proc *p;
    char *state;
    uint pc[10];

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state == UNUSED)
            continue;
        if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
            state = states[p->state];
        else
            state = "???";
        cprintf("%d %s %s", p->pid, state, p->name);
        if(p->state == SLEEPING){
            getcallerpcs((uint*)p->context->ebp+2, pc);
            for(i=0; i<10 && pc[i] != 0; i++)
                cprintf(" %p", pc[i]);
        }
        cprintf("\n");
    }
}

// handles the percentage of stride scheduling
// SYSTEMCALL
int
set_cpu_share(int cpu_share)
{
   struct proc* p;
   proc->mode = 1;
   int totalthread2 = 0;
   double tempcpu = 0;

   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
       if(p->pid == proc->pid && p->threadflag == 1){
           p->mode = 1;
           totalthread2++;
       }
   }

   if(totalcpu + cpu_share > 80){
       return -1;
   }

   totalcpu = totalcpu + (double)cpu_share;
   proc->cpushare = cpu_share;
   
   tempcpu = (double)cpu_share/(double)(totalthread2+1);
    
   //proc->cpushare = tempcpu;
   proc->stride = (1000/tempcpu);

   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->pid == proc->pid && p->threadflag == 1 && p->mode == 1){
            p->stride = proc->stride;
            p->cpushare = tempcpu;
        }
   }

   //cpushareflag = 1;

   return 0;
}

//function to create a thread
int thread_create
(thread_t* thread, void* (*start_routine)(void *), void* arg)
{
    int i;
    struct proc *p;
    //struct proc *th;
    //double tempcpu = 0;
    int ba; //base address

    //allocates threads which looks exactly same as process except kernal stack 
    p = allocproc();
    nextpid--;

    if(p == 0){
        cprintf("process allocation fail!\n");
        return -1;
    }
    
    //declares thread's parent as proc
    if(proc->threadflag == 0){
        p->parent = proc;
    }
    else
        p->parent = proc->parent;

    //'threadYN[i]' is for checking thread existence indexing by tid.
    //checking through threadYN, allocates empty tid.
    /*
    for(i=0; i<64; i++){
        if(proc->threadYN[i]==0){
            p->tid = i;
            proc->threadYN[i] = 1;
            break;
        }
    }*/

    //does not care about tid and stack sequence.
    //just allocates tid sequentially.
    p->tid = nexttid;
    nexttid++;
    
    //returns thread id
    *thread = p->tid;
    
    //copies information of parent process, and turns flag on.
    p->threadflag = 1;
    safestrcpy(p->name, proc->name, sizeof(proc->name));
    p->pid = proc->pid; 
    *p->tf = *proc->tf;
    p->sz = proc->sz;
    //p->tf->eax = 0;
    p->pgdir = proc->pgdir;
    p->mode = proc->mode;
    p->pass = 0;
    p->stride = 0;

    for(i=0;i<NOFILE;i++){
        if(proc->ofile[i])
            p->ofile[i] = filedup(proc->ofile[i]);

        p->cwd = idup(proc->cwd);
    }

    //allocates new stack address for thread.
    //should allocate 4KB, including arg & ret.
    ba = proc->sz + (uint)(2*PGSIZE*(p->tid));
    
    //allocates new memory size using allocuvm();
    p->sz = allocuvm(p->pgdir, ba, ba + PGSIZE*2);
    p->threadbase = ba;

    p->tf->esp = ba + 4092;
    *((uint*)(p->tf->esp)) = (uint)arg;
    p->tf->esp -= 4;
    *((uint*)(p->tf->esp)) = 0xFFFFFFFF;

    //changes eip to 'start_routine'
    p->tf->eip = (uint)start_routine;
    //cprintf("p->mode : %d, proc->mode : %d\n",p->mode,proc->mode); 
    
    /*    
    if(proc->mode == 1){
        proc->totalthread++;
        tempcpu = (double)(proc->cpushare)/(double)(proc->totalthread+1);
        proc->stride = (double)(1000/tempcpu);
        p->stride = (double)(1000/tempcpu);
        
        for(th = ptable.proc; th < &ptable.proc[NPROC]; th++){
            if(th->pid == proc->pid && th->mode == 1){
                th->stride = proc->stride;
                th->cpushare = tempcpu;
            }
        }  
    }
    */

    acquire(&ptable.lock);
    p->state = RUNNABLE;
    release(&ptable.lock);
    
    //cprintf("threadbase : %d p->sz : %d proc->sz : %d\n", p->threadbase, p->sz, proc->sz);
    //cprintf("pid : %d, tid : %d, parent pid : %d, parent tid : %d\n", p->pid, p->tid, p->parent->pid, p->parent->tid);

    return 0;
}

//thread exit function. This is almost same to xv6 exit();
void 
thread_exit(void* retval)
{

    int fd;
    double tempcpu = 0;
    struct proc * p;

    // Close all open files.
    for(fd = 0; fd < NOFILE; fd++){
        if(proc->ofile[fd]){
            fileclose(proc->ofile[fd]);
            proc->ofile[fd] = 0;
        }
    }

    begin_op();
    iput(proc->cwd);
    end_op();
    proc->cwd = 0;
    acquire(&ptable.lock);

    //in addition to normal 'exit()',
    //thread_exit() needs to store 'retval'.
    proc->t_retval = retval;
    
    // Parent might be sleeping in wait().
    wakeup1(proc->parent);

    // Jump into the scheduler, never to return.
    proc->state = ZOMBIE;
    
    proc->parent->totalthread--;
    tempcpu = (double)cpu_share/(double)(proc->parent->totalthread+1);

    //proc->cpushare = tempcpu;
    proc->stride = (1000/tempcpu);

    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->pid == proc->pid && p->threadflag == 1){
            p->stride = proc->stride;
            p->cpushare = tempcpu;
        }
    }
    proc->mode = 0;
    sched();
    panic("zombie exit");

}

//wait for the thread specified by the argument to terminate. If that thread has 
//already terminated, then this returns immediately. 
//this function cleans up the resources allocated to the thread such as a page table,
//allocated memories and stacks. 
//This also contains the return value of thread.
int
thread_join(thread_t thread, void** retval)
{
    struct proc* p;
    
    acquire(&ptable.lock);
    
    for(;;){
       // cprintf("thread : %d\n", thread);
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

            if(p->tid != thread)
                continue;

            //cprintf("p->tid : %d thread : %d\n",p->tid, thread);
            if((p->state == ZOMBIE) && (p->threadflag == 1)){
                //contains retval;
                *retval = p->t_retval; 
                
                kfree(p->kstack);
                p->kstack = 0;

                //deallocates memory size to past state using deallocuvm();
                deallocuvm(p->pgdir, p->threadbase + 2*PGSIZE, p->threadbase);
                //deallocuvm(p->pgdir, p->sz, p->sz - PGSIZE);
                
                p->threadflag = 0;
                p->name[0] = 0;
                p->killed = 0;
                //p->parent->threadYN[thread] = 0;     
                p->parent = 0;
                p->tid = 0;
                p->pid = 0;
                p->state = UNUSED;
                nexttid = 1;
                release(&ptable.lock);
                //if thread_join() succeeds, returns 0;
                return 0;
            }
            
            
            if(proc->killed){
                release(&ptable.lock);
                return -1;
            }
            
            sleep(proc, &ptable.lock);
        }
    }
}
