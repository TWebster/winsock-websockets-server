//For intercepting HTTP requests (e.g. from a .NET application).
//Because Fiddler and Wireshark suck.
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
using namespace std;



struct winsocketT
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

void winsocketT_init(winsocketT* ws, char* ipaddress, char* port, char* origin = "null", char* path = "/echo", DWORD timeout = 2000)
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
void AcceptOne(winsocketT *ws)
{
	// Accept 1 client connection.
	*(ws->connectfd) = accept(*ws->listenfd, ws->clientInfo, NULL);
	/*DWORD timeout = 2000;
	if (setsockopt(*ws->connectfd, SOL_SOCKET, SO_RCVTIMEO, (char*)timeout, sizeof(DWORD)) != SOCKET_ERROR) {
		printf("Set SO_RCVTIMEO: %d\n", timeout);
	}*/
	ws->clientIPv4info = (struct sockaddr_in*)ws->clientInfo;	
	printf("Client connected. Address %s:%d\n", inet_ntoa(ws->clientIPv4info->sin_addr), ntohs(ws->clientIPv4info->sin_port));		
}


//for each connected client:
DWORD WINAPI onConnect( LPVOID lpParam )
{
	int sent = 0, received = 0;
	winsocketT * ws = (struct winsocketT*)lpParam;
	SOCKET connectfd = *ws->connectfd;

	//closesocket(*ws->listenfd);

	//must receive opening message from client (or else server cannot send, and client cannot receive.
	char * recvbuf;
	recvbuf = (char*)calloc(BUFSIZ * 4, sizeof(char));
	recv(connectfd, recvbuf, BUFSIZ * 4, 0);//block until opening message received	
	printf("\n%s\n",recvbuf);
	//printf("\n%s\n", recvbuf);//will not print on this thread.
	char* contentlengthpattern = "(Content-Length:)[[:s:]]([[:d:]]+)";
	match_results<const char*> m;
	tr1::regex rx;
	string contentLength;
	//match Sec-WebSocket-Key1 
	m = match_results<const char*>();
	rx = tr1::regex(contentlengthpattern);
	tr1::regex_search(recvbuf, m, rx);
	contentLength = m[2];		
	int contentlength = atoi(contentLength.c_str());	

	//if any form data exists, add its length (in bytes) to 'received':
	char* formdatapattern = "(\r\n\r\n)(.+)";
	string formdata;
	m = match_results<const char*>();
	rx = tr1::regex(formdatapattern);
	tr1::regex_search(recvbuf, m, rx);
	formdata = m[2];
	received = formdata.length();

	//continue receiving until we've received Content-Length bytes.
	while (received > 0 && received < contentlength)
	{									
		recvbuf = (char*)calloc(BUFSIZ * 4, sizeof(char));
		received += recv(*ws->connectfd, recvbuf, BUFSIZ, 0);//blocks
		printf("Received %d bytes.\n\n%s\n", received,  recvbuf);
	}
	char* msg = "HTTP/1.1 200 OK\r\n";
	send(connectfd, msg, strlen(msg),0);
	closesocket(connectfd);
	return 0;
}


int main()
{
	WSADATA wsaData;	
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)// Initialize Winsock
		printf("Error at WSAStartup()\n");

	struct winsocketT ws = { 0 };
	winsocketT_init(&ws, "127.0.0.1", 
		"38950",//"443", //443 works with "ws://" currently
		"null", "/echo");

	while(true)
	{
		DWORD threadID = 0;
		HANDLE hThread;		
		AcceptOne(&ws);//blocks until 1 client connects.				
		//if client connects:
		struct winsocketT * copy = (struct winsocketT*)malloc(sizeof(struct winsocketT));
		memcpy(copy, &ws, sizeof(struct winsocketT));//create a copy for child thread

		hThread = CreateThread( 
			NULL,                   // default security attributes
			0,                      // use default stack size  
			onConnect, // thread function name
			(LPVOID)copy, // argument to thread function 
			0,// use default creation flags 
			&threadID);   // returns the thread identifier 		
	}
		
	return 0;
}