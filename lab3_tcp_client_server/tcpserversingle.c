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

int sockfd, connfd;

char rbuf[SIZE], sbuf[SIZE];

void * connectionHandler(void* sock);

int main(int argc, char * argv[]){
    if (argc != 2){
        printf("Usage: %s <port> \n", argv[0]);
        exit(0);
    }
    struct sockaddr_in servAddr, clienAddr;
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
    listen(sockfd, 1);
    if ((connfd = accept(sockfd, (struct sockaddr*)&clienAddr, (socklen_t*)&sin_size)) < 0){
        perror("Failed to accept connection to the client");
        close(sockfd);
        exit(1);
    }
    char fnbuf[1024];
    int n = recv(connfd, fnbuf, 1024, 0);
    if (n <= 0) {
        perror("Filename not received");
        close(connfd);
        close(sockfd);
        exit(1);
    }

    FILE * fp = fopen(fnbuf, "r");
    if (fp == NULL){
        perror("File not found");
        printf("File name: %s", fnbuf);
        close(connfd);
        close(sockfd);
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
    close(sockfd);
    return 0;
}
