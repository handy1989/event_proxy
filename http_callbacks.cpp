#include "http_callbacks.h"
#include "logger.h"

#include <sys/queue.h>
#include <event.h>
#include <stdlib.h>

void HttpGenericCallback(struct evhttp_request* request, void* arg)
{
    const struct evhttp_uri* evhttp_uri = evhttp_request_get_evhttp_uri(request);
    char* url = (char*)malloc(1024);
    LOG_DEBUG("uri_join:" << evhttp_uri_join(const_cast<struct evhttp_uri*>(evhttp_uri), url, 1024));
    free(url);

    //sleep(1);

    //evbuffer* buf = evbuffer_new();
    //evbuffer_add_printf(buf, "hello proxy");
    //evhttp_send_reply(request, 200, "OK", buf);
    //evbuffer_free(buf);
    //
    //LOG_DEBUG("HttpGenericCallback finished");
    //return ;

    RequestCtx* request_ctx = new RequestCtx();
    request_ctx->http_service = (HttpService*)arg;
    request_ctx->client_request = request;
    request_ctx->client_conn = evhttp_request_get_connection(request);
    request_ctx->uri = const_cast<struct evhttp_uri*>(evhttp_uri);

    evhttp_request_set_on_complete_cb(request, ReplyCompleteCallback, request_ctx);
    evhttp_connection_set_closecb(request_ctx->client_conn, ClientConnectionCloseCallback, request_ctx);

    LOG_INFO("new request_ctx:" << request_ctx);

    ConnectRemote(request_ctx);
}

void ReplyCompleteCallback(struct evhttp_request* request, void* arg)
{
    RequestCtx* request_ctx = (RequestCtx*)arg;

    evhttp_request* client_request = request_ctx->client_request;
    
    LOG_INFO("reply_complete_cb, request:"<< request << " client_request:" << client_request);

}

void ClientConnectionCloseCallback(struct evhttp_connection* connection, void* arg)
{
    RequestCtx* request_ctx = (RequestCtx*)arg;
    LOG_INFO("client_connection_closecb, cur conn:" << connection
            << " client_conn:" << request_ctx->client_conn);
}

void FreeRemoteConnCallback(int sock, short which, void* arg)
{   
    RequestCtx* request_ctx = (RequestCtx*)arg;
    
    if (request_ctx->remote_conn)
    {   
        LOG_INFO("free remote_conn:" << request_ctx->remote_conn);
        evhttp_connection_free(request_ctx->remote_conn);
        request_ctx->remote_conn = NULL;
    }
    if (request_ctx->clean_timer)
    {
        event_del(request_ctx->clean_timer);
        event_free(request_ctx->clean_timer);

        LOG_INFO("delete request_ctx:" << request_ctx);

        delete request_ctx;
        request_ctx = NULL;
    }
}

void RemoteReadCallback(struct evhttp_request* response, void* arg)
{
    RequestCtx* request_ctx = (RequestCtx*)arg;
    evhttp_request* client_request = request_ctx->client_request;

    evhttp_send_reply_end(client_request);
    LOG_INFO("reply end, request_ctx:" << request_ctx << " response:" << response << " connection:" << request_ctx->remote_conn);
    if (!response)
    {
        return;
    }
    LOG_DEBUG("remote_read_cb, buffer length:" << evbuffer_get_length(evhttp_request_get_input_buffer(response)));
    
    struct timeval interval;
    interval.tv_sec = 0;
    interval.tv_usec = 1000;
    
    request_ctx->clean_timer = event_new(request_ctx->http_service->base(), -1, EV_PERSIST, FreeRemoteConnCallback, request_ctx);
    if (!request_ctx->clean_timer)
    {   
        LOG_ERROR("failed to create clean_timer, request_ctx:" << request_ctx
                << " remote_conn:" << request_ctx->remote_conn);
        return ;
    }
    if (event_add(request_ctx->clean_timer, &interval) != 0)
    {   
        LOG_ERROR("failed to add clean_timer, request_ctx:" << request_ctx
                << " remote_conn:" << request_ctx->remote_conn
                << " clean_timer:" << request_ctx->clean_timer);
        event_free(request_ctx->clean_timer);
    }

    return ;
}

