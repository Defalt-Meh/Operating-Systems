#include <iostream>
#include <random>
#include <pthread.h>
#include <unistd.h>
#include <queue.h>
#include <sched.h>
using namespace std;

#define PRINT_EL_NUM 100
#define THREAD_NUM 2

Queue<pthread_t> q;

void* enq(void* arg) { 
    long int base = (long int) arg;
    base = (base) * PRINT_EL_NUM;
    printf("Thread with base: %ld started.\n",base);
    for (int i = base; i < (base) +PRINT_EL_NUM; i++)
        q.enqueue(i);
    return NULL;

}

int main() {
    printf("Hello, from main.\n");
    pthread_t thr[THREAD_NUM];
    long int nums[THREAD_NUM];
    for(int i=0;i<THREAD_NUM;i++){
        nums[i] = i;
        pthread_create(&thr[i], NULL, enq, (void*)nums[i]);
    }
    //wait until all threads terminated
    for(int i=0;i<THREAD_NUM;i++){
        pthread_join(thr[i], NULL);
    }
    
    for (int i=0; i < THREAD_NUM*PRINT_EL_NUM - PRINT_EL_NUM; i++)
        int x = q.dequeue();
    printf("Threads terminated. Resulting queue state:\n");
    q.print();
    return 0;
}
