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
#define MAX_CLIENTS 10
typedef struct cache_element cache_element

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

// function to remove least recent;y used element
void remove_cache_element();


int port_number = 8080;         //default port
int proxy_socketId;    // socket descriptor of proxy server
pthread_t tid[MAX_CLIENTS];  //array to store thread ids of clients

sem_t semaphore;  //if client requests exceeds the max_clients this semaphore puts waiting threads to sleep and wake them when traffic on queue decreases

// mutex is nothing but binary semaphore
pthread_mutex_t lock; // lock is used for locking the cache

cache_element* head;  //head pointer to cache
int cache_size;  // cache_size denotes current size of cache



void* thread_fn(void* socketNew){
    
    // check if we have any thread available
    sem_wait(&seamaphore);

    int p;
    sem_getvalue(&seamaphore,&p);
    printf("seamaphore value: %d\n",p);

    // socketNew is pointer to socketId of client
    int* t = (int*)(socketNew);
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

    // check for request in cache
    struct cache_element* temp = find(tempReq);

    if(temp!=NULL){
        // request found in cache, so sending the response to client from proxy's cache
        int size = temp->len/size(char);
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
    sem_post(&seamaphore); // increase the value of seamaphore

    sem_getvalue(&seamaphore,&p);
    printf("Seamaphore post value: %d\n",p);
    free(tempReq);
    return NULL;

}


int main(int argc,char* argv[]){

    // argc --> count of command line arguments
    // argv[] --> array of string holding arguments
    // ./proxy 8080 hello.txt

    int client_socketId; // stores socketId of client wants to connect
    int client_len; //store length of adress

    struct sockaddr_in server_addr,client_addr; //address of client and server to be assigned

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

    printf("Starting proxy server at port &d\n",port_number);

    
    // create the socket
    
    // open listening socket on primary side of proxy server
    proxy_socketId = socket(AF_INET,SOCKET_STREAM,0);
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
    int Connected_socketId[MAX_CLIENTS]; //this array stores sockket descriptors of connected clients

    // Infinite Loop for acceoting clients
    while(1){
        bzero((char*)&client_addr,sizeof(client_addr));

        // Accepting the connections
        client_socketId = accept(proxy_socketId,(struct socaddr*)&client_addr,(socklen_t*)&client_len);

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
        pthread_create(&tid[i],NULL,thread_fn,(void*)&Connected_socketId[i]); 
        i++;
    }

    close(proxy_socketId);
    return 0;
} 