int ReadHeaderDoneCallback(struct evhttp_request* remote_rsp, void* arg)
{
    RequestCtx* request_ctx = (RequestCtx*)arg;
    evhttp_request* client_request = request_ctx->client_request;

    struct evkeyvalq* request_headers = evhttp_request_get_input_headers(remote_rsp);
    struct evkeyval* header;
    LOG_DEBUG("remote remote header done:");
    for (header = request_headers->tqh_first; header; header = header->next.tqe_next)
    {
        LOG_DEBUG("header, " << header->key << ":" << header->value);
        evhttp_add_header(evhttp_request_get_output_headers(client_request), header->key, header->value);
    }
    evhttp_add_header(evhttp_request_get_output_headers(client_request), "test", "zhangmenghan");
    evhttp_send_reply_start(client_request, 200, "OK");

    return 0;
}

void ReadChunkCallback(struct evhttp_request* remote_rsp, void* arg)
{
    RequestCtx* request_ctx = (RequestCtx*)arg;
    evhttp_request* client_request = request_ctx->client_request;

    evbuffer* response_buf = evhttp_request_get_input_buffer(remote_rsp);

    LOG_DEBUG("read_chunk_cb, read buf length:" << evbuffer_get_length(response_buf)
            << " client_reqeust:" << request_ctx->client_request
            << " remote_rsp:" << remote_rsp);
    evhttp_send_reply_chunk(client_request, response_buf);
}

void RemoteRequestErrorCallback(enum evhttp_request_error error, void* arg)
{
    LOG_INFO("remote_request_error_cb, error:" << error << " arg:" << arg);
}

void SendRemoteCompleteCallback(struct evhttp_request* request, void* arg)
{
    RequestCtx* request_ctx = (RequestCtx*)arg;

    LOG_INFO("send_remote_complete_cb, cur request:"<< request << " remote_request:" << request_ctx->remote_request);
}

void RemoteConnectionCloseCallback(struct evhttp_connection* connection, void* arg)
{
    LOG_INFO("remote_connection_closecb, conn:" << connection);
}

void ConnectRemote(RequestCtx* request_ctx)
{
    struct evhttp_request* request = evhttp_request_new(RemoteReadCallback, request_ctx);
    evhttp_request_set_header_cb(request, ReadHeaderDoneCallback);
    evhttp_request_set_chunked_cb(request, ReadChunkCallback);
    evhttp_request_set_error_cb(request, RemoteRequestErrorCallback);
    evhttp_request_set_on_complete_cb(request, SendRemoteCompleteCallback, request_ctx);

    const char* host = evhttp_uri_get_host(request_ctx->uri);
    if (!host)
    {
        evhttp_send_error(request_ctx->client_request, 400, "Fail");
        return ;
    }

    int port = evhttp_uri_get_port(request_ctx->uri);
    if (port < 0) port = 80;

    LOG_INFO("request_ctx:" << request_ctx
            << " remote_request:" << request 
            << " host:" << host 
            << " port:" << port 
            << " path:" << evhttp_uri_get_path(request_ctx->uri)
            << " query:" << evhttp_uri_get_query(request_ctx->uri));

    struct evhttp_connection* connection =  evhttp_connection_base_new(
            request_ctx->http_service->base(), 
            request_ctx->http_service->dnsbase(), 
            host,
            port);
    if (!connection)
    {
        evhttp_send_error(request_ctx->client_request, 400, "Fail");
        return ;
    }

    evhttp_connection_set_closecb(connection, RemoteConnectionCloseCallback, request_ctx);

    struct evkeyvalq* request_headers = evhttp_request_get_input_headers(request_ctx->client_request);
    struct evkeyval* header;
    for (header = request_headers->tqh_first; header; header = header->next.tqe_next)
    {
        evhttp_add_header(evhttp_request_get_output_headers(request), header->key, header->value);
        LOG_DEBUG("header, " << header->key << ":" << header->value);
    }

    request_ctx->remote_request = request;
    request_ctx->remote_conn = connection;

    char* url = (char*)malloc(8192);
    evhttp_make_request(connection, request, EVHTTP_REQ_GET, evhttp_uri_join(const_cast<struct evhttp_uri*>(request_ctx->uri), url, 8192));
    free(url);

    LOG_DEBUG("make request finished. request_ctx:" << request_ctx);
}
