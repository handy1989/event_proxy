#include "proxy_server.h"
#include "http_service.h"
#include "logger.h"

using std::vector;

ProxyServer::ProxyServer(const int http_port, const int ssl_port, const int thread_num) :
    http_port_(http_port),
    ssl_port_(ssl_port),
    thread_num_(thread_num) {}

ProxyServer::~ProxyServer()
{
    for (vector<HttpService*>::iterator it = http_services_.begin(); it != http_services_.end(); ++it)
    {
        delete (*it);
    }
}

int ProxyServer::BindSocket()
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sock)
    {
        LOG_ERROR("create socket failed!");
        return -1;
    }

    evutil_make_socket_nonblocking(sock);
    evutil_make_listen_socket_reuseable(sock);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(0);
    addr.sin_port = htons(http_port_);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        LOG_ERROR("bind port:" << http_port_ << " failed!");
        return -1;
    }
    if (listen(sock, 10240) < 0)
    {
        LOG_ERROR("listen failed!");
        return -1;
    }

    return sock;
}

bool ProxyServer::Init()
{
    listen_fd_ = BindSocket();
    if (listen_fd_ < 0)
    {
        LOG_ERROR("BindSocket failed!");
        return false;
    }

    return true;
}

void ProxyServer::Start()
{
    for (int i = 0; i < thread_num_; ++i)
    {
        HttpService* http_service = new HttpService(listen_fd_);
        if (!http_service->Init())
        {
            continue;
        }
        boost::thread* thread = new boost::thread(&HttpService::Start, http_service);
        http_services_.push_back(http_service);
        threads_.push_back(thread);
    }
}

void ProxyServer::Wait()
{
    for (vector<boost::thread*>::iterator it = threads_.begin(); it != threads_.end(); ++it)
    {
        (*it)->join();
    }
}

void ProxyServer::Stop()
{
    for (vector<HttpService*>::iterator it = http_services_.begin(); it != http_services_.end(); ++it)
    {
        LOG_INFO("stop http_service begin");
        (*it)->Stop();
        LOG_INFO("stop http_service finished");
    }
}
