//// crt_begthrdex.cpp
//// compile with: /MT
/*	
	this program demonstrates a potential race condition, where multiple socket clients
	write to a single (server-side) shared buffer. (Actually, the server-side threads running
	on_client_connected write to the shared buffer.
	If we remove the Mutex, we can reliably reproduce the race condition on every run.
	
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>//winsock

#include <windows.h>//threads
#include <process.h>

#pragma comment(lib, "wininet.lib")//winsock
#pragma comment(lib,"Ws2_32.lib")//winsock

#include <list>
#include <regex>
#include "md5\md5.h"
#include "md5\md5_loc.h"

HANDLE ghMutex; 
HANDLE hClientMain;

#define NUMCLIENTS 8
#define MESSAGE_LENGTH 31
#define NUM_MSGS_PER_CLIENT 12
char shared_buffer[MESSAGE_LENGTH + 1];	//+1 for null-termination
SOCKET listenfd;					//server's 1 listening SOCKET.
HANDLE connectHandles[NUMCLIENTS];	//server handles for handling client threads.
SOCKET connectDescriptors[NUMCLIENTS];
unsigned nClients = 0;				//current number of clients connected
const char* _host = "127.0.0.1";
const char* _port = "22201";

/*	SERVER */
void init_resources();	//initialization related to multithreading
void server_startup();
void server_dispose();
SOCKET accept_client();
unsigned __stdcall on_client_connected( void* pArguments );
void acquire_mutex();
void release_mutex();

/*	CLIENT	*/
unsigned threadIDs[NUMCLIENTS];		
HANDLE clientHandles[NUMCLIENTS];	//client threads (clientfd)
void open_clientfd(SOCKET & clientfd);
unsigned __stdcall clientStartupMainThread( void* pArguments );
unsigned __stdcall clientThread( void* pArguments );
void client_dispose();

int main() {

	init_resources();
	server_startup();
	Sleep(50);
	unsigned *threadID = (unsigned*)malloc(sizeof(int));
	hClientMain = (HANDLE)_beginthreadex(NULL, 0, &clientStartupMainThread, threadID, 0, threadID);


	//typically, server should continue to run until user signals to shuts down.
	//we don't have a user signal right now, so we allow the server to shut down when it has received the expected number of client connections.
	while (nClients < NUMCLIENTS) {
		SOCKET connectfd = accept_client();
		connectDescriptors[nClients] = connectfd;
		unsigned *threadid = (unsigned*)malloc(sizeof(int));
		connectHandles[nClients] = (HANDLE)_beginthreadex(NULL, 0, 
			&on_client_connected, 
			&connectDescriptors[nClients], //connectfd
			0, threadid);
		nClients += 1;
	}

	WaitForSingleObject(hClientMain, INFINITE);
	server_dispose();
	return 0;
}

unsigned __stdcall on_client_connected( void* pArguments ) {
	SOCKET connectfd = *(SOCKET*)pArguments;
	int received;
	do {
		acquire_mutex();
		received = recv(connectfd, shared_buffer, MESSAGE_LENGTH, 0);
		printf("server thread %d\tshared buffer: \"%s\"\n", GetCurrentThreadId(), shared_buffer);
		send(connectfd, shared_buffer, strlen(shared_buffer), 0);
		release_mutex();
	} while (received > -1);	//continue until client SOCKET closes.
	printf("Exiting server thread %u...\n", GetCurrentThread());
	return 0;
}

SOCKET accept_client() {
	struct sockaddr clientinfo = { 0 };
	SOCKET connectfd = accept(listenfd, &clientinfo, NULL);		
	struct sockaddr_in* ipv4info = (struct sockaddr_in*)&clientinfo;
	printf("Client connected on socket %d. Address %s:%d\n", connectfd, 
	inet_ntoa(ipv4info->sin_addr), ntohs(ipv4info->sin_port));		
	return connectfd;
}


/*	SERVER		*/
void server_startup() {
	WSADATA wsaData;	
	if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)// Initialize Winsock
		printf("Error at WSAStartup()\n");
	//create the SOCKET
	listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);	

	struct sockaddr_in server = { 0 };
	server.sin_family = AF_INET;	
	server.sin_addr.s_addr = inet_addr(_host);
	server.sin_port = htons((u_short)atoi(_port));
	
	// Bind the socket.
	if (bind( listenfd, (SOCKADDR*)&server, sizeof(server)) == SOCKET_ERROR) 	{
		printf("Error on server bind.\n");
		return;
	}		
	if (listen(listenfd, SOMAXCONN ) == SOCKET_ERROR)
	{
		printf("Error listening on socket.\n");
		return;
	}		
	printf("Server is listening on socket... %s:%d\n", inet_ntoa(server.sin_addr), ntohs(server.sin_port));	
}


