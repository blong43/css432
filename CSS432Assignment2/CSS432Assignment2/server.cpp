// server.cpp file
// Author: Blong Thao
// CSS 432 Lagasse
// May 5, 2015

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
#include <pthread.h>
#include <sstream>
#include <fstream>
#include <string.h>
using namespace std;

#define BUFSIZE     1500
#define BACKLOG		10

int port, serverSd = -1;

// ------- thread_server ----------------------------------------------------

void *thread_server(void *data_struct)
{
	// The client socket, buffer
	int clientSd = *(int*)data_struct; 
	char buffer[1024];
	stringstream ss;
	string request, breakCond = "\r\n\r\n";
	size_t bytesRead;
	
	// Receives data from client request
	while( (bytesRead = read(clientSd, buffer, 1023)) > 0) {	
		buffer[bytesRead] = '\0';
		request += buffer;
		if ( request.find( breakCond ) )
			break;
	}
	cout << "Request: " << request << endl;
	ss.str(request);

	string method, path, response;

	ss >> method >> path;

	//cout << "Method: " << method << endl;
	//cout << "Path:   " << path   << endl;

	if (path[0] != '/')
	{
		response = "HTTP/1.0 400 BadRequest\r\n Connection: Closed\r\n\r\n";
		path = "BadRequest.html";
	} 
	else if (path.find("../") != -1)
	{
		response = "HTTP/1.0 403 Forbidden\r\n Connection: Closed\r\n\r\n";
		path = "Forbidden.html";
	}
	else if (path.find("SecretFile") != -1)
	{
		response = "HTTP/1.0 401 Unauthorized\r\n Connection: Closed\r\n\r\n";
		path = "Unauthorized.html";
	}
	else
	{
		// Got past all checks, remove the '/' if not root
		if (path.length() == 1) {
			path = "Ok.html";
		}
		else {
			path = path.substr(1, path.length());
		}
		// Find file from path
  		int openFile = open (path.c_str(), O_RDONLY);
		//cout << "openFile: " << openFile << endl;

		// File not found
		if ( openFile < 0 ) {
			response = "HTTP/1.0 404 FileNotFound\r\n Connection: Closed\r\n\r\n";
			path = "NotFound.html";
		}
		else {
			response = "HTTP/1.0 200 OK\r\n Connection: Closed\r\n\r\n";
		}
	}

	ifstream file(path.c_str());

	if(file)
	{
		string temp;

		while(getline(file, temp))
			response += temp + "\r\n";

		file.close();
	}
	
	//write response
	int wrote = write(clientSd, response.c_str(), response.length());
	if ( wrote < 0 )
		cout << "Did not write: " << wrote << endl;

	close(clientSd);
	pthread_exit(NULL);
	return 0;
}


// ------- Main ---------------------------------------------------------------
// Must receive the following two arguments:
//	port:		a server IP port

int main( int argc, char* argv[] )
{
	port = 8156;

	sockaddr_in acceptSockAddr;
	bzero( (char*)&acceptSockAddr, sizeof( acceptSockAddr ) );
	acceptSockAddr.sin_family      = AF_INET;
	acceptSockAddr.sin_addr.s_addr = htonl( INADDR_ANY );
	acceptSockAddr.sin_port        = htons( port );

	if ( (serverSd = socket( AF_INET, SOCK_STREAM, 0 )) < 0)
	{
		cerr << "Cannot open a server TCP socket\n";
		return 1;
	}

	// prompt OS to release server port as soon as the server process terminate
	const int on = 1;
	if ( setsockopt( serverSd, SOL_SOCKET, SO_REUSEADDR,
		 (char*)&on, sizeof(int) ) < 0 ) {
			 cerr << "setsockopt SO_REUSEADDR failed\n";
			 return 1;
	}

	// Bind socket to its local address
	if ( (bind ( serverSd, ( sockaddr* )&acceptSockAddr,
		sizeof( acceptSockAddr ) ) < 0 ) )
	{
		cerr << "Cannot bind the local address to the server socket!\n";
		return 1;
	}

	// Instruct the OS to listen to up to n connection requests from client
	listen( serverSd, BACKLOG );

	// Receive a request from a client, return a new socket to specific request
	sockaddr_in newSockAddr;
	socklen_t newSockAddrSize = sizeof( newSockAddr );
	
	// Signifies that server is up and running
	cout << "server: now accepting client connections...\n";

	// Accepts connections until the process is killed
	while(1)
	{
		// Accepting connections
		int newSd = accept( serverSd, ( sockaddr*)&newSockAddr, &newSockAddrSize );
		if ( newSd < 0)
			cerr << "newSd: accept error\n";

		// Create the thread to run the thread_server function with the newSd
		pthread_t thread;
		if (pthread_create(&thread, NULL, thread_server, (void*)&newSd) < 0 )
		{
			cerr << "Thread not created\n";
			return 1;
		}

		// Merge threads back to avoid wasting resources
		pthread_join(thread, NULL);
	}

	return 0;
}
