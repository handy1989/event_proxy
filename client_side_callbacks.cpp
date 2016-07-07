#include "client_side_callbacks.h"
#include "server_side_callbacks.h"
#include "logger.h"
#include "global.h"
#include "http_request.h"

#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <assert.h>
#include <string>

using std::string;

void echo_write_cb(struct bufferevent *bev, void *ctx)
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

void echo_read_cb(struct bufferevent *bev, void *ctx)
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

void echo_event_cb(struct bufferevent *bev, short events, void *ctx)
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

void accept_error_cb(struct evconnlistener *listener, void *ctx)
{
        struct event_base *base = evconnlistener_get_base(listener);
        int err = EVUTIL_SOCKET_ERROR();
        LOG_ERROR("Got an error " << err
                << "(" << evutil_socket_error_to_string(err) << ") on the listener");

        event_base_loopexit(base, NULL);
}
