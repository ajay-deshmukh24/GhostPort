#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>


#define MAX_BYTES 4096    //max allowed size of request/response
#define MAX_CLIENTS 400     //max number of client requests served at a time
#define MAX_SIZE 200*(1<<20)     //size of the cache
#define MAX_ELEMENT_SIZE 10*(1<<20)     //max size of an element in cache

typedef struct cache_element cache_element;

// single element from cache
struct cache_element{
    char *data; //reponse data from server
    int len; //bytes of the data
    char *url; //url of request
    time_t lru_time_track; //time when element entered into cache
    cache_element *next; //pointer to next element
};


// function to find element from linked list with given url
cache_element* find(char *url);

// function to add new node to linked list
int add_cache_element(char *data,int size,char *url);

// function to remove least recently used element
void remove_cache_element();


int port_number = 8080;         //default port
int proxy_socketId;    // socket descriptor of proxy server
pthread_t tid[MAX_CLIENTS];  //array to store thread ids of clients

sem_t semaphore;  //if client requests exceeds the max_clients this semaphore puts waiting threads to sleep and wake them when traffic on queue decreases

// mutex is nothing but binary semaphore
pthread_mutex_t lock; // lock is used for locking the cache

cache_element* head;  //head pointer to cache
int cache_size;  // cache_size denotes current size of cache


