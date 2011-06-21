
#include <iostream>
#include <string>
#include <regex>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "md5.h"
#include "md5_loc.h"


using namespace std;
//L"E:\C#\SocketsProgramming\WinSocketsCLR\Debug\WinSocketsCLR.dll";

void websocketsEx();
void createResponse(string origin, string location, 
	char challenge[], string key1, string key2);

char* calculateAnswerBytes(string key1, string key2, char challenge[8]);
char* createMD5Buffer(int result1, int result2, char challenge[8]);
long long longParse(string str);

int main()
{	
	
	websocketsEx();
	return 0;
}


//
//
void websocketsEx()
{
	match_results<const char*> results1, results2, getresults;	

	//challenge = input.substr(input.length() -8, input.length() - 1);	
	char challenge[] = { 154, 175, 196 ,155, 168,233,177,195 };// "\x154\x175\x196\x155\xMD5_SIZE8\x233\x177\x195";//wchar_t* challenge = L"\x154\x175\x196\x155\xMD5_SIZE8\x233\x177\x195";		
	std::string wsrequest = "GET /consoleappsample HTTP/1.1\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\nHost: localhost:8181\r\nOrigin: null\r\nSec-WebSocket-Key1: ? 17 08910o `31   0\r\nSec-WebSocket-Key2: 4i]1w n5- s0 J22Ww 1  -  2> P73\r\n\r\n";		
	string key1pattern1 = "(Sec-WebSocket-Key1:)[[:s:]]([^:]+\\r\\n)";
	string key2pattern = "(Sec-WebSocket-Key2:)[[:s:]]([^:]+\\r\\n)";
	string resourcePattern = "(GET)[[:s:]](/[[:alnum:]]+)";
	tr1::regex rx1(key1pattern1.c_str());
	tr1::regex rx2(key2pattern.c_str());
	tr1::regex rx3(resourcePattern.c_str());

	//match Sec-WebSocket-Key1 
	tr1::regex_search(wsrequest.c_str(), results1, rx1);
	std::string key1 = results1[2];		

	//match Sec-WebSocket-Key1 		
	tr1::regex_search(wsrequest.c_str(), results2, rx2);	
	std::string key2 = results2[2];
	std::cout << results2[2] << endl;	

	//match GET (resource)
	tr1::regex_search(wsrequest.c_str(), getresults, rx3);
	std::cout << getresults[2] << endl;
	string resouce = getresults[2];

	createResponse("null", "ws://127.0.0.1:49500/echo", challenge, key1, key2);
}

void createResponse(string origin, string location, 
	char challenge[], string key1, string key2)
{	
	char* answer = calculateAnswerBytes(key1, key2, challenge);//length == MD5_SIZE
	std::string bytes1 = "\x48\x54\x54\x50\x2F\x31\x2E\x31\x20\x31\x30\x31\x20\x57\x65\x62\x20\x53\x6F\x63\x6B\x65\x74\x20\x50\x72\x6F\x74\x6F\x63\x6F\x6C\x20\x48\x61\x6E\x64\x73\x68\x61\x6B\x65\x0D\x0A\x55\x70\x67\x72\x61\x64\x65\x3A\x20\x57\x65\x62\x53\x6F\x63\x6B\x65\x74\x0D\x0A\x43\x6F\x6E\x6E\x65\x63\x74\x69\x6F\x6E\x3A\x20\x55\x70\x67\x72\x61\x64\x65\x0D\x0A\x57\x65\x62\x53\x6F\x63\x6B\x65\x74\x2D\x4F\x72\x69\x67\x69\x6E\x3A\x20";	
	std::string bytes2 = "\x0D\x0A\x57\x65\x62\x53\x6F\x63\x6B\x65\x74\x2D\x4C\x6F\x63\x61\x74\x69\x6F\x6E\x3A\x20";
	location = "ws://127.0.0.1:49500/echo";
	origin = "null";
	std::string last4 = "\x0D\x0A\x0D\x0A";
	std::string handshake = bytes1 + origin + bytes2 + location + last4;		
	const char* handshake_bytes = handshake.c_str();	
	char* response = (char*)calloc(handshake.length() + MD5_SIZE, sizeof(char));
	for (int i = 0; i < handshake.length(); i++)
	{
		response[i] = handshake_bytes[i];
	}
	for (int i = 0; i < MD5_SIZE; i++)
	{
		response[handshake.length() + i] = answer[i];
	}
	for (int i = handshake.length(); i < handshake.length() + MD5_SIZE; i++)
		printf("%d\n", (unsigned char)response[i]);
}

char* calculateAnswerBytes(string key1, string key2, char challenge[8])
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
	for (int i = 0; i < MD5_SIZE; i++)
	{
		printf("%d %d\n", raw_answer[i], (unsigned char)sig[i]);
	}
    
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
	printf("raw answser bytes: ");
	for (int i = 0; i < MD5_SIZE; i++)		
		printf(" %d", (unsigned char)raw_answer[i]);	
	printf("\n\n");
		
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

void regex_example1()
{
	string input = "ws://example.com/method";
	tr1::regex rx("(ws://)([[:alpha:]].+)");
	match_results<const char*> matches;
	tr1::regex_search(input.c_str(), matches, rx);
	for (int k = 0; k < matches.length(); k++)
	{
		std::match_results<const char*>::value_type v = matches[k];
		string s = v = matches[k];
		std::cout << v << endl;
	}
}
