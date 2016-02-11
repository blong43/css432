// retriever.cpp file
// 
// Blong Thao
// CSS 432
// Assignment 2
// May 5th, 2015
// ----------------------------------------------------------------------------

#include <sys/types.h>    // socket, bind
#include <sys/socket.h>   // socket, bind, listen, inet_ntoa
#include <netinet/in.h>   // htonl, htons, inet_ntoa
#include <arpa/inet.h>    // inet_ntoa
#include <netdb.h>        // gethostbyname
#include <fcntl.h>        // read, write, close
#include <strings.h>      // bzero
#include <netinet/tcp.h>  // SO_REUSEADDR
#include <sys/uio.h>      // writev
#include <sys/time.h>     // time_t, clock_t
#include <stdlib.h>
#include <iostream>       // cerr
#include <unistd.h>
#include <string.h>
#include <fstream>
#include <sstream>
using namespace std;


int main(int argc, char* argv[])
{
	// Takes input from command line and parses the server address
	// and file that is being requested.

	if (argc != 4)
		cout << "Need 4 Args: ./xxx serverIP port abs_path\n";
	
	int port = atoi(argv[1]);
	char *serverIp = argv[2] + '\0';
	
	// Host must be an IP address; e.g. 127.0.0.
	struct hostent* host = gethostbyname( serverIp );
	if ( host == NULL ) {
		cerr << "Cannot find hostname\n";
		return -1;
	}

	sockaddr_in sendSockAddr;
	bzero( (char*)&sendSockAddr, sizeof( sendSockAddr ) );
	sendSockAddr.sin_family       = AF_INET; // Address Family Internet
	sendSockAddr.sin_addr.s_addr  =
		inet_addr( inet_ntoa( *(struct in_addr*)*host->h_addr_list) );
	sendSockAddr.sin_port         = htons( port );
    inet_aton(serverIp, &sendSockAddr.sin_addr);

	// cout << inet_ntoa(sendSockAddr.sin_addr) << endl;

	// Opens a stream-oriented socked with the Internet address family.
	int clientSd = socket( AF_INET, SOCK_STREAM, 0 );
	if (clientSd < 0 ) {
		cerr << "Cannot open a client TCP socket\n";
		return -1;
	}

	// Find a server to connect to
	connect( clientSd, ( sockaddr* )&sendSockAddr,	sizeof( sendSockAddr) ) ;

	// Issue a GET request to the server for the requested file.
	char buffer[1024];
	string request = "GET ";
	request += argv[3];
	request += " HTTP/1.0";
	request += "\r\n\r\n";
	write(clientSd, request.c_str(), request.length());
	
	string response;
	size_t len;
	bool isChecked = false;
	while( (len = read(clientSd, buffer, 1023)) > 0) {
		buffer[len] = '\0';
		response.append( buffer );
		
		// Make one check to find the end of the response line and take it out
		if (!isChecked && response.find("\r")) {
			istringstream iss (response);
			int statusCode;
			string statusMsg, protocal;
			iss >> protocal >> statusCode >> statusMsg;

			// If the server returns an error code instead of an OK code,
			if (statusCode != 200) {
				cout << "Status Code: " << statusCode << "\r\n"
					 << "Status Msg: " << statusMsg << "\r\n";
				return 1;
			}

			isChecked = true;
		}
	}
	cout << response << endl;
	int pos = response.find("\r\n\r\n");
	response = response.substr(pos + 4);

	// When the file is returned by the server, the retriever outputs the file
	// to the screen and to the file system. 

	ofstream file ("test.txt");
	if (file)
	{
		file << response;
	}
	
	// Exit after receiving the response.
	close(clientSd);
	return 0;
}
