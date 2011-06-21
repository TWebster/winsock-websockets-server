//http://msdn.microsoft.com/en-us/library/ms737550(v=VS.85).aspx
#include <string>
#include <iostream>
#include <winsock2.h>//#include "winsock2.h"
#include <windows.h>
#include <regex>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib,"Ws2_32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md5.h"
#include "md5_loc.h"
#include "nugget client.h"

using namespace std;

#define DEFAULT_BUFLEN 1024
static char recvbuf[DEFAULT_BUFLEN];	

static char* key1pattern1 = "(Sec-WebSocket-Key1:)[[:s:]]([^:]+\\r\\n)";
static char* key2pattern = "(Sec-WebSocket-Key2:)[[:s:]]([^:]+\\r\\n)";
static char* resourcePattern = "(GET)[[:s:]](/[[:alnum:]]+)";
static char* Response;

long long longParse(string str);
char* createMD5Buffer(int result1, int result2, char challenge[8]);
char* calculateAnswerBytes(string key1, string key2, char challenge[8]);
void createResponse(string origin, string location, 
	char challenge[], string key1, string key2, 
	char** response, int* length);
char* parseRequest(char* request, string * key1, string * key2, string* resource);
void OnClientConnect(SOCKET connectfd, char recvbuf[DEFAULT_BUFLEN], char** response, int* length);

