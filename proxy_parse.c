#include "proxy_parse.h"

#define MAX_REQ_LEN 65535
#define MIN_REQ_LEN 4


struct ParsedRequest* ParsedRequest_create(){
    struct ParsedRequest *pr;
    pr = (struct ParsedRequest*) malloc(sizeof(struct ParsedRequest));

    if(pr!=NULL){
        parsedHeader_create(pr);
        pr->method = NULL;
        pr-> protocol = NULL;
        pr->host = NULL;
        pr->path = NULL;
        pr->version = NULL;
        pr->buf = NULL;
        pr->buflen = 0;
    }

    return pr;
}



/*
    Parse request buffer

    Parameters:
    parse: pointer to a newly created ParsedRequest object
    buf: pointer to buffer containing the request (need not be NULL terminated) and the trailing \r\n\r\n
    buflen: length of the buffer including the trailing \r\n\r\n

    Return values:
    -1: failure
    0: success
*/

int ParsedRequest_parse(struct ParsedRequest* parse,const char* buf,int buflen){
    char *full_addr;
    char *saveptr;
    char *index;
    char *currentHeader;

    if(parse->buf!=NULL){
        debug("parse object already assigned to a request\n");
        return -1;
    }

    if(buflen < MIN_REQ_LEN || buflen > MAX_REQ_LEN){
        debug("invalid buflen %d",buflen);
        return -1;
    }

    /*Create NULL terminated temprory buffer*/
    char *tmp_buf = (char *) malloc(buflen+1);/*Includes NULL character*/
    
    /*copy from buf to tmp_buf*/
    memcpy(tmp_buf,buf,buflen);
    tmp_buf[buflen] = '\0'; 

    /*finds first occurence of substring and returns pointer to it*/
    index = strstr(tmp_buf,"\r\n\r\n");
    if(index==NULL){
        debug("invalid request line, no end of header\n");
        free(tmp_buf);
        return -1;
    }

    /*Copy request line into parse->buf*/
    index = strstr(tmp_buf,"\r\n");
    if(parse->buf==NULL){
        parse->buf = (char*) malloc((index-tmp_buf)+1);
        parse->buflen = (index-tmp_buf)+1;
    }
    memcpy(parse->buf,tmp_buf,index-tmp_buf);
    parse->buf[index-tmp_buf]='\0';

    /*Parse request line*/
    /*strtok() function here splits string with first occurence of space*/
    parse->method = strtok(parse->buf," ",&saveptr);

    if(parse->method==NULL){
        debug("invalid request line, no whitespace\n");
        free(tmp_buf);
        free(parse->buf);
        parse->buf=NULL;
        return -1;
    }
    /*check if request method we have is GET*/
    if(strcmp(parse->method,"GET")){
        debug("invalid request line, method not 'GET': %s\n",parse->method);
        free(tmp_buf);
        free(parse->buf);
        parse->buf = NULL;
        return -1;
    }

    /*check for full adress of request*/
    full_addr = strtok_r(NULL," ",&saveptr);
    if(full_addr==NULL){
        debug("invalid request line, no full address\n");
        free(tmp_buf);
        free(parse->buf);
        parse->buf=NULL;
        return -1;
    }

    /*check for version*/
    parse->version = full_addr + strlen(full_adder)+1;

    if(parse->version==NULL){
        debug("invalid request line, missing version\n");
        free(tmp_buf);
        free(prase->buf);
        parse->buf = NULL;
        return -1;
    }
    if(strncmp(parse->version,"HTTP/",5)){
        debug("invalid request line, unspported version %s\n",parse->version);
        free(tmp_buf);
        free(parse->buf);
        parse->buf = NULL;
        return -1;
    }

    /*check for protocol*/
    parse->protocol = strtok(full_addr,"://",&saveptr);
    if(parse->protocol==NULL){
        debug("invalid request line, missing host\n");
        free(tmp_buf);
        free(parse->buf);
        parse->buf = NULL;
        return -1;
    }

    const char *rem = full_addr + strlen(parse->protocol) + strlen("://");
    size_t abs_uri_len = strlen(rem);

    /* check for host */
    parse->host = strtok_r(NULL,"/",&saveptr);
    if(parse->host == NULL){
        debug( "invalid request line, missing host\n");
	    free(tmp_buf);
	    free(parse->buf);
	    parse->buf = NULL;
	    return -1;
    }

    /* check for absolute path*/
    if(strlen(parse->host)==abs_uri_len){
        debug("invalid request line, missing absolute path\n");
        free(tmp_buf);
        free(parse->buf);
        parse->buf=NULL;
        return -1;
    }

    parse->path = strtok_r(NULL," ",&saveptr);
    if(parse->path == NULL){
        int rlen = strlen(root_abs_path);
    }

}