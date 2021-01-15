/**
 * File: scheduler.cc
 * ------------------
 * Presents the implementation of the HTTPProxyScheduler class.
 */

#include "scheduler.h"
#include "proxy-exception.h"
#include "ostreamlock.h"
#include <utility>
#include <iostream>
using namespace std;

HTTPProxyScheduler::HTTPProxyScheduler(int poolThreadNum):threadPool(poolThreadNum){}


HTTPProxyScheduler::~HTTPProxyScheduler(){
    threadPool.wait();
}

void HTTPProxyScheduler::scheduleRequest(int clientfd, const string& clientIPAddress) throw () {
	threadPool.schedule([this,clientfd,clientIPAddress](){
	    try{
	        this->requestHandler.serviceRequest(make_pair(clientfd, clientIPAddress));
	    }
	    catch(...){
	        cout << oslock << "schedule error" << endl << osunlock;
	    }
	});
}

void HTTPProxyScheduler::scheduleForward(int clientfd, const std::string& clientIPAddr, int proxyPort, const string& proxyIPAddr) throw () {
    threadPool.schedule([this,clientfd,clientIPAddr,proxyPort,proxyIPAddr](){
        try{
            this->requestHandler.serviceRequestForward(make_pair(clientfd, clientIPAddr),proxyPort,proxyIPAddr);
        }
        catch(...){
            cout << oslock << "schedule error" << endl << osunlock;
        }
    });
}
