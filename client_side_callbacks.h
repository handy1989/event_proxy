#ifndef _CLIENT_SIDE_CALLBACKS_H_
#define _CLIENT_SIDE_CALLBACKS_H_

#include "event_includes.h"
#include "global.h"

void client_write_cb(struct bufferevent *bev, void *ctx);
void client_read_cb(struct bufferevent *bev, void *ctx);
void client_event_cb(struct bufferevent *bev, short events, void *ctx);
void accept_error_cb(struct evconnlistener *listener, void *ctx);

void accept_conn_cb(
    struct evconnlistener *listener,
    evutil_socket_t fd, 
    struct sockaddr *address, 
    int socklen,
    void *ctx);

void DnsConnect(BufferContext* buffer_context);

#endif // _CLIENT_SIDE_CALLBACKS_H_
