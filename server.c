/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   
   Rensselaer Polytechnic Institute (RPI)

   Modified: June 2018

   Chamezopoulos Savvas
*/

#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define BACKLOG 5
#define STRINGSIZE 256
#define DATABASESIZE 100

pthread_mutex_t mux;
pthread_attr_t pthread_custom_attr;

double cpu_time_used;
clock_t start, end; //for cpu time

char filenameRes[255] = "/root/ex3/results/CPU-WALL.txt";
FILE *fp = NULL; //for stats

struct timeval startwtime, endwtime;
double seq_time; //for elapsed time

char **msgs, **recvs;

void error(char *msg);
void *service(void *arg);
char *getmessage(int newsockfd);
void storeMessage(char *recvaddr, char *message);
void beeper(int newsockfd, char *sendaddr);
void deleteEntry(int i);
void endseq(clock_t start, struct timeval startwtime);

int main(int argc, char *argv[])
{
  int sockfd, portno, clilen, newsockfd;
  
  struct sockaddr_in serv_addr, cli_addr;

  pthread_t *thread = (pthread_t *)malloc(BACKLOG*sizeof(pthread_t));
  pthread_mutex_init (&mux, NULL);
  pthread_attr_init(&pthread_custom_attr);

  
  
  if (argc < 2) {
    fprintf(stderr,"ERROR, no port provided\n");
    exit(1);
  }


  //initialize dbs
  printf("Initializing DBs...\n");
  msgs = (char **) malloc(DATABASESIZE*sizeof(char *));
  recvs = (char **) malloc(DATABASESIZE*sizeof(char *));
  for(int i=0; i<DATABASESIZE; i++){
    msgs[i] = (char *) malloc(STRINGSIZE*sizeof(char));
    memset(msgs[i],0,sizeof(msgs[i]));
    strcpy(msgs[i],"N/A");
    recvs[i] = (char *) malloc(STRINGSIZE*sizeof(char));
    memset(recvs[i],0,sizeof(recvs[i]));
    strcpy(recvs[i],"empty");
  }

  printf("Creating socket...\n");
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  printf("Binding...\n");
  if (bind(sockfd, (struct sockaddr *) &serv_addr,
           sizeof(serv_addr)) < 0) 
    error("ERROR on binding");
  printf("Listening...\n\n");
  listen(sockfd,BACKLOG);
  
  printf("Server online and ready to fly!\n");
  
  if(gettimeofday(&startwtime, NULL)){
    perror("Error on gettimeofday - startwtime");
    exit(EXIT_FAILURE);
  }

  start = clock();
  do{

    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) 
      error("ERROR on accept");

    if(pthread_create(thread, &pthread_custom_attr, service, (void *)&newsockfd) < 0)
      error("ERROR in creating thread");

  }while(1);

  pthread_join(*thread,NULL);
  
  end = clock();

  pthread_attr_destroy(&pthread_custom_attr);
  pthread_mutex_destroy (&mux);
  free(msgs) ; free(recvs);

  printf("Exiting...\n");
  return 0; 
}


void error(char *msg)
{
  perror(msg);
  exit(1);
}


void *service(void *arg)
{
  int newsockfd = *(int *)arg;

  int n,i,s=0,r=0;
  char replymsg[STRINGSIZE], recvaddr[STRINGSIZE], sendaddr[STRINGSIZE], message[STRINGSIZE];

  start = clock();
  if(gettimeofday(&startwtime, NULL)){
    perror("Error on gettimeofday - endtwtime");
    exit(EXIT_FAILURE);
  }

  memset(replymsg,0,sizeof(replymsg));
  if(recv(newsockfd,replymsg,STRINGSIZE-1,0) < 0)
    error("ERROR reading from socket");

  if(send(newsockfd,"option OK",9,0) < 0)
    error("ERROR writing to socket");

  if(strcmp(replymsg,"s\n")==0){

    s=1;
  }
  else if(strcmp(replymsg,"r\n") == 0){

    r=1;
  }
  else if(strcmp(replymsg,"b\n") == 0){

    s=1;
    r=1;
  }

  if(s){
    //receive receiver's address  
    strcpy(recvaddr,getmessage(newsockfd));

    //receive message
    strcpy(message,getmessage(newsockfd));

    if(strcmp(message,"quit\n") == 0){
      endseq(start, startwtime);
    }

    //store message
    storeMessage(recvaddr,message);
  }
  
  if(r){

    //receive sender's addres
    strcpy(sendaddr,getmessage(newsockfd));

    //send user any messages he may have
    beeper(newsockfd,sendaddr);  
  }

  endseq(start, startwtime);
}

