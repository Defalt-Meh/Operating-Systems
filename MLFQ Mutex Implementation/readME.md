Multi‑Level Feedback Queue (MLFQ) Mutex Assignment

This document describes the design and implementation of an MLFQ‑based mutex for a concurrent programming assignment. You can paste this directly into your GitHub repository as README.md or any other .md file.

Table of Contents

Overview
Components
Concurrent Queue
Garage (Thread Parking)
MLFQMutex
Detailed Design
Data Structures
Constructor
Locking Protocol
Unlocking & Priority Adjustment
Printing State
Usage Example
Expected Output Format
Build & Run Instructions
References
Overview

The goal of this assignment is to implement a fair and efficient mutex using a Multi‑Level Feedback Queue (MLFQ) scheduler. Threads that hold the lock for long are demoted to lower‑priority queues; threads that exhaust their quantum are moved down, while short‑running threads stay at higher levels. Parked threads are blocked (no busy‑waiting) via a Garage mechanism.

Components

1. Concurrent Queue
We use a Michael & Scott two‑lock FIFO queue for each priority level:

Fine‑grained locking: separate head_lock and tail_lock allow parallel enqueues and dequeues.
Dummy node simplifies empty‑queue handling.
Supports enqueue(pthread_t tid) and pthread_t dequeue() operations.
2. Garage (Thread Parking)
The Garage class provides:

void setPark();                // prepare the current thread to be parked  
pthread_t park();              // block until another thread calls unpark()  
void unpark(pthread_t tid);    // wake the specified thread  
Threads that cannot acquire the mutex call setPark() → enqueue → park().

3. MLFQMutex
Coordinates threads across multiple priority levels:

class MLFQMutex {
public:
  // levels: number of queues; baseQuantum: time slice in seconds
  MLFQMutex(int levels, double baseQuantum);

  void lock();    // Acquire the mutex (blocks if necessary)
  void unlock();  // Release the mutex and adjust priority
  void print();   // Print debug info about queued threads
};
Detailed Design

Data Structures
atomic_flag imperialSeal
Indicates whether the mutex is held (test_and_set / clear).
std::vector<Queue<pthread_t>*> battleLines
One FIFO queue per priority level.
std::unordered_map<pthread_t,int> threadLevels
Tracks each thread’s current priority level.
double baseQuantum
Base time slice for level 0. Higher levels use multiples:
quantum[level] = baseQuantum * (level + 1)
std::chrono::time_point<> startTime
Recorded when a thread acquires the lock.
Constructor
MLFQMutex::MLFQMutex(int levels, double baseQuantum)
  : numLevels(levels), baseQuantum(baseQuantum)
{
  imperialSeal.clear();  // Ensure unlocked at start
  for (int i = 0; i < levels; ++i) {
    battleLines.push_back(new Queue<pthread_t>());
  }
}
Locking Protocol
Fast path:
if (!imperialSeal.test_and_set()) {
  // Lock was free
  startTime = now();
  if (!threadLevels.count(this_thread))
    threadLevels[this_thread] = 0;
  return;
}
Blocking path:
garage.setPark();
int lvl = threadLevels[this_thread];
battleLines[lvl]->enqueue(this_thread);
garage.park();       // Blocks here
startTime = now();   // Resumed, record entry time
Unlocking & Priority Adjustment
void MLFQMutex::unlock() {
  auto endTime = high_resolution_clock::now();
  double runSec = duration<double>(endTime - startTime).count();
  int lvl = threadLevels[this_thread];
  double q = baseQuantum * (lvl + 1);

  // Demote if ran >= quantum
  if (runSec >= q && lvl + 1 < numLevels)
    ++threadLevels[this_thread];

  // Unpark next waiting thread at highest non‑empty level
  for (int L = 0; L < numLevels; ++L) {
    if (!battleLines[L]->empty()) {
      pthread_t next = battleLines[L]->dequeue();
      garage.unpark(next);
      break;
    }
  }
  imperialSeal.clear();  // Release lock
}
Fairness: Threads that exhaust their slice move down one queue.
Correctness: Atomic flag ensures single ownership.
Printing State
void MLFQMutex::print() {
  for (int i = 0; i < numLevels; ++i) {
    std::cout << "Level " << i << ": ";
    battleLines[i]->print();  // Assumes Queue has print()
    std::cout << "\n";
  }
}
Usage Example

#include "MLFQmutex.h"
#include <thread>

void criticalSection(MLFQMutex &m) {
  m.lock();
  // ... critical work ...
  m.unlock();
}

int main() {
  MLFQMutex mlfq(4, 0.005);  // 4 levels, 5ms base quantum

  std::thread t1(criticalSection, std::ref(mlfq));
  std::thread t2(criticalSection, std::ref(mlfq));
  std::thread t3(criticalSection, std::ref(mlfq));

  t1.join(); t2.join(); t3.join();
  mlfq.print();
  return 0;
}
Expected Output Format

Your implementation must not modify cout formatting. Example runtime output:

Thread with program ID 0 and thread ID 139628515849792 acquired lock
Adding thread with ID: 139628507457088 to level 0
Adding thread with ID: 139628499064384 to level 0
Thread with program ID 0 and thread ID 139628515849792 releasing lock
Adding thread with ID: 139628515849792 to level 1
Thread with program ID 1 and thread ID 139628507457088 acquired lock
...
Build & Run Instructions

# Compile
g++ -std=c++17 -pthread -o test_mlfq \
    queue.h queue.cpp \
    MLFQmutex.h MLFQmutex.cpp \
    park.h park.cpp \
    test_mlfq.cpp

# Run
./test_mlfq
References

Michael, M. M., & Scott, M. L. (1996). Simple, fast, and practical non‑blocking and blocking concurrent queue algorithms.
Tanenbaum, A. S., & Bos, H. (2015). Modern Operating Systems (4th ed.). Prentice Hall.
