# 2019_DataScience
2019 Data Science course


*This markdown is for ELE3021 Hanyang University Operating Systems class.*


## **Project 1**
-----------------------------------------------------------------
### First step : MLFQ(Multilevel feedback queue scheduling)
*  3-level feedback queue
* Each level of queue adopts Round Robin policy with 5t, 10t, 20t time quantum
respectively
* To prevent starvation, priority boost is required

### Second step : Combine the stride scheduling algorithm with MLFQ
* Make the system call (i.e. cpu_share) that requests the portion of CPU and guarantees the calling process to be allocated that CPU time.
* Total stride processes are able to get at most 80% of CPU time. Exception handling is needed for exceeding request.
* The rest 20% of CPU time should run for the MLFQ scheduling which is the default scheduling in this project.


## **Project 2**
-------------------------------------------------------------------


### First step
* General format will be MLFQ comes first and stride scheduling will follow. I will call this as 'MLFQ phase' and 'stride phase'.
* This means there are two schedulers(MLFQ, stride), and processes will be handled by one of these two schedulers.
* I named three queues as Q0, Q1, and Q2. Q0 has the highest priority.
* A time slice(1 tick) of this scheduler is 10ms.
* Then time slices of Q0, Q1, Q2 will be 50ms, 100ms, 200ms each.
* To prevent gaming, I made priority boost to be executed after 100 tick.
* Priority boost will be executed after 100 ticks. Think that only 1 process is on the queue.
* Base scheduling will be MLFQ.

### Second step
* Think about the system call that requests the cpu time. This means each process will request tickets at stride scheduling phase.
* When new process which has the system call that requests cpu time(cpu_share), it means that this process should be handled by stride scheduling. But what if there are processes left unended at MLFQ phase? It will be executed by the percentage of (100 - cpu_share)
* When new process comes during stride scheduling, they will need a criteria for pass. I will make pass of other processes to be 0.
* If there is no process left for stride, only MLFQ will be executed as 100%. Maximum Limit time of stride phase must be less than 80% of total.

### KEY
* So overall, key of this design is to schedule the switching between MLFQ scheduler and stride scheduler efficiently without violating the time limit.


## **Project 3**
---------------------------------------------------------------------

**Main scheduler**
* Each process has a variable called 'mode'.
* If mode is 0, it goes to MLFQ scheduling. If mode is 1, it goes to stride scheduling.
* By declaring 'pass' and 'stride' to each scheduling function, I made each scheduler to be executed with amount of percentage test case required.
**MLFQ_scheduler**
* In MLFQ scheduling, each process has 'priority' and 'ticknum'. Searching through ptable, I set 'lowest' so that scheduler can only execute lowest queue.
* In each priority queue, ticknum will be reseted so that it can execute for each queue's time quantum.
* ticknum will also prevent gaming when process calls 'yield()'.
* After checking totaltick exceeds 100 or not, Priority Boost will be executed and every processes' priority and ticknum will be 0.
**stride_scheduler**
* In stride scheduling, each process has 'stride' and 'pass'.
* Stride is result of (1000/cpu_share). Pass will be the sum of stride in each process.
* All I had to do was just compare each process's pass and add one more stride to process who has lower pass.


## **Project 4**
---------------------------------------------------------------------
