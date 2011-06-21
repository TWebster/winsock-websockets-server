#pragma once
//http://msdn.microsoft.com/en-us/library/ms737550(v=VS.85).aspx
#include <string>
#include <iostream>
#include <istream>
#include <winsock2.h>//#include "winsock2.h"
#include <windows.h>
#include <regex>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib,"Ws2_32.lib")

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "md5.h"
#include "md5_loc.h"
using namespace std;



class webSocketServer
{	
	#define BUFSIZE 1024
	#define CHALLENGE_LENGTH 8;
public:
	webSocketServer(char* ipAddress = "127.0.0.1", char* port = "49500", char* origin = "null", char* path = "/echo")
	{		
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)// Initialize Winsock
			printf("Error at WSAStartup()\n");

		this->path = string(path);
		this->origin = string(origin);		
		this->port = string(port);
		this->ipAddress = ipAddress;	
		this->location = "ws://" + string(ipAddress) + ":" + this->port + this->path;		
		struct sockaddr_in s = { 0 }; this->server = s;
		this->clientInfo = (struct sockaddr*)malloc(sizeof(struct sockaddr));
		this->clientIPv4info = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
		this->server.sin_family = AF_INET;	
		this->server.sin_addr.s_addr = inet_addr(this->ipAddress);//INADDR_ANY; // inet_addr("127.0.0.1");
		this->server.sin_port = htons(atoi(port));//dynamic port range: 49152 - 65535 ;
		this->listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (bind(this->listenfd, (SOCKADDR*)(&this->server), sizeof(server)) == SOCKET_ERROR)
		{
			printf("Error on bind.\n");
			return;
		}			
		this->recvbuf = (char*)calloc(BUFSIZE, sizeof(char));		
	}
	~webSocketServer( ){ }
	void run()
	{		
		//a run method to handle potentially > 1 websocket clients. 
		if (listen(this->listenfd, SOMAXCONN ) == SOCKET_ERROR) 	
		{
			printf("Error listening on socket.\n");
			return;
		}								
		printf("Web socket server is listening on socket. %s:%d\n", inet_ntoa(this->server.sin_addr), ntohs(this->server.sin_port));

		//while (true)//continuously accept new client connections
		//{
		this->connectfd = accept(this->listenfd, this->clientInfo, NULL);
		if (this->connectfd != NULL)
			this->onConnected();
		//might fork a thread here.

		//}
	}

	int Send(string message)
	{
		int nbytes = message.length() + 2;
		char* frame = (char*)calloc(nbytes, sizeof(char));
		frame[0] = 0x00;
		for (int i = 0; i < message.length(); i++)
		{
			frame[1 + i] = message[i];
		}
		frame[nbytes - 1] = 0xFF;
		return send(connectfd, frame, nbytes, 0);		
	}

private:
	SOCKET listenfd;
	SOCKET connectfd;
	char* recvbuf;	
	struct sockaddr_in server;
	struct sockaddr *clientInfo;//later filled in by call to accept()
	struct sockaddr_in *clientIPv4info;// represent a pointer to a IP
	string port;
	char* ipAddress;
	string path;
	string origin;
	string location;
	
		void onConnected()
	{
		this->clientIPv4info = (struct sockaddr_in*)this->clientInfo;	
		printf("Browser client connected: %s:%d\n", inet_ntoa(this->clientIPv4info->sin_addr), ntohs(this->clientIPv4info->sin_port));		

		int sent = negotiateHandshake();

		printf("Enter line(s) to send to websocket client:\n");
		while (sent > 0)
		{
			//accept again? different thread?			
			char message[256];
			std::cin.getline(message, 256);
			//std::cin >> message;			
			sent = this->Send(message);			
		}
	}

	//returns > 0 if web socket client handshake succeeds
	int negotiateHandshake()
	{
		//get client handshake
		int received = recv(connectfd, recvbuf, BUFSIZE, 0);
		printf("%d bytes received from client request.\n\n", received);
		printf("request data:\n%s\n", recvbuf);
		char* request = strdup(recvbuf);
		char* challenge = (char*)calloc(8,sizeof(char));
		char *response = NULL;
		string key1, key2, resource;
		if (!parseHandshakeRequest(request, &key1, &key2, &resource, &challenge))
			return false;
		
		createHandshakeResponse(challenge, key1,key2, &response);
		
		//verify our response against Nugget web socket server:
		//nuggetClient n = nuggetClient();
		//char* response2 = n.SendReceive(request);
		//printf("%s\n", response);
		//printf("%s\n", response2);

		int sent = send(connectfd, response, strlen(response), 0);		
		return sent;
	}

	//returns false if the handshake request fails to parse
	//out: challenge
	bool parseHandshakeRequest(char* request, string * key1, string * key2, string* resource, 
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
	void createHandshakeResponse(char* challenge, string key1, string key2, 
		char** response)//out
	{	
		char* answer = calculateAnswerBytes(key1, key2, challenge);//length == MD5_SIZE
		std::string handshake = "HTTP/1.1 101 Web Socket Protocol Handshake\r\n"
			"Upgrade: WebSocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Origin: " + this->origin + "\r\n" +
			"Sec-WebSocket-Location: " + this->location + "\r\n\r\n";

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
	char* calculateAnswerBytes(string key1, string key2, char* challenge)
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

	char* createMD5Buffer(int result1, int result2, char challenge[8])
	{
		//assuming this machine is Little Endian:	
		//char* raw_answer = (char*)calloc(MD5_SIZE,sizeof(char));
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

		return raw_answer;//raw_answer;
	}

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


};

