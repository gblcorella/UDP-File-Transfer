
#include <ctype.h>          
#include <stdio.h>         
#include <sys/socket.h>    
#include <stdlib.h>        
#include <netinet/in.h>    
#include <string.h>        
#include <unistd.h>        
#include <errno.h> 
#include <netdb.h>  
#include "simulate.h"
#include <stdbool.h>
#include <time.h>           
#include <math.h>   // Need to convert from microseconds to seconds         

#define SIZE 80

/* SERVER_PORT is the port number on which the server listens for
   incoming requests from clients. You should change this to a different
   number to prevent conflicts with others in the class. */

// User able to pick a port: 49152 through 65535
#define SERVER_PORT 49152
#define LOCALHOST "127.0.0.1"

FILE *fp = NULL;

struct UDP_DataPacket {
  unsigned short count;
  unsigned short seq;
  char data[SIZE];
};
struct timeval timestamp;

// Socket id, listens to client 
int socketServer;  

// Client and Server structure to store address information
struct sockaddr_in server_addr;  
struct sockaddr_in client_addr; 
struct UDP_DataPacket transmissionPacket;

// Port and Sequence number, seqNo will begin at 0;
unsigned short server_port; 
unsigned short seqNo;
unsigned int client_addr_len;  /* Length of client address structure */

// Recieve and Send Bytes
int sentBytes;
int recievedBytes; 

