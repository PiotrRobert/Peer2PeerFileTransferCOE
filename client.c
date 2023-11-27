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


#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

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

char peerName[11]={0};
// Storing files currently hosted
int socketArray[100];
char filenames[100][11];
char peernames[100][11];
int socketArrayIndex = 0;
fd_set rfds, afds;

int readFromStdin(char *inputBuf){
    int readBytes = read(0, &inputBuf[0], 11); 
    inputBuf[strcspn(inputBuf,"\r\n")] = 0;
    inputBuf[10] = 0;
    return readBytes;
}

int registerContent(int s, char *contentName){
	
    // TCP Server Setup
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

	// Send Request to index server section
    struct pdu contentRegistrationPDU = createPdu('R');
	int portlen = (int)((ceil(log10(reg_addr.sin_port))+1)*sizeof(char))-1;
	char portStr[11] = {0};
	snprintf(portStr, portlen+1, "%d", ntohs(reg_addr.sin_port));
	memcpy(&contentRegistrationPDU.data[0],peerName,10);
	memcpy(&contentRegistrationPDU.data[10],contentName,10);
	memcpy(&contentRegistrationPDU.data[20],portStr,11);
	contentRegistrationPDU.data[10+10+portlen] = 0;
	write(s, &contentRegistrationPDU, 10 + 10 + portlen+2);

    // Handle Response from index server section
	char tempResponseBuf[100];
	int responseLen = read(s, &tempResponseBuf[0], 100);
	if (responseLen == -1) {
		printf("Error reading response, exiting to menu\n");
        close(contentHostSocket);	
		return -1;				
	}
	struct pdu response = loadPdufromBuf(tempResponseBuf,responseLen);
	while (response.type == 'E'){
		printf("Enter a new peer name \n");
        if(readFromStdin(peerName)<0){
            printf("Error reading user input\n");
            return -1;   
        }
		memset(contentRegistrationPDU.data,0,100);
		memcpy(&contentRegistrationPDU.data[0],peerName,10);
		memcpy(&contentRegistrationPDU.data[10],contentName,10);
		memcpy(&contentRegistrationPDU.data[20],portStr,11);
		contentRegistrationPDU.data[10+10+portlen] = 0;
		write(s, &contentRegistrationPDU, 10 + 10 + portlen+2);
		responseLen = read(s, &tempResponseBuf[0], 100);
        if (responseLen == -1) {
	    	printf("Error reading response, exiting to menu\n");
            close(contentHostSocket);	
		    return -1;				
	    }
		response = loadPdufromBuf(tempResponseBuf,responseLen);   
	}

	memcpy(filenames[socketArrayIndex], contentName,11);
	socketArray[socketArrayIndex] = contentHostSocket;
	memcpy(peernames[socketArrayIndex], peerName,11);
	// FD_ZERO(&afds);
	FD_SET(socketArray[socketArrayIndex], &afds);
	FD_SET(0, &afds); /* Listening on stdin */
	// memcpy(&rfds, &afds, sizeof(rfds));
	socketArrayIndex++;
    printf("Now hosting %s at port %s\n",contentName,portStr);
}

int deregisterContent(int s, int deregIdx){
	char tempResponseBuf[100];
    struct pdu contentDeregistrationPDU = createPdu('T');
    memcpy(&contentDeregistrationPDU.data[0],peernames[deregIdx],10);
    memcpy(&contentDeregistrationPDU.data[10],filenames[deregIdx],10);                        
    write(s, &contentDeregistrationPDU, 10 + 10 + 2);                    
    int deregResponseLen = read(s, &tempResponseBuf[0], 100);
    if (deregResponseLen == -1) {
		printf("Error reading response, exiting to menu\n");
		return -1;				
	}
    struct pdu deregResponse = loadPdufromBuf(tempResponseBuf,deregResponseLen);
    if(deregResponse.type == 'R'){
        printf("Failed to deregister?\n");
        return -1;
    }
    memset(filenames[deregIdx],0,11);
    close(socketArray[deregIdx]);
    FD_CLR(socketArray[deregIdx], &afds);
    // memcpy(&rfds, &afds, sizeof(rfds));
    return 0;
}

