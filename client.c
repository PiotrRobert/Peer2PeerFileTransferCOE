/* time_client.c - main */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>                                                                            
#include <netinet/in.h>
#include <arpa/inet.h>
                                                                                
#include <netdb.h>

#define	BUFSIZE 64

#define	MSG		"Any Message \n"


/*------------------------------------------------------------------------
 * main - UDP client for TIME service that prints the resulting time
 *------------------------------------------------------------------------
 */

// UDP Communi


struct pdu {
	char type;
	char data[100];
};

int main(int argc, char **argv) {
	char	*host = "localhost";
	int	port = 3000;
	char	now[100];		/* 32-bit integer to hold time	*/ 
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, n, type;	/* socket descriptor and socket type	*/

	switch (argc) {
	case 1:
		break;
	case 2:
		host = argv[1];
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "usage: UDPtime [host [port]]\n");
		exit(1);
	}

	memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;                                                                
        sin.sin_port = htons(port);
                                                                                        
    /* Map host name to IP address, allowing for dotted decimal */
        if ( phe = gethostbyname(host) ){
                memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
        }
        else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		fprintf(stderr, "Can't get host entry \n");
                                                                                
    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0)
		fprintf(stderr, "Can't create socket \n");
	
                                                                                
    /* Connect the socket */
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		fprintf(stderr, "Can't connect to %s \n", host);
		exit(0);
	}

	//(void)write(s, MSG, strlen(MSG));

	while(1) {

		char fileName[100];
		int fileNameBytes = 0;
		char packetArray[100];
		int packetBytes = 0;
		
		printf("\nEnter file name to download file, or type * to exit\n");

		struct pdu sendPacket;
		sendPacket.type = 'C';
		
		fileNameBytes = read(0, &sendPacket.data[0], 100); 
		if(sendPacket.data[0] == '*') {
			break;
		}

		write(s, &sendPacket, fileNameBytes + 1);
		
		FILE *fptr;
		int state = 0;

		while(packetBytes = read(s, &packetArray[0], 101)) {
			struct pdu getPacket;
			getPacket.type = packetArray[0];
			memcpy(&getPacket.data[0], &packetArray[1], 100);
			//getPacket.data[99] = '\0';
			if(getPacket.type == 'D') { //Getting data
				if(state == 0) {
					fptr = fopen(sendPacket.data, "wb");
					fputs(&getPacket.data[0], fptr);
					state = 1;
				} else {
					fputs(&getPacket.data[0], fptr);
				}
			} else if(getPacket.type == 'F') { //Final message
				if(state == 0) {
					fptr = fopen(sendPacket.data, "wb");
					fputs(&getPacket.data[0], fptr);
					state = 1;
				} else {
					fputs(&getPacket.data[0], fptr);
				}
				//printf("\nFinal Message\n");
				fclose(fptr);
				break;
			} else if(getPacket.type == 'E') { //Error pdu
				printf("\nThere is an error, file name does not exist.\n");
				break;
			} else if(getPacket.type == 'C') { //Getting filename
				printf("\nI should not be getting a filename\n");
				break;
			} else {
				printf("\nRandom unknown type letter\n");
				break;
			}
		}

	}

	///////////////////////////////////////////////////////////////////

	exit(0);
}
