#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct pdu {
    char type;
    char data[100];
};

struct PDU_R {
    char peerName[11];
    char contentName[11];
    char address[78];
};

struct PDU_S {
    char peerName[11];
    char address[78];
};

struct contentListStruct {
    char peerName[11];
    char contentName[11];
    char address[78];
    int numberTimesUsed;
};

int main(int argc, char *argv[]) {

    struct  sockaddr_in fsin;	/* the from address of a client	*/

    char    *pts;
    int	sock;			/* server socket		*/
    time_t	now;			/* current time			*/
    int	alen;			/* from-address length		*/
    struct  sockaddr_in sin; /* an Internet endpoint address         */
    int     s, type;        /* socket descriptor and socket type    */
    int 	port=3000;

    switch(argc){
        case 1:
            break;
        case 2:
            port = atoi(argv[1]);
            break;
        default:
            fprintf(stderr, "Usage: %s [port]\n", argv[0]);
            exit(1);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        fprintf(stderr, "can't creat socket\n");

    if(bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        fprintf(stderr, "can't bind to %d port\n", port);
    }
    listen(s, 5);
    alen = sizeof(fsin);

    struct contentListStruct contentListArray[100];
    int contentListArrayPos = 0;

    while(1) {

        struct pdu incomingDataPDU;
        char incomingData_Buffer[100];
        int incomingData_Length = 0;
        memset(incomingData_Buffer, 0, sizeof(incomingData_Buffer));

        if(incomingData_Length = recvfrom(s, incomingData_Buffer, sizeof(incomingData_Buffer), 0, (struct sockaddr *)&fsin, &alen) < 0) {

        }
        incomingDataPDU.type = incomingData_Buffer[0];

        printf("\n--------IP: %s----------", inet_ntoa(fsin.sin_addr));

        if(incomingDataPDU.type == 'R') { //Content registration
            struct PDU_R incomingDataPDU_R;
            memset(incomingDataPDU_R.address, 0, sizeof(incomingDataPDU_R.address));
            memcpy(&incomingDataPDU_R.peerName[0], &incomingData_Buffer[1], 10);
            memcpy(&incomingDataPDU_R.contentName[0], &incomingData_Buffer[11], 10);
            strcat(&incomingDataPDU_R.address[0], (char*)inet_ntoa(fsin.sin_addr));
            strcat(&incomingDataPDU_R.address[0], (char*)":");
            char* temp[20];
            memset(temp, 0, sizeof(temp));
            memcpy(&temp, &incomingData_Buffer[21], 10);
            strcat(&incomingDataPDU_R.address[0], (char*)temp);
            int foundDup = 0;
            int i = 0;
            //printf("\nPeer name: %s", incomingDataPDU_R.peerName); printf("\nContent name: %s", incomingDataPDU_R.contentName); printf("\nIP address: %s", incomingDataPDU_R.address);
            for(i = 0; i < contentListArrayPos; i++) {
                if(strcmp(incomingDataPDU_R.peerName, contentListArray[i].peerName) == 0) {
                    if(strcmp(incomingDataPDU_R.contentName, contentListArray[i].contentName) == 0) {
                        foundDup++;
                        struct pdu errorPDU;
                        errorPDU.type = 'E';
                        char errorMsg[] = "Content name with that peer name already exists.";
                        memcpy(&errorPDU.data[0], &errorMsg[0], 50);
                        (void) sendto(s, &errorPDU, sizeof(errorPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
                        break;
                    }
                }
            }
            if((contentListArrayPos + 1) <= 100) {
                if(foundDup == 0) {
                    memcpy(&contentListArray[contentListArrayPos].peerName[0], &incomingDataPDU_R.peerName[0], 11);
                    memcpy(&contentListArray[contentListArrayPos].contentName[0], &incomingDataPDU_R.contentName[0], 11);
                    memcpy(&contentListArray[contentListArrayPos].address[0], &incomingDataPDU_R.address[0], 78);
                    contentListArray[contentListArrayPos].numberTimesUsed = 0;
                    contentListArrayPos++;
                    struct pdu ackPDU;
                    ackPDU.type = 'A';
                    char errorMsg[] = "Content successfully registered.";
                    memcpy(&ackPDU.data[0], &errorMsg[0], 50);
                    (void) sendto(s, &ackPDU, sizeof(ackPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
                }
            } else {
                struct pdu errorPDU;
                errorPDU.type = 'E';
                char errorMsg[] = "Server can only hold up to 100 contents.";
                memcpy(&errorPDU.data[0], &errorMsg[0], 50);
                (void) sendto(s, &errorPDU, sizeof(errorPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
            }
        } else if(incomingDataPDU.type == 'S') { //Search for content and content server
            struct PDU_R incomingDataPDU_R;
            memcpy(&incomingDataPDU_R.peerName[0], &incomingData_Buffer[1], 10);
            memcpy(&incomingDataPDU_R.contentName[0], &incomingData_Buffer[11], 10);
            struct pdu sendPDU;
            int prevLargestTimesUsed = 100;
            int currentSelection = 0;
            int foundContent = 0;
            int i = 0;
            for(i = 0; i < contentListArrayPos; i++) {
                //if(strcmp(incomingDataPDU_R.peerName, contentListArray[i].peerName) == 0) {
                if(strcmp(incomingDataPDU_R.contentName, contentListArray[i].contentName) == 0) {
                    if(contentListArray[i].numberTimesUsed <= prevLargestTimesUsed) {
                        currentSelection = i;
                        prevLargestTimesUsed = contentListArray[i].numberTimesUsed;
                        foundContent = 1;
                        sendPDU.type = 'S';
                        struct PDU_S temp;
                        memcpy(&temp.peerName[0], &contentListArray[i].peerName[0], 11);
                        memcpy(&temp.address[0], &contentListArray[i].address[0], 78);
                        memcpy(&sendPDU.data[0], &temp, 100);
                    }
                }
                //}
            }
            if(foundContent == 1) {
                contentListArray[currentSelection].numberTimesUsed++;
                (void) sendto(s, &sendPDU, sizeof(sendPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
            } else {
                struct pdu errorPDU;
                errorPDU.type = 'R';
                char errorMsg[] = "Cannot Find that specific content.";
                memcpy(&errorPDU.data[0], &errorMsg[0], 50);
                (void) sendto(s, &errorPDU, sizeof(errorPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
            }
        } else if(incomingDataPDU.type == 'T') {//Content deregistration
            struct PDU_R incomingDataPDU_R;
            memcpy(&incomingDataPDU_R.peerName[0], &incomingData_Buffer[1], 10);
            memcpy(&incomingDataPDU_R.contentName[0], &incomingData_Buffer[11], 10);
            strcat(&incomingDataPDU_R.address[0], (char*)inet_ntoa(fsin.sin_addr));
            strcat(&incomingDataPDU_R.address[0], (char*)":");
            char* temp[20];
            memcpy(&temp, &incomingData_Buffer[21], 10);
            strcat(&incomingDataPDU_R.address[0], (char*)temp);
            int i = 0;
            int temp2 = contentListArrayPos;
            int contentDeregistered = 0;
            for(i = 0; i < temp2; i++) {
                if(strcmp(incomingDataPDU_R.peerName, contentListArray[i].peerName) == 0) {
                    if(strcmp(incomingDataPDU_R.contentName, contentListArray[i].contentName) == 0) {
                        struct contentListStruct temp = contentListArray[contentListArrayPos-1];
                        contentListArray[i] = temp;
                        contentListArrayPos--;
                        contentDeregistered = 1;
                        struct pdu ackPDU;
                        ackPDU.type = 'A';
                        char errorMsg[] = "Deregistered the Content.";
                        memcpy(&ackPDU.data[0], &errorMsg[0], 50);
                        (void) sendto(s, &ackPDU, sizeof(ackPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
                        break;
                    }
                }
            }
            if(contentDeregistered == 0) {
                struct pdu errorPDU;
                errorPDU.type = 'R';
                char errorMsg[] = "Cannot Find that Specific Content.";
                memcpy(&errorPDU.data[0], &errorMsg[0], 50);
                (void) sendto(s, &errorPDU, sizeof(errorPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
            }
        } else if(incomingDataPDU.type == 'O') { //LIST OF content
            int i = 0;
            for(i = 0; i < contentListArrayPos; i++) {
                struct pdu listPDU;
                listPDU.type = 'O';
                struct PDU_R temp;
                memcpy(&temp.peerName[0], &contentListArray[i].peerName[0], 11);
                memcpy(&temp.contentName[0], &contentListArray[i].contentName[0], 11);
                memcpy(&temp.address[0], &contentListArray[i].address[0], 78);
                memcpy(&listPDU.data[0], &temp, 100);
                (void) sendto(s, &listPDU, sizeof(listPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
            }
            struct pdu ackPDU;
            ackPDU.type = 'A';
            char errorMsg[] = "Content list successfully sent.";
            memcpy(&ackPDU.data[0], &errorMsg[0], 50);
            (void) sendto(s, &ackPDU, sizeof(ackPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
        } else if(incomingDataPDU.type == 'A') { //Acknowledge



        } else if(incomingDataPDU.type == 'E') { //Error


        } else { //Unknown letter
            printf("\n!Unknown letter received!\n");
        }


        sleep(1);


    } //End of while loop





} //End of main function

