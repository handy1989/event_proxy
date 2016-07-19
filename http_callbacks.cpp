#include "http_callbacks.h"
#include "logger.h"
#include "cache_mgr.h"
#include "store_entry.h"

#include <sys/queue.h>
#include <event.h>
#include <stdlib.h>

using std::vector;
using std::string;
using std::list;

#define SAFELY_DELETE(p) do{if(p)delete p; p = NULL;}while(0);
#define MAX_URL_SIZE 8192

void HttpGenericCallback(struct evhttp_request* request, void* arg)
{
    const struct evhttp_uri* evhttp_uri = evhttp_request_get_evhttp_uri(request);

    RequestCtx* request_ctx = new RequestCtx();
    request_ctx->http_service = (HttpService*)arg;
    request_ctx->client_request = request;
    request_ctx->client_conn = evhttp_request_get_connection(request);
    request_ctx->uri = const_cast<struct evhttp_uri*>(evhttp_uri);
    request_ctx->url = (char*)malloc(MAX_URL_SIZE);

    LOG_DEBUG("uri_join:" << evhttp_uri_join(const_cast<struct evhttp_uri*>(evhttp_uri), request_ctx->url, MAX_URL_SIZE));

    StoreEntry* entry = SingletonCacheMgr::Instance().GetStoreEntry(request_ctx->url);
    LOG_DEBUG("url:" << request_ctx->url << " entry:" << entry);

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
        return ;
    }

    request_ctx->store_entry = entry;

    if (entry->completion_)
    {
        ReplyClient(request_ctx);
        return ;
    }

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

void ReplyClientHeader(StoreClient* client, struct evkeyvalq* headers)
{
    struct evkeyvalq* client_header = evhttp_request_get_output_headers(client->request);
    struct evkeyval* header;
    LOG_DEBUG("reply client header:");
    for (header = headers->tqh_first; header; header = header->next.tqe_next)
    {
        LOG_DEBUG("header, " << header->key << ":" << header->value);
        evhttp_add_header(client_header, header->key, header->value);
    }
    evhttp_add_header(client_header, "TestCache", client->hit == 1 ? "Hit" : "Miss");
    client->reply_header_done = true;
    evhttp_send_reply_start(client->request, 200, "OK");
}

void ReplyClientBody(StoreClient* client, vector<struct evbuffer*>& bodies)
{
    while (client->body_piece_index < bodies.size())
    {
        int len = evbuffer_get_length(bodies[client->body_piece_index]);
        char* buf = (char*)malloc(len);
        evbuffer_copyout(bodies[client->body_piece_index], buf, len);
        struct evbuffer* evbuf = evbuffer_new();
        evbuffer_add(evbuf, buf, len);
        evhttp_send_reply_chunk(client->request, evbuf);
        ++client->body_piece_index;

        free(buf);
        evbuffer_free(evbuf);

        LOG_DEBUG("reply body size:" << len);
    }
}

void ReplyClient(RequestCtx* request_ctx)
{
    StoreEntry* store_entry = request_ctx->store_entry;
    MemObj* mem_obj = store_entry->mem_obj_;

    store_entry->Lock(); // 这个锁加的有点大，如果回复数据量很大，对其它请求会有影响
    for (list<StoreClient*>::iterator it = store_entry->store_clients_.begin(); it != store_entry->store_clients_.end();)
    {
        if (mem_obj->headers)
        {
            // 已完成包头接收，给客户端回包头
            if (!(*it)->reply_header_done)
            {
                ReplyClientHeader(*it, mem_obj->headers);
            }
        }
        if ((*it)->body_piece_index < mem_obj->bodies.size())
        {
            ReplyClientBody(*it, mem_obj->bodies);
        }
        if ((*it)->body_piece_index >= mem_obj->body_piece_num - 1)
        {
            evhttp_send_reply_end((*it)->request);
            SAFELY_DELETE(*it);
            store_entry->store_clients_.erase(it++);
        }
        else
        {
            it++;
        }
        
    }
    store_entry->Unlock();
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
    LOG_DEBUG("remote remote header done:");
    for (header = request_headers->tqh_first; header; header = header->next.tqe_next)
    {
        LOG_DEBUG("header, " << header->key << ":" << header->value);
        evhttp_add_header(mem_obj->headers, header->key, header->value);
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
