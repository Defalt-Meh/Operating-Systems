#ifndef MLFQ_MUTEX_H
#define MLFQ_MUTEX_H

#include <iostream>
#include <pthread.h>
#include <unistd.h>
#include "queue.h"     // implementation of Queue template is already made.
#include "park.h"      // Assumes implementation of Garage class is provided.
#include <sched.h>
#include <chrono>
#include <atomic>
#include <cmath>
#include <vector>

// In the ancient jungles of Lustria, the Saurus caste binds its rites to the flow of time.
using namespace std;
using namespace std::chrono;

// A ritual spinlock forged by the Old Ones, with a measured back-off chant.
class SpinLock {
private:
    atomic_flag flag = ATOMIC_FLAG_INIT;
public:
    // The invocation loops until the spirit of the lock submits, bowing after many spins.
    void lock() {
        int spins = 0;
        const int SPIN_THRESHOLD = 1000; // After this many vortices, yield to another champion.
        while (flag.test_and_set(memory_order_acquire)) {
            spins++;
            if (spins >= SPIN_THRESHOLD) {
                spins = 0;
                sched_yield(); // Offer the battlefield to another warrior.
            }
            // A brief pause in the winds of magic could be invoked here.
        }
    }
    
    // Release the ancient seal, letting the glyph rest until called again.
    void unlock() {
        flag.clear(memory_order_release);
    }
};

class MLFQMutex {
private:
    // Each Salamander warrior carries his caste’s rank in his bones.
    static thread_local int threadPriority;

    // The warding glyph that guards all ritual changes.
    SpinLock internalLock;
    
    // The Eternal Seal stands until a champion breaks and holds it.
    atomic_flag mutexFlag { ATOMIC_FLAG_INIT };
    
    // Queues denoting waiting threads at different priority levels.
    vector<Queue<pthread_t>*> priorityQueues;
    
    // A bitmask of sigils marks which queues are active.
    unsigned int activeQueuesMask = 0;
    
    int levels;             // Total number of priority levels.
    double quantumSeconds;  // Time quantum for priority adjustment.
    
    // Chronicles the moment a champion seizes the seal.
    time_point<high_resolution_clock> startTime;
    time_point<high_resolution_clock> endTime;
    
    // The Spirit Gate sends and recalls warriors.
    Garage garage;
    
    // Inscribe a queue’s banner as active.
    void markQueueActive(int index) {
         activeQueuesMask |= (1u << index);
    }
    
    // If a queue is empty, its banner fades from the sigil mask.
    void markQueueInactive(int index) {
         if (priorityQueues[index]->isEmpty()) {
             activeQueuesMask &= ~(1u << index);
         }
    }
    
    // The Ancestral Rite adjusts a warrior’s station by his feat’s duration.
    void adjustThreadPriority(chrono::seconds duration) {
         double execTime = static_cast<double>(duration.count());
         int currentLevel = threadPriority;
         int newLevel = currentLevel + static_cast<int>(floor(execTime / quantumSeconds));
         if (newLevel >= levels) {
             newLevel = levels - 1;
         }
         threadPriority = newLevel;
    }
    
    // Rally the calling thread into his proper queue of honor.
    void enqueueThread() {
         pthread_t id = pthread_self();
         int lvl = threadPriority;
         priorityQueues[lvl]->enqueue(id);
         markQueueActive(lvl);
         cout << "Adding thread with ID: " << id
              << " to level " << threadPriority << endl;
         cout.flush();
    }
    
    // Summon the next champion from the highest priority non-empty queue.
    pthread_t dequeueNextThread() {
         if (activeQueuesMask == 0) {
             return (pthread_t)-1;  // No threads waiting.
         }
         int idx = __builtin_ctz(activeQueuesMask);
         pthread_t next = priorityQueues[idx]->dequeue();
         markQueueInactive(idx);
         return next;
    }
    
public:
    // Forge the mutex with N levels and the given quantum.
    MLFQMutex(int numLevels, double quantum)
        : levels(numLevels), quantumSeconds(quantum) {
        priorityQueues.reserve(numLevels);
        for (int i = 0; i < numLevels; i++) {
            priorityQueues.push_back(new Queue<pthread_t>());
        }
        // Ensure the Eternal Seal lies dormant.
        mutexFlag.clear(memory_order_release);
    }
    
    // Unbind all queues when the mutex is destroyed.
    ~MLFQMutex() {
        for (auto ptr : priorityQueues) {
            delete ptr;
        }
    }
    
    // Invoke lock: attempt fast path or queue and park.
    void lock() {
         internalLock.lock();
         if (!mutexFlag.test_and_set(memory_order_acquire)) {
             // Fast path: seal acquired.
             internalLock.unlock();
         } else {
             // Slow path: queue and park.
             enqueueThread();
             garage.setPark();
             internalLock.unlock();
             garage.park();
         }
         // Chronicle start of critical section.
         startTime = high_resolution_clock::now();
    }
    
    // Release lock: measure, adjust, and unpark or release seal.
    void unlock() {
         internalLock.lock();
         endTime = high_resolution_clock::now();
         chrono::seconds duration = duration_cast<chrono::seconds>(endTime - startTime);
         adjustThreadPriority(duration);
         
         pthread_t next = dequeueNextThread();
         if (next != (pthread_t)-1) {
             garage.unpark(next);
         } else {
             mutexFlag.clear(memory_order_release);
         }
         internalLock.unlock();
    }
    
    // Display waiting threads per level.
    void print() {
         cout << "Waiting threads:" << endl;
         for (int i = 0; i < levels; i++) {
             cout << "Level " << i << ": ";
             priorityQueues[i]->print();
         }
    }
};

// Each thread begins at the primordial level.
thread_local int MLFQMutex::threadPriority = 0;

#endif  // MLFQ_MUTEX_H

