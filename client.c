/* time_client.c - main */

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>                                                                            
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>                                                                                
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

struct pdu loadPdufromBuf(char data[], int responseSize){
    struct pdu returnPdu;
    returnPdu.type=data[0];
    memcpy(returnPdu.data, &data[1], responseSize-1);
    return returnPdu;
}

struct pdu createPdu(char pduType){
    struct pdu returnPdu;
    returnPdu.type = pduType;
    returnPdu.data[0] = 0;
    return returnPdu;
}


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
        fprintf(stderr, "usage: UDPtime [host [port]]\n");
		exit(1);
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
        printf("1) Content Registration \n2) Content Download \n3) List of On-Line Registered Content\n 4) Content De-Registration \n0) Exit \nPlease choose an option:\n");
        userChoiceBytes = read(0, &userChoiceData[0], 2); userChoiceData[1] = '\0'; 
        switch(userChoiceData[0]){
            case '0':
                run = 0;
                break;
            case '1':
                printf("Begin Registration \n");
                struct pdu contentRegistrationPDU = createPdu('R');
                char contentName[11];
                printf("Enter the 10 character content name. larger names will be shortened\n");
                read(0,&contentName,11);
                // memcpy(contentName,"a23456.txt",10);
                contentName[10] = 0;
                if (fopen(contentName,"rb") == NULL){
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
                
                int portlen = (int)((ceil(log10(reg_addr.sin_port))+1)*sizeof(char))-1;
                snprintf(contentRegistrationPDU.data, 100, "%s%s%d", peerName, contentName, reg_addr.sin_port);
                contentRegistrationPDU.data[10+10+portlen] = 0;
                printf("sending pdu with type %c and data %s \n",contentRegistrationPDU.type,contentRegistrationPDU.data);
                write(s, &contentRegistrationPDU, 10 + 10 + portlen+2);
                char tempResponseBuf[100];
                int responseLen = read(s, &tempResponseBuf[0], 100);
                struct pdu response = loadPdufromBuf(tempResponseBuf,responseLen);
                printf("response pdu type %c",response.type);
                while (response.type == 'E'){
                    printf("Enter a new peer name \n");
                    read(0,&peerName,11);
                    peerName[10] = 0;
                    memset(contentRegistrationPDU.data,0,100);
                    snprintf(contentRegistrationPDU.data, 100, "%s%s%d", peerName, contentName, reg_addr.sin_port);
                    contentRegistrationPDU.data[10+10+portlen] = 0;
                    write(s, &contentRegistrationPDU, 10 + 10 + portlen+2);
                    responseLen = read(s, &tempResponseBuf[0], 100);
                    response = loadPdufromBuf(tempResponseBuf,responseLen);   
                }
                
                // add error and ack handling, also fork process for handling downloads
                close(contentHostSocket); //DELETE THIS
                break;
            case '2':
                printf("Begin Content Download \n");
                printf("Enter the 10 character content name. larger names will be shortened\n");
                char downloadContentName[11];
                memcpy(downloadContentName,"1234567890",10);
                downloadContentName[10] = 0;
                struct pdu findContentRequest;
                findContentRequest.type='S';
                snprintf(findContentRequest.data, 100, "%s%s", peerName, downloadContentName);
                printf("sending pdu with type %c and data %s \n",findContentRequest.type,findContentRequest.data);
                write(s, &findContentRequest, 10 + 10 + 2);
                char findContentResponseBuffer[100];// = "R127.0.0.1:50000";
                int responseSize = read(s,findContentResponseBuffer,100);
                struct pdu findContentResponsePDU = loadPdufromBuf(findContentResponseBuffer,responseSize);
                if (findContentResponsePDU.type == 'R'){
                    printf("got error:%s", findContentResponsePDU.data);
                    break;
                }
                // Download the file
                char contentListPeerName[11]; //= //strtok(findContentResponsePDU.data,0);
                strncpy(contentListPeerName,&findContentResponsePDU.data[0],11);
                char contentListAddress[100]; //= //strtok(NULL,0);
                strcpy(contentListAddress,&findContentResponsePDU.data[11]);
                

                char *host = strtok(contentListAddress,":");
                printf("got file host ip:%s ", host);
                char *port = strtok(NULL,":");
                printf("port:%s\n",port);

                /* Create a stream socket	*/	
                int 	sd;
                struct	sockaddr_in server;
                struct	hostent		*hp;
                if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                    fprintf(stderr, "Can't creat a socket\n");
                    exit(1);
                }
                bzero((char *)&server, sizeof(struct sockaddr_in));
                server.sin_family = AF_INET;
                server.sin_port = htons(atoi(port));
                if (hp = gethostbyname(host)) 
                    bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
                else if ( inet_aton(host, (struct in_addr *) &server.sin_addr) ){
                    fprintf(stderr, "Can't get server's address\n");
                    break;
                }

                /* Connecting to the server */
                if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
                    fprintf(stderr, "Can't connect \n");
                    break;
                }

                char fileReadCharacters[16000];
                char* fileReadPointer = &fileReadCharacters[0];
	            int msgByteLength = 0;
                int i;
                char sbuf[8192];
                while((i = read(sd, &fileReadCharacters[msgByteLength], 16000)) > 0) {
                    msgByteLength += i;
                }

                if(fileReadCharacters[0] == '!') {
                    printf("There is an error with the file\n");
                } else if(fileReadCharacters[0] == 'd') {
                    printf("Downloading file to same directory as this program\n");
                    char* newPointer = &fileReadCharacters[1];
                    char txtEnding[] = ".txt";
                    sbuf[n-1] = 0;
                    FILE *fptr;
                    fptr = fopen(sbuf, "wb");
                    fputs(&fileReadCharacters[2], fptr);
                    fclose(fptr);
                } else {
                    printf("There is an error with the file\n");
                }
                close(sd);
                break;
            case '3':
                printf("Begin Content List \n");
                struct pdu contentListRequestPDU = createPdu('O');
                write(s, &contentListRequestPDU, 2);
                char listResponseBuffer[100];
                int exitContentList = 0;
                do{
                    int responseSize = read(s,listResponseBuffer,100);
                    struct pdu contentListResponsePDU = loadPdufromBuf(listResponseBuffer,responseSize);
                    printf("response: %s\n",listResponseBuffer);
                    if (contentListResponsePDU.type == 'A'){
                        printf("Content list complete\n");
                        exitContentList = 1;
                    }
                    else if (contentListResponsePDU.type == 'E'){
                        printf("No host with that file \n");
                    }
                    else if ((contentListResponsePDU.type == 'O') && (responseSize>20)){
                        char contentListPeerName[11]; //= //strtok(findContentResponsePDU.data,0);
                        char contentListFileName[11]; //= //strtok(NULL,0);
                        char contentListAddress[100]; //= //strtok(NULL,0);
                        strncpy(contentListPeerName,&contentListResponsePDU.data[0],11);
                        strncpy(contentListFileName,&contentListResponsePDU.data[11],11);
                        strcpy(contentListAddress,&contentListResponsePDU.data[22]);
                        printf("address str size %d \n",strlen(&contentListResponsePDU.data[22]));
                        if(contentListPeerName != NULL){
                            printf("name %s\n",contentListPeerName);
                        }
                        if(contentListFileName != NULL){
                            printf("file %s\n",contentListFileName);
                        }if(contentListAddress != NULL){
                            printf("addr %s\n",contentListAddress);

                        }
                    }
                    else{
                        printf("strange error \n");
                        exitContentList = 1;
                    }
                }while(!exitContentList);
                break;
            case '4':
                // NOT DONE 
                printf("Begin Deregistration \n");
                struct pdu contentDeregistrationPDU = createPdu('T');
                char deregisterContentName[11];
                printf("Select the content to deregister\n");
                read(0,&deregisterContentName,11);
                // memcpy(deregisterContentName,"a23456.txt",10);
                deregisterContentName[10] = 0;
                snprintf(contentDeregistrationPDU.data, 100, "%s%s", peerName, deregisterContentName);
                printf("sending pdu with type %c and data %s \n",contentDeregistrationPDU.type,contentDeregistrationPDU.data);
                write(s, &contentDeregistrationPDU, 10 + 10 + 2);
                
                int deregResponseLen = read(s, &tempResponseBuf[0], 100);
                struct pdu deregResponse = loadPdufromBuf(tempResponseBuf,deregResponseLen);
                printf("received: %c %s",deregResponse.type,deregResponse.data);
                if(deregResponse.type == 'R'){
                    printf("Enter a new peer name \n");
                    read(0,&peerName,11);
                    peerName[10] = 0;
                    printf("Select the content to deregister\n");
                    read(0,&deregisterContentName,11);
                    // memcpy(deregisterContentName,"a23456.txt",10);
                    deregisterContentName[10] = 0;
                    snprintf(contentDeregistrationPDU.data, 100, "%s%s", peerName, deregisterContentName);
                    printf("sending pdu with type %c and data %s \n",contentDeregistrationPDU.type,contentDeregistrationPDU.data);
                    write(s, &contentDeregistrationPDU, 10 + 10 + 2);
                    
                    int deregResponseLen = read(s, &tempResponseBuf[0], 100);
                    struct pdu deregResponse = loadPdufromBuf(tempResponseBuf,deregResponseLen);
                    printf("received: %c %s",deregResponse.type,deregResponse.data);
                }

                // close(contentHostSocket); //DELETE THIS
                break;
            default:
                printf("Invalid Choice \n\n");
                sleep(1);
        }
        
    }

	exit(0);
}
