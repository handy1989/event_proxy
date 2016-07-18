#include "proxy_server.h"
#include "logger.h"

#include "boost/weak_ptr.hpp"

#include <stdio.h>
#include <signal.h>

boost::weak_ptr<ProxyServer> g_proxy_service;


void Signal(int sig, void (*signal_handler)(int))
{
    struct sigaction act;
    act.sa_handler = signal_handler;
    sigaction(sig, &act, NULL);
}

void SignalHandler(int sig)
{
    boost::shared_ptr<ProxyServer> proxy_server;
    bool stop = false;
    switch (sig)
    {
        case SIGPIPE:
            LOG_WARNING("received SIGPIPE, ignore");
            break;
        case SIGTERM:
            LOG_WARNING("received SIGTERM");
            stop = true;
            break;
        case SIGINT:
            LOG_WARNING("received SIGINT");
            stop = true;
            break;

    }
    if (stop)
    {
        proxy_server = g_proxy_service.lock();
        if (proxy_server)
        {
            LOG_INFO("stop service");
            proxy_server->Stop();
        }
    }
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("usage: %s thread_num\n", argv[0]);
        return 1;
    }
    google::InitGoogleLogging(argv[0]);

    Signal(SIGPIPE, SignalHandler);
    Signal(SIGTERM, SignalHandler);
    Signal(SIGINT, SignalHandler);
    
    boost::shared_ptr<ProxyServer> proxy_server(new ProxyServer(3333, 0, atoi(argv[1])));
    g_proxy_service = proxy_server;

    if (!proxy_server->Init()) 
    {
        return 1;
    }
    proxy_server->Start();
    proxy_server->Wait();
}
