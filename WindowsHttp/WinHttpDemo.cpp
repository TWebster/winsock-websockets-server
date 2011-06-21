
//#pragma comment(lib, "ole32.lib")
//#pragma comment(lib, "oleaut32.lib")
//#include <Windows.h>
#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <iostream>
using namespace std;
#include <string.h>

int main()
{
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    LPBYTE pszOutBuffer;
    BOOL bResults = FALSE;
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;

// Use WinHttpOpen to obtain a session handle.
hSession = WinHttpOpen( L"Internet Explorer example", 
    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
    WINHTTP_NO_PROXY_NAME, 
    WINHTTP_NO_PROXY_BYPASS, 0 );

// Specify an HTTP server.
if( hSession )
    hConnect = WinHttpConnect( hSession, L"www.my-new-gadget.com",
    INTERNET_DEFAULT_HTTPS_PORT, 0 );

// Create an HTTP request handle.
if( hConnect )
    hRequest = WinHttpOpenRequest( hConnect, L"GET", L"index.php",
    NULL, WINHTTP_NO_REFERER, 
    NULL, NULL);

// Send a request.
if( hRequest )
    bResults = WinHttpSendRequest( hRequest,
    WINHTTP_NO_ADDITIONAL_HEADERS, 0,
    WINHTTP_NO_REQUEST_DATA, 0, 
    0, 0 );

// End the request.
if( bResults )
    bResults = WinHttpReceiveResponse( hRequest, NULL );

HANDLE hFile = CreateFile("C:\\index.php", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

if (bResults)
{
    do     
    {        
        // Check for available data.        
        dwSize = 0;        
        if (!WinHttpQueryDataAvailable( hRequest, &dwSize))
            printf( "Error %u in WinHttpQueryDataAvailable.\n", GetLastError());        
        // Allocate space for the buffer.        
        pszOutBuffer = new byte[dwSize+1];        
        if (!pszOutBuffer)        
        {            
            printf("Out of memory\n");            
            dwSize=0;        
        }        
        else        
        {            
            // Read the Data.            
            ZeroMemory(pszOutBuffer, dwSize+1);            
            if (!WinHttpReadData( hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded))                
            {                
                printf( "Error %u in WinHttpReadData.\n", GetLastError());                
            }            
            else                
            {                        
                //printf("%s", pszOutBuffer); 
                DWORD wmWritten;
                bool fr = WriteFile(hFile, pszOutBuffer, dwSize, &wmWritten, NULL);
                int n = GetLastError();              
            }            
            // Free the memory allocated to the buffer.            
            delete [] pszOutBuffer;        
        }    
    } while (dwSize>0);
}

CloseHandle(hFile);
// Report any errors.
if (!bResults)    
    printf("Error %d has occurred.\n",GetLastError());
// Close any open handles.
if (hRequest) WinHttpCloseHandle(hRequest);
if (hConnect) WinHttpCloseHandle(hConnect);
if (hSession) WinHttpCloseHandle(hSession);

return 0;
}