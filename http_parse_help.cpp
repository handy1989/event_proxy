#include "http_parse_help.h"
#include "logger.h"
#include "global.h"

#include <stdlib.h>
#include <string>

using std::string;

int32_t ParseHostPort(BufferContext* buffer_context)
{
    string& url = buffer_context->http_request.url;
    int32_t ret = 1;
    do
    {
        if (strncmp(url.c_str(), "http://", 7) != 0)
        {
            LOG_ERROR("http:// not in begining of url:" << url);
            break;
        }
        size_t host_begin_pos = 7;
        size_t host_end_pos = url.find_first_of('/', host_begin_pos);
        if (host_end_pos == string::npos)
        {
            LOG_ERROR("/ not found after host in url:" << url);
            break;
        }
        buffer_context->http_request.url_path = string(url, host_end_pos);
        LOG_DEBUG("url:" << url << " url_path:" << buffer_context->http_request.url_path);

        size_t colon_pos = url.find_first_of(':', host_begin_pos);
        if (colon_pos == string::npos || colon_pos > host_end_pos)
        {
            buffer_context->remote_host = string(url, host_begin_pos, host_end_pos - host_begin_pos);
            buffer_context->remote_port = 80;
        }
        else
        {
            buffer_context->remote_host = string(url, host_begin_pos, colon_pos - host_begin_pos);
            buffer_context->remote_port = atoi(string(url, colon_pos + 1, host_end_pos).c_str());
            if (buffer_context->remote_port == 0)
            {
                LOG_ERROR("parse port failed in url:" << url);
                break;
            }
        }

        ret = 0;
    } while (0);

    LOG_DEBUG("parse url:" << url
            << " host:" << buffer_context->remote_host
            << " port:" << buffer_context->remote_port);

    return ret;

}
