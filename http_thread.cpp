#include "http_thread.h"

HttpThread::HttpThread(const int sock) : sock_(sock)
{

}

void HttpThread::Init()
{
    base_ = event_base_new();
    if (!base_)
    {
        LOG_ERROR("create event base failed!");
        return ;
    }

    dnsbase_ = evdns_base_new(base, 1);
    if (!dnsbase_)
    {
        LOG_ERROR("create dnsbase failed!");
        return ;
    }

    http_server_ = evhttp_new(base);
    if (!http_server_)
    {
        LOG_ERROR("create evhttp failed!");
        return ;
    }

    if (evhttp_accept_socket(http_server_, sock_) != 0)
    {
        LOG_ERROR("accept socket failed!");
        return ;
    }

    evhttp_set_gencb(http_server_, HttpThread::HttpGenericCallback, NULL);
}

void HttpThread::Start()
{
    evhttp_base_dispatch(base_);
}

void HttpThread::Stop()
{
    
}