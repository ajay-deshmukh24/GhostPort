#include <stdio.h>
#include <stdlib.h>


/*
   ParsedRequest objects are created from parsing a buffer containg HTTP request. The request buffer consists of a request line followed by a number of headers. Request line fields such as method, protocol etc. are stored explicitly. Headers such as 'Content-Length' and their values are maintained in a linked list. Each node in this list is a ParsedHeader and contains a key-value pair.

   The buf and buflen are used internally to maintain the parsed request line.
*/

struct ParsedRequest{
    char *method;
    char *protocol;
    char *host;
    char *port;
    char *path;
    char *version;
    char *buf;
    size_t buflen;
    struct ParsedHeader *headers;
    size_t headersused;
    size_t headerslen;
};

/*
    ParsedHeader: any header after the request line is a key-value pair with the format "key:value\r\n" and is maintained in the parsedHeader linked list within ParsedRequest
*/

struct ParsedHeader{
    char *key;
    size_t keylen;
    char *value;
    size_t valuelen;
}


/*
    Create an empty parsing object to be used exactly once for parsing a single request buffer
    i.e. ParsedRequest_create() function allocate space in memory for ParsedRequest struct and returns pointer to it
*/
struct ParsedRequest* ParsedRequest_create();


/*
    Parse the request buffer in buf given that buf is of length buflen
*/
int ParsedRequest_parse(struct ParsedRequest* parse,const char* buf,int buflen);