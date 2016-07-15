#include "proxy_server.h"
#include "logger.h"

#include <stdio.h>
#include <signal.h>

void Signal(int sig, void (*signal_handler)(int))
{
    struct sigaction act;
    act.sa_handler = signal_handler;
    sigaction(sig, &act, NULL);
}

void SignalHandler(int sig)
{
    switch (sig)
    {
        case SIGPIPE:
            LOG_WARNING("received SIGPIPE, ignore");
            break;
        case SIGTERM:
            LOG_WARNING("received SIGTERM");
        case SIGINT:
            LOG_WARNING("received SIGINT");
            break;

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
    //Signal(SIGINT, SignalHandler);

    ProxyServer proxy_server(3333, 0, atoi(argv[1]));
    if (!proxy_server.Init()) 
    {
        return 1;
    }
    proxy_server.Start();
    proxy_server.Wait();
}
