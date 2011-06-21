#pragma once

#include <string>
#include <stdio.h>
#include <winsock2.h>//#include "winsock2.h"
#include <windows.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib,"Ws2_32.lib")	//why this wasn't in MSDN's documentation, I don't know.


class winsockClient
{
	#define DEFAULT_BUFLEN 1024
public:
	
	winsockClient(char* ipAddress, char* port)
	{
		this->host_addr = ipAddress;
		this->port = port;
		this->recvbuf = (char*)calloc(DEFAULT_BUFLEN,sizeof(char));
		if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
			printf("Error at WSAStartup()\n");
		this->clientfd = this->open_clientfd();
		Send("initial msg.");
	}
	~winsockClient() 
	{ 
		closesocket(this->clientfd);
		int result = shutdown(clientfd, SD_SEND);
		WSACleanup();
	}
	int Send(char* sendbuf)
	{
		int sent = send(this->clientfd, sendbuf, strlen(sendbuf), 0);
		printf("proxy client sent %d bytes:\n", sent, sendbuf);	
		return sent;
	}

	char* Receive()
	{
		int received = recv(this->clientfd, recvbuf, DEFAULT_BUFLEN, 0);
		printf("proxy client received %d bytes:\n\n%s\n", received, recvbuf);		
		return strdup(recvbuf);
	}

private:
	char* host_addr;
	char* port;
	struct sockaddr_in serverInfo;
	SOCKET clientfd;
	char* recvbuf;
	WSADATA wsaData;

	UINT_PTR open_clientfd()
	{
		struct sockaddr_in serverInfo;//sockaddr_in: http://msdn.microsoft.com/en-us/library/ms740496(v=VS.85).aspx
		SOCKET clientfd;//typedef UINT_PTR
		clientfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		//sockaddr_in
		serverInfo.sin_family = AF_INET;
		serverInfo.sin_addr.s_addr = inet_addr(this->host_addr);
		serverInfo.sin_port =  htons(atoi(this->port));

		// Connect to server.
		if ( connect( clientfd, (SOCKADDR*) &serverInfo, sizeof(serverInfo) ) == SOCKET_ERROR) {
			printf( "Failed to connect.\n" );
			WSACleanup();
			return 1;
		}
		printf("proxy client is connected to server at address: %s:%d\n", inet_ntoa(serverInfo.sin_addr), ntohs(serverInfo.sin_port));		
		return clientfd;
	}
	
};
