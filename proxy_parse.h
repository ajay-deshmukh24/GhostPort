#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>


#ifndef PROXY_PARSE
#define PROXY_PARSE


#define DEBUG 1

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
};


/*
    Create an empty parsing object to be used exactly once for parsing a single request buffer
    i.e. ParsedRequest_create() function allocate space in memory for ParsedRequest struct and returns pointer to it
*/
struct ParsedRequest* ParsedRequest_create();


/*
    Parse the request buffer in buf given that buf is of length buflen
*/
int ParsedRequest_parse(struct ParsedRequest* parse,const char* buf,int buflen);


/* Destroy the parsing object */
void ParsedRequest_destroy(struct ParsedRequest *pr);


/*
   Retrieve the entire buffer from a parsed request object. buf must be an allocated buffer of size buflen, with enough space to write the request line, headers and the trailing \r\n. buf will not be NULL terminated by unparse().
*/
int ParsedRequest_unparse(struct ParsedRequest *pr,char *buf,size_t buflen);


/*
    Retrieve the entire buffer with the exception of request line from a parsed request object. buf must be an allocated buffer of size buflen, with enough space to write the headers and the trailing \r\n. buf will not be NULL terminated by unparse(). if there are no headers, the trailing \r\n is unparsed.
*/
int ParsedRequest_unparse_headers(struct ParsedRequest *pr,char *buf,size_t buflen);


/* Total length including request line, headers and the trailing \r\n */
size_t ParsedRequest_totalLen(struct ParsedRequest *pr);


/*
    Length including headers, if any, and the trailing \r\n but excluding the request line
*/
size_t ParsedHeader_headersLen(struct ParsedRequest *pr);


/* Set, get and remove null-terminated header keys and values */
int ParsedHeader_set(struct ParsedRequest *pr, const char * key, 
    const char * value);
struct ParsedHeader* ParsedHeader_get(struct ParsedRequest *pr, 
            const char * key);
int ParsedHeader_remove (struct ParsedRequest *pr, const char * key);


/* debug() prints out debugging info if DEBUG is set to 1 */
void debug(const char * format, ...);

/* Example usage:

   const char *c = 
   "GET http://www.google.com:80/index.html/ HTTP/1.0\r\nContent-Length:"
   " 80\r\nIf-Modified-Since: Sat, 29 Oct 1994 19:43:31 GMT\r\n\r\n";
   
   int len = strlen(c); 
   //Create a ParsedRequest to use. This ParsedRequest
   //is dynamically allocated.
   ParsedRequest *req = ParsedRequest_create();
   if (ParsedRequest_parse(req, c, len) < 0) {
       printf("parse failed\n");
       return -1;
   }

   printf("Method:%s\n", req->method);
   printf("Host:%s\n", req->host);

   // Turn ParsedRequest into a string. 
   // Friendly reminder: Be sure that you need to
   // dynamically allocate string and if you
   // do, remember to free it when you are done.
   // (Dynamic allocation wasn't necessary here,
   // but it was used as an example.)
   int rlen = ParsedRequest_totalLen(req);
   char *b = (char *)malloc(rlen+1);
   if (ParsedRequest_unparse(req, b, rlen) < 0) {
      printf("unparse failed\n");
      return -1;
   }
   b[rlen]='\0';
   // print out b for text request 
   free(b);
  

   // Turn the headers from the request into a string.
   rlen = ParsedHeader_headersLen(req);
   char buf[rlen+1];
   if (ParsedRequest_unparse_headers(req, buf, rlen) < 0) {
      printf("unparse failed\n");
      return -1;
   }
   buf[rlen] ='\0';
   //print out buf for text headers only 

   // Get a specific header (key) from the headers. A key is a header field 
   // such as "If-Modified-Since" which is followed by ":"
   struct ParsedHeader *r = ParsedHeader_get(req, "If-Modified-Since");
   printf("Modified value: %s\n", r->value);
   
   // Remove a specific header by name. In this case remove
   // the "If-Modified-Since" header. 
   if (ParsedHeader_remove(req, "If-Modified-Since") < 0){
      printf("remove header key not work\n");
     return -1;
   }

   // Set a specific header (key) to a value. In this case,
   //we set the "Last-Modified" key to be set to have as 
   //value  a date in February 2014 
   
    if (ParsedHeader_set(req, "Last-Modified", " Wed, 12 Feb 2014 12:43:31 GMT") < 0){
     printf("set header key not work\n");
     return -1;

    }

   // Check the modified Header key value pair
    r = ParsedHeader_get(req, "Last-Modified");
    printf("Last-Modified value: %s\n", r->value);

   // Call destroy on any ParsedRequests that you
   // create once you are done using them. This will
   // free memory dynamically allocated by the proxy_parse library. 
   ParsedRequest_destroy(req);
*/




// logs-----------------------


