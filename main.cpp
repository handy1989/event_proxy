#include "proxy_server.h"
#include "logger.h"

#include <stdio.h>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("usage: %s thread_num\n", argv[0]);
        return 1;
    }
    google::InitGoogleLogging(argv[0]);

    ProxyServer proxy_server(3333, 0, atoi(argv[1]));
    if (!proxy_server.Init()) 
    {
        return 1;
    }
    proxy_server.Start();
    proxy_server.Wait();
}