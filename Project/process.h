/******************************************************************

process.h
This header file stores the process class for the project.cpp file.
Each process is created in project.cpp, and is ran through each of
the scheduling algorithms as inputted

Authors: Daniel Dukeshire (dukesd3)
Date 4.2.2020

******************************************************************/

#include <stdio.h>
#include <iostream>

class process
{
  private:

    char character;                 // character ID
    int arrival_time;               // arrival time
    int num_bursts;                 // the number of bursts left
    int total_bursts;               // Not to be changed



    int state;                      // 0 = not in use
                                    // 1 = ready queue
                                    // 2 = in CPU
                                    // 3 = in IO
                                    // 4 = terminated


    int current_IO;                 // Counter for the current io burst
    int complete_cpu_burst;         // The time cpu burst will complete
                                    // (time + CPU_burst)
    int complete_io_burst;          // The time io burst will complete
                                    // (time + IO_burst)
    int slice_burst;                // The time the preemption burst ends

    int is_preemption;              // 0 = is not preempted
                                    // 1 = not preempted
                                    // 2 = preempted, but nothing in the queue

    int remaining_burst;            // if preempted, the remaining burst time

    bool is_remaining;              // Used for print statements, if it is "remaining time"
    int start_tau_time;             // The time the CPU tau burst started

  public:
      std::vector<int> CPU_bursts;    // the CPU burst times
      std::vector<int> UnChanged_CPU_bursts;    // the CPU burst times
      std::vector<int> IO_bursts;     // the IO burst time
      std::vector<int> Tau_bursts;
      std::vector<int> Change_Tau_bursts; // Tau Bursts alter over time (save print)
      int wait_time;                  // the amount of time a process spends in the queue
      int total_turnaround_time;
      int start_turnaround;
      int current_CPU;                // Counter for the current cpu burst
      int current_tau;

      // ACCESSORS
      char getCharacter()         { return character; }
      int getArrival()            { return arrival_time; }
      int getBurstNum()           { return num_bursts; }
      int getCPUBurst()           { current_CPU+=1; return CPU_bursts[current_CPU]; }
      int getIOBurst()            { current_IO+=1; return IO_bursts[current_IO]; }
      int getState()              { return state; }
      int getCompleteCPU()        { return complete_cpu_burst; }
      int getCompleteIO()         { return complete_io_burst; }
      int getCurrentCPU()         { return current_CPU; }
      int getCurrentIO()          { return current_IO; }
      int getPreemption()         { return is_preemption; }
      int getRemainingBurst()     { return remaining_burst; }
      int getSliceBurst()         { return slice_burst; }
      bool getRemaining()         { return is_remaining; }
      int getCurrentTau()         { return current_tau; }
      int getTauValue()           { return Tau_bursts[current_tau]; }
      int getTotalBursts()        { return total_bursts; }
      int getStartTau()           { return start_tau_time; }
      int getTauChange()           { return Change_Tau_bursts[current_tau]; }

      // SETTERS
      void setCharacter(char c)   { character = c; return; }
      void setArrival(int a)      { arrival_time = a; return; }
      void setBurstNum(int b)     { num_bursts = b; return; }
      void addCPUBurst(int cpu)   { CPU_bursts.push_back(cpu);
                                    UnChanged_CPU_bursts.push_back(cpu); return; }
      void addIOBurst(int io)     { IO_bursts.push_back(io); return; }
      void addTauValue(int tau)    { Tau_bursts.push_back(tau);
                                      Change_Tau_bursts.push_back(tau); return; }
      void setState(int s)        { state = s; return; }
      void setCompleteCPU(int c)  { complete_cpu_burst = c; return; }
      void setCompleteIO(int i)   { complete_io_burst = i; return; }
      void setCurrentCPU()        { current_CPU = -1; return; }
      void setCurrentIO()         { current_IO = -1; return; }
      void setPreemption(int p)   { is_preemption = p; return; }
      void setRemainingBurst(int b) { remaining_burst = b; return; }
      void setSliceBurst(int p)   { slice_burst = p; return; }
      void setRemaining(bool r)   { is_remaining = r; return; }
      void setCurrentTauVal(int tau) { current_tau = tau; return; }
      void setRemainingTau(int tau) { Tau_bursts[current_tau] = tau; return;}
      void setTotalBursts(int b)  { total_bursts = b; return; }
      void setStartTau(int t)     { start_tau_time = t; return; }

      // FUNCTIONS
      void printProcess(bool is_tau)
      {
        std::cout<< "Process " << character << " [NEW] (arrival time " << arrival_time << " ms) ";
        if(num_bursts > 1) std::cout << num_bursts << " CPU bursts";
        else std::cout << num_bursts << " CPU burst";
        if(is_tau == false) std::cout<<std::endl;
        else std::cout << " (tau " << Tau_bursts[0] << "ms)" << std::endl;
      }
};
