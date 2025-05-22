#ifndef COURT_H
#define COURT_H

#include <pthread.h>
#include <semaphore.h>
#include <stdexcept>
#include <cstdio>
#include <iostream>

/*--------------------------------------------------------------
  Basketball-Court Synchronizer — PA-3 compliant
  --------------------------------------------------------------*/
class Court {
private:
    /* immutable configuration */
    const int  playerCount;          // players needed
    const bool useReferee;           // true ⇢ one entrant is referee
    const int  totalCount;           // playerCount + (useReferee?1:0)

    /* shared state (guarded by mutex) */
    int        insideCount  = 0;     // current threads on court
    bool       matchStarted = false; // becomes true when insideCount==totalCount
    pthread_t  refereeTid   = 0;     // thread ID of referee (if any)

    /* synchronization primitives */
    sem_t slots;      // limits court capacity & blocks while match running
    sem_t mutex;      // binary lock for shared variables
    sem_t leaveSem;   // players wait here until referee leaves first

    /* helper: reopen court for next potential match (mutex already held) */
    void reopenCourt_unlocked() {
        matchStarted = false;
        for (int i = 0; i < totalCount; ++i)
            sem_post(&slots);        // wake blocked entrants / refill capacity
    }

public:
    /* play() body is provided in the test driver — declare only */
    void play();

    /*------------------------ ctor / dtor ---------------------*/
    Court(int playersNeeded, int refereeNeeded)
        : playerCount(playersNeeded),
          useReferee(refereeNeeded == 1),
          totalCount(playersNeeded + (useReferee ? 1 : 0))
    {
        if (playerCount <= 0 || (refereeNeeded != 0 && refereeNeeded != 1))
            throw std::invalid_argument("An error occurred.");

        sem_init(&slots,    0, totalCount);   // all slots free
        sem_init(&mutex,    0, 1);
        sem_init(&leaveSem, 0, 0);
    }

    ~Court() {
        sem_destroy(&slots);
        sem_destroy(&mutex);
        sem_destroy(&leaveSem);
    }

    /*--------------------------- enter ------------------------*/
    void enter() {
        unsigned long tid = (unsigned long)pthread_self();
        printf("Thread ID: %lu, I have arrived at the court.\n", tid);

        sem_wait(&slots);                     // gate / capacity control

        sem_wait(&mutex);
            ++insideCount;
            if (insideCount == totalCount) {  // last required entrant
                matchStarted = true;
                if (useReferee) refereeTid = pthread_self();
                printf("Thread ID: %lu, There are enough players, starting a match.\n", tid);
            } else {
                printf("Thread ID: %lu, There are only %d players, passing some time.\n",
                       tid, insideCount);
            }
        sem_post(&mutex);
    }

    /*--------------------------- leave ------------------------*/
    void leave() {
        unsigned long tid = (unsigned long)pthread_self();

        sem_wait(&mutex);

        /* ---------- case A: no match ever started ------------*/
        if (!matchStarted) {
            --insideCount;
            printf("Thread ID: %lu, I was not able to find a match and I have to leave.\n", tid);
            sem_post(&slots);                 // free my slot
            sem_post(&mutex);
            return;
        }

        /* ---------- case B: match finished — referee branch --*/
        if (useReferee && pthread_equal(pthread_self(), refereeTid)) {
            bool last = (insideCount == 1);  // test before decrement
            --insideCount;
            printf("Thread ID: %lu, I am the referee and now, match is over. I am leaving.\n", tid);

            for (int i = 0; i < totalCount - 1; ++i)
                sem_post(&leaveSem);          // wake players

            if (last) {
                printf("Thread ID: %lu, everybody left, letting any waiting people know.\n", tid);
                reopenCourt_unlocked();       // reset for next match
            }
            sem_post(&mutex);
            return;
        }

        /* ---------- case C: match finished — player branch ---*/
        if (useReferee) {                     // wait only if a referee exists
            sem_post(&mutex);                 // release while waiting
            sem_wait(&leaveSem);              // block until referee posts
            sem_wait(&mutex);                 // re-acquire
        }

        bool last = (insideCount == 1);       // decide before decrement
        --insideCount;
        printf("Thread ID: %lu, I am a player and now, I am leaving.\n", tid);

        if (last) {
            printf("Thread ID: %lu, everybody left, letting any waiting people know.\n", tid);
            reopenCourt_unlocked();
        }
        sem_post(&mutex);
    }
};

#endif /* COURT_H */
