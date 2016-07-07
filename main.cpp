#include "logger.h"
#include "global.h"
#include "http_request.h"

#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include <string>
#include <vector>

#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"
#include "event2/dns.h"
#include "event2/util.h"
#include "event2/event.h"

using std::string;
using std::vector;

struct evdns_base *dnsbase;
struct event_base *base;
struct evconnlistener *listener;

static void echo_write_cb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *output = bufferevent_get_output(bev);

    BufferContext* buffer_context = GetClientBufferContext(bev);
    assert(buffer_context);
    
    if (buffer_context->read_header_finished)
    {
        string rsp;
        string status;
        switch (buffer_context->response_status)
        {
            case 400:
                rsp = "bad request";
                status = "Bad Request"; 
                break;
            case 200:
                rsp = "hello world";
                status = "OK";
                break;
            default:
                rsp = "hello world";
                status = "Unknown";
                break;
        }
        evbuffer_add_printf(output, "HTTP/1.1 %d %s\r\nContent-Length: %lu\r\n\r\n%s", buffer_context->response_status, status.c_str(), rsp.size(), rsp.c_str());
        buffer_context->read_header_finished = false;

        LOG_INFO("response to fd:" << buffer_context->http_request.fd);
    }
    else
    {
        //bufferevent_disable(bev, EV_WRITE);
        //LOG_INFO("disable write");
    }
    LOG_INFO("echo_write_cb finished");

}

int32_t ParseHostPort(BufferContext* buffer_context)
{
    string& url = buffer_context->http_request.url;
    int32_t ret = 1;
    do
    {
        if (strncmp(url.c_str(), "http://", 7) != 0)
        {
            LOG_ERROR("http:// not in begining of url:" << url);
            break;
        }
        size_t host_begin_pos = 7;
        size_t host_end_pos = url.find_first_of('/', host_begin_pos);
        if (host_end_pos == string::npos)
        {
            LOG_ERROR("/ not found after host in url:" << url);
            break;
        }
        buffer_context->http_request.url_path = string(url, host_end_pos);
        LOG_DEBUG("url:" << url << " url_path:" << buffer_context->http_request.url_path);

        size_t colon_pos = url.find_first_of(':', host_begin_pos);
        if (colon_pos == string::npos || colon_pos > host_end_pos)
        {
            buffer_context->remote_host = string(url, host_begin_pos, host_end_pos - host_begin_pos);
            buffer_context->remote_port = 80;
        }
        else
        {
            buffer_context->remote_host = string(url, host_begin_pos, colon_pos - host_begin_pos);
            buffer_context->remote_port = atoi(string(url, colon_pos + 1, host_end_pos).c_str());
            if (buffer_context->remote_port == 0)
            {
                LOG_ERROR("parse port failed in url:" << url);
                break;
            }
        }

        ret = 0;
    } while (0);

    LOG_DEBUG("parse url:" << url
            << " host:" << buffer_context->remote_host
            << " port:" << buffer_context->remote_port);

    return ret;

}

