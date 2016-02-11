// server.cpp file
// Author: Blong Thao
// CSS 432 Lagasse
// April 14, 2015

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
using namespace std;

#define BUFSIZE     1500
#define BACKLOG		10

int port, repetition, serverSd = -1;

// ------- thread_server ----------------------------------------------------

void *thread_server(void *data_struct)
{
	// Socket from main, number of reads, databuf, time stamps
	int socket = *(int*)data_struct, numOfReads = 1;
	char* databuf[BUFSIZE];
	struct timeval startTime, stopTime;

	gettimeofday(&startTime, NULL);

	// Gets the amount of reads
	for (int i = 0; i < repetition; i++)
	{
		for (int nRead = 0;
			(nRead += read(socket, databuf, BUFSIZE - nRead) ) < BUFSIZE;
			++numOfReads);
		++numOfReads;		// add repetitions to number of reads
	}

	write(socket, &numOfReads, sizeof(numOfReads));

	gettimeofday(&stopTime, NULL);

	long dataReceivingTime = (stopTime.tv_sec - startTime.tv_sec) * 1000000 +
						 (stopTime.tv_usec - startTime.tv_usec);
	cout << "data-receiving time = " << dataReceivingTime << " usec\n";
	close(socket);
	return 0;
}


// ------- Main ---------------------------------------------------------------
// Must receive the following two arguments:
//	port:		a server IP port
//	repetition: the repetition of sending a set of data buffers

int main( int argc, char* argv[] )
{
	if ( argc != 3 )
	{
		cerr << "Args: ./server port repetition\n";
		return 1;
	}
	cerr << "Starting Server.. \n";
	port = atoi(argv[1]);
	repetition = atoi(argv[2]);

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
