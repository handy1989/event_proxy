#include "http_callbacks.h"
#include "logger.h"

#include <sys/queue.h>
#include <event.h>
#include <stdlib.h>

void HttpGenericCallback(struct evhttp_request* request, void* arg)
{
    const struct evhttp_uri* evhttp_uri = evhttp_request_get_evhttp_uri(request);
    char* url = (char*)malloc(1024);
    LOG_INFO("uri_join:" << evhttp_uri_join(const_cast<struct evhttp_uri*>(evhttp_uri), url, 1024));
    free(url);

    CallbackPara* callback_para = new CallbackPara();
    callback_para->http_service = (HttpService*)arg;
    callback_para->client_request = request;
    callback_para->client_conn = evhttp_request_get_connection(request);
    callback_para->uri = const_cast<struct evhttp_uri*>(evhttp_uri);

    evhttp_request_set_on_complete_cb(request, ReplyCompleteCallback, callback_para);
    evhttp_connection_set_closecb(callback_para->client_conn, ClientConnectionCloseCallback, callback_para);

    LOG_INFO("new callback_para:" << callback_para);

    ConnectRemote(callback_para);
}

void ReplyCompleteCallback(struct evhttp_request* request, void* arg)
{
    CallbackPara* callback_para = (CallbackPara*)arg;

    evhttp_request* client_request = callback_para->client_request;
    
    LOG_INFO("reply_complete_cb, request:"<< request << " client_request:" << callback_para->client_request);

}

void ClientConnectionCloseCallback(struct evhttp_connection* connection, void* arg)
{
    CallbackPara* callback_para = (CallbackPara*)arg;
    LOG_INFO("client_connection_closecb, cur conn:" << connection
            << " client_conn:" << callback_para->client_conn);
}

void FreeRemoteConnCallback(int sock, short which, void* arg)
{   
    CallbackPara* callback_para = (CallbackPara*)arg;
    
    if (callback_para->remote_conn)
    {   
        LOG_INFO("free remote_conn:" << callback_para->remote_conn);
        evhttp_connection_free(callback_para->remote_conn);
        callback_para->remote_conn = NULL;
    }
    if (callback_para->clean_timer)
    {
        event_del(callback_para->clean_timer);
        event_free(callback_para->clean_timer);

        LOG_INFO("delete callback_para:" << callback_para);

        delete callback_para;
        callback_para = NULL;
    }
}

void RemoteReadCallback(struct evhttp_request* response, void* arg)
{
    CallbackPara* callback_para = (CallbackPara*)arg;
    evhttp_request* client_request = callback_para->client_request;

    evhttp_send_reply_end(client_request);
    LOG_INFO("callback_para:" << callback_para << " response:" << response << " connection:" << callback_para->remote_conn);
    if (!response)
    {
        return;
    }
    LOG_INFO("remote_read_cb, buffer length:" << evbuffer_get_length(evhttp_request_get_input_buffer(response)));
    LOG_INFO("send reply end");
    
    struct timeval interval;
    interval.tv_sec = 0;
    interval.tv_usec = 1000;
    
    callback_para->clean_timer = event_new(callback_para->http_service->base(), -1, EV_PERSIST, FreeRemoteConnCallback, callback_para);
    if (!callback_para->clean_timer)
    {   
        LOG_ERROR("failed to create clean_timer, callback_para:" << callback_para
                << " remote_conn:" << callback_para->remote_conn);
        return ;
    }
    if (event_add(callback_para->clean_timer, &interval) != 0)
    {   
        LOG_ERROR("failed to add clean_timer, callback_para:" << callback_para
                << " remote_conn:" << callback_para->remote_conn
                << " clean_timer:" << callback_para->clean_timer);
        event_free(callback_para->clean_timer);
    }

    return ;
}

int ReadHeaderDoneCallback(struct evhttp_request* remote_rsp, void* arg)
{
    CallbackPara* callback_para = (CallbackPara*)arg;
    evhttp_request* client_request = callback_para->client_request;

    struct evkeyvalq* request_headers = evhttp_request_get_input_headers(remote_rsp);
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

void ReadChunkCallback(struct evhttp_request* remote_rsp, void* arg)
{
    CallbackPara* callback_para = (CallbackPara*)arg;
    evhttp_request* client_request = callback_para->client_request;

    evbuffer* response_buf = evhttp_request_get_input_buffer(remote_rsp);

    LOG_INFO("read_chunk_cb, read buf length:" << evbuffer_get_length(response_buf));
    evhttp_send_reply_chunk(client_request, response_buf);
}

void RemoteRequestErrorCallback(enum evhttp_request_error error, void* arg)
{
    LOG_INFO("remote_request_error_cb, error:" << error << " arg:" << arg);
}

void SendRemoteCompleteCallback(struct evhttp_request* request, void* arg)
{
    CallbackPara* callback_para = (CallbackPara*)arg;

    LOG_INFO("send_remote_complete_cb, cur request:"<< request << " remote_request:" << callback_para->remote_request);
}

void RemoteConnectionCloseCallback(struct evhttp_connection* connection, void* arg)
{
    LOG_INFO("remote_connection_closecb, conn:" << connection);
}

void ConnectRemote(CallbackPara* callback_para)
{
    struct evhttp_request* request = evhttp_request_new(RemoteReadCallback, callback_para);
    evhttp_request_set_header_cb(request, ReadHeaderDoneCallback);
    evhttp_request_set_chunked_cb(request, ReadChunkCallback);
    evhttp_request_set_error_cb(request, RemoteRequestErrorCallback);
    evhttp_request_set_on_complete_cb(request, SendRemoteCompleteCallback, callback_para);

    const char* host = evhttp_uri_get_host(callback_para->uri);
    if (!host)
    {
        evhttp_send_error(callback_para->client_request, 400, "Fail");
        return ;
    }

    int port = evhttp_uri_get_port(callback_para->uri);
    if (port < 0) port = 80;

    LOG_INFO("callback_para:" << callback_para
            << " remote_request:" << request 
            << " host:" << host 
            << " port:" << port 
            << " path:" << evhttp_uri_get_path(callback_para->uri)
            << " query:" << evhttp_uri_get_query(callback_para->uri));

    struct evhttp_connection* connection =  evhttp_connection_base_new(
            callback_para->http_service->base(), 
            callback_para->http_service->dnsbase(), 
            host,
            port);
    if (!connection)
    {
        evhttp_send_error(callback_para->client_request, 400, "Fail");
        return ;
    }

    evhttp_connection_set_closecb(connection, RemoteConnectionCloseCallback, callback_para);

    struct evkeyvalq* request_headers = evhttp_request_get_input_headers(callback_para->client_request);
    struct evkeyval* header;
    for (header = request_headers->tqh_first; header; header = header->next.tqe_next)
    {
        evhttp_add_header(evhttp_request_get_output_headers(request), header->key, header->value);
        LOG_INFO("header, " << header->key << ":" << header->value);
    }

    callback_para->remote_request = request;
    callback_para->remote_conn = connection;

    char* url = (char*)malloc(8192);
    evhttp_make_request(connection, request, EVHTTP_REQ_GET, evhttp_uri_join(const_cast<struct evhttp_uri*>(callback_para->uri), url, 8192));
    free(url);
}
