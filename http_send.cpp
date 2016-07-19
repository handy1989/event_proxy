#include "http_send.h"
#include "logger.h"
#include "http_callbacks.h"

#include <stdlib.h>
#include <sys/queue.h>
#include <event.h>

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
    LOG_DEBUG("send remote header:");
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
