#ifndef HTTP_SERVICE_H_
#define HTTP_SERVICE_H_

#include "event2/http.h"
#include "event2/http_struct.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/dns.h"
#include "event2/thread.h"

class HttpService
{
public:
    HttpService (const int sock);

    bool Init();
    void Start();
    void Stop();

    struct event_base* base() { return base_; }
    struct evdns_base* dnsbase() { return dnsbase_; }

private:

private:
    int sock_;
    struct event_base* base_;
    struct evdns_base* dnsbase_;
    struct evhttp* http_server_;
};




#endif // HTTP_SERVICE_H_
