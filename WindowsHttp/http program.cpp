#include "httpclient.h"
#include "restApis.h"
using namespace std;


int main()
{

	httpInit();
	char* host, *resource, *port, *method;
	string response;
	port = "80";	
	method = "GET";
	DWORD timeout = 2000;
	/*WORKS*/
	// host = "tools.ietf.org";		
	// resource =  "/html/draft-ietf-hybi-thewebsocketprotocol-09.html";
	// host = "www.rfc-editor.org"; 
	// resource = "/rfc/rfc3629.txt";
	// host = "www.cs.cmu.edu";		
	// resource = "/afs/cs/academic/class/15213-f10/www/schedule.html";
	/* host = "www.crockford.com";		
	resource = "/javascript/performance.html";*/
	// host = "www.jqplot.com";		
	// resource = "/docs/files/usage-txt.html";				
	// host = "www.paulgraham.com";//"www.google.com";
	// resource = "/control.html";//"/finance/company_news?q=HKG:8277&output=rss";
	host = "www.indeed.com";
	//resource = "/ads/apisearch?publisher=2945599440210548&q=silverlight&l=chantilly+VA&limit=10";	
	
	

	resource = indeedResource("&q=html5+jquery&format=json&limit=20");	
	response = httpRequest(host, port, resource, method, timeout);
	std::cout << "response:\n\n" << response << std::endl;			
	return 0;
}

