#ifndef _HTTP_THREAD_H_
#define _HTTP_THREAD_H_

#include "event2/http.h"
#include "event2/http_struct.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/dns.h"

class HttpThread
{
public:
    HttpThread(const int sock);

    bool Init();
    void Start();
    void Stop();

private:
    int sock_;
    struct event_base* base_;
    struct evdns_base* dnsbase_;
    struct evhttp* http_server_;
};

#endif // _HTTP_THREAD_H_