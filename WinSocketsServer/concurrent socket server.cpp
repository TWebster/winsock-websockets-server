//http://msdn.microsoft.com/en-us/library/ms737550(v=VS.85).aspx
//http://msdn.microsoft.com/en-us/library/ms682516.aspx
//http://msdn.microsoft.com/en-us/library/ms682453.aspx
#include <string>
#include <istream>
#include <iostream>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>//threads
#include <tchar.h>//threads
#include <strsafe.h>//threads
#pragma comment(lib, "wininet.lib")
#pragma comment(lib,"Ws2_32.lib")


#define MAX_THREADS 3

static DWORD   threadID;
static HANDLE  hThread;
static SOCKET clientfd;
DWORD WINAPI SendOnInputThreadRoutine( LPVOID lpParam );

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
	int error = 0;
	if ( (error = bind( *listenfd, (SOCKADDR*)&server, sizeof(server))) == SOCKET_ERROR) 	
	{
		error = WSAGetLastError();
		printf("Error on bind. (%d)\n", error);
		return;
	}		

	if (listen( *listenfd, SOMAXCONN ) == SOCKET_ERROR)
		printf("Error listening on socket.\n");

	printf("\n");
	printf("Server is listening on socket... %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));	
}

int main()
{	
	WSADATA wsaData;
	SOCKET connectfd;
	SOCKET *listenfd = (SOCKET*)malloc(sizeof(SOCKET));
	struct sockaddr_in *clientIPv4info;// represent a pointer to a IPv4 socket address		
	struct sockaddr *clientInfo;//later filled in by call to accept()
	char* recvbuf;
	int received = 0, sent = 0;//bytes

	if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)// Initialize Winsock
		printf("Error at WSAStartup()\n");
	

	clientInfo = (struct sockaddr*)malloc(sizeof(struct sockaddr));
	clientIPv4info = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));	
	open_listenfd(listenfd, "127.0.0.1", "39750");
	
	// Accept 1 client connection.
	connectfd = accept(*listenfd, clientInfo, NULL);
	clientIPv4info = (struct sockaddr_in*)clientInfo;	
	printf("Client connected. Address %s:%d\n", inet_ntoa(clientIPv4info->sin_addr), ntohs(clientIPv4info->sin_port));		
	
	//must receive opening message from client (or else server cannot send, and client cannot receive.
	recvbuf = (char*)calloc(BUFSIZ, sizeof(char));
	received = recv(connectfd, recvbuf, BUFSIZ, 0);
	printf("Received opening message from client: %d bytes.\n\n%s\n\n", received, recvbuf);

	hThread = CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            SendOnInputThreadRoutine, // thread function name
            (LPVOID)connectfd, // argument to thread function 
            0,// use default creation flags 
            &threadID);   // returns the thread identifier 

	do //receive on main thread (recv must be called on main thread?)
	{									
		recvbuf = (char*)calloc(BUFSIZ, sizeof(char));
		received = recv(connectfd, recvbuf, BUFSIZ, 0);		
		printf("Received %d bytes\n", received);
	}while (received > 0);//(received > 0);//continue receiving all bytes from this 1st connection (connect request)

	
	closesocket(connectfd);
	closesocket(*listenfd);
	WSACleanup();

	return 0;
}

DWORD WINAPI SendOnInputThreadRoutine( LPVOID lpParam )
{	
	SOCKET fd = (SOCKET)lpParam;// *((SOCKET*)lpParam);
	int sent;
	char* sendbuf;	
	printf("\n***Enter text to send to client at any time***\n");
	do
	{		
		char* sendbuf = (char*)calloc(BUFSIZ, sizeof(char));
		std::cin.getline(sendbuf, BUFSIZ);		
		sent = send(fd, sendbuf, strlen(sendbuf), 0);
		printf("\nSent %d bytes.\n", sent);			
	} while (sent > 0);
	return 0;
}
