#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <event.h>
#include "logger.h"

#include "event2/http.h"
#include "event2/http_struct.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/dns.h"

#include <map>

#define FREE_REQUEST(req) do {if (req) evhttp_request_free(req); req = NULL;} while(0);
#define FREE_CONNECTION(conn) do {if (conn) evhttp_connection_free(conn); conn = NULL;} while(0);

using std::map;

struct event_base* base;
struct evdns_base* dnsbase;
struct evhttp* http_server;

struct RequestContext
{
    struct evhttp_uri* evhttp_uri;
    struct evhttp_request* client_request;
    struct evhttp_request* remote_request;
    struct evhttp_connection* remote_conn;
    struct evhttp_connection* client_conn;
};

void remote_read_cb(struct evhttp_request* response, void* arg)
{
    RequestContext* request_ctx = (RequestContext*)arg;
    evhttp_request* client_request = request_ctx->client_request;
    evhttp_send_reply_end(client_request);

    LOG_INFO("remote_read_cb, buffer length:" << evbuffer_get_length(evhttp_request_get_input_buffer(response))
            << " request_ctx:" << request_ctx);
    LOG_INFO("send reply end");

    return ;
}

int read_header_done_cb(struct evhttp_request* request, void* arg)
{
    RequestContext* request_ctx = (RequestContext*)arg;
    evhttp_request* client_request = request_ctx->client_request;

    struct bufferevent* client_buffev = evhttp_connection_get_bufferevent(request_ctx->client_conn);
    struct bufferevent* remote_buffev = evhttp_connection_get_bufferevent(request_ctx->remote_conn);

    LOG_INFO("client input buf:" << evhttp_request_get_input_buffer(request_ctx->client_request)
            << " output buf:" << evhttp_request_get_output_buffer(request_ctx->client_request));
    LOG_INFO("client bev inbuf:" << bufferevent_get_input(client_buffev)
            << " outbuf:" << bufferevent_get_output(client_buffev));
    LOG_INFO("remote input buf:" << evhttp_request_get_input_buffer(request_ctx->remote_request)
            << " output buf:" << evhttp_request_get_output_buffer(request_ctx->remote_request));
    LOG_INFO("remote bev inbuf:" << bufferevent_get_input(remote_buffev)
            << " outbuf:" << bufferevent_get_output(remote_buffev));
    LOG_INFO("read_header_done_cb, request_ctx:" << request_ctx);
    
    //evbuffer_add_printf(bufferevent_get_output(client_buffev), "HTTP/1.1 200 OK\r\n");

    struct evkeyvalq* request_headers = evhttp_request_get_input_headers(request);
    struct evkeyval* header;
    LOG_INFO("remote remote header done:");
    for (header = request_headers->tqh_first; header; header = header->next.tqe_next)
    {
        LOG_INFO("header, " << header->key << ":" << header->value);
        evhttp_add_header(evhttp_request_get_output_headers(client_request), header->key, header->value);
    }
    evhttp_add_header(evhttp_request_get_output_headers(client_request), "test", "zhangmenghan");
    evhttp_send_reply_start(client_request, 200, "OK");

}

void read_chunk_cb(struct evhttp_request* remote_response, void* arg)
{
    RequestContext* request_ctx = (RequestContext*)arg;
    evhttp_request* client_request = request_ctx->client_request;

    evbuffer* response_buf = evhttp_request_get_input_buffer(remote_response);

    LOG_INFO("read_chunk_cb, read buf length:" << evbuffer_get_length(response_buf));
    evhttp_send_reply_chunk(client_request, response_buf);
}


void reply_complete_cb(struct evhttp_request* request, void* arg)
{
    RequestContext* request_ctx = (RequestContext*)arg;
    evhttp_request* client_request = request_ctx->client_request;
    
    LOG_INFO("reply_complete_cb, request:"<< request << " client_request:" << request_ctx->client_request);
    LOG_INFO("free remote_conn:" << request_ctx->remote_conn);
    evhttp_connection_free(request_ctx->remote_conn);

}

void send_remote_complete_cb(struct evhttp_request* request, void* arg)
{
    RequestContext* request_ctx = (RequestContext*)arg;

    LOG_INFO("send_remote_complete_cb, remote_request:"<< request << " remote_request:" << request_ctx->remote_request);
}

