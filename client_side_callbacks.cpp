#include "client_side_callbacks.h"
#include "server_side_callbacks.h"
#include "logger.h"
#include "global.h"
#include "http_request.h"
#include "http_response.h"
#include "http_parse_help.h"
#include "service.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <assert.h>
#include <string>

using std::string;

void client_write_cb(struct bufferevent *bev, void *ctx)
{
//    LOG_DEBUG("call client_write_cb");
//    BufferContext* buffer_context = GetClientBufferContext(bev);
//    if (!buffer_context)
//    {
//        bufferevent_disable(bev, EV_WRITE);
//        return ;
//    }
//    assert(buffer_context->client == bev);
//
//    struct evbuffer *output = bufferevent_get_output(bev);
//    if (buffer_context->read_header_finished)
//    {
//        string rsp;
//        string status;
//        switch (buffer_context->response_status)
//        {
//            case 400:
//                rsp = "bad request";
//                status = "Bad Request"; 
//                break;
//            case 200:
//                rsp = "hello world";
//                status = "OK";
//                break;
//            default:
//                rsp = "hello world";
//                status = "Unknown";
//                break;
//        }
//        evbuffer_add_printf(output, "HTTP/1.1 %d %s\r\nContent-Length: %lu\r\n\r\n%s", buffer_context->response_status, status.c_str(), rsp.size(), rsp.c_str());
//        buffer_context->read_header_finished = false;
//
//        LOG_INFO("response to fd:" << buffer_context->http_request->fd);
//    }
//    else
//    {
//        //bufferevent_disable(bev, EV_WRITE);
//        //LOG_INFO("disable write");
//    }
//    LOG_INFO("client_write_cb finished");

}

void client_read_cb(struct bufferevent *bev, void *ctx)
{
    BufferContext* buffer_context = GetClientBufferContext(bev);
    if (!buffer_context)
    {
        bufferevent_disable(bev, EV_READ);
        return ;
    }
    assert(buffer_context->client == bev);

    if (buffer_context->tunnel)
    {
        struct evbuffer* client_input = bufferevent_get_input(bev);
        struct evbuffer* remote_output = bufferevent_get_output(buffer_context->remote);

        LOG_DEBUG("read client tunnel len:" << evbuffer_get_length(client_input) << ", client:" << buffer_context->client
                << " remote:" << buffer_context->remote);
        evbuffer_add_buffer(remote_output, client_input);
        return ;
    }
   
    struct evbuffer *input = bufferevent_get_input(bev);
    HttpRequest* http_request = buffer_context->http_request;
    while(true)
    {
        char* request_line = evbuffer_readln(input, NULL, EVBUFFER_EOL_CRLF_STRICT);
        if (!request_line) 
        {
            LOG_DEBUG("break");
            break;
        } 
        if (!buffer_context->first_line_parsed)
        {
            LOG_DEBUG("parse first line:" << request_line);
            int32_t ret = http_request->ParseFirstLine(request_line);
            buffer_context->first_line_parsed = true;

            LOG_DEBUG("parsed first req, method:" << http_request->method
                    << " url:" << http_request->url
                    << " http_version:" << http_request->http_version
                    << " url_path:" << http_request->url_path
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
            ConnectRemote(buffer_context);
        }
        else
        {
            http_request->ParseHeaderLine(request_line);    
            LOG_DEBUG("parse line:" << request_line);
        }
        free(request_line);
    }
}

void client_event_cb(struct bufferevent *bev, short events, void *ctx)
{
    if (events & BEV_EVENT_ERROR)
        LOG_ERROR("Error from bufferevent");
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR))
    {
        BufferContext* buffer_context = GetClientBufferContext(bev);
        if (buffer_context)
        {
            RemoveBufferContext(buffer_context);
        }
        bufferevent_free(bev);
        LOG_DEBUG("free client:" << bev);
    }
    LOG_INFO("client_event_cb finished");
}

void accept_conn_cb(
    struct evconnlistener *listener,
    evutil_socket_t fd, 
    struct sockaddr *address, 
    int socklen,
    void *ctx)
{
    evutil_make_socket_nonblocking(fd);
    evutil_make_listen_socket_reuseable(fd);

    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(
            base, fd, BEV_OPT_CLOSE_ON_FREE);

    BufferContext* buffer_context = new BufferContext();
    buffer_context->http_request = new HttpRequest();
    buffer_context->client = bev;
    AddClientBufferContext(bev, buffer_context);

    sockaddr_in* addr_in = (sockaddr_in*)address;
    buffer_context->http_request->ip = inet_ntoa(addr_in->sin_addr);
    buffer_context->http_request->port = ntohs(addr_in->sin_port);
    buffer_context->http_request->fd = fd;

    LOG_INFO("accepte request ip:" << buffer_context->http_request->ip
            << " port:" << buffer_context->http_request->port
            << " fd:" << buffer_context->http_request->fd);

    bufferevent_setcb(bev, client_read_cb, NULL, client_event_cb, NULL);

    bufferevent_enable(bev, EV_READ);
}

void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
        struct event_base *base = evconnlistener_get_base(listener);
        int err = EVUTIL_SOCKET_ERROR();
        LOG_ERROR("Got an error " << err
                << "(" << evutil_socket_error_to_string(err) << ") on the listener");

        event_base_loopexit(base, NULL);
}



void ConnectRemote(BufferContext* buffer_context)
{
    if (ParseHostPort(buffer_context) != 0)
    {
        buffer_context->response_status = 400;
        return ;
    }
    LOG_DEBUG("port:" << buffer_context->remote_port);
    if (buffer_context->remote_port == 443)
    {
        // behave as tunnel
        LOG_DEBUG("connect tunnel");
        buffer_context->tunnel = true;
        buffer_context->remote = bufferevent_socket_new(Service::get_base(), -1, BEV_OPT_CLOSE_ON_FREE);
        bufferevent_setcb(buffer_context->remote, read_remote_tunnel_cb, NULL, event_remote_tunnel_cb, NULL);
        bufferevent_enable(buffer_context->remote, EV_READ);
        bufferevent_socket_connect_hostname(buffer_context->remote, Service::get_dnsbase(), AF_UNSPEC, 
                buffer_context->remote_host.c_str(), buffer_context->remote_port);
        AddRemoteBufferContext(buffer_context->remote, buffer_context);

        return ;
    }
    LOG_DEBUG("here");
    buffer_context->remote = bufferevent_socket_new(Service::get_base(), -1, BEV_OPT_CLOSE_ON_FREE);
    buffer_context->http_response = new HttpResponse();
    bufferevent_setcb(buffer_context->remote, read_remote_cb, NULL, event_remote_cb, NULL);
    bufferevent_enable(buffer_context->remote, EV_READ);
    bufferevent_socket_connect_hostname(buffer_context->remote, Service::get_dnsbase(), AF_UNSPEC, 
            buffer_context->remote_host.c_str(), buffer_context->remote_port);
    AddRemoteBufferContext(buffer_context->remote, buffer_context);
}
