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

    char peerName[11];
    printf("Enter a 10 character peer name. larger names will be shortened\n");
    //read(0,&peerName,11);
    memcpy(peerName,"1234567890",10);
    peerName[10] = 0;

    /* UDP SETUP SECTION */
	char	*host = "localhost";
	int	port = 3000;
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

    /*----- MENU SECTION-----*/
    int run = 1;
    char fileName[100];
    int userChoiceBytes = 0;
    char userChoiceData[2];
    while (run){
        printf("1) Content Registration \n2) Content De-Registration \n3) List of On-Line Registered Content 4) Download Content \n0) Exit \nPlease choose an option:\n");
        userChoiceBytes = read(0, &userChoiceData[0], 2); userChoiceData[1] = '\0'; 
        switch(userChoiceData[0]){
            case '0':
                run = 0;
                break;
            case '1':
                printf("Begin Registration \n");
                struct pdu contentRegistrationPDU;
                contentRegistrationPDU.type='R';
                char contentName[11];
                printf("Enter the 10 character content name. larger names will be shortened\n");
                // read(0,&contentName,11);
                memcpy(contentName,"a23456.txt",10);
                contentName[10] = 0;
                FILE* fileExists;
                fileExists = fopen(contentName,"rb");
				if (fileExists == NULL){
                    printf("file not found\n");
                    break;
                }

            	int	contentHostSocket;
                struct sockaddr_in reg_addr;
                contentHostSocket = socket(AF_INET, SOCK_STREAM, 0);
                reg_addr.sin_family = AF_INET;
                reg_addr.sin_port = htons(0);
                reg_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                bind(contentHostSocket, (struct sockaddr *)&reg_addr, sizeof(reg_addr));
                int alen = sizeof (struct sockaddr_in);
                getsockname(contentHostSocket, (struct sockaddr *) &reg_addr, &alen);
                int portlen = sizeof (reg_addr.sin_port);
                snprintf(contentRegistrationPDU.data, 100, "%s%s%d", peerName, contentName, reg_addr.sin_port);
                printf("sending pdu with type %c and data %s \n",contentRegistrationPDU.type,contentRegistrationPDU.data);
                write(s, &contentRegistrationPDU, 10 + 10 + portlen +1);
                // add error and ack handling
                close(contentHostSocket); //DELETE THIS
                break;
            case '2':
                printf("Begin Content Download \n");
                printf("Enter the 10 character content name. larger names will be shortened\n");
                char downloadContentName[11];
                memcpy(contentName,"a23456.txt",10);
                downloadContentName[10] = 0;
                struct pdu findContentRequest;
                findContentRequest.type='S';
                snprintf(findContentRequest.data, 100, "%s%s", peerName, downloadContentName);
                printf("sending pdu with type %c and data %s \n",findContentRequest.type,findContentRequest.data);
                char findContentResponseBuffer[100];
                int responseSize = read(s,findContentResponseBuffer,100);
                struct pdu findContentResponsePDU;
                findContentResponsePDU.type=findContentResponseBuffer[0];
                if (findContentResponsePDU.type == 'E'){
                    break;
                }
                strncpy(findContentResponsePDU.data, &findContentResponseBuffer[1], responseSize-1);
                // Download the file
                printf("got file host ip:%s", findContentResponsePDU.data);
                break;
            case '3':
                printf("Begin Content List \n");
                struct pdu contentListRequestPDU;
                contentListRequestPDU.type='O';
                contentListRequestPDU.data[0]=0;
                write(s, &contentListRequestPDU, 2);
                char listResponseBuffer[100];
                int exitContentList = 0;
                do{
                    int responseSize = read(s,listResponseBuffer,100);
                    struct pdu contentListResponsePDU;
                    contentListResponsePDU.type=listResponseBuffer[0];
                    if (contentListResponsePDU.type != 'E'){
                        printf("No host with that file \n");
                    }
                    else if (contentListResponsePDU.type != 'O'){
                        exitContentList = 1;
                    }
                    strncpy(contentListResponsePDU.data, &listResponseBuffer[1], responseSize-1);
                    printf("got content: \n",contentListResponsePDU.data);
                }while(!exitContentList);
                break;
            case '4':
                // NOT DONE 
                printf("Begin Deregistration \n");
                struct pdu contentDeregistrationPDU;
                contentDeregistrationPDU.type='R';
                char deregisterContentName[11];
                printf("Select the content to deregister\n");
                // read(0,&contentName,11);
                memcpy(deregisterContentName,"a23456.txt",10);
                deregisterContentName[10] = 0;
                snprintf(contentDeregistrationPDU.data, 100, "%s%s", peerName, deregisterContentName);
                printf("sending pdu with type %c and data %s \n",contentDeregistrationPDU.type,contentDeregistrationPDU.data);
                write(s, &contentDeregistrationPDU, 10 + 10 + portlen +1);
                
                close(contentHostSocket); //DELETE THIS
                break;
                break;
            default:
                printf("Invalid Choice \n\n");
                sleep(1);
        }
        
    }

	exit(0);
}
