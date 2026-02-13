#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#define PLOSTMSG 5
typedef struct {
  int seq_ack;
  int len;
  int cksum;
} Header;
typedef struct {
   Header header;
  char data[10];
} Packet;

//getChecksum()
int getChecksum(Packet packet) {
    packet.header.cksum = 0;
    int checksum = 0;
    char *ptr = (char *)&packet;
    char *end = ptr + sizeof(Header) + packet.header.len;
    while (ptr < end) {
        checksum ^= *ptr++;
    }
    return checksum;
}

//print packet
void printPacket(Packet packet) {
    printf("Packet{ header: { seq_ack: %d, len: %d, cksum: %d }, data: \"",
            packet.header.seq_ack,
            packet.header.len,
            packet.header.cksum);
    fwrite(packet.data, (size_t)packet.header.len, 1, stdout);
    printf("\" }\n");
}

//serverSend()
void serverSend(int sockfd, const struct sockaddr *address, socklen_t addrlen, int seqnum) {
  // Simulating a chance that ACK gets lost
  if (rand() % PLOSTMSG == 0) {
     printf("Dropping ACK\n");
  }
  else{
    Packet packet;
    //prepare and send the ACK
    packet.header.seq_ack = seqnum;
    packet.header.len = 0;
    packet.header.cksum = getChecksum(packet);
    sendto(sockfd, &packet, sizeof(Packet), 0, address, addrlen);

    printf("Sent ACK %d, checksum %d\n", packet.header.seq_ack, packet.header.cksum);
  }
}

Packet serverReceive(int sockfd, socklen_t addrlen, int seqnum) {
    Packet packet;
    while (1) {
        //Receive a packet from the client
        char * recv_packet_char = malloc(sizeof(Packet));
        struct sockaddr_in clienAddr;
        int nr = recvfrom(sockfd, recv_packet_char, sizeof(Packet), 0, (struct sockaddr *) &clienAddr, &addrlen);
        memcpy(&packet, recv_packet_char, sizeof(Packet));
        free(recv_packet_char);
        // validate the length of the packet
        if (nr < 0) {
            continue;
        }
        // print what was received
        printf("Received: ");
        printPacket(packet);
        //verify the checksum and the sequence number
        if (packet.header.cksum != getChecksum(packet)) {
            printf("Bad checksum, expected %d\n", getChecksum(packet));
            serverSend(sockfd, &clienAddr, addrlen, (1+seqnum)%2);  //ACK last correctly recv packet seq number
        } else if (packet.header.seq_ack != seqnum) {
            printf("Bad seqnum, expected %d\n", seqnum);
            serverSend(sockfd, &clienAddr, addrlen, (1+seqnum)%2); 
        } else {
            printf("Good packet\n");
            serverSend(sockfd, &clienAddr, addrlen, seqnum);
            return packet;
        }
    }
}

int main(int argc, char *argv[]) {
    // check arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <outfile>\n", argv[0]);
        exit(1);
    }
    // seed the RNG
    srand((unsigned)time(NULL));
    int sockfd;
    // create a socket
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Failure to setup an endpoint socket");
        exit(1);
    }
  // initialize the server address structure
    struct sockaddr_in servAddr, clienAddr;
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(atoi(argv[1]));
    servAddr.sin_addr.s_addr = INADDR_ANY;
    // bind the socket
    if((bind(sockfd, (struct sockaddr *)&servAddr, sizeof(struct sockaddr))) < 0){
        perror("Failure to bind");
        exit(1);
    }
    // open file using argv[2]
    FILE * fp= fopen(argv[2], "w");
    if(fp<0){
        perror("file failed to open\n");
        exit(1);
    }

    // get file contents from client and save it to the file
    int seqnum = 0;
    Packet packet;
    do {
        packet = serverReceive(sockfd, sizeof(struct sockaddr), seqnum);
        if (packet.header.len == 0) {
            break;
        }
        printf("writing to file: %s\n", packet.data);
        fprintf(fp, "%s", packet.data);
        seqnum = (seqnum+1)%2;
    } while (1);

    fclose(fp);
    close(sockfd);
    return 0;
    }