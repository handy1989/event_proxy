#ifndef _HTTP_HEADER_H_
#define _HTTP_HEADER_H_

#include <stdint.h>

#include <string>
#include <vector>


enum http_hdr_type
{
    HDR_BAD_HDR = -1,
    HDR_ACCEPT = 0,
    HDR_ACCEPT_CHARSET,
    HDR_ACCEPT_ENCODING,
    HDR_ACCEPT_LANGUAGE,
    HDR_ACCEPT_RANGES,
    HDR_AGE,
    HDR_ALLOW,
    HDR_AUTHORIZATION,
    HDR_CACHE_CONTROL,
    HDR_CONNECTION,
    HDR_CONTENT_BASE,
    HDR_CONTENT_DISPOSITION,
    HDR_CONTENT_ENCODING,
    HDR_CONTENT_LANGUAGE,
    HDR_CONTENT_LENGTH,
    HDR_CONTENT_LOCATION,
    HDR_CONTENT_MD5,
    HDR_CONTENT_RANGE,
    HDR_CONTENT_TYPE,
    HDR_COOKIE,
    HDR_DATE,
    HDR_ETAG,
    HDR_EXPIRES,
    HDR_FROM,
    HDR_HOST,
    HDR_IF_MATCH,
    HDR_IF_MODIFIED_SINCE,
    HDR_IF_NONE_MATCH,
    HDR_IF_RANGE,
    HDR_KEEP_ALIVE,
    HDR_LAST_MODIFIED,
    HDR_LINK,
    HDR_LOCATION,
    HDR_MAX_FORWARDS,
    HDR_MIME_VERSION,
    HDR_PRAGMA,
    HDR_PROXY_AUTHENTICATE,
    HDR_PROXY_AUTHENTICATION_INFO,
    HDR_PROXY_AUTHORIZATION,
    HDR_PROXY_CONNECTION,
    HDR_PUBLIC,
    HDR_RANGE,
    HDR_REQUEST_RANGE,		/* some clients use this, sigh */
    HDR_REFERER,
    HDR_RETRY_AFTER,
    HDR_SERVER,
    HDR_SET_COOKIE,
    HDR_TE,
    HDR_TITLE,
    HDR_TRAILERS,
    HDR_TRANSFER_ENCODING,
    HDR_TRANSLATE,             /* IIS custom header we may need to cut off */
    HDR_UNLESS_MODIFIED_SINCE,             /* IIS custom header we may need to cut off */
    HDR_UPGRADE,
    HDR_USER_AGENT,
    HDR_VARY,
    HDR_VIA,
    HDR_WARNING,
    HDR_WWW_AUTHENTICATE,
    HDR_AUTHENTICATION_INFO,
    HDR_X_CACHE,
    HDR_X_CACHE_LOOKUP,		/* tmp hack, remove later */
    HDR_X_FORWARDED_FOR,
    HDR_X_REQUEST_URI,		/* appended if ADD_X_REQUEST_URI is #defined */
    HDR_X_SQUID_ERROR,
    HDR_NEGOTIATE,
    HDR_SURROGATE_CAPABILITY,
    HDR_SURROGATE_CONTROL,
    HDR_FRONT_END_HTTPS,
    HDR_OTHER,
    HDR_ENUM_END
};

enum field_type
{
    ftInvalid = HDR_ENUM_END,	/* to catch nasty errors with hdr_id<->fld_type clashes */
    ftInt,
    ftInt64,
    ftStr,
    ftDate_1123,
    ftETag,
    ftPCc,
    ftPContRange,
    ftPRange,
    ftPSc,
    ftDate_1123_or_ETag
};

struct HttpHeaderFieldAttrs
{
    const char *name;
    http_hdr_type id;
    field_type type;
};

struct HttpHeaderEntry
{
    http_hdr_type id;
    std::string name;
    std::string value;
};

class HttpHeader
{

public:
    HttpHeader() : content_length_(0) {}
    ~HttpHeader() {}
    int32_t Parse(char* line);

    std::vector<HttpHeaderEntry> entries;

    int32_t content_length_;
};

#endif // _HTTP_HEADER_H_
