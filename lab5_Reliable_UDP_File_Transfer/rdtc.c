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

//Declare a Header structure that holds, sequence/ acknowledgement number, checksum, and length of a packet
typedef struct {
    int seq_ack;
    int len;
    int cksum;
} Header;

//Declare a packet structure that holds data and header
typedef struct {
    Header header;
    char data[10];
} Packet;

//Calculate the Checksum
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

//Print received packet
void printPacket(Packet packet) {
    printf("Packet{ header: { seq_ack: %d, len: %d, cksum: %d }, data: \"",
            packet.header.seq_ack,
            packet.header.len,
            packet.header.cksum);
    fwrite(packet.data, (size_t)packet.header.len, 1, stdout);
    printf("\" }\n");
}

//client sending packet with checksum and sequence number, waits for acknowledgement, and sets up a time
void clientSend(int sockfd, const struct sockaddr *address, socklen_t addrlen, Packet packet, unsigned retries) {
    while (1) {	
		//if retries is greater than 3, we give up and move on
        if(retries >= 3){
            return;
        }
        //calculate checksum of packet
        int checksum = getChecksum(packet);

        //Simulate loss of a packet
        if(rand() % 5 == 0) { //simulate a drop of packet (probability = 20%)
            printf("Dropping packet\n");
        }else {
            //send the packet
            printf("Client sending packet\n");
            sendto(sockfd, &packet, sizeof(packet), 0, address, addrlen);
        }
        
        // wait until an ACK is received or timeout using select statement      
        // local variables needed
        struct  timeval tv; // timer
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int rv;     // select returned value

        // set up reads file descriptor at the beginning of the function to be checked for being ready to read
        fd_set  readfds;
        fcntl (sockfd, F_SETFL, O_NONBLOCK);

        // start before calling select
        FD_ZERO (&readfds); //initializes readfds to have zero bits
        FD_SET (sockfd, &readfds); //sets readfds bit 

        rv = select(sockfd+1, &readfds, NULL, NULL, &tv);

        if(rv == 0){
            printf("Timeout, retry count %d \n", retries);
            //increment retries if packet is dropped
            retries++;
        }
        //so, there is an ACK to receive
        else{
            //receive an ACK from the server
            Packet recv_packet;
            char * recv_packet_char = malloc(sizeof(Packet));

            int nr = recvfrom(sockfd, recv_packet_char, sizeof(Packet), 0, address, &addrlen);
            memcpy(&recv_packet, recv_packet_char, sizeof(Packet));
            free(recv_packet_char);

            //print received packet (ACK) and checksum
            printf("Client received ACK %d, checksum %d - \n", recv_packet.header.seq_ack, recv_packet.header.cksum);

            //calculate checksum of received packet (ACK)
            int recv_checksum = getChecksum(recv_packet);
            //check the checksum
            if (recv_checksum != recv_packet.header.cksum) {
                printf("Client: Bad checksum, expected checksum was: %d\n", recv_packet.header.cksum);
                //if bad checksum, resend packet
                clientSend(sockfd, address, addrlen, packet, retries);
            } else if (recv_packet.header.seq_ack != packet.header.seq_ack){
                printf("Client: Bad seqnum, expected sequence number was: %d\n",packet.header.seq_ack);
                clientSend(sockfd, address, addrlen, packet, retries);
            } else {
                //if the ckechsum and sequence numbers are correct, break and return to the main to get the next packet to send
                //good ACK, we're done
                printf("Client: Good ACK\n");
                break;
            }
        }
        
    }
}

int main(int argc, char *argv[]) {
    //Get from the command line, server IP, Port and src file
    if (argc != 4) {
        printf("Usage: %s <ip> <port> <srcfile>\n", argv[0]);
        exit(0);
    }
    
    //Declare socket file descriptor.
    int sockfd; 

    //Open a UDP socket, if successful, returns a descriptor    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
        perror("Failure to setup a client socket");
        exit(1);
    }

    //Declare server address to connect to
    struct sockaddr_in servAddr;
    struct hostent *host;
    host = (struct hostent *) gethostbyname(argv[1]);

    ///Set the server address to send using socket addressing structure
    memset(&servAddr, 0, sizeof(servAddr));
    
    //initialize servAddr structure
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(atoi(argv[2]));
    servAddr.sin_addr = *((struct in_addr *)host->h_addr);

    //Open file using argv[3]
    int fp = open(argv[3], O_RDWR);
    if(fp < 0){
    	perror("Failed to open file\n");
	    exit(1);
    }

    //Send file contents to server packet by packet
    int seq = 0;
    socklen_t addr_len = sizeof(struct sockaddr);
    Packet packet;
    int bytes;
    while((bytes = read(fp, packet.data, sizeof(packet.data))) > 0){
    	//assign seq and checksum to packet and send
        packet.header.seq_ack=seq;
    	packet.header.len=bytes;
    	packet.header.cksum=getChecksum(packet);
    	clientSend(sockfd,(struct sockaddr *)&servAddr,addr_len, packet, 0); //retries = 0
        memset(packet.data, 0, sizeof(packet.data));
    	seq=(seq+1)%2;
    }

    //Send zero-length packet to server to end connection
    Packet final;
    final.header.seq_ack=seq;
    final.header.len=0;
    final.header.cksum=getChecksum(final);
    clientSend(sockfd,(struct sockaddr *)&servAddr,addr_len,final, 0);
    
	//Close file and socket
    close(fp);
    close(sockfd);
    return 0;
}
