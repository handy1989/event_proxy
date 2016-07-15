#include "http_callbacks.h"
#include "logger.h"

#include <stdlib.h>

void HttpGenericCallback(struct evhttp_request* req, void* arg)
{
    CallbackPara* callback_para = (CallbackPara*)arg;
    callback_para->client_request = req;

    const struct evhttp_uri* evhttp_uri = evhttp_request_get_evhttp_uri(req);
    char* url = (char*)malloc(1024);
    LOG_INFO("uri_join:" << evhttp_uri_join(const_cast<struct evhttp_uri*>(evhttp_uri), url, 1024));
    free(url);

    evbuffer* buf = evbuffer_new();
    evbuffer_add_printf(buf, "hello world, proxy test");

    evhttp_send_reply(req, 200, "OK", buf);

    evbuffer_free(buf);
}