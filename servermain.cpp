#include <stdio.h>	
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <string>
#include <iostream>
/* You will to add includes here */
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <map>

using namespace std;
// Included to get the support library
#include <calcLib.h>

#include "protocol.h"

#define MAXBUFLEN 100 
#define RIGHT "OK"
#define WRONG "NOT OK"
#define UPROTOCOL "TEXT UDP 1.0"
const struct calcMessage PROTOCOL = {22, 0, 17, 1, 0};
const struct calcMessage NOTOK_MSG = {htons(2),htonl(2),htonl(17),htons(1),htons(0)};
const struct calcMessage OK_MSG = {htons(2),htonl(1),htonl(17),htons(1),htons(0)};

vector<string> arith = {"add","sub","mul","div","fadd","fsub","fmul","fdiv"};

//store id status and time that has used 
std::map<int,int> clientStatus;

using namespace std;
/* Needs to be global, to be rechable by callback and main */
/*
int terminate=0;
*/
int clientID = 1;
int loopCount=0;

typedef struct Info
{
	char* addr;
	int port;
}clientInfo;

map<int,clientInfo> CI;

//declare functions
int getArith(char* operand);
void *get_in_addr(struct sockaddr *sa);
bool compareProtocol(char* msg);

/* Call back function, will be called when the SIGALRM is raised when the timer expires. */
void checkJobbList(int signum){
  // As anybody can call the handler, its good coding to check the signal number that called it.

	for(auto it = clientStatus.begin(); it != clientStatus.end();it++)
	{
		it->second++;
		//if timeout, delete the assig.
		if(it->second >=10)
		{
			for(auto i = CI.begin();i != CI.end();i++)
			{
				if(i->first == it->first)
				{
					printf("Client[%d] ip[%s] port[%d] is timeout. Lost.\n",it->first,i->second.addr,i->second.port); 
				}
			}
			it = clientStatus.erase(it);
		}else
		{
			it++;
		}
	}
	printf("This is the main loop %d\n",loopCount);

	loopCount++;

  return;
}

