//http://msdn.microsoft.com/en-us/library/ms737550(v=VS.85).aspx
#include <string>
#include <stdio.h>
#include <winsock2.h>//#include "winsock2.h"
#include <windows.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib,"Ws2_32.lib")


#define DEFAULT_BUFLEN 512
char recvbuf[DEFAULT_BUFLEN];
int received, sent;
int recvbuflen = DEFAULT_BUFLEN;

int main()
{		
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
	server.sin_port = htons(serverPort);//dynamic port range: 49152 - 65535 ;
	listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Bind the socket.
	if (bind( listenfd, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR) 	
		return 1;//error
	if (listen( listenfd, SOMAXCONN ) == SOCKET_ERROR)
		printf("Error listening on socket.\n");
	printf("Server is listening on socket... %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));

	while (true)
	{		

		// Accept the connection.
		connectfd = accept(listenfd, clientInfo, NULL);
		clientIPv4info = (struct sockaddr_in*)clientInfo;	
		wprintf(L"Client connected.\n");
		printf("client address %s:%d\n", inet_ntoa(clientIPv4info->sin_addr), ntohs(clientIPv4info->sin_port));		

		//get initial client request:
		received = recv(connectfd, recvbuf, recvbuflen, 0);
		printf("Bytes received: %d\n\n", received);
		printf("Data received:\n %s\n", recvbuf);
		//while (received > 0)
		//{			
		//	received = recv(connectfd, recvbuf, recvbuflen, 0);
		//}//continue receiving all bytes from this 1st connection (connect request)

		//echo response
		printf("sending...");
		sent = send(connectfd, recvbuf, received, 0);
		printf("Bytes sent: %d\n", sent);			
		closesocket(connectfd);
	}	
	closesocket(listenfd);
	WSACleanup();
	
	return 0;
}