#include "http_callbacks.h"
#include "logger.h"
#include "cache_mgr.h"
#include "store_entry.h"
#include "http_reply.h"
#include "http_send.h"
#include "defines.h"

#include <sys/queue.h>
#include <event.h>
#include <stdlib.h>

void ReplyClientCallback(int sock, short which, void* arg)
{
    ReplyClient((RequestCtx*)arg);
}

void HttpGenericCallback(struct evhttp_request* request, void* arg)
{
    const struct evhttp_uri* evhttp_uri = evhttp_request_get_evhttp_uri(request);

    RequestCtx* request_ctx = new RequestCtx();
    request_ctx->http_service = (HttpService*)arg;
    request_ctx->client_request = request;
    request_ctx->client_conn = evhttp_request_get_connection(request);
    request_ctx->uri = const_cast<struct evhttp_uri*>(evhttp_uri);
    request_ctx->url = (char*)malloc(MAX_URL_SIZE);
    evhttp_uri_join(request_ctx->uri, request_ctx->url, MAX_URL_SIZE);

    StoreEntry* entry = SingletonCacheMgr::Instance().GetStoreEntry(request_ctx->url);
    entry->Lock();
    StoreClient* store_client = new StoreClient();
    store_client->request = request;
    entry->AddClient(store_client);
    int client_num = entry->GetClientNum();
    if (client_num > 1 || entry->completion_)
    {
        store_client->hit = 1;
    }
    entry->Unlock();

    if (client_num > 1)
    {
        // 正在处理相同请求
        LOG_INFO("deliver to other process, url:" << request_ctx->url << " client_num:" << client_num << " client_request:" << store_client->request);
        return ;
    }

    request_ctx->store_entry = entry;

    if (entry->completion_)
    {
        LOG_INFO("hit in cache, url:" << request_ctx->url << " client_num:" << client_num << " client:" << store_client);
        struct timeval interval;
        interval.tv_sec = 0;
        interval.tv_usec = 10;
        request_ctx->comm_timer = event_new(request_ctx->http_service->base(), -1, EV_PERSIST, ReplyClientCallback, request_ctx);
        if (!request_ctx->comm_timer)
        {   
            LOG_ERROR("failed to create comm_timer, request_ctx:" << request_ctx
                    << " client_request:" << request_ctx->client_request);
            return ;
        }
        if (event_add(request_ctx->comm_timer, &interval) != 0)
        {   
            LOG_ERROR("failed to add comm_timer, request_ctx:" << request_ctx
                    << " client_reqeust:" << request_ctx->client_request);
            event_free(request_ctx->comm_timer);
        }
        return ;
    }

    LOG_INFO("process url:" << request_ctx->url << " client_num:" << client_num << " client_request:" << store_client->request << " request_ctx:" << request_ctx);

    evhttp_request_set_on_complete_cb(request, ReplyCompleteCallback, request_ctx);
    evhttp_connection_set_closecb(request_ctx->client_conn, ClientConnectionCloseCallback, request_ctx);

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
            << " client_conn:" << request_ctx->client_conn
            << " client_request:" << request_ctx->client_request);
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

        SAFELY_DELETE(request_ctx->url);
        SAFELY_DELETE(request_ctx);
    }
}

void RemoteReadCallback(struct evhttp_request* response, void* arg)
{
    RequestCtx* request_ctx = (RequestCtx*)arg;
    StoreEntry* store_entry = request_ctx->store_entry;
    store_entry->mem_obj_->body_piece_num = store_entry->mem_obj_->bodies.size();
    ReplyClient(request_ctx);
    store_entry->completion_ = true;

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
    StoreEntry* store_entry = request_ctx->store_entry;

    store_entry->mem_obj_ = new MemObj();
    MemObj* mem_obj = store_entry->mem_obj_;
    mem_obj->headers = (struct evkeyvalq*)malloc(sizeof(*(mem_obj->headers)));
    TAILQ_INIT(mem_obj->headers);

    struct evkeyvalq* request_headers = evhttp_request_get_input_headers(remote_rsp);
    struct evkeyval* header;
    LOG_DEBUG("read remote header done:");
    for (header = request_headers->tqh_first; header; header = header->next.tqe_next)
    {
        LOG_DEBUG("header, " << header->key << ":" << header->value);
        evhttp_add_header(mem_obj->headers, header->key, header->value);
        if (strcmp(header->key, "Content-Length") == 0)
        {
            store_entry->body_size_ = atoi(header->value);
        }
    }
    evhttp_add_header(mem_obj->headers, "test", "zhangmenghan");

    ReplyClient(request_ctx);

    return 0;
}

void ReadChunkCallback(struct evhttp_request* remote_rsp, void* arg)
{
    RequestCtx* request_ctx = (RequestCtx*)arg;
    MemObj* mem_obj = request_ctx->store_entry->mem_obj_;
    struct evbuffer* evbuf = evbuffer_new();
    evbuffer_add_buffer(evbuf, evhttp_request_get_input_buffer(remote_rsp));
    mem_obj->bodies.push_back(evbuf);
    LOG_DEBUG("read_chunk_cb, read buf length:" << evbuffer_get_length(evbuf)
            << " bodies size:" << mem_obj->bodies.size()
            << " client_reqeust:" << request_ctx->client_request
            << " remote_rsp:" << remote_rsp);
    
    ReplyClient(request_ctx);

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
