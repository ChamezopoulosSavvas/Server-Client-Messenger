/* A simple client program to communicate with the corresponding
   server executable.
   The IP and TCP port are passed as arguments.
   
   Rensselaer Polytechnic Institute (RPI)

   Modified : June 2018

   Chamezopoulos Savvas
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define STRINGSIZE 256

void error(char *msg)
{
    perror(msg);
    exit(0);
}

void sendMessage(int sockfd, char *msgtoprint);

int main(int argc, char *argv[])
{
    int sockfd, portno, n,s=0,r=0; // s,r are flags for send/receive message

    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[STRINGSIZE]; //used for getting answers from server
  
    if (argc < 3) {
      fprintf(stderr,"usage %s hostname port\n", argv[0]);
      exit(0);
    }

    portno = atoi(argv[2]);
  
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
      error("ERROR opening socket");
  
    server = gethostbyname(argv[1]);
  
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
     }
  
    memset((char *) &serv_addr,0, sizeof(serv_addr));
  
    serv_addr.sin_family = AF_INET;
  
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
  
    serv_addr.sin_port = htons(portno);
  
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
      error("ERROR connecting");

    printf("Welcome, please choose option:\n");
    printf("Send message, Read messages or Both? Please insert (s/r/b)\n");
    do{
    
      memset(buffer,0,sizeof(buffer));
      if(fgets(buffer,sizeof(buffer),stdin) == NULL)
      error("ERROR on fgets");

      if(strcmp(buffer,"s\n")==0){

        s=1;
        break;
      }
      else if(strcmp(buffer,"r\n")==0){

        r=1;
        break;
      }
      else if(strcmp(buffer,"b\n")==0){

        s=1;
        r=1;
        break;
      }
      else

        printf("Invalid choice, please insert (s/r/b):\n");
    }while(1);

    //send option
    if(send(sockfd,buffer,strlen(buffer),0) < 0)
            error("ERROR writing to socket");

    memset(buffer,0,sizeof(buffer));
    if(recv(sockfd,buffer,sizeof(buffer)-1,0) < 0)
      error("ERROR reading from socket");
    
    if(s){
      //Receiver's address
      sendMessage(sockfd, "Receiver's address:");

      //Message
      printf("Press quit to close server\n");
      sendMessage(sockfd, "The message:");
    }

    if(r){

      //Sender's Address
      sendMessage(sockfd, "Your address:");

      //message read
      if(send(sockfd,"Send msgs",9,0) < 0)
        error("ERROR writing to socket");

      memset(buffer,0,sizeof(buffer));
      if(recv(sockfd,buffer,sizeof(buffer)-1,0) < 0)
        //new,no msg, end
        error("ERROR reading from socket");
      printf("%s\n", buffer);
      
      if(send(sockfd,"OK",2,0) < 0)
        error("ERROR writing to socket");

      if(strcmp(buffer,"You got new messages...\n")==0){

        int i = 1;
        //print unread messages
        do{

          memset(buffer,0,sizeof(buffer));

          if(recv(sockfd,buffer,sizeof(buffer)-1,0) < 0)
            error("ERROR reading from socket");
          
          if(send(sockfd,"got it",6,0) < 0)
            error("ERROR reading from socket");
          
          if(strcmp(buffer,"End of messages.")==0) break;

          printf("Message No.%d: %s",i,buffer); 
          i++;      
        }while(1);

        printf("No other messages, Exiting...\n");
      }
      //If no messages
      else if(strcmp(buffer,"No messages :(")==0){

        printf("%s\n", buffer);
      }
      else

        printf("Exiting...\n");
  }
  return 0;
}


void sendMessage(int sockfd, char *msgtoprint){

  char str[256];

  printf("Please enter the %s\n", msgtoprint);
  memset(str,0,sizeof(str));
    
  if(fgets(str,sizeof(str),stdin) == NULL)
    error("ERROR on fgets");
    
  if(send(sockfd,str,strlen(str),0) < 0) 
    error("ERROR writing to socket");
  
  
  memset(str,0,sizeof(str));
  if(recv(sockfd,str,sizeof(str)-1,0) < 0) 
    error("ERROR reading from socket");
  printf("%s\n",str);

  if(strcmp(str,"quit\n") == 0) error("Server shut down, exiting...");
}