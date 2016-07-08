#include "logger.h"
#include "global.h"
#include "http_request.h"
#include "client_side_callbacks.h"
#include "server_side_callbacks.h"
#include "event_includes.h"

#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>

#include <string>
#include <vector>

#include "service.h"
#include "logger.h"

using std::string;
using std::vector;

int main(int argc, char **argv)
{
    google::InitGoogleLogging(argv[0]);
    int port = 3333;

    if (argc > 1) 
    {
        port = atoi(argv[1]);
    }
    if (port<=0 || port>65535)
    {
        LOG_ERROR("Invalid port");
        return 1;
    }

    Service service(port);
    if (!service.Init())
    {
        return 1;
    }
    service.Start();

    return 0;
}
