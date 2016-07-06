#include "utility.h"

#include <string.h>

char* FindFirstNotOf(char* buf, const char* pattern)
{
    if (buf == NULL || pattern == NULL) return NULL;
    char* p = buf;
    for (; *p; ++p)
    {
        if (strchr(pattern, *p) == NULL) 
            break;
    }
    if (*p == 0) return NULL;
    return p;
}

char* FindFirstOf(char* buf, const char* pattern)
{
    if (buf == NULL || pattern == NULL) return NULL;
    char* p = buf;
    for (; *p; ++p)
    {
        if (strchr(pattern, *p) != NULL)
            break;
    }
    if (*p == 0) return NULL;
    return p;
}
