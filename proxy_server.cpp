#include "proxy_server.h"

ProxyServer::ProxyServer(const int http_port, const int ssl_port, const int thread_num) :
    http_port_(http_port),
    ssl_port_(ssh_port),
    thread_num_(thread_num) {}

bool ProxyServer::InitServer()
{

}