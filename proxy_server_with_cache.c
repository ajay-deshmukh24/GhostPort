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




int main(int argc,char* argv[]){

    // argc --> count of command line arguments
    // argv[] --> array of string holding arguments
    // ./proxy 8080 hello.txt

    int client_socketId; // stores socketId of client wants to connect
    int client_len; //store length of adress
    struct sockaddr_in server_addr,client_addr; //address of client and server to be assigned

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
    // check if we can reuse same port
    if (setsockopt(proxy_socketId, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)) < 0) 
    perror("setsockopt(SO_REUSEADDR) failed\n");

}