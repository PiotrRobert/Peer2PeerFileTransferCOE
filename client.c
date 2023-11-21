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
#define BUFLEN		256	/* buffer length */
#define	MSG		"Any Message \n"
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))


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

int echod(int sd)
{

	char	*bp, buf[BUFLEN];
	int 	n, bytes_to_read;
	int readamnt = 0;	
	unsigned char  buffer[100];

	// Read Filename from client
	bytes_to_read = 0;
	n = read(sd, buf, BUFLEN);
	bytes_to_read += n;
	printf("read %d bytes", bytes_to_read);
	printf("filename is %s",buf);
	buf[bytes_to_read-1]=0;//Replace TCP terminating char with string terminating char

	// Read File, and write to client
	FILE* file;
	file = fopen(buf,"rb");

	// If file doesn't exist, notify client and exit
	if (file == NULL){
		printf("FILE %s DOES NOT EXIST",buf);
		write(sd, "!FILE DOES NOT EXIST", sizeof("!FILE DOES NOT EXIST"));
		close(sd);
		return(0);
	}

	// If file found, write contents to client then exit
	write(sd, "d", sizeof("d"));
	while(readamnt = fread(buffer,sizeof(*buffer),ARRAY_SIZE(buffer),	file))
	{
		write(sd, buffer, readamnt);
		printf("\nsent to client %d bytes: %s \n\n",readamnt, buffer);
		memset(buffer, 0, sizeof(buffer));
		sleep(1);

	}
	fclose(file);
	close(sd);
	printf("closed file and socket");

	
	return(0);
}


