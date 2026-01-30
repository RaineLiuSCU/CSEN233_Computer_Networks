#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#define SIZE 10
#define N 100
int threadCount = 0;
pthread_t clients[N];

int sockfd, connfd;

char sbuf[SIZE];

struct sockaddr_in servAddr, clienAddr;

void * connectionHandler(void* sock);

int main(int argc, char * argv[]){
    if (argc != 2){
        printf("Usage: %s <port> \n", argv[0]);
        exit(0);
    }
    int sin_size = sizeof(struct sockaddr_in);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Failed to setup a server socket");
        exit(1);
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(atoi(argv[1]));
    servAddr.sin_addr.s_addr = INADDR_ANY;

    if ((bind(sockfd, (struct sockaddr *)&servAddr, sizeof(struct sockaddr))) < 0){
        perror("Failed to bind server address to socket");
        exit(1);
    }

    printf("Server listening/waiting for client(s) at port %s\n", argv[1]);
    listen(sockfd, 5);
    while (1){
        if ((connfd = accept(sockfd, (struct sockaddr*)&clienAddr, (socklen_t*)&sin_size)) < 0){
            perror("Failed to accept connection to the client");
            exit(1);
        } else {
            if(pthread_create(&clients[threadCount], NULL, connectionHandler, (void*) &connfd) < 0){
                perror("Unable to create thread");
                exit(0);
            }
            
            threadCount++;

        }
        
    }
    for (int i = 0; i < threadCount; i++){
        pthread_join(clients[i], NULL);
    }
    close(sockfd);
    return 0;
}

void * connectionHandler(void* sock){
    char fnbuf[1024];
    int * connfdp = sock;
    int connfd1 = *connfdp;
    
    printf("Conection established with client: IP %s and Port %d\n",inet_ntoa(clienAddr.sin_addr),ntohs(clienAddr.sin_port));

    if (recv(connfd1, fnbuf, 1024,0) <= 0){
        perror("File name not received/ received empty");
        printf("%s", fnbuf);
        close(connfd1);
        exit(1);
    } else {
        printf("Opening file %s\n", fnbuf);
    }
    
    FILE * fp = fopen(fnbuf, "r");
    if (fp == NULL){
        perror("File not found");
        printf("File name: %s\n", fnbuf);
        close(connfd1);
        exit(0);
    }

    while(fgets(sbuf, SIZE, fp)!= NULL){
        if(send(connfd, sbuf, sizeof(sbuf),0) == -1){
            perror("Error in sending file");
            close(connfd);
            close(sockfd);
        }
        bzero(sbuf, SIZE);
    }
    fclose(fp);
    close(connfd);
    pthread_exit(0);
}