//based on the arith to get the corresponding index of arith in the protocol 
int getArith(char* operand)
{
	unordered_map<string,int>element_index;
	for(int i = 0; i < arith.size(); i++)
	{
		element_index[arith[i]] = i;
	}
	
	if(element_index.find(operand) != element_index.end())
	{
		return element_index[operand] + 1;
	}
	return -1;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
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
	
	#ifdef DEBUG
		//printf("The msg of protocol:%d %d %d %d %d\n",info.type,info.message,info.protocol,info.major_version,info.minor_version);
	#endif
	
	if(info.type != PROTOCOL.type || info.message != PROTOCOL.message || info.protocol != PROTOCOL.protocol ||
	 info.major_version != PROTOCOL.major_version || info.minor_version != PROTOCOL.minor_version)
	 { 	
		return false;
	 }
	 return true;
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

#ifdef DEBUG
	printf("DEBUG: PROTOCOL COMPARISION:%d\n",compareProtocol(buf));
#endif

//Main Code Part


 	/* 
	Prepare to setup a reoccurring event every 10s. If it_interval, or it_value is omitted, it will be a single alarm 10s after it has been set. 
	*/
	struct itimerval alarmTime;
	alarmTime.it_interval.tv_sec=1;
	alarmTime.it_interval.tv_usec=0;
	alarmTime.it_value.tv_sec=1;
	alarmTime.it_value.tv_usec=0;

	/* Regiter a callback function, associated with the SIGALRM signal, which will be raised when the alarm goes of */
	signal(SIGALRM,checkJobbList);
	setitimer(ITIMER_REAL,&alarmTime,NULL); // Start/register the alarm. 

	#ifdef DEBUG
		printf("DEBUGGER LINE ");
	#endif

	struct calcProtocol proto;
	while(1)
	{
		memset(buf,0,sizeof(buf));
		
		//receive message from client	
		if((numbytes=recvfrom(sockfd,buf,MAXBUFLEN-1,0,(struct sockaddr*)&their_addr,&addr_len)) == -1){
			perror("recv:error\n");
			exit(1);
		}
		
		
		//output client's info to terminal
		struct sockaddr_in* clientAddrInfo;
		char clientIP[INET_ADDRSTRLEN];
		clientAddrInfo = (struct sockaddr_in *)&their_addr;
		inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr*)clientAddrInfo),clientIP,sizeof(clientIP));
		int clientPort = ntohs(clientAddrInfo->sin_port);

		clientInfo info = {clientIP,clientPort};

		char* operand = randomType();
		
		//to check whether the received msg is calcMessage or calcProtocol
		//if received the message: calcMessage
		if(numbytes == sizeof(calcMessage))
		{
			//Check if the protocol matches
			//if not, send  clac Message
			if(!compareProtocol(buf))
			{
				//reset the char array.
				memset(buf,0,sizeof(buf));
				if((numbytes = sendto(sockfd,&NOTOK_MSG,sizeof(NOTOK_MSG),0,(struct sockaddr*)&their_addr,addr_len)) == -1)
				{
					perror("error:sendto\n");
				}
				continue;
			}
			//protocol right
			else
			{		
				//record client status
				clientStatus.insert(pair<int,int>(clientID,0));
				CI.insert(pair<int,clientInfo>(clientID,info));
				
				printf("Server receive client[%d] from ip[%s] and port[%d].\n",clientID,clientIP,clientPort);
				
				if(operand[0] == 'f')
				{
					proto.arith = htonl(getArith(operand));
					proto.inValue1 = htonl(0);
					proto.inValue2 = htonl(0);
					proto.inResult = htonl(0);
					proto.flValue1 = randomFloat();
					proto.flValue2 = randomFloat();
					proto.flResult = 0.0f;
					printf("The operand of client[%d]: %s , %d , fa1 %lf , fa2 %lf\n\n",clientID,operand,ntohl(proto.arith),proto.flValue1,proto.flValue2);
				}else
				{
					proto.arith = htonl(getArith(operand));
					proto.inValue1 = htonl(randomInt());
					proto.inValue2 = htonl(randomInt());
					proto.inResult = htonl(0);
					proto.flValue1 = 0.0f;
					proto.flValue2 = 0.0f;
					proto.flResult = 0.0f;
					printf("The operand of client[%d]: %s , %d , ia1 %d , ia2 %d\n\n",clientID,operand,ntohl(proto.arith),ntohl(proto.inValue1),ntohl(proto.inValue2));
				}
				
				proto.type = htonl(1);
				proto.major_version = htonl(1);
				proto.minor_version = htonl(0);
				proto.id = htonl(clientID);
				
				clientID++;
				
		
				if((numbytes=sendto(sockfd,&proto,sizeof(proto),0,(struct sockaddr*)&their_addr,addr_len)) == -1)
				{
					perror("error:sendto\n");
				}
			}
		
		}else if(numbytes == sizeof(calcProtocol))
		{		
			bool flag = false;
			
			memset(&proto,0,sizeof(proto));
			
			memcpy(&proto,buf,sizeof(proto));		
			
			printf("Receive calcProtocol from client[%d] ip[%s] and port[%d].\n",ntohl(proto.id),clientIP,clientPort);                
			
			//check out the correctness of client ID 
			for(auto it = clientStatus.begin(); it != clientStatus.end(); it++)
			{
				if(ntohl(proto.id) == it->first)
				{
					flag = true;
					break;
				}	
			}
			
			#ifdef DEBUG
				printf("Flag: %d\n",flag);
			#endif
			//if not exist, send NOTOK_MSG
			if(!flag)
			{
				if((numbytes = sendto(sockfd,&NOTOK_MSG,sizeof(NOTOK_MSG),0,(struct sockaddr*)&their_addr,addr_len)) == -1)
				{
					perror("error:sendto\n");
				}
				printf("REJECTED. Client[%d] ip[%s] port[%d] is LOST or not in the client status.\n",ntohl(proto.id),clientIP,clientPort);
				continue;
			}
			
			//simplify the calculation
			int value1 = ntohl(proto.inValue1);
			int value2 = ntohl(proto.inValue2);
			int result = ntohl(proto.inResult);
			double fvalue1 = proto.flValue1;
			double fvalue2 = proto.flValue2;
			double fresult = proto.flResult;
			
			bool isEqual;
			
			switch (ntohl(proto.arith))
			{
				case 1:
					isEqual = ((value1 + value2 - result) == 0);
					break;
				case 2:
					isEqual = ((value1 - value2 - result) == 0);
					break;
				case 3:
					isEqual = ((value1 * value2 - result) == 0);
					break;
				case 4:
					isEqual = ((value1 / value2 - result) == 0);
					break;
				case 5: 
					isEqual = (fabs(fvalue1 + fvalue2 - fresult) < 0.0001);
					break;
				case 6:
					isEqual = (fabs(fvalue1 - fvalue2 - fresult) < 0.0001);
					break;
				case 7:
					isEqual = (fabs(fvalue1 * fvalue2 - fresult) < 0.0001);	
					break;
				case 8:
					isEqual = (fabs(fvalue1 / fvalue2 - fresult) < 0.0001);
					break;
				default:
					isEqual = false;
					break;	
			}
			#ifdef DEBUG
				printf("The correctness: %d",isEqual);
			#endif
			
			memset(buf,0,sizeof(buf));
			//if the result is right
			if(isEqual)
			{	
				
				memcpy(buf,&OK_MSG,sizeof(OK_MSG));	
				if((numbytes=sendto(sockfd,&buf,sizeof(buf),0,(struct sockaddr*)&their_addr,addr_len)) == -1)
				{
					perror("send error:OK");
					continue;
				}
				printf("Client[%d] ip[%s] port[%d] answer OK.\n",ntohl(proto.id),clientIP,clientPort);
			}else
			{
				memcpy(buf,&NOTOK_MSG,sizeof(NOTOK_MSG));	
				if((numbytes=sendto(sockfd,&buf,sizeof(buf),0,(struct sockaddr*)&their_addr,addr_len)) == -1)
				{
					perror("send error:NOTOK");
					continue;
				}
				printf("Client[%d] ip[%s] port[%d] answer NOTOK.\n",ntohl(proto.id),clientIP,clientPort);
			}	
			clientStatus.erase(ntohl(proto.id));
			printf("Client[%d] ip[%s] port[%d] is finished.\n\n",ntohl(proto.id),clientIP,clientPort);  
		}else
		{
			if(buf == UPROTOCOL)
			{
				printf("Receive the \'TEXT UDP 1.0\' protocol.\n");
				break;
			}
		}		
	
	}

	printf("done.\n");
	
	close(sockfd);
	return 0;  
}