char* getmessage(int newsockfd){

  char *str;
  str = (char *) malloc(STRINGSIZE*sizeof(char));
  
  //receive receiver's address
  if(recv(newsockfd,str,STRINGSIZE-1,0) < 0)
    error("ERROR reading from socket");

  //reply
  if(send(newsockfd,"received.\n",10,0) < 0) 
    error("ERROR writing to socket");
  
  return str;
}

void storeMessage(char recvaddr[STRINGSIZE], char message[STRINGSIZE])
{
  int i=0;
  
  do{
    
    if(strcmp(recvs[i],"empty")==0){
      
      pthread_mutex_lock(&mux);
      memset(recvs[i],0,sizeof(recvs[i]));
      strcpy(recvs[i],recvaddr);
      memset(msgs[i],0,sizeof(msgs[i]));
      strcpy(msgs[i], message);
      pthread_mutex_unlock(&mux);
      break;
    }
    i++;
  }while(i<STRINGSIZE);
}

void beeper(int newsockfd, char sendaddr[STRINGSIZE])
{
  int flag = 1;
  char replymsg[STRINGSIZE];

  //recv "Send msgs" just to keep the recv-send flow right
  memset(replymsg,0,sizeof(replymsg));
  if(recv(newsockfd,replymsg,sizeof(replymsg)-1,0) < 0)
    error("ERROR reading from socket");

  for(int i=0; i<DATABASESIZE; i++){

    if(strcmp(recvs[i],sendaddr) == 0 ){
      
      if(flag){

        if(send(newsockfd,"You got new messages...\n",24,0) < 0) 
          error("ERROR writing to socket");

        flag = 0;

        memset(replymsg,0,sizeof(replymsg));
        if(recv(newsockfd,replymsg,sizeof(replymsg)-1,0) < 0) 
          error("ERROR writing to socket");
      }

      memset(replymsg,0,sizeof(replymsg));
      strcpy(replymsg,msgs[i]);
      if(send(newsockfd,replymsg,strlen(replymsg),0) < 0) 
        error("ERROR writing to socket");
      
      //delete message
      deleteEntry(i);

      memset(replymsg,0,sizeof(replymsg));
      if(recv(newsockfd,replymsg,sizeof(replymsg)-1,0) < 0) 
        error("ERROR reading from socket");
    }

  }

  if(!flag){
    if(send(newsockfd,"End of messages.",16,0) < 0) 
      error("ERROR writing to socket");

     if(recv(newsockfd,replymsg,sizeof(replymsg)-1,0) < 0) 
        error("ERROR reading from socket");
  }
  else{
    if(send(newsockfd,"No messages :(",14,0) < 0) 
      error("ERROR writing to socket");
    if(recv(newsockfd,replymsg,sizeof(replymsg)-1,0) < 0) 
      error("ERROR reading from socket");
  }
}

void deleteEntry(int i){

  pthread_mutex_lock(&mux);
  memset(recvs[i],0,sizeof(recvs[i]));
  strcpy(recvs[i],"empty");
  memset(msgs[i],0,sizeof(msgs[i]));
  strcpy(msgs[i],"N/A");
  pthread_mutex_unlock(&mux);
}

void endseq(clock_t start, struct timeval startwtime)
{
  //keep stats for analysis
  end = clock();
  if(gettimeofday(&endwtime, NULL)){
    perror("Error on gettimeofday - endtwtime");
    exit(EXIT_FAILURE);
  }

  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
  seq_time = (double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6 
    + endwtime.tv_sec - startwtime.tv_sec);

  fp = fopen( filenameRes ,"a");
  if(fp == NULL){
    perror("Error reading file(fp)\n");
    exit(EXIT_FAILURE);
  }
  fprintf(fp, "%f\n%f\n" ,cpu_time_used, seq_time);

  fclose(fp);

  pthread_exit(NULL);

}