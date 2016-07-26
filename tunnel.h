#ifndef TUNNEL_H_
#define TUNNEL_H_

#include <string>

struct TunnelState
{
    struct evhttp_connection* client_conn;
    struct bufferevent* client;
    struct bufferevent* remote;
    std::string host;
    int port;
};

struct RequestCtx;

void TunnelStart(RequestCtx* request_ctx);
void TunnelReadClientCallback(struct bufferevent* bev, void* arg);
void TunnelReadRemoteCallback(struct bufferevent* bev, void* arg);
void TunnelRemoteEventCallback(struct bufferevent* bev, short events, void* arg);

#endif // TUNNEL_H_