int main(int argc, char **argv) {

    char peerName[11]={0};
    printf("Enter a 10 character peer name. larger names will be shortened\n");
    int nameSize = read(0,&peerName,11);
    // int nameSize = 11;
    // memcpy(peerName,"qwertyuiop",10);
    printf("name size %d\n",nameSize);
    peerName[nameSize-1] = 0;
    // for (int idx=0; idx<11; idx++){
    //     printf("name char %d %c %d\n",idx,peerName[idx],peerName[idx]);
    // }

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
    
    // Storing files currently hosted
    int socketArray[100];
    char filenames[100][11];
    char peernames[100][11];
    int socketArrayIndex = 0;
    int zeroNames = 0;
    for (zeroNames = 0; zeroNames<100;zeroNames++){filenames[zeroNames][0]=0;}

    /* ----- Select Setup -----*/
    fd_set rfds, afds;    
    FD_ZERO(&afds);
    FD_SET(0, &afds); /* Listening on stdin */
    memcpy(&rfds, &afds, sizeof(rfds));

    /*----- MENU SECTION-----*/
    int run = 1;
    char fileName[100];
    int userChoiceBytes = 0;
    char userChoiceData[2];
    while (run){
        printf("1) Content Registration \n2) Content Download \n3) List of On-Line Registered Content\n 4) Content De-Registration \n0) Exit \nPlease choose an option:\n");
        
        memcpy(&rfds, &afds, sizeof(rfds));
        int selRet = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
        printf("\nselect returned : %d\n",selRet);
        if (FD_ISSET(0, &rfds)) {
            userChoiceBytes = read(0, &userChoiceData[0], 2); userChoiceData[1] = '\0'; 
            printf("user input: %c\n",userChoiceData[0]);
            switch(userChoiceData[0]){
                case '1':
                    printf("Begin Registration \n");
                    struct pdu contentRegistrationPDU = createPdu('R');
                    char contentName[11] = {0};
                    printf("Enter the 10 character content name. larger names will be shortened\n");
                    read(0,&contentName,11);
                    contentName[10] = 0;
					contentName[strcspn(contentName,"\r\n")] = 0;
                    if (fopen(contentName,"rb") == NULL){
                        printf("file not found\n");
                        break;
                    }
                    memcpy(filenames[socketArrayIndex], contentName,11);
                    // TCP Port Setup
                    int	contentHostSocket;
                    struct sockaddr_in reg_addr;
                    contentHostSocket = socket(AF_INET, SOCK_STREAM, 0);
                    reg_addr.sin_family = AF_INET;
                    reg_addr.sin_port = htons(0);
                    reg_addr.sin_addr.s_addr = htonl(INADDR_ANY);
                    bind(contentHostSocket, (struct sockaddr *)&reg_addr, sizeof(reg_addr));
                    int alen = sizeof (struct sockaddr_in);
                    getsockname(contentHostSocket, (struct sockaddr *) &reg_addr, &alen);
                    listen(contentHostSocket, 5);

                    // Server Response section
                    int portlen = (int)((ceil(log10(reg_addr.sin_port))+1)*sizeof(char))-1;
                    printf("host on port %d\n",ntohs(reg_addr.sin_port));
                    char portStr[11] = {0};
                    snprintf(portStr, portlen+1, "%d", ntohs(reg_addr.sin_port));
                    printf("port string is: %s\n",portStr);
                    memcpy(&contentRegistrationPDU.data[0],peerName,10);
                    memcpy(&contentRegistrationPDU.data[10],contentName,10);
                    memcpy(&contentRegistrationPDU.data[20],portStr,11);
                    contentRegistrationPDU.data[10+10+portlen] = 0;
                    printf("memcpy data: %s\n",contentRegistrationPDU.data);
                    
                    printf("sending pdu with type %c and data %s%s%s \n",contentRegistrationPDU.type,&contentRegistrationPDU.data[0],&contentRegistrationPDU.data[11],&contentRegistrationPDU.data[21]);
                    write(s, &contentRegistrationPDU, 10 + 10 + portlen+2);
                    char tempResponseBuf[100];
                    int responseLen = read(s, &tempResponseBuf[0], 100);
					if (responseLen == -1) {
						printf("Error reading response, exiting to menu\n");	
						break;				
					}
                    struct pdu response = loadPdufromBuf(tempResponseBuf,responseLen);
                    printf("response pdu type %c",response.type);
                    while (response.type == 'E'){
                        printf("Enter a new peer name \n");
                        read(0,&peerName,11);
                        peerName[10] = 0;
						peerName[strcspn(peerName,"\r\n")] = 0;
                        memset(contentRegistrationPDU.data,0,100);
                        memcpy(&contentRegistrationPDU.data[0],peerName,10);
                    	memcpy(&contentRegistrationPDU.data[10],contentName,10);
                    	memcpy(&contentRegistrationPDU.data[20],portStr,11);
                        contentRegistrationPDU.data[10+10+portlen] = 0;
                        write(s, &contentRegistrationPDU, 10 + 10 + portlen+2);
                        responseLen = read(s, &tempResponseBuf[0], 100);
                        response = loadPdufromBuf(tempResponseBuf,responseLen);   
                    }
                    printf("\ncontent host socket %d\n",contentHostSocket);                    
                    socketArray[socketArrayIndex] = contentHostSocket;
                    memcpy(peernames[socketArrayIndex], peerName,11);
                    FD_ZERO(&afds);
                    FD_SET(socketArray[socketArrayIndex], &afds);
                    FD_SET(0, &afds); /* Listening on stdin */
                    memcpy(&rfds, &afds, sizeof(rfds));
                    socketArrayIndex++;

                    break;
                case '2':
                    printf("Begin Content Download \n");
                    printf("Enter the 10 character content name. larger names will be shortened\n");
                    char downloadContentName[11]={0};
                    read(0,&downloadContentName,11);
                    downloadContentName[10] = 0;
					downloadContentName[strcspn(downloadContentName,"\r\n")] = 0;
                    struct pdu findContentRequest;
                    findContentRequest.type='S';
                    memcpy(&findContentRequest.data[0],peerName,10);
                    memcpy(&findContentRequest.data[10],downloadContentName,10);
                    // snprintf(findContentRequest.data, 100, "%s%s", peerName, downloadContentName);
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
                    while((i = read(sd, &fileReadCharacters[msgByteLength], 16000)) > 0) {
                        msgByteLength += i;
                    }

                    if(fileReadCharacters[0] == '!') {
                        printf("There is an error with the file\n");
                    } else if(fileReadCharacters[0] == 'd') {
                        printf("Downloading file to same directory as this program\n");
                        char* newPointer = &fileReadCharacters[1];
                        FILE *fptr;
                        fptr = fopen(downloadContentName, "wb");
                        // fptr = fopen("testaaaa.txt", "wb");
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
                    deregisterContentName[10] = 0;
					deregisterContentName[strcspn(deregisterContentName,"\r\n")] = 0;
                    memcpy(&contentDeregistrationPDU.data[0],peerName,10);
                    memcpy(&contentDeregistrationPDU.data[10],deregisterContentName,10);
                    contentDeregistrationPDU.data[10+10] = 0;
                    printf("memcpy data: %s\n",contentDeregistrationPDU.data);
                    printf("sending pdu with type %c and data %s \n",contentDeregistrationPDU.type,contentDeregistrationPDU.data);
                    write(s, &contentDeregistrationPDU, 10 + 10 + 2);
                    
                    int deregResponseLen = read(s, &tempResponseBuf[0], 100);
                    struct pdu deregResponse = loadPdufromBuf(tempResponseBuf,deregResponseLen);
                    printf("received: %c %s",deregResponse.type,deregResponse.data);
                    if(deregResponse.type == 'R'){
                        printf("Enter a new peer name \n");
                        read(0,&peerName,11);
                        peerName[10] = 0;
						peerName[strcspn(peerName,"\r\n")] = 0;
                        printf("Select the content to deregister\n");
                        read(0,&deregisterContentName,11);
                        deregisterContentName[10] = 0;
						deregisterContentName[strcspn(deregisterContentName,"\r\n")] = 0;
                        memcpy(&contentDeregistrationPDU.data[0],peerName,10);
                    	memcpy(&contentDeregistrationPDU.data[10],deregisterContentName,10);
                    
                        printf("sending pdu with type %c and data %s \n",contentDeregistrationPDU.type,contentDeregistrationPDU.data);
                        write(s, &contentDeregistrationPDU, 10 + 10 + 2);
                        
                        int deregResponseLen = read(s, &tempResponseBuf[0], 100);
                        struct pdu deregResponse = loadPdufromBuf(tempResponseBuf,deregResponseLen);
                        printf("received: %c %s",deregResponse.type,deregResponse.data);
                    }

                    // close(contentHostSocket); //WE NEED A BETTER THIS
                    break;
                case '0':
                    printf("deregister all files before exiting\n");
                    int deregIdx = 0;
                    for (deregIdx = 0; deregIdx < socketArrayIndex; deregIdx++){
                        struct pdu contentDeregistrationPDU = createPdu('T');
                        printf("deregister file %s\n",filenames[deregIdx]);
	                    memcpy(&contentDeregistrationPDU.data[0],peernames[deregIdx],10);
    	                memcpy(&contentDeregistrationPDU.data[10],filenames[deregIdx],10);                        
						// snprintf(contentDeregistrationPDU.data, 100, "%s%s", peernames[deregIdx], filenames[deregIdx]);
                        printf("sending pdu with type %c and data %s \n",contentDeregistrationPDU.type,contentDeregistrationPDU.data);
                        write(s, &contentDeregistrationPDU, 10 + 10 + 2);                    
                        int deregResponseLen = read(s, &tempResponseBuf[0], 100);
                        struct pdu deregResponse = loadPdufromBuf(tempResponseBuf,deregResponseLen);
                        printf("received: %c %s",deregResponse.type,deregResponse.data);
                    }
                    run = 0;
                    break;
                default:
                    printf("Invalid Choice \n\n");
                    sleep(1);
            }
        }
        else{
            printf("tcp input \n");
            int tcpSocketLoop = 0;
            for(tcpSocketLoop=0; tcpSocketLoop<socketArrayIndex; tcpSocketLoop++){
                printf("check socket %d at index %d",socketArray[tcpSocketLoop],tcpSocketLoop);
                // if(0){
                if(FD_ISSET(socketArray[tcpSocketLoop], &rfds)){
                    int 	client_len;
	                struct sockaddr_in client;
                    client_len = sizeof(client);
                    int sd = accept(socketArray[tcpSocketLoop], (struct sockaddr *)&client, &client_len);
                    printf("new connection at socket %d",sd);
                    int readamnt;
                    char	*bp, buf[BUFLEN];
                	unsigned char  buffer[100];

                    FILE* file;
                    file = fopen(filenames[tcpSocketLoop],"rb");

                    // If file doesn't exist, notify client and exit
                    if (file == NULL){
                        printf("FILE %s DOES NOT EXIST",buf);
                        write(sd, "!FILE DOES NOT EXIST", sizeof("!FILE DOES NOT EXIST"));
                        close(sd);
                        return(0);
                    }

                    // If file found, write contents to client then exit
                    write(sd, "d", sizeof("d"));
                    while(readamnt = fread(buffer,sizeof(*buffer),ARRAY_SIZE(buffer),	file))
                    {
                        write(sd, buffer, readamnt);
                        printf("\nsent to client %d bytes: %s \n\n",readamnt, buffer);
                        memset(buffer, 0, sizeof(buffer));
                        sleep(1);

                    }
                    fclose(file);
                    close(sd);
                    printf("closed file and socket");
                    // exit(0);
                }
            }
        }
    }

	exit(0);
}