void client_connection_closecb(struct evhttp_connection* connection, void* arg)
{
    LOG_INFO("client_connection_closecb");
}

void remote_connection_closecb(struct evhttp_connection* connection, void* arg)
{
    LOG_INFO("remote_connection_closecb");
}

void remote_request_error_cb(enum evhttp_request_error error, void* arg)
{
    LOG_INFO("remote_request_error_cb, error:" << error << " arg:" << arg);
}

void ConnectRemote(RequestContext* request_ctx)
{
    struct evhttp_request* request = evhttp_request_new(remote_read_cb, request_ctx);
    evhttp_request_set_header_cb(request, read_header_done_cb);
    evhttp_request_set_chunked_cb(request, read_chunk_cb);
    evhttp_request_set_error_cb(request, remote_request_error_cb);
    evhttp_request_set_on_complete_cb(request, send_remote_complete_cb, request_ctx);

    const char* host = evhttp_uri_get_host(request_ctx->evhttp_uri);
    if (!host)
    {
        evhttp_send_error(request_ctx->client_request, 400, "Fail");
        return ;
    }

    int port = evhttp_uri_get_port(request_ctx->evhttp_uri);
    if (port < 0) port = 80;

    struct evhttp_connection* connection =  evhttp_connection_base_new(base, dnsbase, 
            evhttp_uri_get_host(request_ctx->evhttp_uri),
            evhttp_uri_get_port(request_ctx->evhttp_uri) > 0 ? evhttp_uri_get_port(request_ctx->evhttp_uri) : 80);
    if (!connection)
    {
        evhttp_send_error(request_ctx->client_request, 400, "Fail");
        return ;
    }

    evhttp_connection_set_closecb(connection, remote_connection_closecb, request_ctx);
    LOG_INFO("request_ctx:" << request_ctx
            << " remote_request:" << request 
            << " host:" << host 
            << " port:" << port 
            << " path:" << evhttp_uri_get_path(request_ctx->evhttp_uri));

    struct evkeyvalq* request_headers = evhttp_request_get_input_headers(request_ctx->client_request);
    struct evkeyval* header;
    for (header = request_headers->tqh_first; header; header = header->next.tqe_next)
    {
        evhttp_add_header(evhttp_request_get_output_headers(request), header->key, header->value);
        LOG_INFO("header, " << header->key << ":" << header->value);
    }

    request_ctx->remote_request = request;
    request_ctx->remote_conn = connection;

    evhttp_make_request(connection, request, EVHTTP_REQ_GET, evhttp_uri_get_path(request_ctx->evhttp_uri));
}


void generic_cb(struct evhttp_request* req, void* arg)
{

    const struct evhttp_uri* evhttp_uri = evhttp_request_get_evhttp_uri(req);
    char* url = (char*)malloc(1024);
    LOG_INFO("uri_join:" << evhttp_uri_join(const_cast<struct evhttp_uri*>(evhttp_uri), url, 1024));
    free(url);
    
    RequestContext* request_ctx = new RequestContext();
    request_ctx->client_request = req;
    request_ctx->client_conn = evhttp_request_get_connection(req);
    request_ctx->evhttp_uri = const_cast<struct evhttp_uri*>(evhttp_uri);

    evhttp_request_set_on_complete_cb(req, reply_complete_cb, request_ctx);
    evhttp_connection_set_closecb(request_ctx->client_conn, client_connection_closecb, request_ctx);

    ConnectRemote(request_ctx);

    LOG_INFO("client_request:" << req << " request_ctx:" << request_ctx);
}

int main(int argc, char **argv)
{
    google::InitGoogleLogging(argv[0]);
    base = event_base_new();
    dnsbase = evdns_base_new(base, 1);

    http_server = evhttp_new(base);
    if (!http_server)
    {
        LOG_INFO("create http_base failed");
        return 1;
    }

    int ret = evhttp_bind_socket(http_server, "0.0.0.0", 3334);
    if (ret != 0)
    {
        LOG_INFO("bind failed");
        return 1;
    }

    evhttp_set_gencb(http_server, generic_cb, NULL);
    event_base_dispatch(base);
    evhttp_free(http_server);

    return 0;
}
