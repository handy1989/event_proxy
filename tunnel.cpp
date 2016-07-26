#include "tunnel.h"
#include "http_callbacks.h"
#include "logger.h"

#include <stdlib.h>

using std::string;

void TunnelStart(RequestCtx* request_ctx)
{
    TunnelState* tunnel_state = new TunnelState();

    char* url = request_ctx->url;
    char* host_end = strchr(url, ':');
    if (!host_end)
    {
        LOG_ERROR("url error:" << url);
        return ;
    }

    LOG_DEBUG("url:" << url);

    tunnel_state->host = string(url, host_end - url);
    tunnel_state->port = atoi(host_end + 1);
    tunnel_state->client = evhttp_connection_get_bufferevent(request_ctx->client_conn);
    tunnel_state->client_conn = request_ctx->client_conn;


    tunnel_state->remote = bufferevent_socket_new(request_ctx->http_service->base(), -1, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(tunnel_state->remote, TunnelReadRemoteCallback, NULL, TunnelRemoteEventCallback, tunnel_state);
    bufferevent_enable(tunnel_state->remote, EV_READ|EV_WRITE);
    bufferevent_socket_connect_hostname(tunnel_state->remote, request_ctx->http_service->dnsbase(), AF_UNSPEC, tunnel_state->host.c_str(), tunnel_state->port);

    LOG_DEBUG("TunnelStart, connect to " << tunnel_state->host << ":" << tunnel_state->port);
}

void TunnelRemoteEventCallback(struct bufferevent* bev, short events, void* arg)
{
    TunnelState* tunnel_state = (TunnelState*)arg;
    LOG_DEBUG("TunnelRemoteEventCallback");
    if (events & BEV_EVENT_CONNECTED) 
    {
        LOG_INFO("create tunnel " << tunnel_state->host << ":" << tunnel_state->port << " OK");
        LOG_DEBUG("reply established info to client");
        bufferevent_enable(tunnel_state->client, EV_READ|EV_WRITE);
        struct evbuffer* client_output = bufferevent_get_output(tunnel_state->client);
        evbuffer_add_printf(client_output, "HTTP/1.0 200 Connection established\r\n");
        evbuffer_add_printf(client_output, "\r\n");
        bufferevent_setcb(tunnel_state->client, TunnelReadClientCallback, NULL, NULL, tunnel_state);
    } 
    else if (events & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) 
    {
        if (events & BEV_EVENT_ERROR) 
        {
            int err = bufferevent_socket_get_dns_error(bev);
            if (err)
            {
                LOG_ERROR("DNS error:" << evutil_gai_strerror(err));
            }
        }
        bufferevent_free(bev);
        evhttp_connection_free(tunnel_state->client_conn);
        delete tunnel_state;
        LOG_DEBUG("free remote:" << bev << " delete tunnel_state:" << tunnel_state);
    }
}

void TunnelReadClientCallback(struct bufferevent* bev, void* arg)
{
    TunnelState* tunnel_state = (TunnelState*)arg;
    struct evbuffer* client_input = bufferevent_get_input(tunnel_state->client);
    struct evbuffer* remote_output = bufferevent_get_output(tunnel_state->remote);
    
    LOG_DEBUG("tunnel read client length:" << evbuffer_get_length(client_input));

    evbuffer_add_buffer(remote_output, client_input);
}

void TunnelReadRemoteCallback(struct bufferevent* bev, void* arg)
{
    TunnelState* tunnel_state = (TunnelState*)arg;
    struct evbuffer* remote_input = bufferevent_get_input(tunnel_state->remote);
    struct evbuffer* client_output = bufferevent_get_output(tunnel_state->client);

    LOG_DEBUG("tunnel read remote length:" << evbuffer_get_length(remote_input));

    evbuffer_add_buffer(client_output, remote_input);
}
