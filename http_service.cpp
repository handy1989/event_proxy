#include "http_service.h"
#include "http_callbacks.h"
#include "logger.h"

HttpService::HttpService(const int sock) : sock_(sock)
{

}

bool HttpService::Init()
{
    bool ret = false;
    do
    {
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

        if (evhttp_accept_socket(http_server_, sock_) != 0)
        {
            LOG_ERROR("accept socket failed!");
            break;
        }

        CallbackPara* callback_para = new CallbackPara();
        callback_para->http_service = this;
        evhttp_set_gencb(http_server_, HttpGenericCallback, callback_para);
        ret = true;

    } while(0);

    return ret;
}

void HttpService::Start()
{
    event_base_dispatch(base_);
}

void HttpService::Stop()
{
    event_base_loopexit(base_, NULL);
}

