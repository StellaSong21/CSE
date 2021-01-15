/**
 * File: scheduler.h
 * -----------------
 * This class defines the HTTPProxyScheduler class, which eventually takes all
 * proxied requests off of the main thread and schedules them to 
 * be handled by a constant number of child threads.
 */

#ifndef _scheduler_
#define _scheduler_
#include <string>
#include "request-handler.h"
#include "thread-pool.h"

class HTTPProxyScheduler {
 public:
  HTTPProxyScheduler(int poolThreadNum=64);
  ~HTTPProxyScheduler();
  void setProxy(const std::string& server, unsigned short port);
  void clearCache() { requestHandler.clearCache(); }
  void setCacheMaxAge(long maxAge) { requestHandler.setCacheMaxAge(maxAge); }
  void scheduleForward(int clientfd, const std::string& clientIPAddr, int proxyPort, const std::string& proxyIPAddr) throw ();
  void scheduleRequest(int clientfd, const std::string& clientIPAddr) throw ();
    
private:
  HTTPRequestHandler requestHandler;
  ThreadPool threadPool;
};

#endif
