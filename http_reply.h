#ifndef HTTP_REPLY_H_
#define HTTP_REPLY_H_

struct StoreClient;
class StoreEntry;
struct RequestCtx;

void ReplyClientHeader(StoreClient* client, StoreEntry* store_entry);
void ReplyClientBody(StoreClient* client, StoreEntry* store_entry);
void ReplyClient(RequestCtx* request_ctx);

#endif // HTTP_REPLY_H
