// client.cpp file
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
using namespace std;

// ------- Main ---------------------------------------------------------------
// Must receive the following six arguments:
//	port:		a server IP port
//	repetition: the repetition of sending a set of data buffers
//	nbufs:		the number of data buffers
//	bufsize:	the size of each data buffer (in bytes)
//	serverIp:	a server IP name
//	type:		the type of transfer scenario: 1, 2, or 3

int main(int argc, char *argv[])
{
	int port = 0, repetition = 0, nbufs = 0, bufsize = 0, type = 0;
	const char* serverIp;
	struct timeval startTime, lapTime, endTime;

	if (argc == 7)
	{
		port = atoi(argv[1]);
	    repetition = atoi(argv[2]);
		nbufs = atoi(argv[3]);
		bufsize = atoi(argv[4]);
		serverIp = argv[5] + '\0';
		type = atoi(argv[6]);
	}
	else
	{
		cout << "Args: ./client port repetition nbufs bufsize serverIp type\n";
		return -1;
	}

	// Ports below 1024 are busy doing other things.
	if (port < 1024) port += 1024;

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

	cout << inet_ntoa(sendSockAddr.sin_addr) << "/" << port << endl;

	// Opens a stream-oriented socked with the Internet address family.
	int clientSd = socket( AF_INET, SOCK_STREAM, 0 );
	if (clientSd < 0 ) {
		cerr << "Cannot open a client TCP socket\n";
		return -1;
	}

	// Find a server to connect to
	connect( clientSd, ( sockaddr* )&sendSockAddr,	sizeof( sendSockAddr) ) ;

	char databuf[nbufs][bufsize];

	gettimeofday(&startTime, NULL);

	// Chooses the specified write type and writes it repetition amount of time
	for (int i = 0; i < repetition; i++)
	{
		// Ensure databuf size is correct
		if (nbufs * bufsize == 1500)
		{
			// multiple writes
			if (type == 1)
			{
				for (int j = 0; j < nbufs; j++) {
					write( clientSd, databuf[j], bufsize );
				}
			}
			// writev: sends all data buffers at once
			else if (type == 2)
			{
				struct iovec vector[nbufs];
				for ( int j = 0; j < nbufs; j++) {
					vector[j].iov_base = databuf[j];
					vector[j].iov_len = bufsize;
				}
				writev( clientSd, vector, nbufs );
			}
			// single write
			else // type == 3
			{
				write( clientSd, databuf, nbufs * bufsize );
			}
		}
		else {
			cerr << "nbufs * bufsize != 1500\n";
		}
	}

	gettimeofday(&lapTime, NULL);

	long dataSendingTime = (lapTime.tv_sec - startTime.tv_sec) * 1000000 +
		                   (lapTime.tv_usec - startTime.tv_usec);

	// Holds the number of reads that gets passed over by server
	int numOfReads = 0;

	// Receive server acknowledgements
	while (1)
	{
		// Read from server
		int numBytes = read(clientSd, &numOfReads, sizeof(numOfReads));
		
		// End program if read not successful
		if ( numBytes == -1 ) {
			cerr << "Read Error!\n";
			return -1;
		}
		// Break when finished reading
		else if ( numBytes == 0 )
			break;
	}

	gettimeofday(&endTime, NULL);

	long roundTripTime = (endTime.tv_sec - startTime.tv_sec) * 1000000 +
		                   (endTime.tv_usec - startTime.tv_usec);

	cout << "Test " << type << ": data-sending time = " << dataSendingTime
		<< " usec, round-trip time = " << roundTripTime << " usec, #read = "
		<< numOfReads << endl;

	close(clientSd);
	return 0;
}
