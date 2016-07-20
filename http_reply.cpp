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
    for (header = headers->tqh_first; header; header = header->next.tqe_next)
    {
        LOG_DEBUG("header, " << header->key << ":" << header->value);
        evhttp_add_header(client_header, header->key, header->value);
    }
    evhttp_add_header(client_header, "TestCache", client->hit == 1 ? "Hit" : "Miss");
    LOG_DEBUG("header, " << "TestCache:" << (client->hit == 1 ? "Hit" : "Miss"));
    client->reply_header_done = true;
    evhttp_send_reply_start(client->request, store_entry->code_, store_entry->code_str_.c_str());
}

void ReplyClientBody(StoreClient* client, StoreEntry* store_entry)
{
    vector<struct evbuffer*>& bodies = store_entry->mem_obj_->bodies;
    while (client->body_piece_index < bodies.size())
    {
        int len = evbuffer_get_length(bodies[client->body_piece_index]);
        char* buf = (char*)malloc(len);
        evbuffer_copyout(bodies[client->body_piece_index], buf, len);
        struct evbuffer* evbuf = evbuffer_new();
        evbuffer_add(evbuf, buf, len);
        LOG_DEBUG("begin reply chunk, client_request:" << client->request);
        evhttp_send_reply_chunk(client->request, evbuf);
        ++client->body_piece_index;

        free(buf);
        evbuffer_free(evbuf);

        client->reply_body_size += len;

        LOG_DEBUG("reply body, size:" << len << " client:" << client->request
                << " reply_size:" << client->reply_body_size << " body_size:" << store_entry->body_size_);
    }
}

void ReplyClient(RequestCtx* request_ctx)
{
    StoreEntry* store_entry = request_ctx->store_entry;
    MemObj* mem_obj = store_entry->mem_obj_;
    StoreClient* store_client = request_ctx->store_client;

    store_entry->Lock();
    LOG_INFO("lock entry:" << store_entry);
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
        if (mem_obj->headers)
        {
            if (!store_client->reply_header_done)
            {
                ReplyClientHeader(store_client, store_entry);
            }
        }
        if (store_client->body_piece_index < mem_obj->bodies.size())
        {
            ReplyClientBody(store_client, store_entry);
        }
        if (store_client->body_piece_index >= mem_obj->body_piece_num - 1)
        {
            evhttp_send_reply_end(store_client->request);
            if (request_ctx->comm_timer)
            {
                event_del(request_ctx->comm_timer);
                event_free(request_ctx->comm_timer);
                request_ctx->comm_timer = NULL;
                LOG_INFO("free comm_timer, request:" << request_ctx->client_request);

            }
            store_entry->DelClient(store_client);
            LOG_INFO("reply client finished, delele client, now client_num:" << store_entry->GetClientNum()
                    << " store_status:" << store_entry->StatusStr()
                    << " request:" << store_client->request);
        }
    }

    LOG_INFO("unlock entry:" << store_entry);
    store_entry->Unlock();
}
