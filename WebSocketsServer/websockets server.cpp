//http://msdn.microsoft.com/en-us/library/ms737550(v=VS.85).aspx
//http://msdn.microsoft.com/en-us/library/ms682516.aspx
//http://msdn.microsoft.com/en-us/library/ms682453.aspx
#include <string>
#include <istream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>//winsock

#include <windows.h>//threads
#include <tchar.h>//threads
#include <strsafe.h>//threads

#pragma comment(lib, "wininet.lib")//winsock
#pragma comment(lib,"Ws2_32.lib")//winsock

#include <regex>
#include "md5\md5.h"
#include "md5\md5_loc.h"
using namespace std;

#define MAX_THREADS 3
static DWORD   threadID;
static HANDLE  hThread;
static SOCKET clientfd;
DWORD WINAPI SendOnUserInput( LPVOID lpParam );

struct websocketT
{
	SOCKET* listenfd;
	SOCKET* connectfd;
	struct sockaddr_in server;
	struct sockaddr *clientInfo;//later filled in by call to accept()
	struct sockaddr_in *clientIPv4info;// represent a pointer to a IP
	string port;
	string ipAddress;
	string path;
	string origin;
	string location;
};
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

//assumes this machine is Little Endian
char* createMD5Buffer(int result1, int result2, char challenge[8])
{	
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
	/*printf("raw answser bytes: ");
	for (int i = 0; i < MD5_SIZE; i++)		
	printf(" %d", (unsigned char)raw_answer[i]);	
	printf("\n\n");*/
	return raw_answer;
}



char* createHash(string key1, string key2, char* challenge)
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

//returns false if the handshake request fails to parse
//out: challenge
bool parseHandshake(websocketT *ws, char* request, string * key1, string * key2, string* resource, 
	char** challenge)//out
{	
	char* key1pattern = "(Sec-WebSocket-Key1:)[[:s:]](.+\\r\\n)";
	char* key2pattern = "(Sec-WebSocket-Key2:)[[:s:]](.+\\r\\n)";
	char* resourcePattern = "(GET)[[:s:]](/[[:alnum:]]+)";
	match_results<const char*> key1M, key2M, getresrcM;

	string wsrequest(request);

	//get challenge
	for (int i = 0; i < 8; i++)
		(*challenge)[i] = wsrequest[wsrequest.length() - 8 + i];

	tr1::regex rx1(key1pattern);
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

	if (*key1 == "" || *key2 == "" || *resource == "")
		return false;
	else
		return true;
}

//returns handshake response
void createHandshakeResponse(websocketT *ws, char* challenge, string key1, string key2, 
	char** response)//out
{	
	char* answer = createHash(key1, key2, challenge);//length == MD5_SIZE
	std::string handshake = "HTTP/1.1 101 Web Socket Protocol Handshake\r\n"
		"Upgrade: WebSocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Origin: " + ws->origin + "\r\n" +
		"Sec-WebSocket-Location: " + ws->location + "\r\n\r\n";

	const char* handshake_bytes = handshake.c_str();	
	int responseLength = handshake.length() + MD5_SIZE;
	*response = (char*)calloc(responseLength, sizeof(char));
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
	(*response)[responseLength] = '\0';//null-terminate (for debug or strlen)
}


//returns TRUE if web socket client handshake succeeds
bool negotiateHandshake(websocketT *ws, char* request)
{
	//get client handshake	
	printf("Received opening message from client:\n\n%s\n", request);
	char* challenge = (char*)calloc(8,sizeof(char));
	char *response = NULL;
	string key1, key2, resource;
	if (!parseHandshake(ws, request, &key1, &key2, &resource, &challenge))
		return false;
	createHandshakeResponse(ws, challenge, key1,key2, &response);	
	int sent = send(*ws->connectfd, response, strlen(response), 0);		
	return sent > 0;
}