int main(int argc, char *argv[]) {
  // Init time
  srand(time(0)); 

  printf("***************************\n");
  printf("*   UDP Server Transfer   *\n");
  printf("***************************\n");
  printf("\n");

  printf("Timeout stored as: 10^%s\n", argv[1]);
  printf("Packet Loss Ratio: %s\n", argv[2]);

  int userTimeout;
  int timeoutPackets, successfulPackets, totalAckPackets, failedPackets, totalBytes, totalPackets, transmittedPackets = 0;
  float userLossRatio;
  struct UDP_DataPacket clientUDPacket;


  printf("Listening on port: %d\n\n", SERVER_PORT);
  client_addr_len = sizeof (client_addr);

  seqNo = 0;

  if(sscanf(argv[1], "%d", &userTimeout) != 1 || sscanf (argv[2], "%f", &userLossRatio)!=1 || argc < 3 ){ //print help text
  perror("Error: Please check the README and provide the necessary arguments...\n");
  exit(1);
  }
  
  // open socket
  // TCP almost always uses SOCK_STREAM and UDP uses SOCK_DGRAM. Change from first assignment
  if ((socketServer = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
    perror("Server: can't open stream socket\n");
  }

  // Server Address intialize 
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl (INADDR_ANY);  
  server_addr.sin_port = htons(SERVER_PORT);

  // Timeout  
  // 10^n, where n is the timeout integer inputted by the user
  double micros = pow(10, userTimeout);
  int millis = micros / 1000;
  timestamp.tv_sec = millis/1000;
  timestamp.tv_usec = millis%1000;

  // Same Binding
  if (bind(socketServer, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0) {
    perror("Error: Could not bind connection. Make clean, and restart connection.\n");
    close(socketServer);
    exit(1);
  }
  recievedBytes = recvfrom(socketServer, &clientUDPacket, sizeof(clientUDPacket), 0, (struct sockaddr *) &client_addr, &client_addr_len);
  //make sure receive was good
  if(recievedBytes < 0){
    perror("Filename data receive error");
    exit(1);
  }

  //copy filename string out of received packet
  unsigned int filename_len = ntohs(clientUDPacket.count);
  char filename[filename_len];
  
  
  setsockopt(socketServer, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timestamp, sizeof(timestamp));
 
  // Uncomment for testing
  //printf("Timeout is %ld seconds and %d us\n", timestamp.tv_sec,timestamp.tv_usec);
  strncpy(filename, clientUDPacket.data, filename_len); 
  filename[filename_len] = '\0'; 
  fp = fopen(filename, "r");


  if(fp != NULL){
    printf("Transferring UDP Packets from file: %s\n\n", filename);
    char *buffer = calloc(1,SIZE);

    while(fgets(buffer,SIZE,fp) != NULL){
      int bufferSize = strlen(buffer);
      int ackPacket = 0;
      
      struct UDP_DataPacket tempPacket;
      
      
      tempPacket.count = htons(bufferSize);
      tempPacket.seq = htons(seqNo);
      strncpy(tempPacket.data, buffer, bufferSize);

      printf("Packet %d generated for transmission with %d data bytes\n", seqNo, bufferSize);
      totalPackets++;
      totalBytes += bufferSize;

      while(!ackPacket){
        transmittedPackets++;
        if(!simulateLoss(userLossRatio)){
          //send data packet
          
          sentBytes = sendto(socketServer, &tempPacket, bufferSize+4, 0, (struct sockaddr*) &client_addr, client_addr_len);
          if(sentBytes < 0){
            perror("Data Packet sending error");
            exit(1);
          }
          //print stats
          successfulPackets++;
          printf("\nPacket %d successfully transmitted with %d data bytes\n", seqNo, sentBytes-4);
        }
        else{
          failedPackets++;
          printf("\n****************************\n");
          printf("* Packet %d lost            *\n", seqNo);
          printf("* Total Failed Packets: %d *\n", failedPackets);
          printf("*****************************\n");
          
        }

        //clear buffer
        memset(buffer, 0, SIZE);
        unsigned short recv_ack;
        recievedBytes = recvfrom(socketServer, &recv_ack, sizeof(recv_ack), 0, (struct sockaddr *) 0, (socklen_t *) 0);
        //wait for ACK packet
        
        if(recievedBytes < 0){
          if( errno == EAGAIN || errno == EWOULDBLOCK ){
            timeoutPackets++;
            
            printf("\n");
            printf("* Timeout expired for packet: %d                             \n", seqNo);
            printf("* Packet %d generated for re-transmission with %d data bytes \n", seqNo, bufferSize);
            printf("\n");
            
          }
          else{
            perror("\n - ACK Packet receive error - \n");
            exit(1);
          }
        }
        else{
          //Successfully received ACK
          recv_ack = ntohs(recv_ack);
          // TODO If match, print successful recieve
          if( recv_ack == seqNo ){ 
            printf("\n - ACK %d received -\n", recv_ack);
            // Set the sequenceNumber back to 0
            seqNo = 1 - seqNo;
            totalAckPackets += 1;
            ackPacket = 1;
          }  
        } 
      }
    }
    //free memory to cleanup
    free(buffer);
    fclose(fp);
  }
  else{
      printf("ERROR: \n");
      printf("File name not found... please make sure file is in local directory \n");
      printf("Make clean and try again...");
      exit(1);
  }

  //Send end of transmission packet
  transmissionPacket.seq = htons(seqNo);
  transmissionPacket.count = htons(0);
  

  sentBytes = sendto(socketServer, &transmissionPacket, 4, 0, (struct sockaddr*) &client_addr, client_addr_len);
  if(sentBytes < 0){
    perror("End of Transmission Error: Failed to Send Packet\n");
    exit(1);
  }

  //print stats
  printf("\nEnd of transmission packet, sequence number %d transmitted\n", seqNo);

  
  printf("\n");
  printf("|----------------- Server Statistics ---------------- \n");
  printf("| Total Number of Timeouts:                        | %d\n", timeoutPackets);
  printf("| Data Packets Dropped due to Loss:                | %d\n", failedPackets);
  printf("| Total Bytes Generated for Retransmission:        | %d\n", totalBytes);
  printf("| Total ACK Packets Recieved:                      | %d\n", totalAckPackets);
  printf("| Number of Successfully Transmitted Data Packets: | %d\n", successfulPackets);
  printf("| Total Number of Retransmission Packets:          | %d\n",transmittedPackets);
  printf("| Total Data Packets Generate for Transmission:    | %d\n", totalPackets);
  printf("|------------------------------------------------------\n");
  close(socketServer);
  exit(1);
}
