
/*
Scotty Fulton                                   March/3/2019    

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

*/

#include <queue>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

/*
class for jobs object
*/
class jobs
{
  public:
    int idNum;      //job id
    int arrival;    //arrival time
    int priority;   //favorite number of avocados to eat?
    int cpuTime;    //burst time/ time needed on cpu
    int cpuLeft;    //how much is still needed on cpu
    int cpuDone;    //how much had been completed
    int nightNight; //completion time
    int currRR;     //used for RR Q=2

    /*contructors*/
    jobs() {}

    jobs(int id, int arriv, int pri, int cpuT)
    {
        idNum = id;
        arrival = arriv;
        priority = pri;
        cpuTime = cpuT;
        cpuLeft = cpuT;
        cpuDone = 0;
        cpuLeft = 0;
        nightNight = 0;
        currRR = 0;
    }

    ~jobs() {}
    // {cout << "decon" << endl;}
};

/*
overload for the > (for min in front priority queue) comparison 
operators for pri queue comparisons... what to order queue based on
*/
struct ComparePriority
{
    bool operator()(const jobs &p1, const jobs &p2)
    {
        return p1.priority > p2.priority; //attempt at a min queue
    }
};

/*
Forward function declarations
*/
int getNumberOfJobs(); //collects # of jobs to schedule
void getJobAttributes(vector<jobs> &vec, int n); //puts created jobs into vector
void emitVector( const vector<jobs> v); //prints job vector data
int getTotalTime( vector<jobs> &v); //calcs time needed to run all jobs
void checkArrivals( vector<jobs> v, int t); //notes arrivals as needed
void checkPQ(); //manages cpu usage
void emitTime(int t); //displays time units
void check4Preempt(int tempID); //notes preemptions
void check4Tie(); //manages round robin secondary
void turnAroundTimes( vector<jobs> v);//calcs job TaT's and avg TaT
void check4Exit(int t, int bigT,vector<jobs> &v);//note/update when jobs finish
void ganttyOnDude(int bigT, vector<jobs> v); //driver for scheduling of jobs
void ludacrisSpeed(); //speed controller

/*
global vars/structures
*/
priority_queue<jobs, vector<jobs>, ComparePriority> PQ;
int prevRR;
int selector = 1;

/*
receive number of jobs
made function to clean up main driver function
*/
int getNumberOfJobs()
{
    int number;
    cout << "# of jobs: ";
    cin >> number;
    cout << number << endl;
    return number;
}

/*
populate the jobs that are read in from file/input.
passed vector to populate by reference, and n number of 
jobs to populate
*/
void getJobAttributes(vector<jobs> &vec, int n)
{
    int ar, pri, cpu;
    for (int i = 0; i < n; i++)
    {
        cout << endl
             << "Job: " << i;
        cout << endl
             << "Arrival: ";
        cin >> ar;
        cout << ar << endl
             << "Priority: ";
        cin >> pri;
        cout << pri << endl
             << "cpuTime: ";
        cin >> cpu;
        cout << cpu << endl;
        jobs newJob(i, ar, pri, cpu);
        newJob.cpuLeft = newJob.cpuTime;
        vec.push_back(newJob);
    }
}

/*
formatted job list for output
Ar = arrival, Pr = priority, CT = cpuTime,
CL = cpu time Left, CD = cpu time completed,
NN = night night or exit time unit,
given a vector 
*/
void emitVector(const vector<jobs> v)
{
    cout << endl
         << "Job Chart" << endl
         << "#  Ar Pr CT CL CD NN" << endl;
    for (int i = 0; i < v.size(); i++)
    {
        cout << v[i].idNum << "  "
             << v[i].arrival << "  "
             << v[i].priority << "  "
             << v[i].cpuTime << "  "
             << v[i].cpuLeft << "  "
             << v[i].cpuDone << "  "
             << v[i].nightNight
             << endl;
    }
    cout << endl;
}

/*
adds the total time needed to complete all jobs
sum of all cpuTimes
*/
int getTotalTime(vector<jobs> &v)
{
    int bigT = 0;
    for (int i = 0; i < v.size(); i++)
    {
        bigT += v.at(i).cpuTime;
    }
    return bigT;
}

/*
quick is-empty function
*/
bool isEmpty()
{
    if (PQ.empty())
    {
        return true;
    }
}

/*
as time passes, we need to make sure arrivals are checked 
for preemption, or just added to the priority queue
*/
void checkArrivals(vector<jobs> v, int t)
{
    jobs temp;
    int preemptJobNum;
    //if not empty, then get a copy of top objs id#
    if (!isEmpty())
    {
        preemptJobNum = PQ.top().idNum;
    }
    for (int i = 0; i < v.size(); i++) //for size of vector
    {
        if (v.at(i).arrival == t) //if job arrival == current time, then push to PQ and emit arrival
        {
            cout << "job: " << v.at(i).idNum << " arrives" << endl;
            temp = v.at(i);
            //if not empty, if next in job list priority < priority queue top's priority, then
            //we have a preemption, else keep on going.
            if (!isEmpty())
            {
                if (temp.priority < PQ.top().priority)
                {
                    cout << "Context switch"
                         << endl
                         << "job " << preemptJobNum << " preempts" << endl;
                }
            }
            PQ.push(temp);
        }
    }
}

