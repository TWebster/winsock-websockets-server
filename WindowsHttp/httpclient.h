#include <string>
#include <istream>
#include <iostream>
#include <stdio.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib,"Ws2_32.lib")	
using namespace std;
#define MAX_HTTP_BUFFER 10000000//1048576

struct httpResponseT
{
	string header;
	string content;
	string resource;
	string transfer_encoding;	
	string status_code;
	
};

struct httpInfoT
{
	char* hostname;
	char* ipaddress;
	std::string port;
	struct sockaddr_in *server;
	SOCKET *clientfd;
	httpResponseT response;
};

//string httpRequest(char* host, char* port, char* resource, char* method, DWORD timeout);
string httpRequest(string host, string port, string resource, string method, DWORD timeout);

void open_clientfd(const char* host_addr, const char* port, SOCKET* clientfd, sockaddr_in * serverInfo, DWORD timeout);

void httpInfoT_new(httpInfoT * s, std::string host_name, const char* port, DWORD timeout);

void httpInit();