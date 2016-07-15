#ifndef _PROXY_SERVER_H_
#define _PROXY_SERVER_H_

#include "boost/thread.hpp"

#include <vector>

class HttpService;

class ProxyServer
{
public:
    ProxyServer(const int http_port, const int ssl_port, const int thread_num);
    ~ProxyServer();
    bool Init();
    void Start();
    void Wait();
    void Stop();

private:
    void ThreadProc();
    int BindSocket();
private:
    int listen_fd_;
    int http_port_;
    int ssl_port_;
    int thread_num_;
    std::vector<HttpService*> http_handlers_;
    std::vector<boost::thread*> threads_;
};

#endif // _PROXY_SERVER_H_