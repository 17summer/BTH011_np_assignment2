#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
/* You will to add includes here */
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
// Included to get the support library
#include <calcLib.h>

#include "protocol.h"

#define MAXBUFLEN 100 

const struct calcMessage PROTOCOL = {22, 0, 17, 1, 0};

using namespace std;
/* Needs to be global, to be rechable by callback and main */
int loopCount=0;
int terminate=0;

static int clientID = 1;

/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum){
  // As anybody can call the handler, its good coding to check the signal number that called it.

  printf("Let me be, I want to sleep, loopCount = %d.\n", loopCount);

  if(loopCount>20){
    printf("I had enough.\n");
    terminate=1;
  }
  
  return;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

//use thread to handle several clients at a time
void* handleclients(void* arg)
{




	return NULL;
}

bool compareProtocol(char* msg)
{
	struct calcMessage info;

	memset(&info,0,sizeof(info));
	
	memcpy(&info,msg,sizeof(info));
	
	info.type = ntohs(info.type);
	info.message = ntohl(info.message);
	info.protocol = ntohs(info.protocol);
	info.major_version = ntohs(info.major_version);
	info.minor_version = ntohs(info.minor_version);
	
	printf("The msg of protocol:%u %u %u %u %u\n",info.type,info.message,info.protocol,info.major_version,info.minor_version);
	
	if(info.type != PROTOCOL.type || info.message != PROTOCOL.message || info.protocol != PROTOCOL.protocol ||
	 info.major_version != PROTOCOL.major_version || info.minor_version != PROTOCOL.minor_version)
	 { 	
		return false;
	 }
	 return true;
}

void sendCalcMsg(int sockfd,struct sockaddr_storage their)
{
	calcMessage msg;
	msg.type = htons(2);
	msg.message = htons(2);
	msg.major_version = htons(1);
	msg.minor_version = htons(0);
	
	struct sockaddr_storage their_addr = their;
	socklen_t addr_len;
	addr_len = sizeof(their_addr);

	int numbytes;

	if((numbytes = sendto(sockfd,&msg,sizeof(msg),0,(struct sockaddr*)&their_addr,addr_len))==-1)
	{
		perror("talker:sendto.");
	}
		
}

int main(int argc, char *argv[]){
  
	/* Do more magic */
	initCalcLib();
	

	/* accept the para to create a server and clarify IP(DNS,IPv4 or IPv6) */
	if(argc != 2)
	{
		fprintf(stderr, "usage: %s hostname (%d)\n", argv[0], argc);
		exit(1);
	}


	//resolve IP address and port
	//split the desthost and destport
	char *str = argv[1];
	char *Desthost = "";
	char *Destport = "";
	char *colon_pos = strrchr(str, ':');
	if (colon_pos != NULL) {
	*colon_pos = '\0';
	Desthost = str;
	Destport = colon_pos + 1;
	}

//Debug specific information	
#ifdef DEBUG
	printf("DEBUG:Desthost: %s and Port: %s \n",Desthost,Destport);
#endif


#ifdef DEBUG
	printf("The info of supported protol:%u %u %u %u %u\n",PROTOCOL.type,PROTOCOL.message,PROTOCOL.protocol,PROTOCOL.major_version,PROTOCOL.minor_version);
#endif
	//create UDP socket and bind the address
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int yes = 1;
	int numbytes;
	struct sockaddr_storage their_addr;
	struct sockaddr_in *the_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	addr_len = sizeof(their_addr);


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // allow IPv4, IPv6 and DNS.
	hints.ai_socktype = SOCK_DGRAM; //use UDP
	hints.ai_flags = AI_PASSIVE; // use my IP

	//the first para
	if ((rv = getaddrinfo(Desthost, Destport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		void *addr;
		char ipstr[INET6_ADDRSTRLEN];

		// get the address from the address struct
		if (p->ai_family == AF_INET) {
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
		} else {
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
			addr = &(ipv6->sin6_addr);
		}
		
		
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}
		
		//Allows reuse of socket addresses in TIME_WAIT state when binding addresses
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		    perror("setsockopt");
		    exit(1);
		}
		
		/* up to this point its similar as client */ 
		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}
		inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
		printf("UDP Server with address[%s] and Port[%s] is starting...\n",ipstr,Destport);
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return 2;
	}

	freeaddrinfo(servinfo);

	
	//while(1)
	//{
		memset(buf,0,sizeof(buf));
	
		
		if((numbytes=recvfrom(sockfd,buf,MAXBUFLEN-1,0,(struct sockaddr*)&their_addr,&addr_len)) == -1){
			perror("recv:error\n");
			exit(1);
		}

		//Check if the protocol matches
		if(!compareProtocol(buf))
		{
			sendCalcMsg(sockfd,their_addr);	
		}else
		{
			struct calcProtocol proto;	
			proto.type = htons(1);
			proto.major_version = htons(1);
			proto.minor_version = htons(0);
			proto.id = htons(clientID++);
			proto.arith = htons(randomType());
			proto.intValue1 = htons(randomInt());
			proto.intValue2 = htons(randomInt());
			
			
			
		}
	
	//}
	
#ifdef DEBUG
	printf("DEBUG: PROTOCOL COMPARISION:%d\n",compareProtocol(buf));
#endif

	/* 
	Prepare to setup a reoccurring event every 10s. If it_interval, or it_value is omitted, it will be a single alarm 10s after it has been set. 
	*/
	struct itimerval alarmTime;
	alarmTime.it_interval.tv_sec=10;
	alarmTime.it_interval.tv_usec=10;
	alarmTime.it_value.tv_sec=10;
	alarmTime.it_value.tv_usec=10;

	/* Regiter a callback function, associated with the SIGALRM signal, which will be raised when the alarm goes of */
	signal(SIGALRM, checkJobbList);
	setitimer(ITIMER_REAL,&alarmTime,NULL); // Start/register the alarm. 

	#ifdef DEBUG
		printf("DEBUGGER LINE ");
	#endif

  
	while(terminate==0){
		printf("This is the main loop, %d time.\n",loopCount);
		sleep(1);
		loopCount++;
	}

	printf("done.\n");
	return(0);


  
}
