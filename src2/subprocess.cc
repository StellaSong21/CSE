#include "subprocess.h"

using namespace std;

subprocess_t subprocess(char *argv[], bool supplyChildInput, bool ingestChildOutput) 
	throw (SubprocessException){
	int supply_fds[2]; // 0, read; 1, write
	int ingest_fds[2];

	if(supplyChildInput){
		if(pipe(supply_fds) == -1){
			throw SubprocessException("pipe");
		}
	}

	if(ingestChildOutput){
		if(pipe(ingest_fds) == -1){
			throw SubprocessException("pipe");
		}
	}

	pid_t pid = fork();
	struct subprocess_t subproc = {pid, kNotInUse, kNotInUse};

	if(pid < 0){
		throw SubprocessException("fork");
	}else if(!pid){
		if(supplyChildInput){
			if(close(supply_fds[1]) == -1){
				throw SubprocessException("close");
			}
			if(dup2(supply_fds[0], STDIN_FILENO) == -1){
				throw SubprocessException("dup2");
			}
		} 

		if(ingestChildOutput){
			if(close(ingest_fds[0]) == -1){
				throw SubprocessException("close");
			} 
			if(dup2(ingest_fds[1], STDOUT_FILENO) == -1){
				throw SubprocessException("dup2");
			}
		}

		execvp(argv[0], argv);
		throw SubprocessException("execvp");
	}

	if(supplyChildInput){ 
		if(close(supply_fds[0]) == -1){
			throw SubprocessException("close");
		}
		subproc.supplyfd = supply_fds[1]; 
	}

	if(ingestChildOutput){
		if(close(ingest_fds[1]) == -1){
			throw SubprocessException("close");
		}
	 	subproc.ingestfd = ingest_fds[0]; 
	 } 

	return subproc;
}