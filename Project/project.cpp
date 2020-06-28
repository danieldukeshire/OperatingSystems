/******************************************************************

project.cpp
This program runs a simulation of CPU scheduling algorithms:
First Come First Serve, Round Robin, Shortest Job First, and
Shortest Remaining Time

Authors: Daniel Dukeshire (dukesd3)
Date 4.1.2020

******************************************************************/
#include <fstream>
#include <stdio.h>
#include <vector>
#include <iomanip>
#include <cmath>
#include <iostream>
#include <string>
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include "process.h"
#include <deque>
#include <sstream>

struct generator                  // Generator for the input parameters
{
  int seed;                       // the seed for srand() number generation
  double lambda;                  // lambda for the number generation formula
  int upper_bound;                // the upper bound for the number calculation
  int n;                          // number of processes to be run
  int t_cs;                       // context switch time
  double alpha;                   // the alpha value
  double t_slice;                 // time slice value for the RR algorithm
  std::string rr_add;             //
  std::ofstream out_file;
};

char alphabet[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                  'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                  'U', 'V', 'W', 'X', 'Y', 'Z'};
std::vector<process> all_processes;

/******************************************************************
compareArrival(): used as a lambda function for sort, to sort
the all_processes vector by the arrival time.
#TODO: Implement ties (page 5 of the .pdf)
******************************************************************/
bool compareArrival(process a, process b)
{
  if(a.getArrival() != b.getArrival()) return a.getArrival() < b.getArrival();
  else return a.getCharacter() > b.getCharacter();                              // is this fucking up??
}

/******************************************************************
compareChar(): used as a lambda function for sort, to sort
the characters for all algorithms
******************************************************************/
bool compareChar(process a, process b)
{
  return a.getCharacter() < b.getCharacter();
}

/******************************************************************
compareTau(): used as a lambda function for sort, to sort
by Tau Value for SJF and SRT
******************************************************************/
bool compareTau(process a, process b)
{
  if(a.getTauChange() == b.getTauChange()) return compareChar(a, b);
  return a.getTauChange() < b.getTauChange();
}

/******************************************************************
printQueue(): passes a ready queue, prints accordingly
******************************************************************/
void printQueue(std::deque<process> ready_queue)
{
  std::cout << "[Q";
  if((int)ready_queue.size() == 0) std::cout << " <empty>]" << std::endl;
  else
  {
    for(int i =0; i<(int)ready_queue.size(); i++) std::cout << " " << ready_queue[i].getCharacter();
    std::cout<< "]" << std::endl;
  }
  return;
}

/******************************************************************
getProcesses() (Dan)
takes the command line values as input (stored in
the struct parameters). With the random.c file given in lecture,
this function calculates the random values for the CPU scheduling
algorithms.
******************************************************************/
void getProcesses(generator* parameters)
{
  double lambda = parameters->lambda;
  srand48(parameters->seed);
  for ( int i = 0 ; i < parameters->n; i+=1 )
  {
    double x = -log(drand48()) / lambda;
    if(x > parameters->upper_bound){
      while(x > parameters->upper_bound) x = -log(drand48()) / lambda;
    }
    x = floor(x);
    int num_bursts = trunc(drand48() * 100) + 1;

    process temp;                       // A temp process object to be added to the all_processes global vector
    temp.setState(0);                   // The starting state for each process is 0 ( not in use )
    temp.setCharacter(alphabet[i]);     // Sets each process character ( a - z )
    temp.setArrival(x);                 // Sets the arrival time (found above )
    temp.setBurstNum(num_bursts);       // Sets the total number of bursts (found above)
    temp.setTotalBursts(num_bursts);    // Sets the total number of bursts, not to be changed
    temp.setCurrentIO();                // Sets current_IO, i.e. the current io burst location in the array (see process.h)
    temp.setCurrentCPU();               // Sets current_CPU, i.e. the current cpu burst location in the array (see process.h)
    temp.setPreemption(0);              // Never starts preempted !!
    temp.setRemainingBurst(0);          // The remaining time starts at 0
    temp.setCurrentTauVal(0);
    temp.wait_time = 0;
    temp.total_turnaround_time = 0;     // The total turnaround time to complete ALL BURSTS
    temp.start_turnaround = 0;          // The time a process starts a burst (to be calculated for turnaround)

    // Finding all of the CPU and IO burst values
    for(int j=0; j<num_bursts; j++)
    {
      double burstTime = -log(drand48()) / lambda;
      if( burstTime > parameters->upper_bound ) {
        while(burstTime > parameters->upper_bound) burstTime = -log(drand48()) / lambda;
      }
      temp.addCPUBurst(burstTime+1);        // pushes the bursttime back to a private vector in the object

      if(num_bursts - 1 != j)
      {
        double IOTime =  -log(drand48()) / lambda;
        if(IOTime > parameters->upper_bound)
          while(IOTime > parameters->upper_bound) IOTime = -log(drand48()) / lambda;
        temp.addIOBurst( ceil(IOTime + parameters->t_cs/2) );   // pushes the iotime back to a private vector in the object
      }
    }
    all_processes.push_back(temp);        // Adds this object to the global variable
  }

  return;
}

/******************************************************************
GetTauVals (Dan)
Calculates all of the tau values and stores them in the object vector
******************************************************************/
void getTauVals(generator* parameters)
{
  for(int i=0; i<(int)all_processes.size(); i++)
  {
    all_processes[i].addTauValue(ceil(1/parameters->lambda));
    for(int j=1; j<(int)all_processes[i].CPU_bursts.size(); j++)
    {;
      int tau = ceil(all_processes[i].CPU_bursts[j-1] * (parameters->alpha) + (1- parameters->alpha) * all_processes[i].Tau_bursts[j-1]);
      all_processes[i].addTauValue(tau);
    }
  }
  return;
}

/******************************************************************
First Come First Serve (Dan)
Simulates the first come first serve algorithm for CPU scheduling.
******************************************************************/
void FCFS(generator* parameters, std::ostream &out_file)
{
  for(int i=0; i<parameters->n; i+=1) all_processes[i].printProcess(false);

  std::vector<process> fcfs_processes = all_processes;                        // Make a new vector, as we want to keep the original
  std::sort(fcfs_processes.begin(), fcfs_processes.end(), compareArrival);    // will be sorted by the arrival time, and cpu burst

  std::deque<process> ready_queue;                                            // the queue for processes in the "ready" state
  int CPU = -1;                                 // represents the current process in the CPU, storing its process loc. in fcfs
  int time = fcfs_processes[0].getArrival();    // the current time to be printed to the console
  int c_arrival = 0;                            // as the vector is sorted, stores the next arrival value

  bool context_in = false;                      // Bool, represents if the CPU is currently in a context switch (into the CPU)
  int context_in_count = 0;                     // the number of seconds that context_in switch has been running for
  bool context_out = false;                     // Bool, represents if the CPU is currently in a context switch (out of the CPU)
  int context_out_count = 0;                    // the number of seconds that context_out switch has been running for
  int in_burst_time = 0;
  int total_context_switches = 0;               // keeps track of the number of context switches


  // Starting the simulation
  std::cout << "time 0ms: Simulator started for FCFS [Q <empty>]" << std::endl;
  while(true)
  {
    // Adding to the wait times.... allowing for calculations
    for(int o = 0; o<(int)fcfs_processes.size(); o++) if(fcfs_processes[o].getState() == 1) fcfs_processes[o].wait_time +=1;

    if(context_out == true){
      context_out_count +=1;
      if(context_out_count == parameters->t_cs/2){
        context_out = false;
        context_out_count = 0;
      }
    }

    // Check to see if any processes have completed their CPU burst
    if(CPU != -1 && fcfs_processes[CPU].getCompleteCPU() == time && context_in == false)
    {

      fcfs_processes[CPU].setCompleteCPU(0);        // Resets the CPU
      if(fcfs_processes[CPU].getBurstNum() == 1)    // Check for terminated process
      {
        std::cout << "time " << time << "ms: Process " << fcfs_processes[CPU].getCharacter() <<
        " terminated ";
        printQueue(ready_queue);
        fcfs_processes[CPU].setState(4);            // Process is now terminated
        fcfs_processes[CPU].total_turnaround_time += (time - fcfs_processes[CPU].start_turnaround + parameters->t_cs/2);
        context_out = true;
        CPU = -1;                                   // reset CPU availability
      }
      else                                          // if there are still more bursts to go ...
      {
        fcfs_processes[CPU].setBurstNum(fcfs_processes[CPU].getBurstNum()-1);// subtracts a burst value
        if(time < 1000)
        {
          std::cout << "time " << time << "ms: Process " << fcfs_processes[CPU].getCharacter() <<
        " completed a CPU burst; " << fcfs_processes[CPU].getBurstNum();
        if(fcfs_processes[CPU].getBurstNum() > 1) std::cout << " bursts to go ";
        else std::cout << " burst to go ";
        printQueue(ready_queue);
        }

        // Now .. moves to IO
        if(time < 1000) std::cout << "time " << time << "ms: Process " << fcfs_processes[CPU].getCharacter() <<
        " switching out of CPU; will block on I/O until time ";

        fcfs_processes[CPU].setState(3);
        fcfs_processes[CPU].setCompleteIO(time + fcfs_processes[CPU].getIOBurst());
        fcfs_processes[CPU].total_turnaround_time += (time - fcfs_processes[CPU].start_turnaround + parameters->t_cs/2);

        if(time < 1000) std::cout << fcfs_processes[CPU].getCompleteIO() << "ms ";
        if(time < 1000) printQueue(ready_queue);
        context_out = true;
        CPU = -1;                                       // reset CPU availability
      }
    }

    // Check to see if any processes have arrived
    if(fcfs_processes[c_arrival].getState() == 0 && fcfs_processes[c_arrival].getArrival() == time)
    {
      if(time < 1000) std::cout << "time " << time << "ms: Process " << fcfs_processes[c_arrival].getCharacter() <<
      " arrived; added to ready queue ";
      ready_queue.push_back(fcfs_processes[c_arrival]);           // pushes back to the ready queue
      fcfs_processes[c_arrival].setState(1);                      // sets the state to "in ready queue" (1)

      if(time < 1000) printQueue(ready_queue);
      if(c_arrival != parameters->n-1) c_arrival +=1;
    }


    // Check status of context in switch
    if(context_in == true && context_in_count == parameters->t_cs/2 -1)
    {
      if(time < 1000) std::cout << "time " << time << "ms: Process " << fcfs_processes[CPU].getCharacter() <<
      " started using the CPU for " << in_burst_time << "ms burst ";
      if(time < 1000) printQueue(ready_queue);
      total_context_switches +=1;
      context_in = false;
      context_in_count = 0;
      in_burst_time = 0;
    }
    else if(context_in == true && context_in_count != parameters->t_cs/2 -1) context_in_count +=1;


    // Check to see if any processes have completed their IO burst
    // Check to see if any processes have completed their IO burst
    std::vector<process> sorted_processes;
    for(int i=0; i<(int)fcfs_processes.size(); i+=1)
    {
      if(fcfs_processes[i].getState() == 3 && fcfs_processes[i].getCompleteIO() == time)
      {
        fcfs_processes[i].setCompleteIO(0);
        fcfs_processes[i].setState(1);                                      // Once gets out of I/O, moves into ready queue
        sorted_processes.push_back(fcfs_processes[i]);                           // pushes back to the ready queue
      }
    }
    std::sort(sorted_processes.begin(), sorted_processes.end(), compareChar);
    for(int i=0; i<(int)sorted_processes.size(); i+=1)
    {
      if(time < 1000) std::cout << "time " << time << "ms: Process " << sorted_processes[i].getCharacter() <<
      " completed I/O; added to ready queue ";

      int j;
      for(j =0; j<(int)fcfs_processes.size();j++) if( fcfs_processes[j].getCharacter() == sorted_processes[i].getCharacter()) break;


      ready_queue.push_back(sorted_processes[i]);
      if(time < 1000) printQueue(ready_queue);
    }

    // Check to see if any processes can start their CPU Burst from the queue
    if(CPU == -1 && (int)ready_queue.size() > 0 && context_out == false)
    {
      int burst_time = 0;
      int loc = 0;
      for(int i=0; i<(int)fcfs_processes.size(); i+=1)
      {
        if(ready_queue[0].getCharacter() == fcfs_processes[i].getCharacter()) { loc = i; break;}
      }

      burst_time = fcfs_processes[loc].getCPUBurst();
      fcfs_processes[loc].setState(2);
      fcfs_processes[loc].setCompleteCPU(time + burst_time + parameters->t_cs/2);
      fcfs_processes[loc].start_turnaround = time;

      ready_queue.pop_front();
      CPU = loc;
      context_in = true;
      in_burst_time = burst_time;
    }

    // Check to see if each process in fcfs_processes have terminated (process state == 4)
    bool exit = true;
    if( context_out == false)
    {
      for(int i = 0; i<(int)fcfs_processes.size(); i++) {
        if(fcfs_processes[i].getState() != 4) { exit = false; break; }
      }
      if(exit == true) {
        std::cout << "time " << time << "ms: Simulator ended for FCFS [Q <empty>]" << std::endl;
        break;
      }
    }
  time +=1;
  }

  // Calculating the metrics for the output file:
  double total_wait_time = 0.0;
  double total_turnaround_time = 0.0;
  double total_bursts = 0.0;
  double total_burst_time = 0.0;
  double average_wait_time = 0.0;
  double average_cpu_burst_time = 0.0;

  for(int i =0; i<(int)fcfs_processes.size(); i++)
  {
    for(int j=0; j<(int)fcfs_processes[i].CPU_bursts.size(); j++) total_burst_time += fcfs_processes[i].CPU_bursts[j];
    total_bursts += fcfs_processes[i].getTotalBursts();
    total_wait_time += fcfs_processes[i].wait_time;
    total_turnaround_time += fcfs_processes[i].total_turnaround_time;
  }

  //std::ofstream out_file = parameters->out_file;
  average_wait_time = total_wait_time/total_bursts;
  average_cpu_burst_time = total_burst_time/total_bursts;
  out_file << std::setprecision(3) << std::fixed;
  out_file << "Algorithm FCFS" << std::endl;
  out_file << "-- average CPU burst time: " << average_cpu_burst_time << " ms" << std::endl;
  out_file << "-- average wait time: " << average_wait_time << " ms" << std::endl;
  out_file << "-- average turnaround time: " << total_turnaround_time/total_bursts + average_wait_time << " ms" << std::endl;
  out_file << "-- total number of context switches: " << total_context_switches << std::endl;
  out_file << "-- total number of preemptions: " << 0 << std::endl;
  std::cout << std::endl;
  return;
}

/******************************************************************
Shortest Job First (Dan)
******************************************************************/
void SJF(generator * parameters, std::ostream &out_file)
{
  for(int i=0; i<parameters->n; i+=1) { all_processes[i].printProcess(true);  }

  std::vector<process> sjf_processes = all_processes;                        // Make a new vector, as we want to keep the original
  std::sort(sjf_processes.begin(), sjf_processes.end(), compareArrival);    // will be sorted by the arrival time, and cpu burst

  std::deque<process> ready_queue;                                            // the queue for processes in the "ready" state
  int CPU = -1;                                 // represents the current process in the CPU, storing its process loc. in fcfs
  int time = sjf_processes[0].getArrival();    // the current time to be printed to the console
  int c_arrival = 0;                            // as the vector is sorted, stores the next arrival value

  bool context_in = false;                      // Bool, represents if the CPU is currently in a context switch (into the CPU)
  int context_in_count = 0;                     // the number of seconds that context_in switch has been running for
  bool context_out = false;                     // Bool, represents if the CPU is currently in a context switch (out of the CPU)
  int context_out_count = 0;                    // the number of seconds that context_out switch has been running for
  int in_burst_time = 0;
  int total_context_switches = 0;               // keeps track of the number of context switches


  // Starting the simulation
  std::cout << "time 0ms: Simulator started for SJF [Q <empty>]" << std::endl;
  while(true)
  {
    // Adding to the wait times.... allowing for calculations
    for(int o = 0; o<(int)sjf_processes.size(); o++) if(sjf_processes[o].getState() == 1) sjf_processes[o].wait_time +=1;

    if(context_out == true){
      context_out_count +=1;
      if(context_out_count == parameters->t_cs/2){
        context_out = false;
        context_out_count = 0;
      }
    }

    // Check to see if any processes have completed their CPU burst
    if(CPU != -1 && sjf_processes[CPU].getCompleteCPU() == time && context_in == false)
    {

      sjf_processes[CPU].setCompleteCPU(0);        // Resets the CPU
      if(sjf_processes[CPU].getBurstNum() == 1)    // Check for terminated process
      {
        std::cout << "time " << time << "ms: Process " << sjf_processes[CPU].getCharacter() <<
        " terminated ";
        printQueue(ready_queue);
        sjf_processes[CPU].setState(4);            // Process is now terminated
        sjf_processes[CPU].total_turnaround_time += (time - sjf_processes[CPU].start_turnaround + parameters->t_cs/2);
        context_out = true;
        CPU = -1;                                   // reset CPU availability
      }
      else                                          // if there are still more bursts to go ...
      {
        sjf_processes[CPU].setBurstNum(sjf_processes[CPU].getBurstNum()-1);// subtracts a burst value
        if(time < 1000)
        {
          std::cout << "time " << time << "ms: Process " << sjf_processes[CPU].getCharacter() <<
          " (tau " << sjf_processes[CPU].getTauValue() << "ms) completed a CPU burst; " << sjf_processes[CPU].getBurstNum();
          if(sjf_processes[CPU].getBurstNum() > 1) std::cout << " bursts to go ";
          else std::cout << " burst to go ";
          printQueue(ready_queue);
        }

        sjf_processes[CPU].setCurrentTauVal(sjf_processes[CPU].getCurrentTau()+1);
        if(time < 1000) std::cout << "time " << time << "ms: Recalculated tau = " << sjf_processes[CPU].getTauValue()
        << "ms for process " << sjf_processes[CPU].getCharacter()<< " ";
        if(time < 1000) printQueue(ready_queue);

        // Now .. moves to IO
        if(time < 1000) std::cout << "time " << time << "ms: Process " << sjf_processes[CPU].getCharacter() <<
        " switching out of CPU; will block on I/O until time ";

        sjf_processes[CPU].setState(3);
        sjf_processes[CPU].setCompleteIO(time + sjf_processes[CPU].getIOBurst());
        sjf_processes[CPU].total_turnaround_time += (time - sjf_processes[CPU].start_turnaround + parameters->t_cs/2);

        if(time < 1000) std::cout << sjf_processes[CPU].getCompleteIO() << "ms ";
        if(time < 1000) printQueue(ready_queue);
        context_out = true;
        CPU = -1;                                       // reset CPU availability
      }
    }

    // Check status of context in switch
    if(context_in == true && context_in_count == parameters->t_cs/2 -1)
    {
      if(time < 1000) std::cout << "time " << time << "ms: Process " << sjf_processes[CPU].getCharacter() <<
      " (tau " << sjf_processes[CPU].getTauValue() << "ms) started using the CPU for " << in_burst_time << "ms burst ";
      if(time < 1000) printQueue(ready_queue);
      context_in = false;
      total_context_switches += 1;
      context_in_count = 0;
      in_burst_time = 0;
    }
    else if(context_in == true && context_in_count != parameters->t_cs/2 -1) context_in_count +=1;

    // Check to see if any processes have arrived
    if(sjf_processes[c_arrival].getState() == 0 && sjf_processes[c_arrival].getArrival() == time)
    {
      if(time < 1000) std::cout << "time " << time << "ms: Process " << sjf_processes[c_arrival].getCharacter() << " (tau " <<
      sjf_processes[c_arrival].getTauValue() << "ms) arrived; added to ready queue ";
      ready_queue.push_back(sjf_processes[c_arrival]);           // pushes back to the ready queue
      std::sort(ready_queue.begin(), ready_queue.end(), compareTau);

      sjf_processes[c_arrival].setState(1);                      // sets the state to "in ready queue" (1)

      if(time < 1000) printQueue(ready_queue);
      if(c_arrival != parameters->n-1) c_arrival +=1;
    }

    // Check to see if any processes have completed their IO burst
    std::vector<process> sorted_processes;
    for(int i=0; i<(int)sjf_processes.size(); i+=1)
    {
      if(sjf_processes[i].getState() == 3 && sjf_processes[i].getCompleteIO() == time)
      {
        sjf_processes[i].setCompleteIO(0);
        sjf_processes[i].setState(1);                                      // Once gets out of I/O, moves into ready queue
        sorted_processes.push_back(sjf_processes[i]);                           // pushes back to the ready queue

      }
    }
    std::sort(sorted_processes.begin(), sorted_processes.end(), compareChar);
    for(int i=0; i<(int)sorted_processes.size(); i+=1)
    {
      if(time < 1000) std::cout << "time " << time << "ms: Process " << sorted_processes[i].getCharacter() <<
      " (tau " << sorted_processes[i].getTauValue() << "ms) completed I/O; added to ready queue ";


      ready_queue.push_back(sorted_processes[i]);
      std::sort(ready_queue.begin(), ready_queue.end(), compareTau);
      if(time < 1000) printQueue(ready_queue);
    }

    // Check to see if any processes can start their CPU Burst from the queue
    if(CPU == -1 && (int)ready_queue.size() > 0 && context_out == false)
    {
      int burst_time = 0;
      int loc = 0;
      for(int i=0; i<(int)sjf_processes.size(); i+=1)
      {
        if(ready_queue[0].getCharacter() == sjf_processes[i].getCharacter()) { loc = i; break;}
      }

      burst_time = sjf_processes[loc].getCPUBurst();
      sjf_processes[loc].setState(2);
      sjf_processes[loc].setCompleteCPU(time + burst_time + parameters->t_cs/2);
      sjf_processes[loc].start_turnaround = time;

      ready_queue.pop_front();
      CPU = loc;
      context_in = true;
      in_burst_time = burst_time;
    }

    // Check to see if each process in srt_processes have terminated (process state == 4)
    bool exit = true;
    if( context_out == false)
    {
      for(int i = 0; i<(int)sjf_processes.size(); i++) {
        if(sjf_processes[i].getState() != 4) { exit = false; break; }
      }
      if(exit == true) {
        std::cout << "time " << time << "ms: Simulator ended for SJF [Q <empty>]" << std::endl;
        break;
      }
    }

  time +=1;
  }
  // Calculating the metrics for the output file:
  double total_wait_time = 0.0;
  double total_turnaround_time = 0.0;
  double total_bursts = 0.0;
  double total_burst_time = 0.0;
  double average_wait_time = 0.0;
  double average_cpu_burst_time = 0.0;

  for(int i =0; i<(int)sjf_processes.size(); i++)
  {
    for(int j=0; j<(int)sjf_processes[i].CPU_bursts.size(); j++) total_burst_time += sjf_processes[i].CPU_bursts[j];
    total_bursts += sjf_processes[i].getTotalBursts();
    total_wait_time += sjf_processes[i].wait_time;
    total_turnaround_time += sjf_processes[i].total_turnaround_time;
  }

  //std::ofstream out_file = parameters->out_file;
  average_wait_time = total_wait_time/total_bursts;
  average_cpu_burst_time = total_burst_time/total_bursts;
  out_file << std::setprecision(3) << std::fixed;
  out_file << "Algorithm SJF" << std::endl;
  out_file << "-- average CPU burst time: " << average_cpu_burst_time << " ms" << std::endl;
  out_file << "-- average wait time: " << average_wait_time << " ms" << std::endl;
  out_file << "-- average turnaround time: " << total_turnaround_time/total_bursts + average_wait_time << " ms" << std::endl;
  out_file << "-- total number of context switches: " << total_context_switches << std::endl;
  out_file << "-- total number of preemptions: " << 0 << std::endl;
  std::cout << std::endl;
  return;
}

/******************************************************************
Shortest Remaining Time (Dan)
******************************************************************/
void SRT(generator * parameters, std::ostream &out_file)
{
  for(int i=0; i<parameters->n; i+=1) { all_processes[i].printProcess(true);  }

  std::vector<process> srt_processes = all_processes;                        // Make a new vector, as we want to keep the original
  std::sort(srt_processes.begin(), srt_processes.end(), compareArrival);    // will be sorted by the arrival time, and cpu burst

  std::deque<process> ready_queue;                                            // the queue for processes in the "ready" state
  int CPU = -1;                                 // represents the current process in the CPU, storing its process loc. in fcfs
  int time = srt_processes[0].getArrival();    // the current time to be printed to the console
  int c_arrival = 0;                            // as the vector is sorted, stores the next arrival value

  bool context_in = false;                      // Bool, represents if the CPU is currently in a context switch (into the CPU)
  int context_in_count = 0;                     // the number of seconds that context_in switch has been running for
  bool context_out = false;                     // Bool, represents if the CPU is currently in a context switch (out of the CPU)
  int context_out_count = 0;                    // the number of seconds that context_out switch has been running for
  int in_burst_time = 0;
  int total_context_switches = 0;               // keeps track of the number of context switches
  int total_preemptions = 0;
  int add_later = -1;

  // Starting the simulation
  std::cout << "time 0ms: Simulator started for SRT [Q <empty>]" << std::endl;
  while(true)
  {
    // Adding to the wait times.... allowing for calculations
    for(int o = 0; o<(int)srt_processes.size(); o++) if(srt_processes[o].getState() == 1) srt_processes[o].wait_time +=1;

    if(context_out == true){
      context_out_count +=1;
      if(context_out_count == parameters->t_cs/2){
        if(add_later!=-1)
        {
          //srt_processes[add_later].setPreemption(1);
          ready_queue.push_back(srt_processes[add_later]);
          //srt_processes[add_later].setPreemption(0);
          std::sort(ready_queue.begin(), ready_queue.end(), compareTau);
          srt_processes[add_later].setState(1);
          add_later = -1;
        }
        context_out = false;
        context_out_count = 0;
      }
    }

    // Check to see if any processes have completed their CPU burst
    if(CPU != -1 && srt_processes[CPU].getCompleteCPU() == time && context_in == false)
    {

      srt_processes[CPU].setCompleteCPU(0);        // Resets the CPU
      if(srt_processes[CPU].getBurstNum() == 1)    // Check for terminated process
      {
        std::cout << "time " << time << "ms: Process " << srt_processes[CPU].getCharacter() <<
        " terminated ";
        printQueue(ready_queue);
        srt_processes[CPU].setState(4);            // Process is now terminated
        srt_processes[CPU].total_turnaround_time += (time - srt_processes[CPU].start_turnaround + parameters->t_cs/2);
        context_out = true;
        CPU = -1;                                   // reset CPU availability
      }
      else                                          // if there are still more bursts to go ...
      {
        srt_processes[CPU].setBurstNum(srt_processes[CPU].getBurstNum()-1);// subtracts a burst value
        if(time < 1000)
        {
          std::cout << "time " << time << "ms: Process " << srt_processes[CPU].getCharacter() <<
          " (tau " << srt_processes[CPU].getTauValue() << "ms) completed a CPU burst; " << srt_processes[CPU].getBurstNum();
          if(srt_processes[CPU].getBurstNum() > 1) std::cout << " bursts to go ";
          else std::cout << " burst to go ";
        }
        if(time < 1000) printQueue(ready_queue);

        srt_processes[CPU].setCurrentTauVal(srt_processes[CPU].getCurrentTau()+1);

        if(time < 1000) std::cout << "time " << time << "ms: Recalculated tau = " << srt_processes[CPU].getTauValue()
        << "ms for process " << srt_processes[CPU].getCharacter()<< " ";
        if(time < 1000) printQueue(ready_queue);

        // Now .. moves to IO
        if(time < 1000) std::cout << "time " << time << "ms: Process " << srt_processes[CPU].getCharacter() <<
        " switching out of CPU; will block on I/O until time ";

        srt_processes[CPU].setState(3);
        srt_processes[CPU].setCompleteIO(time + srt_processes[CPU].getIOBurst());
        srt_processes[CPU].total_turnaround_time += (time - srt_processes[CPU].start_turnaround + parameters->t_cs/2);

        if(time < 1000) std::cout << srt_processes[CPU].getCompleteIO() << "ms ";
        if(time < 1000) printQueue(ready_queue);
        context_out = true;
        CPU = -1;                                       // reset CPU availability
      }
    }

    // Check status of context in switch
    if(context_in == true && context_in_count == parameters->t_cs/2 -1)
    {
      if(time < 1000) std::cout << "time " << time << "ms: Process " << srt_processes[CPU].getCharacter() <<
      " (tau " << srt_processes[CPU].getTauValue() << "ms) started using the CPU with " << in_burst_time << "ms burst remaining ";
      srt_processes[CPU].setStartTau(time);           // Sets the beginning start tau
      if(time < 1000) printQueue(ready_queue);

      if( (int)ready_queue.size() > 0 && ready_queue[0].getTauChange() < srt_processes[CPU].getTauChange())// && ready_queue[0].getPreemption() == 0)       // If that process will be preempted again
      {
        if(time < 1000) std::cout << "time " << time << "ms: Process " << ready_queue[0].getCharacter() <<
        " (tau " << ready_queue[0].getTauValue() << "ms) will preempt " << srt_processes[CPU].getCharacter() << " ";
        if(time < 1000) printQueue(ready_queue);
        total_preemptions +=1;
        srt_processes[CPU].setState(0);
        srt_processes[CPU].current_CPU -= 1;

        add_later = CPU;
        CPU = -1;
        context_out = true;
      }

      context_in = false;
      total_context_switches += 1;
      context_in_count = 0;
      in_burst_time = 0;

    }
    else if(context_in == true && context_in_count != parameters->t_cs/2 -1) context_in_count +=1;

    // Check to see if any processes have arrived
    if(srt_processes[c_arrival].getState() == 0 && srt_processes[c_arrival].getArrival() == time)
    {
      if(time < 1000) std::cout << "time " << time << "ms: Process " << srt_processes[c_arrival].getCharacter() << " (tau " <<
      srt_processes[c_arrival].getTauValue() << "ms) arrived; added to ready queue ";
      ready_queue.push_back(srt_processes[c_arrival]);           // pushes back to the ready queue
      std::sort(ready_queue.begin(), ready_queue.end(), compareTau);

      srt_processes[c_arrival].setState(1);                      // sets the state to "in ready queue" (1)

      if(time < 1000) printQueue(ready_queue);
      if(c_arrival != parameters->n-1) c_arrival +=1;
    }

    // Check to see if any processes have completed their IO burst
    std::vector<process> sorted_processes;
    for(int i=0; i<(int)srt_processes.size(); i+=1)
    {
      if(srt_processes[i].getState() == 3 && srt_processes[i].getCompleteIO() == time)
      {
        srt_processes[i].setCompleteIO(0);
        srt_processes[i].setState(1);                                      // Once gets out of I/O, moves into ready queue
        sorted_processes.push_back(srt_processes[i]);                           // pushes back to the ready queue
      }
    }
    std::sort(sorted_processes.begin(), sorted_processes.end(), compareChar);
    for(int i=0; i<(int)sorted_processes.size(); i+=1)
    {
      // Check to preempt arriving IO process to the CPU
      if(CPU != -1 && sorted_processes[i].getTauChange() < (srt_processes[CPU].getTauChange()-(time - srt_processes[CPU].getStartTau()))
      && context_in == false)
      {
        // PREEMPT
        if(time < 1000) std::cout << "time " << time << "ms: Process " << sorted_processes[i].getCharacter() <<
        " (tau " << sorted_processes[i].getTauValue() << "ms) completed I/O; preempting " << srt_processes[CPU].getCharacter() << " ";
        ready_queue.push_front(sorted_processes[i]);
        total_preemptions +=1;
        sorted_processes[i].setState(1);            // adds that process to the ready queue
        if(time < 1000) printQueue(ready_queue);

        // The CPU was just preempted. I need to set a remaining burst time, as well as
        srt_processes[CPU].setState(0);
        srt_processes[CPU].CPU_bursts[srt_processes[CPU].current_CPU] -= (time - srt_processes[CPU].getStartTau()); // sets the remaining time
        srt_processes[CPU].current_CPU -= 1;
        srt_processes[CPU].Change_Tau_bursts[srt_processes[CPU].current_tau] -= (time - srt_processes[CPU].getStartTau());

        add_later = CPU;
        CPU = -1;
        context_out = true;
      }
      else
      {
        if(time < 1000) std::cout << "time " << time << "ms: Process " << sorted_processes[i].getCharacter() <<
        " (tau " << sorted_processes[i].getTauValue() << "ms) completed I/O; added to ready queue ";

        ready_queue.push_back(sorted_processes[i]);
        sorted_processes[i].setState(1);
        std::sort(ready_queue.begin(), ready_queue.end(), compareTau);
        if(time < 1000) printQueue(ready_queue);
      }
    }

    // Check to see if any processes can start their CPU Burst from the queue
    if(CPU == -1 && (int)ready_queue.size() > 0 && context_out == false)
    {
      int burst_time = 0;
      int loc = 0;
      for(int i=0; i<(int)srt_processes.size(); i+=1)
      {
        if(ready_queue[0].getCharacter() == srt_processes[i].getCharacter()) { loc = i; break;}
      }

      burst_time = srt_processes[loc].getCPUBurst();
      srt_processes[loc].setState(2);
      srt_processes[loc].setCompleteCPU(time + burst_time + parameters->t_cs/2);
      srt_processes[loc].start_turnaround = time;

      ready_queue.pop_front();
      CPU = loc;
      context_in = true;
      in_burst_time = burst_time;
    }

    // Check to see if each process in srt_processes have terminated (process state == 4)
    bool exit = true;
    if( context_out == false)
    {
      for(int i = 0; i<(int)srt_processes.size(); i++) {
        if(srt_processes[i].getState() != 4) { exit = false; break; }
      }
      if(exit == true) {
        std::cout << "time " << time << "ms: Simulator ended for SRT [Q <empty>]" << std::endl;
        break;
      }
    }

  time +=1;
  }
  // Calculating the metrics for the output file:
  double total_wait_time = 0.0;
  double total_turnaround_time = 0.0;
  double total_bursts = 0.0;
  double total_burst_time = 0.0;
  double average_wait_time = 0.0;
  double average_cpu_burst_time = 0.0;

  for(int i =0; i<(int)srt_processes.size(); i++)
  {
    for(int j=0; j<(int)srt_processes[i].CPU_bursts.size(); j++) { total_burst_time += srt_processes[i].UnChanged_CPU_bursts[j];
      total_turnaround_time += srt_processes[i].UnChanged_CPU_bursts[j] + parameters->t_cs;  }
    total_bursts += srt_processes[i].getTotalBursts();
    total_wait_time += srt_processes[i].wait_time;
    //total_turnaround_time += srt_processes[i].getTotalBursts;
  }

  //std::ofstream out_file = parameters->out_file;
  average_wait_time = total_wait_time/total_bursts;
  average_cpu_burst_time = total_burst_time/total_bursts;
  out_file << std::setprecision(3) << std::fixed;
  out_file << "Algorithm SRT" << std::endl;
  out_file << "-- average CPU burst time: " << average_cpu_burst_time << " ms" << std::endl;
  out_file << "-- average wait time: " << average_wait_time << " ms" << std::endl;
  out_file << "-- average turnaround time: " << total_turnaround_time/total_bursts + (total_preemptions * parameters->t_cs)/total_bursts + average_wait_time << " ms" << std::endl;
  out_file << "-- total number of context switches: " << total_context_switches << std::endl;
  out_file << "-- total number of preemptions: " << total_preemptions << std::endl;
  std::cout << std::endl;
  return;
}

/******************************************************************
Round Robin (Dan)
******************************************************************/
void RR(generator * parameters, std::ostream &out_file)
{
  for(int i=0; i<parameters->n; i+=1) all_processes[i].printProcess(false);

  std::vector<process> rr_processes = all_processes;                          // Make a new vector, as we want to keep the original
  std::sort(rr_processes.begin(), rr_processes.end(), compareArrival);        // will be sorted by the arrival time, and cpu burst

  std::deque<process> ready_queue;                                            // the queue for processes in the "ready" state
  int CPU = -1;                                 // represents the current process in the CPU, storing its process loc. in rr
  int time = rr_processes[0].getArrival();      // the current time to be printed to the console
  int c_arrival = 0;                            // as the vector is sorted, stores the next arrival value

  bool addFront = false;
  if(parameters->rr_add == "BEGINNING") addFront = true;

  bool context_in = false;                      // Bool, represents if the CPU is currently in a context switch (into the CPU)
  int context_in_count = 0;                     // the number of seconds that context_in switch has been running for
  bool context_out = false;                     // Bool, represents if the CPU is currently in a context switch (out of the CPU)
  int context_out_count = 0;                    // the number of seconds that context_out switch has been running for
  int in_burst_time = 0;
  int timeslice = parameters->t_slice;          // The timeslice to be allowed in the cpu
  int add_later = -1;
  int total_context_switches = 0;               // The total number of times a value switches into the CPU
  int total_preemptions = 0;

  // Starting the simulation
  std::cout << "time 0ms: Simulator started for RR [Q <empty>]" << std::endl;
  while(true)
  {

    if(context_out == true)
    {
      context_out_count +=1;
      if(context_out_count == parameters->t_cs/2){
        if(add_later!=-1)
        {
          rr_processes[add_later].setState(1);
          if(addFront == false) ready_queue.push_back(rr_processes[add_later]);
          else ready_queue.push_front(rr_processes[add_later]);
          add_later = -1;
        }
        context_out = false;
        context_out_count = 0;
      }
    }

    // Check to see if any processes have completed their CPU burst
    if( CPU != -1 && rr_processes[CPU].getPreemption() == 1 && time == rr_processes[CPU].getSliceBurst() && context_in == false)
    {
      if((int)ready_queue.size() == 0)         // if there are no values in the queue, we can skip preemption
      {
        if(time < 1000) std::cout << "time " << time << "ms: Time slice expired; no preemption because ready queue is empty ";
        if(time < 1000) printQueue(ready_queue);
        rr_processes[CPU].setState(2);
        int burst_time = rr_processes[CPU].getRemainingBurst();

        if( burst_time > timeslice)         //There are still more preemptions
        {
          rr_processes[CPU].setPreemption(1);
          rr_processes[CPU].setRemainingBurst(burst_time - timeslice);
          rr_processes[CPU].setSliceBurst(time + timeslice + parameters->t_cs/2);

          rr_processes[CPU].setCompleteCPU(time + burst_time + parameters->t_cs/2);
        }
        else                              // There are no more preemptions
        {
          rr_processes[CPU].setPreemption(0);
          rr_processes[CPU].setCompleteCPU(time + burst_time );
          rr_processes[CPU].setSliceBurst(-1);
        }
      }
      else                            // If there are values in the queue, we must preempt
      {
        if(time < 1000) std::cout << "time " << time << "ms: Time slice expired; process " << rr_processes[CPU].getCharacter() <<
        " preempted with " << rr_processes[CPU].getRemainingBurst() << "ms to go ";
        total_preemptions +=1;
        if(time < 1000) printQueue(ready_queue);

        rr_processes[CPU].setState(2);            // Add it to the back of the ready queue
        rr_processes[CPU].setSliceBurst(-1);
        add_later = CPU;
        context_out = true;
        CPU = -1;
      }
    }
    else if(CPU != -1 && rr_processes[CPU].getCompleteCPU() == time && context_in == false)
    {

      rr_processes[CPU].setCompleteCPU(0);          // Resets the CPU
      if(rr_processes[CPU].getBurstNum() == 1)      // Check for terminated process
      {
        std::cout << "time " << time << "ms: Process " << rr_processes[CPU].getCharacter() <<
        " terminated ";
        rr_processes[CPU].total_turnaround_time += (time - rr_processes[CPU].start_turnaround + parameters->t_cs/2);
        printQueue(ready_queue);
        rr_processes[CPU].setState(4);              // Process is now terminated
        context_out = true;
        CPU = -1;                                   // reset CPU availability
      }
      else
      {
        rr_processes[CPU].setBurstNum(rr_processes[CPU].getBurstNum()-1);// subtracts a burst value
        if(time < 1000)
        {
          std::cout << "time " << time << "ms: Process " << rr_processes[CPU].getCharacter() <<
          " completed a CPU burst; " << rr_processes[CPU].getBurstNum();
          if(rr_processes[CPU].getBurstNum() > 1) std::cout << " bursts to go ";
          else std::cout << " burst to go ";
        }
        if(time < 1000) printQueue(ready_queue);
        rr_processes[CPU].setRemainingBurst(0);                   // Resets burst as it is done
        rr_processes[CPU].setPreemption(0);                       // Resets burst as it is done
        rr_processes[CPU].total_turnaround_time += (time - rr_processes[CPU].start_turnaround + parameters->t_cs/2);

        // Now .. moves to IO
        if(time < 1000) std::cout << "time " << time << "ms: Process " << rr_processes[CPU].getCharacter() <<
        " switching out of CPU; will block on I/O until time ";

        rr_processes[CPU].setState(3);
        rr_processes[CPU].setCompleteIO(time + rr_processes[CPU].getIOBurst());

        if(time < 1000) std::cout << rr_processes[CPU].getCompleteIO() << "ms ";
        if(time < 1000) printQueue(ready_queue);
        context_out = true;
        CPU = -1;                                     // reset CPU availability
      }

    }

    // Check to see if any processes have arrived
    if(rr_processes[c_arrival].getState() == 0 && rr_processes[c_arrival].getArrival() == time)
    {
      if(time < 1000) std::cout << "time " << time << "ms: Process " << rr_processes[c_arrival].getCharacter() <<
      " arrived; added to ready queue ";

      if(addFront == false) ready_queue.push_back(rr_processes[c_arrival]);           // pushes back to the ready queue
      else ready_queue.push_front(rr_processes[c_arrival]);

      rr_processes[c_arrival].setState(1);                      // sets the state to "in ready queue" (1)

      if(time < 1000) printQueue(ready_queue);
      if(c_arrival != parameters->n-1) c_arrival +=1;
    }

    // Check status of context in switch
    if(context_in == true && context_in_count == parameters->t_cs/2 -1)
    {
      if(rr_processes[CPU].getRemaining() == false)
      {
        if(time < 1000) std::cout << "time " << time << "ms: Process " << rr_processes[CPU].getCharacter() <<
        " started using the CPU for " << in_burst_time << "ms burst ";
        if(time < 1000) printQueue(ready_queue);
      }
      else
      {
        if(time < 1000) std::cout << "time " << time << "ms: Process " << rr_processes[CPU].getCharacter() <<
        " started using the CPU with " << in_burst_time << "ms burst remaining ";
        if(time < 1000) printQueue(ready_queue);
      }
      total_context_switches +=1;
      context_in = false;
      context_in_count = 0;
      in_burst_time = 0;
    }
    else if(context_in == true && context_in_count != parameters->t_cs/2 -1) context_in_count +=1;


    // Check to see if any processes have completed their IO burst
    std::vector<process> sorted_processes;
    for(int i=0; i<(int)rr_processes.size(); i+=1)
    {
      if(rr_processes[i].getState() == 3 && rr_processes[i].getCompleteIO() == time)
      {
        rr_processes[i].setCompleteIO(0);
        rr_processes[i].setState(1);                                      // Once gets out of I/O, moves into ready queue
        sorted_processes.push_back(rr_processes[i]);                           // pushes back to the ready queue
      }
    }
    std::sort(sorted_processes.begin(), sorted_processes.end(), compareChar);
    for(int i=0; i<(int)sorted_processes.size(); i+=1)
    {
      if(time < 1000) std::cout << "time " << time << "ms: Process " << sorted_processes[i].getCharacter() <<
      " completed I/O; added to ready queue ";
      if(addFront == false) ready_queue.push_back(sorted_processes[i]);
      else ready_queue.push_front(sorted_processes[i]);
      if(time < 1000)printQueue(ready_queue);
    }

    // Check to see if any processes can start their CPU Burst from the queue
    if(CPU == -1 && (int)ready_queue.size() > 0 && context_out == false)
    {
      int burst_time = 0;
      int loc = 0;
      for(int i=0; i<(int)rr_processes.size(); i+=1)
      {
        if(ready_queue[0].getCharacter() == rr_processes[i].getCharacter()) { loc = i; break; }
      }
      rr_processes[loc].setRemaining(false);

      if( rr_processes[loc].getPreemption() == 0 ) { burst_time = rr_processes[loc].getCPUBurst(); rr_processes[loc].start_turnaround = time;}  // Either starting ....
      else { burst_time = rr_processes[loc].getRemainingBurst();  rr_processes[loc].setRemaining(true); }  // Or was preempted


      if(burst_time > timeslice)                     // If the burst time is greater than the timeslice
      {
        rr_processes[loc].setPreemption(1);               // There is a preemption needed for the next CPU burst
        rr_processes[loc].setRemainingBurst(burst_time - timeslice);  // The remaining time on the burst (stored)
        rr_processes[loc].setSliceBurst(time + timeslice + parameters->t_cs/2);
        // burst_time = timeslice;                                       // Sets the burst time accordingly
      }
      else
      {
        rr_processes[loc].setPreemption(0);               // Otherwise, there is no preemption
        rr_processes[loc].setRemainingBurst(burst_time);  // The remaining time on the burst (stored)
      }

      rr_processes[loc].setState(2);
      rr_processes[loc].setCompleteCPU(time + burst_time + parameters->t_cs/2);

      ready_queue.pop_front();
      CPU = loc;
      context_in = true;
      in_burst_time = burst_time;
    }

    // Check to see if each process in rr_processes have terminated (process state == 4)
    bool exit = true;
    if( context_out == false)
    {
      for(int i = 0; i<(int)rr_processes.size(); i++) {
        if(rr_processes[i].getState() != 4) { exit = false; break; }
      }
      if(exit == true) {
        std::cout << "time " << time << "ms: Simulator ended for RR [Q <empty>]" << std::endl;
        break;
      }
    }
    for(int o = 0; o<(int)rr_processes.size(); o++) if(rr_processes[o].getState() == 1) rr_processes[o].wait_time +=1;
    time +=1;
  }

  // Calculating the metrics for the output file:
  double total_wait_time = 0.0;
  double total_turnaround_time = 0.0;
  double total_bursts = 0.0;
  double total_burst_time = 0.0;
  double average_wait_time = 0.0;
  double average_cpu_burst_time = 0.0;

  for(int i =0; i<(int)rr_processes.size(); i++)
  {
    for(int j=0; j<(int)rr_processes[i].CPU_bursts.size(); j++) { total_burst_time += rr_processes[i].CPU_bursts[j];
      total_turnaround_time += rr_processes[i].UnChanged_CPU_bursts[j] + parameters->t_cs; }
    total_bursts += rr_processes[i].getTotalBursts();
    total_wait_time += rr_processes[i].wait_time;
    //total_turnaround_time += rr_processes[i].total_turnaround_time;
  }

  average_wait_time = total_wait_time/total_bursts;
  average_cpu_burst_time = total_burst_time/total_bursts;
  out_file << std::setprecision(3) << std::fixed;
  out_file << "Algorithm RR" << std::endl;
  out_file << "-- average CPU burst time: " << average_cpu_burst_time << " ms" << std::endl;
  out_file << "-- average wait time: " << average_wait_time << " ms" << std::endl;
  out_file << "-- average turnaround time: " << total_turnaround_time/total_bursts + (total_preemptions * parameters->t_cs)/total_bursts + average_wait_time << " ms" << std::endl;
  out_file << "-- total number of context switches: " << total_context_switches << std::endl;
  out_file << "-- total number of preemptions: " << total_preemptions << std::endl;
  return;
}

int main(int argc, char*argv[])
{
  if(argc < 8)
  {
    std::cerr << "ERROR: <Improper Arguments>" << std::endl;
    exit(EXIT_FAILURE);
  }

  generator * parameters = new generator();
  parameters->seed = atoi(argv[1]);                     // The random seed for the number generator
  parameters->lambda = atof(argv[2]);                   // Lambda value for determining interarrival times
  parameters->upper_bound = atoi(argv[3]);              // The upper bound value for exponential distribution
  parameters->n = atoi(argv[4]);                        // The number of processes to simulate
  parameters->t_cs = atoi(argv[5]);                     // The time in milliseconds that it takes to perform a context switch
  parameters->alpha = atof(argv[6]);                    // The alpa value in SJF and SRT algorithms
  parameters->t_slice = atof(argv[7]);                  // The time slice for RR, measured in milliseconds
  parameters->rr_add = "END";                           // Optional command-line argument, added to the beginning or end (default) of
  if(argc == 9) parameters->rr_add = argv[8];           // the ready queue

  std::ofstream file;
  file.open("simout.txt");

  getProcesses(parameters);
  getTauVals(parameters);
  FCFS(parameters, file);
  SJF(parameters, file);
  SRT(parameters, file);
  RR(parameters, file);

  file.close();
  free(parameters);
  exit(EXIT_SUCCESS);
}
