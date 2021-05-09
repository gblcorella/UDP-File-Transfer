/*
Computer Networks: Programming Assignment 2
Gabriel Corella_Xing Gao
*/    

#include <ctype.h>          
#include <stdio.h>         
#include <sys/socket.h>    
#include <stdlib.h>        
#include <netinet/in.h>    
#include <errno.h>         
#include <string.h>        
#include <unistd.h>        
#include <netdb.h>  
#include <time.h>          
#include "simulate.h"
#include <stdbool.h>

/* SERVER_PORT is the port number on which the server listens for
   incoming requests from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

// User able to pick a port: 49152 through 65535
#define PORT 49152
#define LOCALHOST "127.0.0.1"
struct UDP_DataPacket {
  unsigned short count;
  unsigned short seq;
  char data[80];
};

// Socke that we will use to communicate with the server
int clientSocket;  

// Initialize both, server and client IP Address Structures
struct sockaddr_in client_addr;  
struct sockaddr_in server_addr; 
struct hostent * server_hp;     

struct UDP_DataPacket tempPacket;
struct UDP_DataPacket serverPackets;
                              
// Recieve and Send Bytes
int sentBytes;
int recievedBytes; 

FILE *fp = NULL;

int main(int argc, char *argv[]) {

  srand(time(0)); 
  int recievedPackets = 0;
  int totalBytes = 0;
  int totalAcksTrans = 0;
  printf("***************************\n");
  printf("*   UDP Client Transfer   *\n");
  printf("***************************\n");
  printf("\n");

  printf("\n File Requested: %s", argv[1]);
  printf("\n ACK Loss Ratio: %s\n", argv[2]);

  
  unsigned int sequenceNum = 0;
  
  unsigned short client_port;  
  float userAckLoss;

  int failedAcks = 0;
  int totalAcks = 0;
  char filename[128];
  strcpy(filename, argv[1]);
  unsigned int buffLen = strlen(filename);
  

  // Command line error check
  if(argc < 3 || argc > 4 || (sscanf (argv[2], "%f", &userAckLoss)!=1)) { 
    perror("Error: Please check the README and provide the necessary arguments...\n");
    exit(1);
  }
  /* open a socket */
  if ((clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("Client: can't open datagram socket\n");
    exit(1);
  }

  client_port = 0; 

  // Server Address intialize 
  memset(&client_addr, 0, sizeof(client_addr));
  client_addr.sin_family = AF_INET;
  client_addr.sin_addr.s_addr = htonl(INADDR_ANY); /* This allows choice of any host interface, if more than one are present */
  client_addr.sin_port = htons(client_port);

  // Same Binding
  if(bind(clientSocket, (struct sockaddr *) &client_addr, sizeof (client_addr)) < 0) {
    perror("Error: Could not bind connection. Make clean, and restart connection.\n");
    close(clientSocket);
    exit(1);
  }
  if ((server_hp = gethostbyname(LOCALHOST)) == NULL) {
    perror("Client: invalid server hostname\n");
    close(clientSocket);
  }

  // Server Address initialize
  memset(&server_addr, 0, sizeof(server_addr));
  memcpy((char *)&server_addr.sin_addr, server_hp->h_addr, server_hp->h_length);
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);

  int dupPackets = 0;
  int nonDupPackets = 0;  
  tempPacket.count = htons(buffLen);
  tempPacket.seq = htons(sequenceNum);
  strncpy(tempPacket.data, filename, buffLen);

  sentBytes = sendto(clientSocket, &tempPacket, buffLen+4, 0, (struct sockaddr *) &server_addr, sizeof (server_addr));
  if(sentBytes < 0){
    perror("Filename Packet sending error");
    printf("** Ctrl + C and Start Over and Remake\n");
  }

  int transmissionPacket = 1;
  FILE *fp = fopen("out.txt", "wb");

  //Receive in a loop until end of transmission received
  while(transmissionPacket){
    recievedBytes = recvfrom(clientSocket, &serverPackets, sizeof(serverPackets), 0, (struct sockaddr *) 0, (socklen_t *) 0);
   if(recievedBytes < 0){
      perror("Data Packet receive error");
    }
    unsigned int count = ntohs(serverPackets.count);
    unsigned int seq = ntohs(serverPackets.seq);

    // If we are at the end of the file, reset to 0, otherwise we can increment recieved packets
    if(0 == count){ 
      printf("\n** End of Transmission Packet with sequence number %d received\n", seq);
      printf(" Not Included in Total Count \n");
      transmissionPacket = 0;
    }
    else {
      recievedPackets+=1;
      if(sequenceNum == seq){
        printf("Packet %d received with %d data bytes\n", seq, recievedBytes-4);
        nonDupPackets++;
        totalBytes += count;
        if(fp){
          fwrite(serverPackets.data,1,count, fp);
          printf("Packet %d Delivered\n", seq);
        }
        else{
          perror("Error: Could not write to out.txt file");
        }
        // Value should always be either 0 or 1
        sequenceNum = 1 - sequenceNum;
      }
      else{
        printf("Duplicate packet %d received with %d data bytes\n", seq, recievedBytes-4);
        dupPackets++;
      }
    
      printf("\nACK %d generated for transmission\n", seq);
      totalAcks++;
      if(!simulateACKLoss(userAckLoss)){
        unsigned short ack_seq = htons(seq);        
        sentBytes = sendto(clientSocket, &ack_seq, sizeof(ack_seq), 0, (struct sockaddr *) &server_addr, sizeof (server_addr));
        if(sentBytes == -1){
          perror("\n - ACK Packet Sending Error - \n");
          exit(1);
        }
        printf("ACK %d successfully transmitted\n", seq);
        totalAcksTrans++;
      }
      else{
        failedAcks++;
        printf("- ACK %d lost -\n", seq);
      }
    }
  }

  printf("|----------------- Client Statistics ------------------ \n");
  printf("| Total Number of Duplicate Data Packets Received:    | %d\n", dupPackets);
  printf("| Total Data Bytes Delivered:                         | %d\n", totalBytes);
  printf("| Total Number of Data Packets Received Successfully: | %d\n", recievedPackets);
  printf("| Successfully Transmitted ACK:                       | %d\n", totalAcksTrans);
  printf("| Successfully Recieved Packets without Duplicates:   | %d\n", nonDupPackets);
  printf("| ACKS dropped due to loss:                           | %d\n", failedAcks);
  printf("| Total Number of ACKs generated:                     | %d\n", totalAcks);
  printf("|-------------------------------------------------------|\n");
  
  // Remember to close socket and file we write to 
  if(fp){
    fclose(fp); 
  }
  close(clientSocket);
}
