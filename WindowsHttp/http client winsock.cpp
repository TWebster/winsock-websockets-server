#include <string>
#include <istream>
#include <iostream>
#include <stdio.h>
#include <winsock2.h>//#include "winsock2.h"
#include <windows.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib,"Ws2_32.lib")	//why this wasn't in MSDN's documentation, I don't know.

#define MAX_THREADS 3
#define MAX_HTTP_BUFFER 1048576

static DWORD   threadID;
static HANDLE  hThread;
static SOCKET clientfd;

struct socketInfoT
{
	char* hostname;
	char* ipaddress;
	char* port;
	struct sockaddr_in *server;
	SOCKET *clientfd;
};

void open_clientfd(char* host_addr, char* port, SOCKET* clientfd, sockaddr_in * serverInfo);
void socketInfoT_new(socketInfoT * s, std::string host_name, char* port);

int main() 
{  	
	int received;
	char* recvbuf = (char*)calloc(MAX_HTTP_BUFFER,sizeof(char));
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
		printf("Error at WSAStartup()\n");
	//208.43.96.32/ads/apisearch?publisher=2945599440210548&q=java&l=austin%2C+tx
	//http://api.indeed.com/ads/apisearch?publisher=2945599440210548&q=java&l=austin%2C+tx&limit=20

	socketInfoT s;
	std::string host = "www.crockford.com";		
	std::string resource = "/javascript/performance.html";
	//std::string host = "www.indeed.com";//
	//std::string resource = "/ads/apisearch?publisher=2945599440210548&q=java&l=austin%2C+tx&limit=10";//
	//std::string host = "www.paulgraham.com";//"www.google.com";
	//std::string resource = "/control.html";//"/finance/company_news?q=HKG:8277&output=rss";
	socketInfoT_new(&s, host, "80");	

	SOCKET clientfd = *s.clientfd;
	std::string header = "GET " + resource  + " HTTP/1.1\r\n"
		"Host: " + host + "\r\n" 
		"Connection: keep-alive\r\n"
		"User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/535.1 (KHTML, like Gecko) Chrome/13.0.782.24 Safari/535.1\r\n"
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
		"Accept-Encoding: gzip,deflate,sdch\r\n"
		"Accept-Language: en-US,en;q=0.8\r\n"
		"Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.3\r\n\r\n";
	const char* headerbytes = header.c_str();
	
	int sent = send(*s.clientfd, headerbytes, strlen(headerbytes), 0);//SEND does not block
	printf("Sent %d bytes.\n\n\%s\n", sent, headerbytes);
	
	do//receive on main thread (recv must be called on main thread?)
	{		
		recvbuf = (char*)calloc(MAX_HTTP_BUFFER, sizeof(char));
		received = recv(*s.clientfd, recvbuf, MAX_HTTP_BUFFER, 0);//should block thread
		printf("\nreceived %d bytes:\n%s\n", received, recvbuf);			
	} while (received > 0);

	WSACleanup();
	return 0;
}


void socketInfoT_new(socketInfoT * s, std::string host_name, char* port)
{
	//struct socketInfoT s;	
	struct hostent* remoteHost;  
	remoteHost = gethostbyname(host_name.c_str());
	struct in_addr my_in_addr;	
	my_in_addr.s_addr = *(u_long *) remoteHost->h_addr_list[0];		
	s->ipaddress = inet_ntoa(my_in_addr);	
	s->clientfd = (SOCKET*)malloc(sizeof(SOCKET));	
	s->server = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	s->port = port;	
	open_clientfd(s->ipaddress, s->port, s->clientfd, s->server);
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
