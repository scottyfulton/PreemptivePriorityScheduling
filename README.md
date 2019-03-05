# PreemptivePriorityScheduling
C++ Preemptive Priority Queue Scheduling algorithm
Preemptive-Priority Scheduler with a Priority Queue using a
secondary (tie breaking) algo of round robin with 
a quantum factor of 2 (every 2 cycles, tieing jobs alternate).

This application was compiled on a unix OS using g++ compiler.
To compile:
$ g++ ScottyFultonPrePriQ.cpp
$ ./a.out < indata.txt > outdata.txt

indata.txt example:
0       //speed multiplier 0-9
2       //# of jobs
0 1 2   //Job's data as:
1 8 1   //Arrival=1 Priority=8 cpuTime=1

This application utilizes the STL for priority_queues and vectors.
Unfortunately, c++ priority_queue objects are immutable, so it was 
a challenge to update the objects data members which had the same 
sorting quantifier (obj's priority).  This was overcome by copying 
each altered object to a temp obj, and literally popping the top
object and pushing it back on, and comparing the new top with the 
altered copy until a match was found.  Definitely sacrificed some
runtime for this.

Overview:
The application prompts for # of jobs, and collects data for each job.
Using a jobs class to contain the data.  These jobs are collected in
a vector initially.  After this, the vector is printed out, and then 
sent to the ganttOnDude() function to initiate the gantt chart process.

The program will record for each time unit:
display job arrival, job that is on cpu, context switches, preemptions,
job completions(night nights), and empty queue (job finished).

***Also, the leading integer 0-9 of input will determine the speed(in seconds) 
that the time units will take to propogate to the next unit. 
