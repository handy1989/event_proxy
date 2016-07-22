#include "http_service.h"
#include "http_callbacks.h"
#include "logger.h"

HttpService::HttpService(const int sock) : 
    sock_(sock), 
    base_(NULL),
    dnsbase_(NULL),
    http_server_(NULL)
{

}

bool HttpService::Init()
{
    bool ret = false;
    do
    {
        evthread_use_pthreads();
        base_ = event_base_new();
        if (!base_)
        {
            LOG_ERROR("create event base failed!");
            break;
        }

        dnsbase_ = evdns_base_new(base_, 1);
        if (!dnsbase_)
        {
            LOG_ERROR("create dnsbase failed!");
            break;
        }

        http_server_ = evhttp_new(base_);
        if (!http_server_)
        {
            LOG_ERROR("create evhttp failed!");
            break;
        }

        evhttp_set_allowed_methods(http_server_, EVHTTP_REQ_CONNECT);

        if (evhttp_accept_socket(http_server_, sock_) != 0)
        {
            LOG_ERROR("accept socket failed!");
            break;
        }

        evhttp_set_gencb(http_server_, HttpGenericCallback, this);
        ret = true;

    } while(0);

    return ret;
}

void HttpService::Start()
{
    event_base_dispatch(base_);
    LOG_INFO("event_base_dispatch finished");
}

void HttpService::Stop()
{
    event_base_loopexit(base_, NULL);
    //event_base_loopbreak(base_);
    LOG_INFO("event_base_loopexit called");
}