int sendErrorMessage(int socket,int status_code){
    char str[1024];
    char currentTime[50];
    time_t now = time(0);

    struct tm data = *gmtime(&now);
   strftime(currentTime,sizeof(currentTime),"%a, %d %b %Y %H:%M:%S %Z", &data);

   switch(status_code)
	{
		case 400: snprintf(str, sizeof(str), "HTTP/1.1 400 Bad Request\r\nContent-Length: 95\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n<BODY><H1>400 Bad Rqeuest</H1>\n</BODY></HTML>", currentTime);
				  printf("400 Bad Request\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 403: snprintf(str, sizeof(str), "HTTP/1.1 403 Forbidden\r\nContent-Length: 112\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n<BODY><H1>403 Forbidden</H1><br>Permission Denied\n</BODY></HTML>", currentTime);
				  printf("403 Forbidden\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 404: snprintf(str, sizeof(str), "HTTP/1.1 404 Not Found\r\nContent-Length: 91\r\nContent-Type: text/html\r\nConnection: keep-alive\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n<BODY><H1>404 Not Found</H1>\n</BODY></HTML>", currentTime);
				  printf("404 Not Found\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 500: snprintf(str, sizeof(str), "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 115\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>500 Internal Server Error</TITLE></HEAD>\n<BODY><H1>500 Internal Server Error</H1>\n</BODY></HTML>", currentTime);
				  //printf("500 Internal Server Error\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 501: snprintf(str, sizeof(str), "HTTP/1.1 501 Not Implemented\r\nContent-Length: 103\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>404 Not Implemented</TITLE></HEAD>\n<BODY><H1>501 Not Implemented</H1>\n</BODY></HTML>", currentTime);
				  printf("501 Not Implemented\n");
				  send(socket, str, strlen(str), 0);
				  break;

		case 505: snprintf(str, sizeof(str), "HTTP/1.1 505 HTTP Version Not Supported\r\nContent-Length: 125\r\nConnection: keep-alive\r\nContent-Type: text/html\r\nDate: %s\r\nServer: VaibhavN/14785\r\n\r\n<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n<BODY><H1>505 HTTP Version Not Supported</H1>\n</BODY></HTML>", currentTime);
				  printf("505 HTTP Version Not Supported\n");
				  send(socket, str, strlen(str), 0);
				  break;

		default:  return -1;

	}
    return 1;
}

int connectRemoteServer(char* host_addr, int port_num) {
    int remoteSocket;
    struct addrinfo hints, *res, *p;
    char port_str[6]; // max length of port number string (65535 + \0)

    snprintf(port_str, sizeof(port_str), "%d", port_num);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;        // IPv4
    hints.ai_socktype = SOCK_STREAM;  // TCP

    int status = getaddrinfo(host_addr, port_str, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return -1;
    }

    for (p = res; p != NULL; p = p->ai_next) {
        remoteSocket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (remoteSocket == -1) continue;

        if (connect(remoteSocket, p->ai_addr, p->ai_addrlen) == -1) {
            close(remoteSocket);
            continue;
        }

        // successfully connected
        break;
    }

    freeaddrinfo(res); // clean up

    if (p == NULL) {
        fprintf(stderr, "Error: Failed to connect to remote server\n");
        return -1;
    }

    printf("remote server connected\n");
    return remoteSocket;
}



int handle_request(int clientSocket,struct ParsedRequest* request,char *tempReq){

    // we need to go through unparsing because raw request received from client i.e. stored in tempReq may have malformed or partial request. Headers like Host, connection, or User-Agent might be missing 

    // printf("In handel_request started unparsing request\n");
    char* buf = (char*) malloc(sizeof(char)*MAX_BYTES);
    strcpy(buf,"GET ");
    strcat(buf,request->path);
    strcat(buf," ");
    strcat(buf,request->version);
    strcat(buf,"\r\n");

    size_t len = strlen(buf);

    if(ParsedHeader_set(request,"Connection","close") < 0){
        printf("set header key not work\n");
    }

    if(ParsedHeader_get(request,"Host") == NULL){
        if(ParsedHeader_set(request,"Host",request->host)<0){
            printf("set Host header key not working\n");
        }
    }

    if(ParsedRequest_unparse_headers(request,buf+len,(size_t)MAX_BYTES-len)<0){
        printf("unparse failed\n");
        //if this happens still try to send request without headers
    }

    int server_port = 80; //default remote server port
    if(request->port != NULL){
        server_port = atoi(request->port);
    }

    // Establish a TCP connection to the actual destination server (like www.google.com on port 80).

    // printf("calling connectRemoteServer with host %s and port %d\n",request->host,server_port);
    int remoteSocketID = connectRemoteServer(request->host,server_port);
    // printf("back in handle request function\n");

    if(remoteSocketID < 0)return -1;

    // Send the HTTP request you constructed (buf) to the remote server.
    printf("constructed request to send to remote sever is \n%s\n",buf);
    int bytes_send = send(remoteSocketID,buf,strlen(buf),0);
    
    bzero(buf,MAX_BYTES);

    // now receive response in chunks
    bytes_send = recv(remoteSocketID,buf,MAX_BYTES-1,0);
    char* temp_buffer = (char*)malloc(sizeof(char)*MAX_BYTES); //temp buffer
    int temp_buffer_size = MAX_BYTES;
    int temp_buffer_index = 0;

    while(bytes_send > 0){

        // send what received to actual client
        bytes_send = send(clientSocket,buf,bytes_send,0);

        for(int i=0;i<bytes_send/sizeof(char);i++){
            temp_buffer[temp_buffer_index] = buf[i];
            // printf("%c",buf[i]); // Response Printing
			temp_buffer_index++;
        }
        temp_buffer_size += MAX_BYTES;
        temp_buffer = (char*)realloc(temp_buffer,temp_buffer_size);

        if(bytes_send < 0){
            perror("Error in sending data to client socket\n");
            break;
        }
        bzero(buf,MAX_BYTES);

        bytes_send = recv(remoteSocketID,buf,MAX_BYTES-1,0);
    }
    temp_buffer[temp_buffer_index]='\0';
	free(buf);
    
    // printf("received reponse from server now its time to add it in cache\n");
    add_cache_element(temp_buffer,strlen(temp_buffer),tempReq);
    printf("added to cache\n");
    printf("done\n");
    free(temp_buffer);

    close(remoteSocketID);
    return 0;

}


int checkHTTPversion(char* msg){
    int version = -1;

    if(strncmp(msg,"HTTP/1.1",8)==0)version=1;
    else if(strncmp(msg,"HTTP/1.0",8)==0)version=1;
    else version =-1;
    printf("checkHTTPversion returns %d\n",version);
    
    return version;
}


void* thread_fn(void* socketNew){
    // socketNew is pointer to socketId of client

    // printf("thread function started\n");
    
    // check if we have any thread available
    sem_wait(&semaphore);

    int p;
    sem_getvalue(&semaphore,&p);
    printf("semaphore value: %d\n",p);

    int* t = (int*)socketNew;
    int socket = *t; //socket is socket descriptor of connected client
    int bytes_send_client,len;    //Bytes Transfered

    char* buffer = (char*) calloc(MAX_BYTES,sizeof(char));  // creating buffer of 4kb for a client

    bzero(buffer,MAX_BYTES);
    bytes_send_client = recv(socket,buffer,MAX_BYTES,0); // receving the request of client by proxy server
    // recv() waits until data is available

    // recv() doesn't guarantee that you'll get all the data at once — it might come in chunks
    while(bytes_send_client > 0){
        len = strlen(buffer);
        //loop until u find "\r\n\r\n" in the buffer
        if(strstr(buffer,"\r\n\r\n")==NULL){
            bytes_send_client = recv(socket,buffer+len,MAX_BYTES-len,0);
        }
        else break;
    }

    char* tempReq = (char*) malloc(strlen(buffer)*sizeof(char)+1);
    // tempReq, buffer both store the http request sent by client
    for(int i=0;i<strlen(buffer);i++){
        tempReq[i] = buffer[i];
    }

    tempReq[strlen(buffer)] = '\0';  // Ensure null-terminated string

    // Print the copied HTTP request

    // printf("HTTP Request:\n%s\n", tempReq);

    // check for request in cache
    printf("now finding data in cache\n");
    struct cache_element* temp = find(tempReq);

    if(temp!=NULL){
        // request found in cache, so sending the response to client from proxy's cache
        printf("data found in the cache\n");
        int size = temp->len/sizeof(char);
        int pos = 0;
        char response[MAX_BYTES];
        while(pos<size){
            bzero(response,MAX_BYTES);
            for(int i=0;i<MAX_BYTES;i++){
                response[i] = temp->data[pos];
                pos++;
            }
            send(socket,response,MAX_BYTES,0);
        }
        printf("Data retrived from the cache\n");
        printf("%s\n\n",response);
    }
    else if(bytes_send_client > 0){
        // pending...
        printf("data not found in the cache\n");
        
        len = strlen(buffer);
        // parsing the request
        struct ParsedRequest* request = ParsedRequest_create();

        // ParsedRequest_parse returns 0 on success and -1 on failure. On success it stores parsed request in request 

        if(ParsedRequest_parse(request,buffer,len)<0){
            printf("Parsing failed\n");
        }
        else{

            // printf("Parsing succesfull\n");
            // printf("request method is %s\n",request->method);
            // printf("request protocol is %s\n",request->protocol);
            // printf("request host is %s\n",request->host);         
            // printf("request port is %s\n",request->port);         
            // printf("request path is %s\n",request->path);         
            // printf("request version is %s\n",request->version);         
            // printf("request is %s\n",request->buf);         

            bzero(buffer,MAX_BYTES);
            if(!strcmp(request->method,"GET")){
                if(request->host && request->path && (checkHTTPversion(request->version)==1)){

                    // printf("Now go for handeling the request\n");
                    bytes_send_client = handle_request(socket,request,tempReq);
                    if(bytes_send_client==-1){
                        sendErrorMessage(socket, 500);
                    }
                }
                else{
                    sendErrorMessage(socket, 500);
                }
            }
            else{
                printf("This code doesn't support any method other than GET\n");
            }
        }

        // freeing up the request pointer
        ParsedRequest_destroy(request);

    }
    else if(bytes_send_client < 0){
        perror("Error in receiving from client\n");
    }
    else if(bytes_send_client == 0){
        printf("client disconnected !\n");
    }

    shutdown(socket,SHUT_RDWR);
    close(socket);
    free(buffer);
    sem_post(&semaphore); // increase the value of semaphore

    sem_getvalue(&semaphore,&p);
    printf("Semaphore post value: %d\n",p);
    free(tempReq);
    // printf("thread function completed\n");
    return NULL;

}


int main(int argc,char* argv[]){

    // argc --> count of command line arguments
    // argv[] --> array of string holding arguments
    // ./proxy 8080 hello.txt

    struct sockaddr_in server_addr,client_addr; //address of client and server to be assigned

    int client_socketId; // stores socketId of client wants to connect
    socklen_t client_len = sizeof(client_addr); //store length of adress

    /*
        struct sockaddr_in {
            sa_family_t    sin_family;   // Address family (AF_INET for IPv4)
            in_port_t      sin_port;     // Port number (must be in network byte order)
            struct in_addr sin_addr;     // IP address (in struct form)
            char           sin_zero[8];  // Padding (not used, just to match struct size with sockaddr)
        };
    */

    sem_init(&semaphore,0,MAX_CLIENTS); //initialize semaphore
    // 0 --> minimum value
    // MAX_CLIENTS --> max value

    pthread_mutex_init(&lock,NULL); //intialize lock for cache
    // NULL --> beacause in c by default garbage values are there

    if(argc == 2){
        // ./proxy 8080 
        port_number = atoi(argv[1]); //convert string to int 
    }
    else{
        printf("Too few arguments\n");
        exit(1);
    }

    printf("Starting proxy server at port %d\n",port_number);

    
    // create the socket
    
    // open listening socket on primary side of proxy server
    proxy_socketId = socket(AF_INET,SOCK_STREAM,0);
    // creates TCP socket that uses IPv4
    // AF_INET --> address family --> Use IPv4
    // SCOKET_STREAM --> socket type --> TCP
    // 0 --> default protocol

    if(proxy_socketId < 0){
       perror("Failed to create a socket\n");
       exit(1);
    }

    int reuse = 1;
    // check if we can reuse same adress
    if (setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) 
    perror("setsockopt(SO_REUSEADDR) failed\n");

    bzero((char*)&server_addr,sizeof(server_addr));
    server_addr.sin_family = AF_INET;  // communication over an IPv4 network
    server_addr.sin_port = htons(port_number); //assign port to proxy
    server_addr.sin_addr.s_addr = INADDR_ANY; // any available address assigned
 

    // Binding the socket

    if(bind(proxy_socketId,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0){
        perror("Port is not free\n");
        exit(1);
    }
    printf("binding on port: %d\n",port_number);


    // proxy socket listening to the requests

    int listen_status = listen(proxy_socketId,MAX_CLIENTS);

    if(listen_status < 0){
        perror("Error while Listening !\n");
        exit(1);
    }


    int i=0; //Iterator for thread_id(tid) and Accept client_socket for each thread
    int Connected_socketId[MAX_CLIENTS]; //this array stores socket descriptors of connected clients

    // Infinite Loop for accepting clients
    while(1){
        bzero((char*)&client_addr,sizeof(client_addr));
        client_len = sizeof(client_addr);

        // Accepting the connections
        client_socketId = accept(proxy_socketId,(struct sockaddr*)&client_addr,(socklen_t*)&client_len);

        // NOTE : accept() waits forever if there is no client to connect currently and do not give error

        if(client_socketId < 0){
            fprintf(stderr,"Error in Accepting connection !\n");
            exit(1);
        }
        else{
            Connected_socketId[i] = client_socketId; //store accepted client into array
        }

        //Getting Ip adress and port number of client
        struct sockaddr_in* client_pt = (struct sockaddr_in*)& client_addr;

        /*
            struct in_addr {
                unsigned long s_addr;    // IP address in binary (network byte order)
            };
        */
        struct in_addr ip_addr = client_pt->sin_addr;
        
        // allocate character array with size sufficient to store any valid IPv4 address
        // INET_ADDRSTRLEN: Default ip address size
        char str[INET_ADDRSTRLEN];

        // convert binary form of IPv4 address into string
        inet_ntop(AF_INET,&ip_addr,str,INET_ADDRSTRLEN);
        printf("Client is connected with port number: %d and ip adress: %s\n",ntohs(client_addr.sin_port),str);

        // creating thread for each accepted client
        
        printf("assigning thread to the client\n");
        pthread_create(&tid[i],NULL,thread_fn,(void*)&Connected_socketId[i]); 
        i++;
    }

    close(proxy_socketId);
    return 0;
} 


cache_element* find(char* url){
    // Checks for url in the cache if found returns pointer to the respective cache element or else returns NULL
    cache_element* site = NULL;
    
    int temp_lock_val = pthread_mutex_lock(&lock);
    printf("find cache lock Acquired %d\n", temp_lock_val);

    site = head;
    while(site!=NULL){
        if(!strcmp(site->url,url)){
            printf("LRU Time Track Before: %ld\n",site->lru_time_track);
            printf("url found\n");
            // update the time_track
            site->lru_time_track = time(NULL);
            printf("LRU Time Track After:%ld\n",site->lru_time_track);
            break;
        }
        site=site->next;
    }
    
    if(!site){
        printf("url not found\n");
    }

    temp_lock_val = pthread_mutex_unlock(&lock);
    printf("find Cache Lock Unlocked %d\n",temp_lock_val); 
    return site;

}


void remove_cache_element(){
    //if cache is not empty searches for the node which has least lru_time_track and deletes it
    cache_element* p; //previous pointer
    cache_element* q; //next pointer;
    cache_element* temp; //element to remove

    int temp_lock_val = pthread_mutex_lock(&lock);
    printf("Remove Cache Lock Acquired %d\n",temp_lock_val); 

    if(head!=NULL){
        q=head;p=head;temp=head;
        while(q->next!=NULL){
            // iterate through entire cache and search for oldest time track
            if(((q->next)->lru_time_track) < (temp->lru_time_track)){
                temp=q->next;
                p=q;
            }

            q=q->next;
        }
        
        if(temp==head)head=head->next;
        else p->next = temp->next;

        // update cache size
        cache_size = cache_size - (temp->len)-sizeof(cache_element)-strlen(temp->url)-1;
        free(temp->data);
        free(temp->url);
        free(temp);
    }
    temp_lock_val = pthread_mutex_unlock(&lock);
	printf("Remove Cache Lock Unlocked %d\n",temp_lock_val);
}


int add_cache_element(char* data,int size,char* url){
    // add element to cache
    // make sure you lock it before use
    int temp_lock_val = pthread_mutex_lock(&lock);
    printf("Add cache lock acquired %d\n",temp_lock_val);
    int element_size = size+1+strlen(url)+sizeof(cache_element); //size of element to add
    if(element_size>MAX_ELEMENT_SIZE){
        // don't add to cache
        temp_lock_val = pthread_mutex_unlock(&lock);
        printf("Add Cache fail due to element exceeded max size, Lock Unlocked %d\n", temp_lock_val);
		// free(data);
		// printf("--\n");
		// free(url);
        return 0;
    }
    else{
        while(cache_size+element_size > MAX_SIZE){
            // We keep removing elements from cache until we get enough space to add the element
            remove_cache_element();
        }
        cache_element* element = (cache_element*)malloc(sizeof(cache_element)); 
        element->data = (char*)malloc(size+1);
        strcpy(element->data,data);
        element->url = (char*)malloc(1+strlen(url)*sizeof(char));
        strcpy(element->url,url);
        element->lru_time_track = time(NULL); //update time track
        element->next = head;
        head = element;
        element->len = size;
        cache_size+=element_size;
        temp_lock_val = pthread_mutex_unlock(&lock);
		printf("Add Cache successfull, Lock Unlocked %d\n", temp_lock_val);
        return 1;
    }
    return 0;

}