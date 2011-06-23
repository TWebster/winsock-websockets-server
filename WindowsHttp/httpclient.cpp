#include "httpclient.h"
#include <regex>
#include <hash_map>
#define MIN_RECEIVED 1024	//minimum received bytes


static string string_replace(string operand, char x, char y);
static void parseResponseHeaders(const char* received_bytes, httpResponseT * response)
{
	char* transfer_encoding_pattern = "(Transfer-Encoding):[[:s:]]([[:alnum:]]+)\r\n";
	char* status_code_pattern = "(HTTP/1.1[[:s:]])([[:d:]]{3}).*\r\n";
	
	//std::hash_map<string,string> map;
	match_results<const char*> m;
	tr1::regex rx;

	rx = tr1::regex(status_code_pattern);	
	tr1::regex_search(received_bytes, m, rx);	
	response->status_code = m[2];	

	if (response->status_code == "200")
	{
		m = match_results<const char*>();
		rx = tr1::regex(transfer_encoding_pattern);	
		tr1::regex_search(received_bytes, m, rx);	
		response->transfer_encoding = m[2];
	}		
	
}


//better to send the literal payload to javascript
//and parse the response in javascript. C/C++ is really bad for regex and string manipulation.
//static string parseResponseContent(string received, httpResponseT * response)
//{	
//	//JSON parsing assumes transfer encoding: "chunked" and the string ends with '0' , not 0x00;
//	//(assumes indeed.com API)
//	if (response->transfer_encoding == "chunked")
//	{
//		//char* json_pattern = "(1ff8)(([[:alnum:]]|[[:s:]])+)";
//		//char* json_pattern = "(1ff8)[\s]*";		
//		char* json_pattern = "(1ff8)";
//		//received = string_replace(string_replace(received, '\r', ' '), '\n', ' ');						
//		tr1::regex rx(json_pattern);
//		match_results<const char*> m;
//		string json;
//		m = match_results<const char*>();
//		tr1::regex_search(received.c_str(), m, rx);	
//		if (!m.empty())
//		{
//			json = received.substr(4, received.length());
//			/*json = m[2];
//			for (int i = 0; i < m.length(); i++)
//			{
//				std::cout << m[i] << endl;
//			}*/
//			return json;
//		}		 		
//	}		
//
//	return received;
//}

string httpRequest(string host, string port, string resource, string method, DWORD timeout)
{
	httpInfoT h;		
	const char* header_bytes;
	int received_per_call = 0, received_total = 0, sent = 0;
	char* recvbuf  = (char*)calloc(MAX_HTTP_BUFFER, sizeof(char));

	httpInfoT_new(&h, host, port.c_str(), timeout);	
	SOCKET clientfd = *h.clientfd;
	std::string header = "GET " + string(resource)  + " HTTP/1.1\r\n"			
		"Host: " + host + "\r\n" 		
		"Accept: text/html,application/xhtml+xml,application/json,application/javascript,application/xml;q=0.9,*/*;q=0.8\r\n"
		"Connection : Keep-Alive\r\n\r\n";
	header_bytes = header.c_str();
	sent = send(*h.clientfd, header_bytes, strlen(header_bytes), 0);//SEND does not block
	printf("HTTP %s : %d bytes.\n\n\%s\n", method, sent,  header_bytes);
	
	received_per_call = recv(*h.clientfd, recvbuf, MAX_HTTP_BUFFER, 0);
	parseResponseHeaders(recvbuf, &h.response);
	h.response.header += string(recvbuf);
	string response;
	while (received_per_call > 0 && h.response.status_code == "200")
	{		
		recvbuf = (char*)calloc(MAX_HTTP_BUFFER, sizeof(char));
		received_per_call = recv(*h.clientfd, recvbuf, MAX_HTTP_BUFFER, 0);//should block thread					
		//printf("\nreceived %d bytes:\n\n", received_per_call);	
		//printf("\n%s\n", recvbuf);				
		response += string(recvbuf);//h.response.content += parseResponseContent(recvbuf, & h.response);						
		received_total += received_per_call;		
	} 
	h.response.content = response;
	closesocket(*h.clientfd);
	return response;
}

void httpInit()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
		printf("Error at WSAStartup()\n%d\n", wsaData);
}
void httpInfoT_new(httpInfoT * s, std::string host_name, const char* port, DWORD timeout)
{
	//struct httpInfoT s;	
	struct hostent* remoteHost;  
	remoteHost = gethostbyname(host_name.c_str());
	struct in_addr my_in_addr;	
	my_in_addr.s_addr = *(u_long *) remoteHost->h_addr_list[0];		
	s->ipaddress = inet_ntoa(my_in_addr);	
	s->clientfd = (SOCKET*)malloc(sizeof(SOCKET));	
	s->server = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
	s->port = port;	
	open_clientfd(s->ipaddress, s->port.c_str(), s->clientfd, s->server, timeout);
}


//out: sockaddr_in, SOCKET
//
void open_clientfd(const char* host_addr, const char* port, SOCKET* clientfd, sockaddr_in * serverInfo, DWORD timeout)
{
		//sockaddr_in: http://msdn.microsoft.com/en-us/library/ms740496(v=VS.85).aspx
		//SOCKET clientfd;//typedef UINT_PTR
		*clientfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		BOOL bOptVal = TRUE;				
		if (setsockopt(*clientfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&bOptVal, sizeof(BOOL)) != SOCKET_ERROR) {
			printf("Set SO_KEEPALIVE: ON\n");
		}
		//DWORD timeout = 2000;
		if (setsockopt(*clientfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(DWORD)) != SOCKET_ERROR) {
			printf("Set SO_RCVTIMEO: %d\n", timeout);
		}
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
		printf("connected to server at address: %s:%d\n", 
			inet_ntoa(serverInfo->sin_addr), ntohs(serverInfo->sin_port));		
}


//since TR1 regex documentation doesn't have a clear single-line/multi-line option, and because 
//the STL C++ basic_string::replace function is worthless...
static string string_replace(string operand, char x, char y)
{
	for (int i = 0; i < operand.length(); i++)
	{
		if (operand[i] == x)
			operand[i] = y;
	}
	return operand;
}
