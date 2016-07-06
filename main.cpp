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

#include "event2/listener.h"
#include "event2/bufferevent.h"
#include "event2/buffer.h"

using std::string;

static void echo_write_cb(struct bufferevent *bev, void *ctx)
{
    struct evbuffer *output = bufferevent_get_output(bev);

    BufferContext* buffer_context = GetClientBufferContext(bev);
    assert(buffer_context);
    
    if (buffer_context->read_header_finished)
    {
        string rsp = "hello world";
        evbuffer_add_printf(output, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n%s", rsp.size(), rsp.c_str());
        LOG_INFO("response to fd:" << buffer_context->http_request.fd);
        bufferevent_flush(bev, EV_WRITE, BEV_FINISHED);
        bufferevent_disable(bev, EV_WRITE);
    }
    LOG_INFO("echo_write_cb finished");

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
                buffer_context->read_header_finished = true;
                LOG_DEBUG("break");
                break;
            } 
            if (!buffer_context->first_line_parsed)
            {
                LOG_DEBUG("parse first line:" << request_line);
                buffer_context->http_request.ParseFirstLine(request_line);
                buffer_context->first_line_parsed = true;
            }
            else if (strlen(request_line) == 0)
            {
                buffer_context->read_header_finished = true;
                bufferevent_enable(bev, EV_WRITE);
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
        fprintf(stderr, "Got an error %d (%s) on the listener. "
                "Shutting down.\n", err, evutil_socket_error_to_string(err));

        event_base_loopexit(base, NULL);
}

int
main(int argc, char **argv)
{
        struct event_base *base;
        struct evconnlistener *listener;
        struct sockaddr_in sin;

        int port = 3333;

        if (argc > 1) {
                port = atoi(argv[1]);
        }
        if (port<=0 || port>65535) {
                puts("Invalid port");
                return 1;
        }

        base = event_base_new();
        if (!base) {
                puts("Couldn't open event base");
                return 1;
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
                perror("Couldn't create listener");
                return 1;
        }
        evconnlistener_set_error_cb(listener, accept_error_cb);

        event_base_dispatch(base);
        return 0;
}
