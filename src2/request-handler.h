/**
 * File: request-handler.h
 * -----------------------
 * Defines the HTTPRequestHandler class, which fully proxies and
 * services a single client request.  
 */

#ifndef _request_handler_
#define _request_handler_

#include "blacklist.h"
#include "cache.h"
#include <utility>
#include <string>

class HTTPRequestHandler {
 public:
 	HTTPRequestHandler();
    ~HTTPRequestHandler();
  	void serviceRequest(const std::pair<int, std::string>& connection) throw();
  	void clearCache();
  	void setCacheMaxAge(long maxAge);
    /* forward proxy request */
    void serviceRequestForward(const std::pair<int, std::string>& connection,int proxyPort,const std::string& proxyIPAddr) throw();
private:
    HTTPBlacklist blacklist;
    HTTPCache cache;
    /*  */
    bool blackListLookUp(const std::string& ip);

};

#endif
