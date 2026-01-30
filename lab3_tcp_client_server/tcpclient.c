#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#define SIZE 10

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: %s <ip of server> <port #> <src_file> <dst_file>\n", argv[0]);
        exit(0);
    }
    int sockfd;
    char rbuf[SIZE], sbuf[SIZE];

    struct sockaddr_in servAddr;
    struct hostent * host;
    host = (struct hostent *) gethostbyname(argv[1]);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Failed to set up client socket");
        exit(0); 
    }

    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(atoi(argv[2]));
    servAddr.sin_addr = *((struct in_addr *)host->h_addr);

    if (connect(sockfd, (struct sockaddr *) &servAddr, sizeof(struct sockaddr))){
        perror("Failed to connect client to server");
        exit(1);
    }

    FILE * fp = fopen(argv[3], "r");
    if (fp == NULL){
        perror("src_file unable to be opened"); 
    }

    int sb;
    if ((sb = send(sockfd, argv[3], strlen(argv[3]),0)) == -1){
        perror("Error in sending file name to server");
        exit(1);
    }

    FILE * fp2 = fopen(argv[4], "w");
    if (fp2 == NULL){
        perror("dst_file unable to be opened"); 
    }

    while ((recv(sockfd, rbuf, SIZE,0)) > 0){
        fprintf(fp2, "%s", rbuf);
        bzero(rbuf, SIZE);
    }

    printf("File transfer from server complete\n");
    close(sockfd);
    fclose(fp2);
    return 0;
}
