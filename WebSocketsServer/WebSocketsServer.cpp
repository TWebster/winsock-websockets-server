//http://msdn.microsoft.com/en-us/library/ms737550(v=VS.85).aspx
#include <string>
#include <iostream>
#include <winsock2.h>//#include "winsock2.h"
#include <windows.h>
#include <regex>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib,"Ws2_32.lib")
//#using <system.dll>


#define DEFAULT_BUFLEN 512

int received, sent;


int main()
{		
	char recvbuf[DEFAULT_BUFLEN];	
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

		
		

		//send response
		std::string bytes1 = "\x48\x54\x54\x50\x2F\x31\x2E\x31\x20\x31\x30\x31\x20\x57\x65\x62\x20\x53\x6F\x63\x6B\x65\x74\x20\x50\x72\x6F\x74\x6F\x63\x6F\x6C\x20\x48\x61\x6E\x64\x73\x68\x61\x6B\x65\x0D\x0A\x55\x70\x67\x72\x61\x64\x65\x3A\x20\x57\x65\x62\x53\x6F\x63\x6B\x65\x74\x0D\x0A\x43\x6F\x6E\x6E\x65\x63\x74\x69\x6F\x6E\x3A\x20\x55\x70\x67\x72\x61\x64\x65\x0D\x0A\x57\x65\x62\x53\x6F\x63\x6B\x65\x74\x2D\x4F\x72\x69\x67\x69\x6E\x3A\x20null";
		std::string bytes2 = "\x0D\x0A\x57\x65\x62\x53\x6F\x63\x6B\x65\x74\x2D\x4C\x6F\x63\x61\x74\x69\x6F\x6E\x3A\x20";
		std::string location = "ws://127.0.0.1:49500/echo";
		std::string last4 = "\x0D\x0A\x0D\x0A";
		std::string response = bytes1 + bytes2 + location + last4;		
		const char* bytes = response.c_str();		
		std::cout << "response:\n\n"  << response << std::endl;
		sent = send(connectfd, bytes, response.length(), 0);		
		closesocket(connectfd);
	}	
	closesocket(listenfd);
	WSACleanup();
	
	return 0;
}

void HandleClientConnect(SOCKET connectfd, char recvbuf[DEFAULT_BUFLEN])
{
		//get initial client request:		
		received = recv(connectfd, recvbuf, DEFAULT_BUFLEN, 0);
		printf("Bytes received: %d\n\n", received);
		printf("Data received:\n %s\n", recvbuf);
}