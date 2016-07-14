#ifndef _PROXY_SERVER_H_
#define _PROXY_SERVER_H_

class HttpHandle;

class ProxyServer
{
public:
    ProxyServer(const int http_port, const int ssl_port, const int thread_num);
    bool InitServer();
    bool StartServer();
private:
    int listen_fd_;
    int http_port_;
    int ssl_port_;
    int thread_num_;
    vector<HttpHandler*> http_handles_;
};

#endif // _PROXY_SERVER_H_