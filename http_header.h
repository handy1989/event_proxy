#ifndef _HTTP_HEADER_H_
#define _HTTP_HEADER_H_

class HttpHeader
{

public:
    HttpHeader();
    ~HttpHeader();
    /* Interface functions */
    void clean();
    void append(const HttpHeader * src);
    void update (HttpHeader const *fresh, HttpHeaderMask const *denied_mask);
    int parse(const char *header_start, const char *header_end);
    HttpHeaderEntry *getEntry(HttpHeaderPos * pos) const;
    HttpHeaderEntry *findEntry(http_hdr_type id) const;
    void addEntry(HttpHeaderEntry * e);
    void insertEntry(HttpHeaderEntry * e);
    String getByName(const char *name) const;
    String getByNameListMember(const char *name, const char *member, const char separator) const;
    String getListMember(http_hdr_type id, const char *member, const char separator) const;
    int has(http_hdr_type id) const;
    Vector<HttpHeaderEntry *> entries;		/* parsed fields in raw format */
    int len;			/* length when packed, not counting terminating '\0' */

private:
    // Make it non-copyable. Our destructor is a bit nasty...
    HttpHeader(const HttpHeader &);
    //assignment is used by the reset method, can't block it..
    //const HttpHeader operator=(const HttpHeader &);
};

#endif // _HTTP_HEADER_H_