/*
this bad boy is responsible for delegating who is on the cpu, 
but also updates the data members for these immutable objs!!!!!!
*/
void checkPQ()
{
    if (!isEmpty())
    {
        int tempID = PQ.top().idNum;
        jobs temp = PQ.top();
        cout << "job: " << tempID << " runs" << endl;
        PQ.pop();
        //updates the has done and has left data members of jobs object, then pushes back on
        temp.cpuDone++;
        temp.cpuLeft--;
        temp.currRR++;
        PQ.push(temp);

        /*after updating and pushing the temp object, make sure it ends up
            back in the front of the queue, rightfully that is... as long as 
            it's not the only job in queue && (PQ.size()>1).

            while not a match, pop current top and push back*/
        while ((PQ.top().idNum != temp.idNum))
        {
            jobs placeholder = PQ.top();
            PQ.pop();
            PQ.push(placeholder);
        }
    }
}

/*
formatted time unit header
*/
void emitTime(int t)
{
    cout << "\n-----------------"
         << "\n|   Time = " << t
         << "   |"
         << "\n-----------------" << endl;
}

/*
outputs job that is preempted 
*/
void check4Preempt(int tempID)
{
    cout << "job: " << tempID << " preempts" << endl;
}

/*
handles the secondary quantifier, for tie breaking
in this case Round robin Q = 2.
*/
void check4Tie()
{
    if (PQ.top().currRR > 1) //if top()'s been on CPU for 2 cycles
    {
        jobs temp = PQ.top();
        int tempID = temp.idNum;
        PQ.pop();
        //round robin this does
        if ((PQ.top().priority == temp.priority) && (PQ.top().idNum != temp.idNum))
        /* if popped top obj priority is == new top obj priority AND objs 
        are not same job number, then reset old top RR counter and push back on for preempt  */
        {
            temp.currRR = 0;
            PQ.push(temp);
            check4Preempt(tempID);
        }
        else //if objs are the same or old top is lower priority, push back on
        {
            PQ.push(temp);
        }
    }
}

/*
calculates the turn around times per job, and avg 
TaT from orig jobs vector, super fancy formatting.
*/
void turnAroundTimes(vector<jobs> v)
{
    float avgTaT = 0;
    cout << "\n---------------------------\n"
         << "|    Turn Around Times    |\n"
         << "---------------------------" << endl;
    cout << "job "
         << "n Tat: T(f) - T(i) = xx\n"
         << "---------------------------" << endl;
    for (int i = 0; i < v.size(); i++) //for all jobs in vector
    {
        //calc indiv TaT
        float jobTaT = v.at(i).nightNight - v.at(i).arrival;
        //add to total for avg
        avgTaT += jobTaT;
        cout << "job " << i << " Tat:  "
             << v.at(i).nightNight << "  -  " << v.at(i).arrival << "  =  " << jobTaT << endl;
    }
    //output and calc avg TaT, (avgtotal / number of jobs = avgTaT)
    cout << "---------------------------\n"
         << "      AVG TaT: " << avgTaT / v.size()
         << "\n---------------------------\n"
         << endl;
}

/*
checks if the top() object.cpuLeft is at 0, 
if the job has completed after all preemptions 
*/
void check4Exit(int t, int bigT, vector<jobs> &v)
{
    if (!isEmpty())
    {
        jobs temp = PQ.top();
        int tempID = temp.idNum;
        if (temp.cpuLeft == 0) //if PQ.top() has 0 left to run then night night
        {
            PQ.pop();
            cout << "job: " << tempID << " night night" << endl;
            //update night night time in vector
            for (int i = 0; i < v.size(); i++)
            {
                if (v.at(i).idNum == tempID)
                {
                    v.at(i).nightNight = t;
                }
            }
            if (isEmpty()) //if empty now, will fall through initial check and go to else
            {
                check4Exit(t, bigT, v);
            }
            else //PriQueue not empty, then must be a context switch
            {
                cout << "Context switch" << endl;
            }
        }
    }
    else // queue is empty meaning: empty cycle or job done.
    {
        if (t < bigT) //if current time is < total calculated time
        {
            cout << "Empty CPU cycle" << endl;
        }
        else //current time is at its end...and must be dealt with appropriately
        {
            cout << "***Queue empty***" << endl;
            turnAroundTimes(v);
            exit(0);
        }
    }
}

/*
processes the jobs and prints the actions per time unit
*/
void ganttyOnDude(int bigT, vector<jobs> v)
{
    for (int t = 0; t <= bigT; t++)
    {
        sleep(selector);        //how long is time unit
        emitTime(t);            //show the heading for each time unit
        checkArrivals(v, t);    //check for arrivals and preempt is necessary
        check4Exit(t, bigT, v); //any one exiting?  context switching
        check4Tie();            //this if where RR Q=2 happens
        checkPQ();              //this simulates the CPU usage
        if (!isEmpty() && t == bigT)
        /*if need more time units due to empty cpu cycles... feed bigT!!!!*/
        {
            bigT++;
        }
    }
}

/*
collects the speed limit, recursive authentication 
0 = plaid speed, 1 = 1 sec between units, etc...
*/
void ludacrisSpeed()
{
    cout << "How many second(s) delay between time units; 0 - 9 seconds?: ";
    cin >> selector;
    cout << selector << endl
         << endl;
    if ((selector < 0) || (selector > 9))
    {
        cout << "please enter a valid response" << endl;
        ludacrisSpeed();
    }
}

/*
driver for collecting and scheduling data for the jobs received.
*/
int main()
{
    //get speed multiplier
    ludacrisSpeed();
    // get number of jobs
    int n = getNumberOfJobs();
    //create vector of size n
    vector<jobs> jobVec;
    //get each job object into vector
    getJobAttributes(jobVec, n);
    emitVector(jobVec);
    //get the time needed for scheduling
    int time = getTotalTime(jobVec);
    //start big T (cpu scheduling)!!!!
    ganttyOnDude(time, jobVec);

    return 0;
}