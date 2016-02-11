// udp.cpp file
// Author: Blong Thao
// Course: CSS 432
// Date:   May 19th, 2015
// ----------------------------------------------------------------------------

#include "UdpSocket.h"
#include "Timer.h"

#define TIMEOUT 1500

// client packet sending functions
int clientStopWait( UdpSocket &sock, const int max, int message[] );
int clientSlidingWindow( UdpSocket &sock, const int max, int message[],
						int windowSize );

// server packet receiving fucntions
void serverReliable( UdpSocket &sock, const int max, int message[] );
void serverEarlyRetrans( UdpSocket &sock, const int max, int message[],
						int windowSize );

// Global Vars 
Timer timer;

// ----------------------------------------------------------------------------
// Case 2: Stop-and-wait Test
// Follows the stop-and-wait alg. in that a client writes a msg sequence number
// in msg[0], sends msg[] and waits until it recieves an int ACK from a server,
// while server receives msg[], copies the sequence # from msg[0] to an ACK
// and returns it to client.

int clientStopWait( UdpSocket &sock, const int max, int message[] ) {
	cerr << "client: stop-and-wait test:" << endl;

	int retransmits = 0;

	// transfer message[] max times
	for ( int i = 0; i < max; ) {
		message[0] = i;                           
		sock.sendTo( ( char * )message, MSGSIZE );
		
		timer.start();
		
		// If the client cannot receive an ack immediately, start a timer
		while (sock.pollRecvFrom( ) <= 0) {

			// If a timeout has happened, client must resend the same message
			if ( timer.lap() >= TIMEOUT ) {
				retransmits++;
				break;
			}
		}
		// Did not retransmit;
		if ( timer.lap() < TIMEOUT ) {
			// Receive ACK from server
			sock.recvFrom( ( char * ) message, MSGSIZE );
			i++;
		}
	}
	return retransmits;
}

// ----------------------------------------------------------------------------
// Case 2: Server Reliable
// Repeat receiving message[] and sending an ack at a server side
// max (=20,000) times using the sock object

void serverReliable( UdpSocket &sock, const int max, int message[] ) {
	cerr << "server reliable test:" << endl;

	// receive message[] max times
	for ( int i = 0; i < max; ) {
		sock.recvFrom( ( char * ) message, MSGSIZE );   // udp message receive

		if ( message[0] >= max ) {
			cerr << "ERROR: " << message[0] << " is larger than max\n";
			break;
		}

		// Drops the message[] if its a duplicate
		if ( message[0] == i ) {
			sock.ackTo( ( char * ) message, MSGSIZE);		// send ACK to client
			//cerr << " ACKed: " << message[0] << endl; 
			i++;
		}
	}
}

// ----------------------------------------------------------------------------
// Case 3: Sliding Window
// Follows the sliding window algorithm, a client keeps writing a msg seq # and
// sending msg[] as far as the number of in-transmit messages is less than a
// given window size, while the server receives msg[], record this msg's seq #
// in its array and returns as its ACK the minimum seq # of msgs it has not yet
// received (called a cumulative acknowledgement)
// Note: window size = number of msgs, not bytes (as it does in TCP)

int clientSlidingWindow(UdpSocket &sock, const int max, int message[], 
						int windowSize) {
	cerr << "Client sliding window test:" << endl;

	int retransmits = 0;         // Tracks the number of retransmitted messages
	int ack;					 // Hold the acknowledgment from the server
	int base = 0;				 // Tracks the client's message seq#
	int sequence = 0;			 // The sequence # counter

	// Send and receive messages
	while (sequence < max || base < max) {

		// Open sliding window for use
		if (base + windowSize > sequence && sequence < max) {	
			message[0] = sequence;
			sock.sendTo((char *)message, MSGSIZE);

			// Check if ack arrived
			if (sock.pollRecvFrom() > 0) {
				sock.recvFrom((char *)message, MSGSIZE);
				ack = message[0];

				if (ack == base) {
					base++;
				}
			}
			
			// Increment sequence and shrink the window
			sequence++;
		}
		// Sliding window is full, start timer
		else {
			timer.start();

			// Looks for ACKs and timeouts
			while (true) {
				// Found ack
				if (sock.pollRecvFrom() > 0) {
					sock.recvFrom((char *)message, MSGSIZE);
					ack = message[0];

					// Move base up if the server is ahead in messages 
					if (ack >= base) { 
						base = ack + 1;
					}
					// Resend the server the message that it needs
					else {
						message[0] = ack;
						sock.sendTo((char *)message, MSGSIZE);
						retransmits++;
					}
					break;
				}
				// Timed Out
				else if (timer.lap() > TIMEOUT) {
					// Resend message to server
					message[0] = base;
					sock.sendTo((char *)message, MSGSIZE);
					retransmits++;
					break;
				}
			}
		}
	}
	return retransmits;
}

// ----------------------------------------------------------------------------
// Case 3: Server Early Retrans
// repeats receiving message[] and sending an acknowledgement at a server side 
// max (=20,000) times using the sock object. Every time the server receives a 
// new message[], it must record this message's sequence number in its array and 
// returns a cumulative acknowledgement stating which message number it expects 
// to receive next.

void serverEarlyRetrans(UdpSocket &sock, const int max, int message[], 
						int windowSize) {
	cerr << "Server early retrans test:" << endl;

	int ack;				// Holds the acknowledgment
	bool serverArray[max];	// Holds messages that server needs from client
	int sequence = 0;		// The sequence # counter

	// Initialize all values in serverArray to false
	for (int i = 0; i < max; i++)
		serverArray[i] = false;

	while (sequence < max) {
		// Read the message from the client
		sock.recvFrom((char *)message, MSGSIZE);
		
		// Got needed message from client
		if (message[0] == sequence) {
			// Set the serverArray at sequence
			serverArray[sequence] = true;
			
			// Find the lowest number the server needs
			while (sequence < max) {
				if (serverArray[sequence] == false) {
					break;
				}
				sequence++;
			}
			
			// Send an ACK for previous seq #
			ack = sequence - 1;
		} 
		else {
			// Set the serverArray at message
			serverArray[message[0]] = true;
			
			// Send the number that is still needed 
			ack = sequence;
		}
		// Send the acknowledgment
		sock.ackTo((char *)&ack, sizeof(ack));
	}
}
