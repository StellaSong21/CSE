/**
 * File: request-handler.cc
 * ------------------------
 * Provides the implementation for the HTTPRequestHandler class.
 */

#include "request-handler.h"  // for request
#include "response.h"  //response body
#include "request.h"  // request annalyse
#include "client-socket.h"  // get socket file descrptor by ip and socket
#include "ostreamlock.h"  // lock stream
#include <socket++/sockstream.h> // for sockbuf, iosockstream
#include <set>  // unique set to check loop
using namespace std;

const string BLACK_LIST_PATH = "blocked-domains.txt";
HTTPRequestHandler::HTTPRequestHandler():blacklist(),cache(){
    blacklist.addToBlacklist(BLACK_LIST_PATH);
    /* don't set TTL */
    cache.setMaxAge(-1);
}

HTTPRequestHandler::~HTTPRequestHandler(){}

/* 检查请求中是否有环 */
inline bool cycle(string IPaddrs){
    string ip;
    size_t count=0;
    size_t pos=0; 
    set<string> set;
    while((pos = IPaddrs.find(",")) != string::npos){
        ip = IPaddrs.substr(0, pos);
        /* insert into set */
        set.insert(ip);
        count++;
        IPaddrs.erase(0, pos+1);
    }
    /* 如果有环那么集和中的元素数量小于ip数 */
    return set.size() < count;
}

inline void responseWith(HTTPResponse& response, int code, const string protocol, const string payload){
    response.setResponseCode(code);
    response.setProtocol(protocol);
    response.setPayload(payload);
}

inline int formRequest(HTTPRequest& request,HTTPResponse& response,iostream& ss, 
						const string& ip){
    try{
        request.ingestRequestLine(ss);
    }
    catch(...){
        return -1;
    }
    request.ingestHeader(ss,ip);
    string value = request.getHeader().getValueAsString("x-forwarded-for");
    if(cycle(value)){
        cout << oslock << "Cycle occur: " << request.getURL() << endl << endl;
        responseWith(response,504,"HTTP/1.1","Gateway Timeout");
        ss << response;
        return -2;
    }
    string type = request.getMethod();
    if(type != "HEAD"){
        request.ingestPayload(ss);
    }
    return 0;
}

inline void getResponseFrom(HTTPRequest &request, iostream &stream){
    stream << request.getMethod() << " " << request.getPath() 
        << " " << request.getProtocol() << "\r\n"
        << request.getHeader() << "\r\n"
        << request.getPayload();
    stream.flush();
}

/* 传递给下一个代理 */
inline void forwardTo(HTTPRequest &request, iostream &stream){
    stream << request.getMethod() << " " << request.getURL() 
        << " " << request.getProtocol() << "\r\n"
        << request.getHeader() << "\r\n"
        << request.getPayload();
    stream.flush();
}

/* 从目标服务器上获取请求资源 */
inline void handleResponse(HTTPResponse& response, iostream &stream){
    response.ingestResponseHeader(stream);
    response.ingestPayload(stream);
}

/* 检查是否在黑名单中 */
bool HTTPRequestHandler::blackListLookUp(const string& ip){
    return !blacklist.serverIsAllowed(ip);
}

/* 加入需要的头部信息 */
inline void addHeaderFlag(HTTPRequest &request, const string& ip){
    /* add x-forwarded-proto:http in the header */
    request.getHeader().addHeader("x-forwarded-proto","http");
    string value;
    /* add x-forwarded-for value in the list */
    if(request.containsName("x-forwarded-for")){
        value = request.getHeader().getValueAsString("x-forwarded-for");
        value += ',' + ip;
        request.getHeader().addHeader("x-forwarded-for",value);
    }
    else{
        value = ip;
        request.getHeader().addHeader("x-forwarded-for",value);
    }
}


void HTTPRequestHandler::serviceRequestForward(
					const std::pair<int, std::string>& connection,
                    int proxyPort,
                    const std::string& proxyIP) throw(){
    HTTPRequest request;
    HTTPResponse response;
    sockbuf sb(connection.first);
    iosockstream ss(&sb);
    int fd;
    if (formRequest(request,response,ss,connection.second) != 0){
        return;
    }
    cout << oslock << "Forward Request: " << request.getURL() << endl << osunlock;
    /* 检查黑名单 */
    if(blackListLookUp(request.getServer())){
        cout << oslock << "Block Reuest: " << request.getURL() << endl << osunlock;
        responseWith(response,403,"HTTP/1.1","Forbidden Content");
        ss << response;
        ss.flush();
        return;
    }
    /* 给缓存上锁 */
    size_t idx = cache.getHashRequest(request);
    mutex &mtx = cache.locks[idx];
    mtx.lock();
    addHeaderFlag(request,connection.second);

    /* 检查缓存 */
    if(cache.containsCacheEntry(request,response)){
        cout << oslock << "Cache Hit: " << request.getURL() << endl << osunlock;
        ss << response;
        ss.flush();
        mtx.unlock();
        return;
    }
    cout << oslock << "Handle : " << request.getURL() << endl << osunlock;
    /* 获取目标的文件描述符 */
    fd = createClientSocket(proxyIP,proxyPort);
    // 如果请求失败
    if(fd == kClientSocketError){
        /* Server not found */
        responseWith(response,403,"HTTP/1.1","Server not found");
        ss << response;
        ss.flush();
        /* pay attention to unlocking the mutex */
        mtx.unlock();
        return;
    }
    sockbuf sf(fd);
    iosockstream ff(&sf);
    /* 发送给下一个代理 */
    forwardTo(request,ff);
    handleResponse(response,ff);
    if(cache.shouldCache(request,response)){
        cache.cacheEntry(request,response);
    }
    ss << response;
    ss.flush();
    mtx.unlock();
}



void HTTPRequestHandler::serviceRequest(const pair<int, string>& connection) throw() {
    HTTPRequest request;
    HTTPResponse response;
    sockbuf sb(connection.first);
    iosockstream ss(&sb);
    int fd;
    if (formRequest(request,response,ss,connection.second) != 0){
        return;
    }
    cout << oslock << "Handle Request: " << request.getURL() << endl << osunlock;
    if(blackListLookUp(request.getServer())){
        cout << oslock << "Block Reuest: " << request.getURL() << endl << osunlock;
        responseWith(response,403,"HTTP/1.1","Forbidden Content");
        ss << response;
        ss.flush();
        return;
    }
    mutex &mtx = cache.locks[cache.getHashRequest(request)];
    mtx.lock();
    addHeaderFlag(request,connection.second);
    if(cache.containsCacheEntry(request,response)){
        cout << oslock << "Cache Hit: " << request.getURL() << endl << osunlock;
        ss << response;
        ss.flush();
        mtx.unlock();
        return;
    }
    cout << oslock << "Handle: " << request.getURL() << endl << osunlock;
    fd = createClientSocket(request.getServer(),request.getPort());
    if(fd == kClientSocketError){
        responseWith(response,403,"HTTP/1.1","Server not found");
        ss << response;
        ss.flush();
        mtx.unlock();
        return;
    }
    sockbuf sf(fd);
    iosockstream ff(&sf);
    getResponseFrom(request,ff);
    handleResponse(response,ff);
    if(cache.shouldCache(request,response)){
        cache.cacheEntry(request,response);
    }
    ss << response;
    ss.flush();
    mtx.unlock();
}

// the following two methods needs to be completed 
// once you incorporate your HTTPCache into your HTTPRequestHandler
void HTTPRequestHandler::clearCache() {
	cache.clear();
}
void HTTPRequestHandler::setCacheMaxAge(long maxAge) {
	cache.setMaxAge(maxAge);
}
