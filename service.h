#ifndef _SERVICE_H_
#define _SERVICE_H_

#include "event_includes.h"

class Service
{
public:
    Service(uint32_t port) : port_(port) {}
    bool Init();
    void Start();

    static event_base* get_base();
    static evdns_base* get_dnsbase();

private:
    uint32_t port_;

};

#endif // _SERVICE_H_