void init_resources() {
	ghMutex = CreateMutex( 
	NULL,              // default security attributes
	FALSE,             // initially not owned
	NULL);             // unnamed mutex

	if (ghMutex == NULL) 
		printf("CreateMutex error: %d\n", GetLastError());
	
}

void acquire_mutex() {

	if ( WaitForSingleObject(ghMutex, INFINITE) != WAIT_OBJECT_0) { 
		printf("Error on WaitForSingleObject (thread %d)\n", GetCurrentThreadId()); 
	}
}

void release_mutex() {
	if (!ReleaseMutex(ghMutex)) { 
		printf("Error releasing Mutex on thread %d.\n", GetCurrentThreadId()); 
	}
}

void server_dispose() {
	CloseHandle(ghMutex);
	CloseHandle(hClientMain);
	for (int i = 0; i < NUMCLIENTS; i++) {
		closesocket(connectDescriptors[i]);
		CloseHandle(connectHandles[i]);
	}
	closesocket(listenfd);
}


/* CLIENT */


/*	
	Starts all the SOCKET client threads.
	TODO: allow user to input (1) number of threads (2) number of messages
*/
unsigned __stdcall clientStartupMainThread( void* pArguments ) {
		
	HANDLE hThread;	
	for (unsigned i = 0; i < NUMCLIENTS; i++) {
		
		unsigned* threadID = (unsigned*)malloc(sizeof(int));
		hThread = (HANDLE)_beginthreadex( NULL, 0, 
			&clientThread, 
			NULL, //arglist
			0, threadID );
		clientHandles[i] = hThread;
		threadIDs[i] = *threadID;
	}    
	for (int i = 0; i < NUMCLIENTS; i++) {
		hThread = clientHandles[i];
		WaitForSingleObject( hThread, INFINITE );
	}  
	client_dispose();
	printf("Exiting client Main startup thread %u...\n", GetCurrentThreadId());
	return 0;
}

unsigned __stdcall clientThread( void* pArguments ) {	
	SOCKET clientfd;
	open_clientfd(clientfd);
	int sent, sent_count = 0, threadid = GetCurrentThreadId();	
	char *expected, *actual;	//actual: server echoes message back
	expected =	(char*)calloc(MESSAGE_LENGTH, sizeof(char) + 1);
	actual =	(char*)calloc(MESSAGE_LENGTH, sizeof(char) + 1);
	expected[MESSAGE_LENGTH] = 0x00;
	//char* repeatstr = (char*)calloc(MESSAGE_LENGTH + 1, sizeof(char));		
	do {				
		sprintf(expected, "client thread:%d[%d]", threadid , sent_count);	
		for (int i = 0; i < MESSAGE_LENGTH; i++) {
			if (expected[i] == 0x00)
				expected[i] = 0x20;//fill rest of buffer with spaces.
		}
		sent = send(clientfd, expected, strlen(expected), 0);
		recv(clientfd, actual, MESSAGE_LENGTH, 0);
		if ( strcmp(expected, actual) != 0) {//compare echoed message.
			printf("\tRACE CONDITION\n");
			printf("\texpected:\n\t\t%s\n", expected);
			printf("\tactual:\n\t\t%s\n", actual);
		}
		sent_count += 1;
	} while (sent > -1 && sent_count < NUM_MSGS_PER_CLIENT);
	closesocket(clientfd);
	printf("Exiting client thread %u...\n", threadid);
	return 0;
}

void open_clientfd(SOCKET & clientfd) {
	clientfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in serverinfo = { 0 };
	//sockaddr_in
	serverinfo.sin_family = AF_INET;
	serverinfo.sin_addr.s_addr = inet_addr(_host);
	serverinfo.sin_port =  htons(atoi(_port));
	// Connect to server.
	if ( connect( clientfd, (SOCKADDR*)&serverinfo, sizeof(sockaddr_in) ) == SOCKET_ERROR) {
		printf( "Failed to connect.\n" );
		WSACleanup();
		return;
	}
	printf("Thread %d client is connected to server at address: %s:%d\n", 
		GetCurrentThreadId(),
		inet_ntoa(serverinfo.sin_addr), ntohs(serverinfo.sin_port));	
}


void client_dispose() {	
	for (int i = 0; i < NUMCLIENTS; i++) {
		CloseHandle( clientHandles[i] );
	}
}