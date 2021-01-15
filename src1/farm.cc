#include <cassert>
#include <ctime>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>
#include <sched.h>
#include "subprocess.h"

using namespace std;

static const char *kWorkerArguments[] = {"python3", "./factor.py", "--selfhalting", NULL};

struct worker {  
	worker() {}  
	worker(char *argv[]) : 
		sp(subprocess(argv, true, false)), available(false) {}  
	subprocess_t sp;  
	bool available; 
}; 
static const size_t kNumCPUs = sysconf(_SC_NPROCESSORS_ONLN); 
static vector<worker> workers(kNumCPUs); // space for kNumCPUs, zero-arg constructed workers 
static size_t numWorkersAvailable = 0;

static void spawnAllWorkers();
static void markWorkersAsAvailable(int sig);
static void broadcastNumbersToWorkers();
static size_t getAvailableWorker();
static void waitForAllWorkers();
static void closeAllWorkers();


int main(int argc, char *argv[]) {  
	signal(SIGCHLD, markWorkersAsAvailable);  
	spawnAllWorkers();  
	broadcastNumbersToWorkers();  
	waitForAllWorkers();  
	closeAllWorkers();  
	return 0; 
}

/*
 * spawns a self-halting factor.py process
 * for each core and 
 */
static void spawnAllWorkers(){
	cout << "There are this many CPUs: " << kNumCPUs << ", numbered 0 through "
		<< (kNumCPUs -1) << ". " << endl;
	sigset_t sigMask;
	sigemptyset(&sigMask);
	sigaddset(&sigMask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &sigMask, NULL);

	for(size_t i = 0; i < kNumCPUs; i++){
		struct worker ww(const_cast<char **>(kWorkerArguments));
		workers[i] = ww;
		cpu_set_t cpu_set;
		CPU_ZERO(&cpu_set);
		CPU_SET(i, &cpu_set);
		sched_setaffinity(workers[i].sp.pid, sizeof(cpu_set_t), &cpu_set);
		cout << "Worker " << workers[i].sp.pid << " is set to run on CPU " << i << "." << endl;
	}
	sigprocmask(SIG_UNBLOCK, &sigMask, NULL);
}

static void markWorkersAsAvailable(int sig){
	int status;
		pid_t pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
		if(pid < 0){
			return;
		}
		if(!WIFSTOPPED(status)){
			return;
		}
		for(size_t i = 0; i < kNumCPUs; i++){
			if(workers[i].sp.pid == pid){
				workers[i].available = true;
				numWorkersAvailable++;
				break;
			}
		}
	}


static void broadcastNumbersToWorkers() {
	while (true) {
	    string line;
	    getline(cin, line);
	    if (cin.fail()) break;
	    size_t endpos;
	    /* long long num = */ stoll(line, &endpos); /* string to long long */
	    if (endpos != line.size()) break;
	    size_t i = getAvailableWorker();
	    write(workers[i].sp.supplyfd, line.c_str()/* string to char* */, line.size());
	    write(workers[i].sp.supplyfd, "\n", 1);
	    kill(workers[i].sp.pid, SIGCONT);
	    // you shouldn't need all that many lines of additional code
	}
}

static size_t getAvailableWorker(){
	sigset_t additions, existingmask;
	sigemptyset(&additions);
	sigaddset(&additions, SIGCHLD);
	sigprocmask(SIG_BLOCK, &additions, &existingmask);
	while(numWorkersAvailable == 0){
		sigsuspend(&existingmask);
	}
	for(size_t i = 0; i < workers.size(); i++){
		if(workers[i].available){
			workers[i].available = false;
			numWorkersAvailable--;
			sigprocmask(SIG_UNBLOCK, &additions, NULL);
			return i;
		}
	}
	return 0;
}


static void waitForAllWorkers(){
	sigset_t additions, existingmask;
	sigemptyset(&additions);
	sigaddset(&additions, SIGCHLD);
	sigprocmask(SIG_BLOCK, &additions, &existingmask);
	while(numWorkersAvailable != workers.size()){
		sigsuspend(&existingmask);
	}
	sigprocmask(SIG_UNBLOCK, &additions, NULL);
}

static void closeAllWorkers(){
	signal(SIGCHLD, SIG_DFL);
	for(size_t i = 0; i < workers.size(); i++){
		worker ww = workers[i];
		close(ww.sp.supplyfd);
		kill(ww.sp.pid, SIGCONT);
	}
	while(true){
		pid_t pid  = waitpid(-1, NULL, 0);
		if(pid == -1) break;
	}
}