// root@Ajay:/GhostPort# ./proxy 8080
// Starting proxy server at port 8080
// binding on port: 8080
// Client is connected with port number: 0 and ip adress: 0.0.0.0
// assigning thread to the client
// thread function started
// semaphore value: 399
// HTTP Request:
// GET http://172.26.255.52:8080/https://www.cs.princeton.edu/ HTTP/1.1
// Host: 172.26.255.52:8080
// Proxy-Connection: keep-alive
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7
// Accept-Encoding: gzip, deflate
// Accept-Language: en-US,en;q=0.9


// now finding data in cache
// Remove cache lock Acquired 0
// url not found
// Remove Cache Lock Unlocked 0
// data not found in the cache
// Parsing succesfull
// request method is GET
// request protocol is http
// request host is 172.26.255.52
// request port is 8080
// request path is /https://www.cs.princeton.edu/
// request version is HTTP/1.1
// request is GET
// checkHTTPversion returns 1
// Now go for handeling the request
// In handel_request started unparsing request
// calling connectRemoteServer with host 172.26.255.52 and port 8080
// inside connectRemoteServer
// host returned from gethostbyname function 172.26.255.52      
// remote server connected
// Client is connected with port number: 36856 and ip adress: 172.26.255.52
// assigning thread to the client
// thread function started
// semaphore value: 398
// HTTP Request:
// GET /https://www.cs.princeton.edu/ HTTP/1.1
// Host: 172.26.255.52:8080
// Proxy-Connection: keep-alive
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36
// Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7
// Accept-Encoding: gzip, deflate
// Accept-Language: en-US,en;q=0.9
// Connection: close


// now finding data in cache
// Remove cache lock Acquired 0
// url not found
// Remove Cache Lock Unlocked 0
// data not found in the cache
// Parsing succesfull
// request method is GET
// request protocol is https
// request host is www.cs.princeton.edu
// request port is (null)
// request path is /
// request version is HTTP/1.1
// request is GET
// checkHTTPversion returns 1
// Now go for handeling the request
// In handel_request started unparsing request
// calling connectRemoteServer with host www.cs.princeton.edu and port 80
// inside connectRemoteServer
// host returned from gethostbyname function wwwprx.cs.princeton.edu
// remote server connected
// Add cache lock acquired 0
// Add Cache Lock Unlocked 0
// done
// Semaphore post value: 399
// thread function completed
// Add cache lock acquired 0
// Add Cache Lock Unlocked 0
// done
// Semaphore post value: 400
// thread function completed
// Client is connected with port number: 53159 and ip adress: 172.26.240.1
// assigning thread to the client
// thread function started
// semaphore value: 399
// HTTP Request:
// GET http://ipv6.msftconnecttest.com/connecttest.txt HTTP/1.1 
// Connection: Close
// User-Agent: Microsoft NCSI
// Host: ipv6.msftconnecttest.com


// now finding data in cache
// Remove cache lock Acquired 0
// Remove Cache Lock Unlocked 0
// data not found in the cache
// Parsing succesfull
// request method is GET
// request protocol is http
// request host is ipv6.msftconnecttest.com
// request port is (null)
// request path is /connecttest.txt
// request version is HTTP/1.1
// request is GET
// checkHTTPversion returns 1
// Now go for handeling the request
// In handel_request started unparsing request
// Remove Cache Lock Unlocked 0
// data not found in the cache
// Parsing succesfull
// request method is GET
// request protocol is http
// request host is ipv6.msftconnecttest.com
// request port is (null)
// request path is /connecttest.txt
// request version is HTTP/1.1
// request is GET
// checkHTTPversion returns 1
// Now go for handeling the request
// In handel_request started unparsing request
// calling connectRemoteServer with host ipv6.msftconnecttest.com and port 80
// inside connectRemoteServer
// Segmentation fault (core dumped)
// Parsing succesfull
// request method is GET
// request protocol is http
// request host is ipv6.msftconnecttest.com
// request port is (null)
// request path is /connecttest.txt
// request version is HTTP/1.1
// request is GET
// checkHTTPversion returns 1
// Now go for handeling the request
// In handel_request started unparsing request
// request host is ipv6.msftconnecttest.com
// request port is (null)
// request path is /connecttest.txt
// request version is HTTP/1.1
// request is GET
// checkHTTPversion returns 1
// Now go for handeling the request
// In handel_request started unparsing request
// request path is /connecttest.txt
// request version is HTTP/1.1
// request is GET
// checkHTTPversion returns 1
// Now go for handeling the request
// In handel_request started unparsing request
// request is GET
// checkHTTPversion returns 1
// Now go for handeling the request
// In handel_request started unparsing request
// Now go for handeling the request
// In handel_request started unparsing request
// calling connectRemoteServer with host ipv6.msftconnecttest.com and port 80  
// inside connectRemoteServer
// Segmentation fault (core dumped)



#endif