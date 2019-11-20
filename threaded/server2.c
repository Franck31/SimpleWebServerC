#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include "threadpool.h"

int tasks = 0, done = 0;
pthread_mutex_t lock;

long getimgleng(char* file)
{
    FILE * f = fopen(file, "r");
    fseek(f, 0, SEEK_END);
    long len = (unsigned long)ftell(f);
    fclose(f);
    return len;
}


void runnable(void *param) 
{
    int img,image_hlen;
    int new_socket = *((int *)param);
    long length,valread;
    char buffer[3000000];
    char * fbuffer = 0;
    char imageheader [20048];
    FILE *f;

    printf("\n Le hicieron un GET / \n");
    pthread_mutex_lock(&lock);
    recv(new_socket , buffer, 3000000 , 0);
 //  si queres mas data: printf("\%s\n",buffer );
   
    img = open("index.jpg", O_RDONLY); 
    //html asco para responder http
    image_hlen = snprintf(imageheader, sizeof(imageheader),"HTTP/1.1 200 OK\nContent-Type: image/jpeg\nContent-Length: %ld\n\n", getimgleng("index.jpg"));
    send(new_socket, imageheader, image_hlen, 0);
    sendfile(new_socket, img, NULL, getimgleng("index.jpg"));
    close(img);
    close(new_socket);
    printf("end task...\n");
    pthread_mutex_unlock(&lock);
};

int main() {
    
    int server_socket;
    int new_socket;
    int i = 0;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    pthread_mutex_init(&lock, NULL);

    struct addrinfo hints;
	struct addrinfo     *serverInfo;
    struct addrinfo     *serverAddrInfo = NULL;
    struct sockaddr_in  clientAddrInfo;
    socklen_t           clientAddrInfoLen = sizeof(clientAddrInfo);
    
	memset(&hints, 0, sizeof(hints));
	hints.ai_family 	= AF_UNSPEC;
	hints.ai_socktype 	= SOCK_STREAM;
	hints.ai_flags 		= AI_PASSIVE;
    //bnnd
	getaddrinfo(NULL, "5000", &hints, &serverInfo);
    
    // iterate over all serverinfo results
    for (struct addrinfo *currentAddr = serverInfo; currentAddr != NULL; currentAddr = currentAddr->ai_next) {
        
        if(currentAddr->ai_family == AF_INET) {
            
            serverAddrInfo = currentAddr;
        }
    }

    
    server_socket = socket(serverAddrInfo->ai_family,
                         serverAddrInfo->ai_socktype,
                         serverAddrInfo->ai_protocol);
	
    
    bind(server_socket, serverAddrInfo->ai_addr, serverAddrInfo->ai_addrlen);
    
    // change to passive mode
    listen(server_socket, 20);
    

    threadpool_t *pool = threadpool_create(2, 5, 0);
        printf("Pool started with 2 threads and queue size of 5\n");
    
    
    while(1)
    {
        printf("\nTr running");
        new_socket = accept(server_socket, (struct sockaddr *) &clientAddrInfo, &clientAddrInfoLen);
        
        if (threadpool_add(pool, &runnable,&new_socket, 0) == 0)
        {
            printf("\nOk.");
        }
        else
        {
            perror("\nError ");
        }
    }
    
    // destroy pool
    threadpool_destroy(pool, 0);
    
    return 0;
}
