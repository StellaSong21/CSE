/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include "thread-pool.h"
#include <unistd.h>
using namespace std;

void ThreadPool::dispatcher(){
	/* check queue */
}

void ThreadPool::worker(){
    while(running){
        function<void(void)> task;
        /* check is there a task */
        {
            unique_lock<mutex> lock(mtx);
            if(!tasks.empty()){
                task = tasks.front();
                tasks.pop();
            } 
            else if(running && tasks.empty()){
                cond.wait(lock);
            }
        }
        /* do task */
        if(task){
            task();
        }
    }
}

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads) {
    dt = thread([this]() {
        dispatcher();  
    });

    for (size_t workerID = 0; workerID < numThreads; workerID++) {
        wts[workerID] = thread([this]() {
            worker();
        });
    }
}

void ThreadPool::schedule(const function<void(void)>& thunk) {
    if(running){
        unique_lock<mutex> lock(mtx);
        tasks.push(thunk);
        /* notify a thread to run the task */
        cond.notify_one();
    }
}

void ThreadPool::wait() {
    bool empty = false;
    while(!empty){
        unique_lock<mutex> lock(mtx);
        empty = tasks.empty();
        lock.unlock();
    }
}

ThreadPool::~ThreadPool() {
    wait();
    {
        // stop thread pool, should notify all threads to wake
        std::unique_lock<std::mutex> lk(mtx);
        running = false;
        cond.notify_all(); // must do this to avoid thread block
    }
    if(dt.joinable()){
        dt.join();
    }
    for (std::thread& t : wts){
        if (t.joinable())
            t.join();
    }
}
