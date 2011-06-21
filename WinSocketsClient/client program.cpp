#include <string.h>
#include <istream>
#include <iostream>
#include <stdio.h>
#include <winsock2.h>//#include "winsock2.h"
#include <windows.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib,"Ws2_32.lib")	//why this wasn't in MSDN's documentation, I don't know.


#define MAX_THREADS 3

static DWORD   threadID;
static HANDLE  hThread;
static SOCKET clientfd;

struct socketInfoT
{
	char* serverIp;
	char* port;
	struct sockaddr_in server;
	SOCKET clientfd;
};
DWORD WINAPI SendOnInputThreadRoutine( LPVOID lpParam );
struct socketInfoT socketInfoT_new(char* ipaddress, char* port);

int main() 
{  		
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
		printf("Error at WSAStartup()\n");
	
	int received;
	char* recvbuf;	
	socketInfoT s = socketInfoT_new("127.0.0.1", "8777");

	char* sendbuf = "opening message from client.";
	int sent = send(s.clientfd, sendbuf, strlen(sendbuf), 0);//SEND does not block

	hThread = CreateThread( 
            NULL,                   // default security attributes
            0,                      // use default stack size  
            SendOnInputThreadRoutine, // thread function name
            (LPVOID)s.clientfd, // argument to thread function 
            0,// use default creation flags 
            &threadID);   // returns the thread identifier 
	
	do//receive on main thread (recv must be called on main thread?)
	{		
		recvbuf = (char*)calloc(BUFSIZ, sizeof(char));
		received = recv(s.clientfd, recvbuf, BUFSIZ, 0);//should block thread
		printf("\nreceived %d bytes:\n", received);			
	} while (received > 0);

	WSACleanup();
	return 0;
}

DWORD WINAPI SendOnInputThreadRoutine( LPVOID lpParam )
{	
	SOCKET fd = (SOCKET)lpParam;// *((SOCKET*)lpParam);
	int sent;
	char* sendbuf;	
	printf("\n***Enter text to send to server at any time***\n");
	do
	{		
		sendbuf = (char*)calloc(BUFSIZ, sizeof(char));
		std::cin.getline(sendbuf, BUFSIZ);										
		sent = send(fd, sendbuf, strlen(sendbuf), 0);
		printf("\nSent %d bytes.\n", sent);		
	} while (sent > 0);
	return 0;
}


//out: sockaddr_in, SOCKET
//
void open_clientfd(char* host_addr, char* port, SOCKET* clientfd, sockaddr_in * serverInfo)
{
		//sockaddr_in: http://msdn.microsoft.com/en-us/library/ms740496(v=VS.85).aspx
		//SOCKET clientfd;//typedef UINT_PTR
		*clientfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		//sockaddr_in
		serverInfo->sin_family = AF_INET;
		serverInfo->sin_addr.s_addr = inet_addr(host_addr);
		serverInfo->sin_port =  htons(atoi(port));

		// Connect to server.
		if ( connect( *clientfd, (SOCKADDR*)serverInfo, sizeof(sockaddr_in) ) == SOCKET_ERROR) {
			printf( "Failed to connect.\n" );
			WSACleanup();
			return;
		}
		printf("proxy client is connected to server at address: %s:%d\n", 
			inet_ntoa(serverInfo->sin_addr), ntohs(serverInfo->sin_port));		
}

struct socketInfoT socketInfoT_new(char* ipaddress, char* port)
{
	struct socketInfoT s;
	s.serverIp = ipaddress;
	s.port = port;
	open_clientfd(ipaddress, port, &(s.clientfd), &(s.server));
	return s;
}