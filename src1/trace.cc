/**
 * File: trace.cc
 * ----------------
 * Presents the implementation of the trace program, which traces the execution of another
 * program and prints out information about ever single system call it makes.  For each system call,
 * trace prints:
 *
 *    + the name of the system call,
 *    + the values of all of its arguments, and
 *    + the system calls return value
 */
#include <string>
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <unistd.h> // for fork, execvp
#include <string.h> // for memchr, strerror
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/wait.h>
#include "trace-options.h"
#include "trace-error-constants.h"
#include "trace-system-calls.h"
#include "trace-exception.h"
#include <errno.h>
#include <climits>

using namespace std;

// #define DEBUG

std::map<int, std::string> systemCallNumbers;
std::map<std::string, int> systemCallNames;
std::map<std::string, systemCallSignature> systemCallSignatures;
std::map<int, std::string> errorConstants;

static const int systemCallArgRegs[8] = {RDI, RSI, RDX, R10, R8, R9};

int do_child(pid_t pid, char *argv[]);
void do_trace(pid_t pid, bool simple);
int wait_for_syscall(pid_t child);
long print_syscall_info(pid_t pid, bool simple, int &exit);
void print_syscall_return(pid_t pid, long syscall, bool simple);
void process_args(pid_t pid, systemCallSignature args) throw (TraceException);
string read_string(pid_t pid, char *addr);

int do_child(char *argv[]){
    ptrace(PTRACE_TRACEME);
    raise(SIGSTOP);
    return execvp(argv[0], argv);
}

void do_trace(pid_t pid, bool simple){
    int status, exit;
    waitpid(pid, &status, 0);
    assert(WIFSTOPPED(status)); /* waitpid return because SIGSTOP */
    ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACESYSGOOD);
    while(1){
        if(wait_for_syscall(pid)) break;/* exit */
        long syscall = print_syscall_info(pid, simple, exit);
        if(wait_for_syscall(pid)) break;
        print_syscall_return(pid, syscall, simple);
    }
    cout << "= <no return>" << endl;
    cout << "Program exited normally with status " << exit << endl;
}

int wait_for_syscall(pid_t child) {
    int status;
    while (1) {
        ptrace(PTRACE_SYSCALL, child, 0, 0);
        waitpid(child, &status, 0);
        if (WIFSTOPPED(status) && WSTOPSIG(status) & 0x80)
            return 0;
        if (WIFEXITED(status))
            return 1;
    }
}

long print_syscall_info(pid_t pid, bool simple, int &exit){
    long syscall = ptrace(PTRACE_PEEKUSER, pid, ORIG_RAX * sizeof(long));
    if(simple){
        cout << "syscall(" << syscall << ") ";
    } else{
#ifdef DEBUG
    cout << "print_syscall_info" << endl;
#endif 
        string syscall_name = systemCallNumbers[syscall];
        cout << syscall_name;
        systemCallSignature args = systemCallSignatures[syscall_name];
        process_args(pid, args); /* print argments */
        if (syscall == systemCallNames["exit_group"]) {
          exit = ptrace(PTRACE_PEEKUSER, pid, RDI * sizeof(long));  // RDI register will house the first call 
        }
    }
    return syscall;
}

void print_syscall_return(pid_t pid, long syscall, bool simple){
    long ret = ptrace(PTRACE_PEEKUSER, pid, RAX * sizeof(long));
    if(simple){
        cout << "= "<< ret;
    } else{
        if(ret < 0){
            cout << "= -1";
            string errorinfo = errorConstants[-ret];
            cout << " " << errorinfo << "(" << strerror(-ret) << ")";
        } else{
            if(ret > INT_MAX || syscall == systemCallNames["brk"] || syscall == systemCallNames["mmap"]) cout << "= 0x" << std::hex << ret << std::dec;
            else cout << "= " << ret;
        }
    }
    cout << endl;
}


void process_args(pid_t pid, systemCallSignature args) throw (TraceException){
    cout << "(";
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == SYSCALL_INTEGER) {
            cout << ptrace(PTRACE_PEEKUSER, pid, systemCallArgRegs[i] * sizeof(long));
#ifdef DEBUG
    cout << "args int" << endl;
#endif
        } else if(args[i] == SYSCALL_STRING){
#ifdef DEBUG
    cout << "args string" << endl;
#endif
            long tmp = ptrace(PTRACE_PEEKUSER, pid, systemCallArgRegs[i] * sizeof(long));
            char *addr = reinterpret_cast<char *>(tmp);
            string str = read_string(pid, addr);
            cout << "\"" << str <<"\"";
        } else if (args[i] == SYSCALL_POINTER) {
            long pointer = ptrace(PTRACE_PEEKUSER, pid, systemCallArgRegs[i] * sizeof(long));
            if (pointer == 0){
                cout << "NULL"; 
            }else{
                cout << "0x" << std::hex << pointer << std::dec;
            }
        } else if(args[i] == SYSCALL_UNKNOWN_TYPE){
            throw TraceException("Unknow argument type.");
        }
        if(i != args.size() - 1){
            cout << ",";
        }
    }
    cout << ") ";
}

string read_string(pid_t pid, char *addr){
    string str;
    size_t read = 0;
    bool endflag = false;
    while(!endflag){
        long tmp = ptrace(PTRACE_PEEKDATA, pid, addr + read);
        char *word = reinterpret_cast<char *>(&tmp);
        for(size_t i = 0; i < sizeof(long); i++){
            char ch = *(word+i);
            if(ch == '\0'){
                endflag = true;
                break;
            }
            str += ch;
        }
        read += sizeof(long);
    }   
    return str;
}

int main(int argc, char *argv[]) {
    bool simple = false, rebuild = false;
    int numFlags = processCommandLineFlags(simple, rebuild, argv);

    if (argc - numFlags == 1) {
        cout << "Nothing to trace... exiting." << endl;
        return 0;
    }

    pid_t pid = fork();
    if(pid == 0){
        return do_child(argv+numFlags+1);
    }

    if(!simple){
        try{
            compileSystemCallData(systemCallNumbers, systemCallNames, systemCallSignatures, rebuild);
            compileSystemCallErrorStrings(errorConstants);
        } catch(const MissingFileException& e2){
            cerr << "Missing file: " << e2.what() << endl;
        } catch(const TraceException& e1){
            cerr << "Invalid args: " << e1.what() << endl;
        } catch(...){
            cerr << "Unknow internal error." << endl;
        }
    }

    do_trace(pid, simple);
    return 0;
}