void ConnectRemoteServer(BufferContext* buffer_context)
{
    LOG_DEBUG("begin connect to host:" << buffer_context->remote_host
            << " ip:" << buffer_context->remote_ip
            << " port:" << buffer_context->remote_port);

    buffer_context->response_status = 200;
    bufferevent_enable(buffer_context->client, EV_WRITE);
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
    buffer_context->remote = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(buffer_context->remote, read_remote_cb, write_remote_cb, event_remote_cb, NULL);
    bufferevent_enable(buffer_context->remote, EV_WRITE);
    bufferevent_socket_connect_hostname(buffer_context->remote, dnsbase, AF_UNSPEC, 
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

static void echo_read_cb(struct bufferevent *bev, void *ctx)
{
        /* This callback is invoked when there is data to read on bev. */
        struct evbuffer *input = bufferevent_get_input(bev);

        BufferContext* buffer_context = GetClientBufferContext(bev);
        assert(buffer_context);
        
        size_t line_len;
        while(true)
        {
            char* request_line = evbuffer_readln(input, &line_len, EVBUFFER_EOL_CRLF_STRICT);
            if (!request_line) 
            {
                LOG_DEBUG("break");
                break;
            } 
            if (!buffer_context->first_line_parsed)
            {
                LOG_DEBUG("parse first line:" << request_line);
                int32_t ret = buffer_context->http_request.ParseFirstLine(request_line);
                buffer_context->first_line_parsed = true;

                LOG_DEBUG("parsed first req, method:" << buffer_context->http_request.method
                        << " url:" << buffer_context->http_request.url
                        << " http_version:" << buffer_context->http_request.http_version
                        << " ret:" << ret);
                if (ret != 0)
                {
                    buffer_context->response_status = 400;
                    buffer_context->read_header_finished = true;
                }
            }
            else if (strlen(request_line) == 0)
            {
                buffer_context->read_header_finished = true;
                LOG_DEBUG("read header finished, enable write");
                DnsConnect(buffer_context);
                //bufferevent_enable(bev, EV_WRITE);
            }
            else
            {
                buffer_context->http_request.ParseHeaderLine(request_line);    
                LOG_DEBUG("parse line:" << request_line);
            }
            free(request_line);
        }
}

static void
echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
    BufferContext* buffer_context = GetClientBufferContext(bev);
    assert(buffer_context);
    delete buffer_context;

    if (events & BEV_EVENT_ERROR)
            LOG_ERROR("Error from bufferevent");
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
            bufferevent_free(bev);
    }
    LOG_INFO("echo_event_cb finished");
}

static void
accept_conn_cb(struct evconnlistener *listener,
    evutil_socket_t fd, struct sockaddr *address, int socklen,
    void *ctx)
{


    evutil_make_socket_nonblocking(fd);
    evutil_make_listen_socket_reuseable(fd);

    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(
            base, fd, BEV_OPT_CLOSE_ON_FREE);

    BufferContext* buffer_context = new BufferContext();
    buffer_context->client = bev;
    AddClientBufferContext(bev, buffer_context);

    sockaddr_in* addr_in = (sockaddr_in*)address;
    buffer_context->http_request.ip = inet_ntoa(addr_in->sin_addr);
    buffer_context->http_request.port = ntohs(addr_in->sin_port);
    buffer_context->http_request.fd = fd;

    LOG_INFO("accepte request ip:" << buffer_context->http_request.ip
            << " port:" << buffer_context->http_request.port
            << " fd:" << buffer_context->http_request.fd);

    bufferevent_setcb(bev, echo_read_cb, echo_write_cb, echo_event_cb, NULL);

    bufferevent_enable(bev, EV_READ);
}

static void
accept_error_cb(struct evconnlistener *listener, void *ctx)
{
        struct event_base *base = evconnlistener_get_base(listener);
        int err = EVUTIL_SOCKET_ERROR();
        LOG_ERROR("Got an error " << err
                << "(" << evutil_socket_error_to_string(err) << ") on the listener");

        event_base_loopexit(base, NULL);
}

int
main(int argc, char **argv)
{
        struct sockaddr_in sin;

        int port = 3333;

        if (argc > 1) {
                port = atoi(argv[1]);
        }
        if (port<=0 || port>65535) {
                LOG_ERROR("Invalid port");
                return 1;
        }

        base = event_base_new();
        if (!base) {
                LOG_ERROR("Couldn't open event base");
                return 1;
        }
        dnsbase = evdns_base_new(base, 1);
        if (!dnsbase)
        {
            LOG_ERROR("create dnsbase failed");
            return 2;
        }

        /* Clear the sockaddr before using it, in case there are extra
         * platform-specific fields that can mess us up. */
        memset(&sin, 0, sizeof(sin));
        /* This is an INET address */
        sin.sin_family = AF_INET;
        /* Listen on 0.0.0.0 */
        sin.sin_addr.s_addr = htonl(0);
        /* Listen on the given port. */
        sin.sin_port = htons(port);

        listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
            LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
            (struct sockaddr*)&sin, sizeof(sin));
        if (!listener) {
                LOG_ERROR("Couldn't create listener");
                return 1;
        }
        evconnlistener_set_error_cb(listener, accept_error_cb);

        event_base_dispatch(base);
        return 0;
}