//accepts a pointer (SOCKET*) instead of returning SOCKET to make initialization errors clearer.
//
void open_listenfd(SOCKET* listenfd, char* ipAddress, char* port)
{		
	struct sockaddr_in server = { 0 }; //struct. must be zero-d out. later cast to (SOCKADDR*)						
	server.sin_family = AF_INET;	
	server.sin_addr.s_addr = inet_addr(ipAddress);//INADDR_ANY; // inet_addr("127.0.0.1");
	server.sin_port = htons((u_short)atoi(port));//dynamic port range: 49152 - 65535 ;
	*listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Bind the socket.
	if (bind( *listenfd, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR) 	
		return;
	if (listen( *listenfd, SOMAXCONN ) == SOCKET_ERROR)
		printf("Error listening on socket.\n");
	printf("Server is listening on socket... %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));	
}

void websocketT_init(websocketT* ws, char* ipaddress, char* port, char* origin = "null", char* path = "/echo")
{			
	ws->origin = origin;
	ws->port = port;
	ws->path= path;
	ws->ipAddress = ipaddress;
	ws->location = "ws://" + ws->ipAddress + ":" + ws->port + ws->path;		
	ws->listenfd = (SOCKET*)malloc(sizeof(SOCKET));
	ws->connectfd = (SOCKET*)malloc(sizeof(SOCKET));
	ws->clientIPv4info = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));	
	ws->clientInfo = (struct sockaddr*)malloc(sizeof(struct sockaddr));
	open_listenfd(ws->listenfd, ipaddress, port);
}

//blocks until a client connects
void AcceptOne(websocketT *ws)
{
	// Accept 1 client connection.
	*(ws->connectfd) = accept(*ws->listenfd, ws->clientInfo, NULL);
	ws->clientIPv4info = (struct sockaddr_in*)ws->clientInfo;	
	printf("Client connected. Address %s:%d\n", inet_ntoa(ws->clientIPv4info->sin_addr), ntohs(ws->clientIPv4info->sin_port));		
}



char* frame(char* text, int* length)
{
	*length = strlen(text) + 3;
	char* frame = (char*)calloc(*length, sizeof(char));
	frame[0] = 0x00;
	for (int i = 0; i < strlen(text); i++)
	{
		frame[1 + i] = text[i];
	}
	frame[*length - 2] = 0xFF;//rest is null-terminated (calloc)
	return frame;
}

char* unframe(char* payload, int received)
{
	char end = 0xFF;
	char* text = (char*)calloc(received, sizeof(char));	
	for (int i = 1; payload[i] != end && i < received; i++)
	{
		text[i - 1] = payload[i];	
	}	
	return text;
}

DWORD WINAPI SendOnUserInput( LPVOID lpParam )
{		
	SOCKET fd = (SOCKET)lpParam;// *((SOCKET*)lpParam);
	int sent;
	char* sendbuf;	
	printf("\n***Enter text to send to client at any time***\n");
	do
	{		
		char* sendbuf = (char*)calloc(BUFSIZ, sizeof(char));
		std::cin.getline(sendbuf, BUFSIZ);	
		//TODO: process data frame (0x00, 0xFF)...
		int length = 0;
		char* data = frame(sendbuf, &length);
		sent = send(fd, data, length, 0);
		printf("\nSent %d bytes.\n", sent);			
	} while (sent > 0);
	return 0;
}

int main()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)// Initialize Winsock
		printf("Error at WSAStartup()\n");

	struct websocketT ws = { 0 };
	websocketT_init(&ws, "127.0.0.1", 
		"38950",//"443", //443 works with "ws://" currently
		"null", "/echo");
	AcceptOne(&ws);//blocks until 1 client connects.

	//for each client:
	//must receive opening message from client (or else server cannot send, and client cannot receive.
	char* request0 = (char*)calloc(BUFSIZ, sizeof(char));
	int received = recv(*ws.connectfd, request0, BUFSIZ, 0);
	//block until opening message received	

	if (negotiateHandshake(&ws, request0))
	{
		hThread = CreateThread( 
			NULL,                   // default security attributes
			0,                      // use default stack size  
			SendOnUserInput, // thread function name
			(LPVOID)*ws.connectfd, // argument to thread function 
			0,// use default creation flags 
			&threadID);   // returns the thread identifier 

		char * recvbuf;
		int received;
		do //receive on main thread
		{									
			recvbuf = (char*)calloc(BUFSIZ, sizeof(char));
			received = recv(*ws.connectfd, recvbuf, BUFSIZ, 0);//blocks
			printf("Received %d bytes.\n\n%s\n", received,  unframe(recvbuf, received));
		}while (received > 0);


	}
	return 0;
}