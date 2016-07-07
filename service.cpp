#include "service.h"
#include "logger.h"

#include "client_side_callbacks.h"

static struct evdns_base *dnsbase = NULL;
static struct event_base *base = NULL;
static struct evconnlistener *listener = NULL;

event_base* Service::get_base()
{
    return base;
}

evdns_base* Service::get_dnsbase()
{
    return dnsbase;
}

bool Service::Init()
{
        base = event_base_new();
        if (!base) 
        {
            LOG_ERROR("Couldn't open event base");
            return false;
        }
        dnsbase = evdns_base_new(base, 1);
        if (!dnsbase)
        {
            LOG_ERROR("create dnsbase failed");
            return false;
        }

        struct sockaddr_in sin;
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(0);
        sin.sin_port = htons(port_);

        listener = evconnlistener_new_bind(
            base,
            accept_conn_cb, 
            NULL, 
            LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, 
            -1, 
            (struct sockaddr*)&sin, sizeof(sin));

        if (!listener) 
        {
            LOG_ERROR("Couldn't create listener");
            return false;
        }
        evconnlistener_set_error_cb(listener, accept_error_cb);

        return true;
}

void Service::Start()
{
    event_base_dispatch(base);
}