int main(int argc, char **argv) {

    /*      PARSE ARGS     */
    int	port = 3000;
    char	*host = "localhost";
	switch (argc) {
    	case 3:
	    	host = argv[1];
		    port = atoi(argv[2]);
		    break;
	    default:
            fprintf(stderr, "usage: client [host] [port]\n");
		    exit(1);
	}
	

    /*  SETUP UDP CONNECTION TO INDEX SERVER */
    int	s;
    struct sockaddr_in sockIn;
	memset(&sockIn, 0, sizeof(sockIn));
    sockIn.sin_family = AF_INET;                                                                
    sockIn.sin_port = htons(port);                                         
    // Map host name to IP address, allowing for dotted decimal
    struct hostent	*phe; // pointer to host information entry
    if ( phe = gethostbyname(host) ){
        memcpy(&sockIn.sin_addr, phe->h_addr, phe->h_length);
    }else if ( (sockIn.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		fprintf(stderr, "Can't get host entry \n");                                                      
    // Allocate UDP Socket
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0){
		fprintf(stderr, "Can't create socket \n");
        exit(0);
    }
    // Make the socket a connected socket 
    if (connect(s, (struct sockaddr *)&sockIn, sizeof(sockIn)) < 0) {
		fprintf(stderr, "Can't connect to %s \n", host);
		exit(0);
	}
    
    // Initialize filename array with empty strings
    int zeroNames = 0;
    for (zeroNames = 0; zeroNames<100;zeroNames++){filenames[zeroNames][0]=0;}

    /* ----- Select Setup -----*/    
    FD_ZERO(&afds);
    FD_SET(0, &afds); /* Listening on stdin */
    // memcpy(&rfds, &afds, sizeof(rfds));

    // Request user to enter a peer name
    printf("Enter a 10 character peer name. larger names will be shortened\n");
    if(readFromStdin(peerName)<0){
        printf("Error reading user input\n");
        exit(0);   
    }

    /*----- MENU SECTION-----*/
    char userChoiceData[11];
    int run = 1;
    while (run){
        printf("\n\n|----------MENU----------|\n1) Content Registration \n2) Content Download \n3) List of On-Line Registered Content\n 4) Content De-Registration \n0) Exit \nPlease choose an option:\n");
        
        memcpy(&rfds, &afds, sizeof(rfds)); // Reload rfds, as previous call to select has modified it
        int selRet = select(FD_SETSIZE, &rfds, NULL, NULL, NULL);
        if (FD_ISSET(0, &rfds)) {
            if(readFromStdin(userChoiceData)<0){
                printf("Error reading user input\n");
                exit(0);   
            }
            printf("\n"); // Space after user choice 
            switch(userChoiceData[0]){
                case '1':
                    //Ask user for filename
                    printf("Begin File Registration Process \n");
                    printf("Enter the 10 character content name. larger names will be shortened\n");
                    char contentName[11] = {0};
                    if(readFromStdin(contentName)<0){
                        printf("Error reading user input\n");
                        break;   
                    }
                    // Check if file exists
                    if (fopen(contentName,"rb") == NULL){
                        printf("file not found\n");
                        break;
                    }
                    // Register file to index server, and open TCP socket 
                    registerContent(s, contentName);
                    break;
                case '2':
                    printf("Begin Content Download \n");
                    // Step 1) Get content name from user
                    printf("Enter the 10 character content name. larger names will be shortened\n");
                    char downloadContentName[11]={0};
                    if(readFromStdin(downloadContentName)<0){
                        printf("Error reading user input\n");
                        break;   
                    }
                    // Step 2) Send request to server and handle response
                    struct pdu findContentRequest = createPdu('S');
                    memcpy(&findContentRequest.data[0],peerName,10);
                    memcpy(&findContentRequest.data[10],downloadContentName,10);
                    write(s, &findContentRequest, 10 + 10 + 2);
                    char findContentResponseBuffer[100];
                    int responseSize = read(s,findContentResponseBuffer,100);
                    if (responseSize == -1) {
                        printf("Error reading response, exiting to menu\n");
                        break;				
                    }
                    struct pdu findContentResponsePDU = loadPdufromBuf(findContentResponseBuffer,responseSize);
                    if (findContentResponsePDU.type == 'R'){
                        printf("Cannot Find that specific content.\n");
                        break;
                    }

                    // Step 4) Download the file
                    char contentListPeerName[11];
                    char contentListAddress[100];
                    strncpy(contentListPeerName,&findContentResponsePDU.data[0],11);
                    strcpy(contentListAddress,&findContentResponsePDU.data[11]);
                    // Step 4.1) Extract IP and PORT
                    char *host = strtok(contentListAddress,":");
                    char *port = strtok(NULL,":");
                    printf("Requesting %s from %s:%s\n",downloadContentName,host,port);
                    // Step 4.2) Create a TCP socket	
                    int 	sd;
                    struct	sockaddr_in server;
                    struct	hostent		*hp;
                    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                        fprintf(stderr, "Can't creat a socket\n");
                        break;
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
                    // Step 4.3) Connecting to the server
                    if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
                        fprintf(stderr, "Can't connect \n");
                        break;
                    }
                    
                    // Step 5) Send D type PDU
                    struct pdu requestDownloadPDU = createPdu('D');
                    memcpy(&requestDownloadPDU.data[0],peerName,10);
                    memcpy(&requestDownloadPDU.data[10],downloadContentName,10);
                    requestDownloadPDU.data[21]=0;
                    write(sd, &requestDownloadPDU, 10 + 10 + 1);

                    // Step 6) Download File
                    char fileReadCharacters[160000];
                    int msgByteLength = 0;
                    int i;
                    while((i = read(sd, &fileReadCharacters[msgByteLength], 160000)) > 0) {
                        msgByteLength += i;
                    }
                    if(fileReadCharacters[0] != 'C') {
                        printf("There is an error with the file\n");
                        break;
                    } else{
                        printf("Content Found. Downloading file to same directory as this program\n");
                        char* newPointer = &fileReadCharacters[1];
                        FILE *fptr;
                        fptr = fopen(downloadContentName, "wb");
                        fputs(&fileReadCharacters[2], fptr);
                        fclose(fptr);
                    }
                    close(sd);
			
					registerContent(s, downloadContentName);		

                    break;
                case '3':
                    printf("Begin Content List \n");
                    struct pdu contentListRequestPDU = createPdu('O');
                    write(s, &contentListRequestPDU, 2);
                    char listResponseBuffer[100];
                    int exitContentList = 0;
                    do{
                        int responseSize = read(s,listResponseBuffer,100);
                        if (responseSize == -1) {
                            printf("Error reading response, exiting to menu\n");
                            exitContentList = 1;
                            break;				
                        }
                        struct pdu contentListResponsePDU = loadPdufromBuf(listResponseBuffer,responseSize);
                        if (contentListResponsePDU.type == 'A'){
                            exitContentList = 1;
                        }
                        else if ((contentListResponsePDU.type == 'O') && (responseSize>20)){
                            char contentListPeerName[11];char contentListFileName[11];char contentListAddress[100];
                            strncpy(contentListPeerName,&contentListResponsePDU.data[0],11);
                            strncpy(contentListFileName,&contentListResponsePDU.data[11],11);
                            strcpy(contentListAddress,&contentListResponsePDU.data[22]);
                            if(contentListPeerName != NULL){
                                printf("Peer '%s' is hosting ",contentListPeerName);
                            }
                            if(contentListFileName != NULL){
                                printf("file named '%s' ",contentListFileName);
                            }if(contentListAddress != NULL){
                                printf("with address '%s'\n",contentListAddress);
                            }
                        }
                        else{
                            exitContentList = 1;
                        }
                    }while(!exitContentList);
                    break;
                case '4':
                    printf("Begin Deregistration \n");
                    printf("Select the content to deregister, -1 to exit\n");
                    int deregListIdx = 0;
                    for (deregListIdx=0;deregListIdx<socketArrayIndex;deregListIdx++){
                        if(strlen(filenames[deregListIdx])>0){
                            printf("%d) %s \n",deregListIdx,filenames[deregListIdx]);
                        }
                    }
                    char deregisterContentName[11];
                    if(readFromStdin(deregisterContentName)<0){
                        printf("Error reading user input \n");
                        break;
                    }
                    int choiceIdx = atoi(deregisterContentName);
                    if (choiceIdx == -1) 
                        break;                    
                    deregisterContent(s, choiceIdx);

                    break;
                case '0':
                    printf("deregister all files before exiting\n");
                    int deregIdx = 0;
                    for (deregIdx = 0; deregIdx < socketArrayIndex; deregIdx++){
                        if(strlen(filenames[deregIdx])>0){
                            deregisterContent(s, deregIdx);
                        }
                    }
                    run = 0;
                    break;
                default:
                    printf("Invalid Choice \n\n");
                    sleep(1);
            }
        }
        else{
            int tcpSocketLoop = 0;
            for(tcpSocketLoop=0; tcpSocketLoop<socketArrayIndex; tcpSocketLoop++){
                if(FD_ISSET(socketArray[tcpSocketLoop], &rfds)){
                    int 	client_len;
	                struct sockaddr_in client;
                    client_len = sizeof(client);
                    int sd = accept(socketArray[tcpSocketLoop], (struct sockaddr *)&client, &client_len);
                    printf("Handling Download Request for file %s\n",filenames[tcpSocketLoop]);
                    
                    char downloadRequestBuf[22] = {0};
                    int downloadRequestLen = read(sd, downloadRequestBuf, 22);
                    if (downloadRequestLen == -1) {
                        printf("Error reading response, exiting to menu\n");
                        break;				
                    }
                    struct pdu downloadRequestPDU = loadPdufromBuf(downloadRequestBuf,downloadRequestLen);
                    FILE* file;
                    file = fopen(filenames[tcpSocketLoop],"rb");
                    // If file doesn't exist, notify client and exit
                    if (file == NULL){
                        struct pdu noFilePdu = createPdu('E');
                        printf("FILE %s DOES NOT EXIST",filenames[tcpSocketLoop]);
                        write(sd, &noFilePdu, 2);
                        close(sd);
                        break;
                    }

                    // If file found, write contents to client then exit
                    int readamnt;
                	unsigned char  buffer[100];
                    write(sd, "C", sizeof("C"));
                    while(readamnt = fread(buffer,sizeof(*buffer),ARRAY_SIZE(buffer),	file))
                    {
                        write(sd, buffer, readamnt);
                        memset(buffer, 0, sizeof(buffer));
                    }
                    fclose(file);
                    close(sd);
                    printf("closed file and socket\n");
                }
            }
        }
    }

	exit(0);
}
