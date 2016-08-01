#include "http_reply.h"
#include "store_entry.h"
#include "http_callbacks.h"
#include "logger.h"
#include "defines.h"

#include "event2/buffer.h"

#include <stdlib.h>
#include <sys/queue.h>
#include <event.h>

#include <vector>
#include <list>

using std::vector;
using std::list;

void ReplyClientHeader(StoreClient* client, StoreEntry* store_entry)
{
    struct evkeyvalq* headers = store_entry->mem_obj_->headers;
    struct evkeyvalq* client_header = evhttp_request_get_output_headers(client->request);
    struct evkeyval* header;

    LOG_DEBUG("reply client header, client:" << client);
    TAILQ_FOREACH(header, headers, next)
    {
        LOG_DEBUG("header, " << header->key << ":" << header->value);
        evhttp_add_header(client_header, header->key, header->value);
    }
    evhttp_add_header(client_header, "TestCache", client->hit == 1 ? "Hit" : "Miss");
    LOG_DEBUG("header, " << "TestCache:" << (client->hit == 1 ? "Hit" : "Miss"));

    evhttp_send_reply_start(client->request, store_entry->code_, store_entry->code_str_.c_str());
}

void ReplyClientBody2(StoreClient* client, StoreEntry* store_entry)
{
    // 采用evbuffer引用，避免内存拷贝，目前只在单线程下可用
    struct evhttp_connection* evcon = evhttp_request_get_connection(client->request);
    struct bufferevent* bev = evhttp_connection_get_bufferevent(evcon);
    struct evbuffer* output = bufferevent_get_output(bev);

    vector<struct evbuffer*>& bodies = store_entry->mem_obj_->bodies;
    while (client->body_piece_index < (int)bodies.size())
    {   
        int len = evbuffer_get_length(bodies[client->body_piece_index]);
        evbuffer_add_buffer_reference(output, bodies[client->body_piece_index]);

        ++client->body_piece_index;
        client->reply_body_size += len;

        LOG_DEBUG("reply body, size:" << len << " client_request:" << client->request
                << " reply_size:" << client->reply_body_size << " body_size:" << store_entry->body_size_);
    }   
}

void ReplyClientBody(StoreClient* client, StoreEntry* store_entry)
{
    vector<struct evbuffer*>& bodies = store_entry->mem_obj_->bodies;
    while (client->body_piece_index < (int)bodies.size())
    {
        int len = evbuffer_get_length(bodies[client->body_piece_index]);
        char* buf = (char*)malloc(len);
        evbuffer_copyout(bodies[client->body_piece_index], buf, len);
        struct evbuffer* evbuf = evbuffer_new();
        evbuffer_add(evbuf, buf, len);

        LOG_DEBUG("begin reply chunk, client_request:" << client->request);
        evhttp_send_reply_chunk(client->request, evbuf);
        ++client->body_piece_index;
        client->reply_body_size += len;

        free(buf);
        evbuffer_free(evbuf);

        LOG_DEBUG("reply body, size:" << len << " client:" << client->request
                << " reply_size:" << client->reply_body_size << " body_size:" << store_entry->body_size_);
    }
}

void ReplyClient(RequestCtx* request_ctx)
{
    StoreEntry* store_entry = request_ctx->store_entry;
    MemObj* mem_obj = store_entry->mem_obj_;
    StoreClient* store_client = request_ctx->store_client;
    bool delete_client = false;

    store_entry->Lock(READ_LOCKER);
    LOG_DEBUG("lock entry:" << store_entry);
    if (store_entry->status_ == STORE_ERROR)
    {
        // todo
        //if (store_client->reply_header_done)
        //{
        //    evhttp_send_reply_end(store_client->request);
        //}
        //else
        //{
        //    evhttp_send_error(store_client->request, "400", "Bad Request");
        //}
        //return ;
    }
    else
    {
        LOG_DEBUG("mem_obj header:" << mem_obj->headers << " client body_piece_index:" << store_client->body_piece_index
                << " mem_obj body_size:" << mem_obj->bodies.size() << " mem_obj body_piece_num:" << mem_obj->body_piece_num);
        if (mem_obj->headers)
        {
            if (!store_client->reply_header_done)
            {
                ReplyClientHeader(store_client, store_entry);
                store_client->reply_header_done = true;
                LOG_INFO("reply header finished, client_request:" << store_client->request
                        << " now client_num:" << store_entry->GetClientNum());
            }
        }
        if (store_client->body_piece_index < (int)mem_obj->bodies.size())
        {
            ReplyClientBody(store_client, store_entry);
        }
        if (mem_obj->body_piece_num >= 0 && store_client->body_piece_index >= mem_obj->body_piece_num - 1)
        {
            evhttp_send_reply_end(store_client->request);
            if (request_ctx->comm_timer)
            {
                event_del(request_ctx->comm_timer);
                event_free(request_ctx->comm_timer);
                request_ctx->comm_timer = NULL;
                LOG_INFO("free comm_timer, request:" << request_ctx->client_request);

            }
            delete_client = true;
            LOG_INFO("reply client finished, delele client, now client_num:" << store_entry->GetClientNum()
                    << " store_status:" << store_entry->StatusStr()
                    << " request:" << store_client->request);
        }
    }

    LOG_DEBUG("unlock entry:" << store_entry);
    store_entry->Unlock();

    if (delete_client)
    {
        store_entry->Lock(WRITE_LOCKER);
        LOG_DEBUG("lock entry:" << store_entry);

        store_entry->DelClient(store_client);

        LOG_DEBUG("unlock entry:" << store_entry);
        store_entry->Unlock();

    }
}