int main()
{		
	int received, sent;
	
	struct sockaddr_in server = { 0 }; //struct. must be zero-d out. later cast to (SOCKADDR*)
	struct sockaddr *clientInfo;//later filled in by call to accept()
	struct sockaddr_in *clientIPv4info;// represent a pointer to a IPv4 socket address
	SOCKET listenfd;		//typedef UINT_PTR. socket LISTENING descriptor. Later used on call to accept().
	SOCKET connectfd;
	WSADATA wsaData;
	char* serverIpAddress = "127.0.0.1";
	u_short serverPort = 49500;	
	u_short port = htons((u_short)serverPort);
	clientInfo = (struct sockaddr*)malloc(sizeof(struct sockaddr));
	clientIPv4info = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)// Initialize Winsock
		printf("Error at WSAStartup()\n");

	server.sin_family = AF_INET;	
	server.sin_addr.s_addr = inet_addr(serverIpAddress);//INADDR_ANY; // inet_addr("127.0.0.1");
	server.sin_port = port;//dynamic port range: 49152 - 65535 ;
	listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	// Bind the socket.
	if (bind( listenfd, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR) 	
		return 1;//error
	if (listen( listenfd, SOMAXCONN ) == SOCKET_ERROR)
		printf("Error listening on socket.\n");
	printf("Web socket server is listening on socket... %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));

	while (true)
	{		

		// Accept the connection.
		connectfd = accept(listenfd, clientInfo, NULL);
		clientIPv4info = (struct sockaddr_in*)clientInfo;	
	printf("Browser client connected: %s:%d\n", inet_ntoa(clientIPv4info->sin_addr), ntohs(clientIPv4info->sin_port));		
		char* response = NULL;
		int length = 0;
		OnClientConnect(connectfd, recvbuf, &response, &length);				
		
		//printf("response (length %d):\n\n", response, strlen(response));		
		sent = send(connectfd, response, length, 0);		
		if (sent > 0)
		{
			//success, websocket client accepted.
			//start receiving (data "frame")
		}
		//closesocket(connectfd);
	}	
	closesocket(listenfd);
	WSACleanup();
	
	return 0;
}


void OnClientConnect(SOCKET connectfd, char recvbuf[DEFAULT_BUFLEN], char** response, int* length)
{
		//get initial client request:		
		int received = recv(connectfd, recvbuf, DEFAULT_BUFLEN, 0);
		printf("%d bytes received from client request.\n\n", received);
		printf("request data:\n%s\n", recvbuf);
		char* request = strdup(recvbuf);

		//verify our response against Nugget web socket server:
		/*nuggetClient n = nuggetClient();
		Response = n.SendReceive(request);*/

		string key1, key2, resource;
		string origin = "null";
		string location = "ws://127.0.0.1:49500/echo";
		char * challenge;
		challenge = parseRequest(request, &key1, &key2, &resource);
		createResponse(origin, location, challenge, key1, key2, response, length);
		
}


//returns challenge
char* parseRequest(char* request, string * key1, string * key2, string* resource)
{
	int i;
	match_results<const char*> key1M, key2M, getresrcM;
	char* challenge = (char*)calloc(8,sizeof(char));
	string wsrequest = request;
	//get challenge
	for (i = 0; i < 8; i++)
		challenge[i] = wsrequest[wsrequest.length() - 8 + i];

	tr1::regex rx1(key1pattern1);
	tr1::regex rx2(key2pattern);
	tr1::regex rx3(resourcePattern);

	//match Sec-WebSocket-Key1 
	tr1::regex_search(wsrequest.c_str(), key1M, rx1);
	*key1 = key1M[2];		

	//match Sec-WebSocket-Key1 		
	tr1::regex_search(wsrequest.c_str(), key2M, rx2);	
	*key2 = key2M[2];

	//match GET (resource)
	tr1::regex_search(wsrequest.c_str(), getresrcM, rx3);
	*resource = getresrcM[2];

	return challenge;
}




void createResponse(string origin, string location, 
	char challenge[], string key1, string key2, 
	char** response, int* length)
{	
	char* answer = calculateAnswerBytes(key1, key2, challenge);//length == MD5_SIZE
	location = "ws://127.0.0.1:49500/echo";
	origin = "null";
	std::string handshake = "HTTP/1.1 101 Web Socket Protocol Handshake\r\n"
                              "Upgrade: WebSocket\r\n"
                              "Connection: Upgrade\r\n"
                              "Sec-WebSocket-Origin: " + origin + "\r\n" +
                              "Sec-WebSocket-Location: " + location + "\r\n\r\n";
		
	const char* handshake_bytes = handshake.c_str();	
	*length = handshake.length() + MD5_SIZE;
	*response = (char*)calloc(*length, sizeof(char));
	for (int i = 0; i < handshake.length(); i++)
	{
		(*response)[i] = handshake_bytes[i];
	}
	for (int i = 0; i < MD5_SIZE; i++)
	{
		(*response)[handshake.length() + i] = answer[i];
	}
	/*for (int i = handshake.length(); i < *length; i++)
		printf("%d\n", (unsigned char)(*response)[i]);*/
	(*response)[*length] = '\0';//null-terminate (for debug or strlen)
}

char* calculateAnswerBytes(string key1, string key2, char challenge[8])
{
	int spaces1 = 0;
	int spaces2 = 0;
	string digits1, digits2;
	int result1, result2;
	for (int i = 0; i < key1.length(); i++)
	{
		if (key1[i] == 0x20)
			spaces1++;
	}
	for (int i = 0; i < key2.length(); i++)
	{
		if (key2[i] == 0x20)
			spaces2++;
	}
	
	for (int i = 0; i < key1.length(); i++)
	{
		if (isdigit(key1[i]))
		{
			digits1 += key1[i];
		}			
	}
	for (int i = 0; i < key2.length(); i++)
	{
		if (isdigit(key2[i]))
		{
			digits2 += key2[i];
		}			
	}
	result1 = longParse(digits1) / spaces1;
	result2 = longParse(digits2) / spaces2;


	char* raw_answer = createMD5Buffer(result1,result2,challenge);	
	
	 /* calculate the sig */
	char * sig = (char*)calloc(MD5_SIZE,sizeof(char));

    md5_buffer(raw_answer, MD5_SIZE, sig);	//sig is the MD5 hash

	//debug
	/*for (int i = 0; i < MD5_SIZE; i++)
	{
		printf("%d %d\n", raw_answer[i], (unsigned char)sig[i]);
	}*/    
	/* convert from the sig to a string rep */
	//char* str = (char*)calloc(64, sizeof(char));    
    //md5_sig_to_string(sig, str, sizeof(str));
	
	return sig;
}


char* createMD5Buffer(int result1, int result2, char challenge[8])
{
		//assuming this machine is Little Endian:	
	//char* raw_answer = (char*)calloc(MD5_SIZE,sizeof(char));
	char* raw_answer = (char*)calloc(MD5_SIZE,sizeof(char));
	raw_answer[0] = ((unsigned char*)&result1)[3];
	raw_answer[1] = ((unsigned char*)&result1)[2];
	raw_answer[2] = ((unsigned char*)&result1)[1];
	raw_answer[3] = ((unsigned char*)&result1)[0];
	raw_answer[4] = ((unsigned char*)&result2)[3];
	raw_answer[5] = ((unsigned char*)&result2)[2];
	raw_answer[6] = ((unsigned char*)&result2)[1];
	raw_answer[7] = ((unsigned char*)&result2)[0];
	for (int i = 0; i < 8; i++)
	{				
		raw_answer[8 + i] = challenge[i];		
	}

	//display: (debugging)
	printf("raw answser bytes: ");
	for (int i = 0; i < MD5_SIZE; i++)		
		printf(" %d", (unsigned char)raw_answer[i]);	
	printf("\n\n");
		
	return raw_answer;//raw_answer;
}

long long longParse(string str)
{
	long long result = 0;
	for (int i = 0; i < str.length(); i++)
	{
		unsigned digit = str[str.length() - 1 - i] - '0';
		result += digit * pow((double)10, i);
	}
	return result;
}