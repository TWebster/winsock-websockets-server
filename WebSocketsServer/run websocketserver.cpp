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
#include "md5\md5.h"
#include "md5\md5_loc.h"
#include "webSocketServer.h"
using namespace std;


int main()
{
	//struct sockaddr_in server = { 0 }; //struct. must be zero-d out. later cast to (SOCKADDR*)
	//struct sockaddr *clientInfo;//later filled in by call to accept()
	//struct sockaddr_in *clientIPv4info;// represent a pointer to a IPv4 socket address
	//SOCKET listenfd;		//typedef UINT_PTR. socket LISTENING descriptor. Later used on call to accept().
	//SOCKET connectfd;
	//WSADATA wsaData;
	//char* serverIpAddress = "127.0.0.1";
	//u_short serverPort = 49500;	
	//u_short port = htons((u_short)serverPort);
	//clientInfo = (struct sockaddr*)malloc(sizeof(struct sockaddr));
	//clientIPv4info = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	//
	//if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)// Initialize Winsock
	//	printf("Error at WSAStartup()\n");

	//server.sin_family = AF_INET;	
	//server.sin_addr.s_addr = inet_addr(serverIpAddress);//INADDR_ANY; // inet_addr("127.0.0.1");
	//server.sin_port = port;//dynamic port range: 49152 - 65535 ;
	//listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//
	//// Bind the socket.
	//if (bind( listenfd, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR) 	
	//	return 1;//error

	webSocketServer ws = webSocketServer("127.0.0.1", "49500", "null", "/echo");
	ws.run();

	WSACleanup();
	return 0;
}