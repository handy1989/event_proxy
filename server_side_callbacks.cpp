#include "server_side_callbacks.h"
#include "logger.h"
#include "http_parse_help.h"
#include "service.h"

#include <assert.h>
#include <vector>

using std::vector;

void read_remote_cb(struct bufferevent* bev, void* arg)
{
    char buf[1024];
    int n;
    struct evbuffer *input = bufferevent_get_input(bev);
    while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0) {
        fwrite(buf, 1, n, stdout);
    }  
}

void write_remote_cb(struct bufferevent* bev, void* arg)
{
    BufferContext* buffer_context = GetRemoteBufferContext(bev);
    assert(buffer_context);
    assert(buffer_context->remote == bev);
    
    if (!buffer_context->write_remote_finished)
    {
        LOG_DEBUG("send request to remote server");

        struct evbuffer *output = bufferevent_get_output(bev);
        evbuffer_add_printf(output, "%s %s HTTP/%d.%d\r\n", MethodStr(buffer_context->http_request.method), 
                buffer_context->http_request.url_path.c_str(),
                buffer_context->http_request.http_version.major, buffer_context->http_request.http_version.minor);
        for (vector<HttpHeaderEntry>::iterator it = buffer_context->http_request.header.entries.begin();
                it != buffer_context->http_request.header.entries.end(); ++it)
        {
            evbuffer_add_printf(output, "%s: %s\r\n", it->name.c_str(), it->value.c_str());
        }
        evbuffer_add_printf(output, "\r\n");

        buffer_context->write_remote_finished = true;
        bufferevent_enable(bev, EV_READ);

        LOG_DEBUG("write_remote_cb finished");
        
    }
}

void event_remote_cb(struct bufferevent* bev, short events, void* arg)
{
    BufferContext* buffer_context = GetRemoteBufferContext(bev);
    assert(buffer_context);
    assert(buffer_context->remote == bev);
    if (events & BEV_EVENT_CONNECTED) 
    {
        LOG_INFO("Connect " << buffer_context->remote_host 
                 << ":" << buffer_context->remote_port << " okay");
        //bufferevent_enable(bev, EV_WRITE);
        write_remote_cb(bev, NULL);
    } 
    else if (events & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) 
    {
        if (events & BEV_EVENT_ERROR) 
        {
            int err = bufferevent_socket_get_dns_error(bev);
            if (err)
            {
                LOG_ERROR("DNS error:" << evutil_gai_strerror(err));
                buffer_context->response_status = 400;
                bufferevent_enable(buffer_context->client, EV_WRITE);
            }
        }
        bufferevent_free(bev);
    }
}

void DnsConnect(BufferContext* buffer_context)
{
    if (ParseHostPort(buffer_context) != 0)
    {
        buffer_context->response_status = 400;
        bufferevent_enable(buffer_context->client, EV_WRITE);
        return ;
    }
    buffer_context->remote = bufferevent_socket_new(Service::get_base(), -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(buffer_context->remote, read_remote_cb, write_remote_cb, event_remote_cb, NULL);
    bufferevent_enable(buffer_context->remote, EV_WRITE);
    bufferevent_socket_connect_hostname(buffer_context->remote, Service::get_dnsbase(), AF_UNSPEC, 
            buffer_context->remote_host.c_str(), buffer_context->remote_port);
    AddRemoteBufferContext(buffer_context->remote, buffer_context);

    //struct evutil_addrinfo hints;
    //struct evdns_getaddrinfo_request *req;
    //memset(&hints, 0, sizeof(hints));
    //hints.ai_family = AF_UNSPEC;
    //hints.ai_flags = EVUTIL_AI_CANONNAME;
    //hints.ai_socktype = SOCK_STREAM;
    //hints.ai_protocol = IPPROTO_TCP;

    //evdns_getaddrinfo(dnsbase, buffer_context->remote_host.c_str(), NULL,
    //                  &hints, DnsConnectCallback, buffer_context);


    //buffer_context->response_status = 200;
    //bufferevent_enable(buffer_context->client, EV_WRITE);
}

void DnsConnectCallback(int errcode, struct evutil_addrinfo *addr, void *arg)
{
    BufferContext* buffer_context = (BufferContext*)arg;
    if (errcode) 
    {
        LOG_ERROR("query dns of host:" << buffer_context->remote_host
                << " failed, error:"  << evutil_gai_strerror(errcode));
        buffer_context->response_status = 400;
        bufferevent_enable(buffer_context->client, EV_WRITE);
        return ;
    }
    struct evutil_addrinfo *ai;
    for (ai = addr; ai; ai = ai->ai_next) 
    {
        char buf[128];
        const char *s = NULL;
        if (ai->ai_family == AF_INET) 
        {
            struct sockaddr_in *sin = (struct sockaddr_in *)ai->ai_addr;
            s = evutil_inet_ntop(AF_INET, &sin->sin_addr, buf, 128);
        }
        else if (ai->ai_family == AF_INET6)
        {
            struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ai->ai_addr;
            s = evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, buf, 128);
        }
        if (s)
        {
            buffer_context->remote_ip = s;
            break;
        }
        
    }
    evutil_freeaddrinfo(addr);
    
    if (buffer_context->remote_ip.size() > 0)
    {
        ConnectRemoteServer(buffer_context);
    }
    else
    {
        buffer_context->response_status = 400;
        bufferevent_enable(buffer_context->client, EV_WRITE);
    }
}

void ConnectRemoteServer(BufferContext* buffer_context)
{
    LOG_DEBUG("begin connect to host:" << buffer_context->remote_host
            << " ip:" << buffer_context->remote_ip
            << " port:" << buffer_context->remote_port);

    buffer_context->response_status = 200;
    bufferevent_enable(buffer_context->client, EV_WRITE);
}
