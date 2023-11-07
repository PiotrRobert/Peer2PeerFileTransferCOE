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
    char peerName[10];
    char contentName[10];
    char address[80];
};

struct contentListStruct {
    char peerName[10];
    char contentName[10];
    char address[80];
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

    struct PDU_R contentListArray[100];
    int contentListArrayPos = 0;

    FILE* contentListFile;
    contentListFile = fopen("contentList.txt", "a");
    if(contentListFile == NULL) {
        printf("\n!Cannot open content list file!\n");
        exit(0);
    }

    while(1) {

        struct pdu incomingDataPDU;
        char incomingData_Buffer[100];
        int incomingData_Length = 0;

        incomingData_Length = recvfrom(s, incomingData_Buffer, sizeof(incomingData_Buffer), 0, (struct sockaddr *)&fsin, &alen);
        incomingDataPDU.type = incomingData_Buffer[0];

        if(incomingDataPDU.type == 'R') { //Content registration
            struct PDU_R incomingDataPDU_R;
            memcpy(&incomingDataPDU_R.peerName[0], &incomingData_Buffer[1], 10);
            memcpy(&incomingDataPDU_R.contentName[0], &incomingData_Buffer[11], 10);
            memcpy(&incomingDataPDU_R.address[0], &incomingData_Buffer[21], 10);
            int i = 0;
            for(i = 0; i < sizeof(contentListArray); i++) {
                if(strcmp(incomingDataPDU_R.peerName, contentListArray[i].peerName) == 0) {
                    if(strcmp(incomingDataPDU_R.contentName, contentListArray[i].contentName) == 0) {
                        struct pdu errorPDU;
                        errorPDU.type = 'R';
                        char errorMsg[] = "Content name with that peer name already exists.";
                        memcpy(&errorPDU.data[0], &errorMsg[0], 50);
                        (void) sendto(s, &errorPDU, sizeof(errorPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
                        break;
                    }
                }
            }
            if((contentListArrayPos + 1) <= 100) {
                contentListArray[contentListArrayPos] = incomingDataPDU_R;
                contentListArrayPos++;
                struct pdu ackPDU;
                ackPDU.type = 'A';
                char errorMsg[] = "Content successfully registered.";
                memcpy(&ackPDU.data[0], &errorMsg[0], 50);
                (void) sendto(s, &ackPDU, sizeof(ackPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
            } else {
                struct pdu errorPDU;
                errorPDU.type = 'R';
                char errorMsg[] = "Cannot hold anymore content.";
                memcpy(&errorPDU.data[0], &errorMsg[0], 50);
                (void) sendto(s, &errorPDU, sizeof(errorPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
            }
        } else if(incomingDataPDU.type == 'S') { //Search for content and content server




        } else if(incomingDataPDU.type == 'T') {//Content deregistration
            struct PDU_R incomingDataPDU_R;
            memcpy(&incomingDataPDU_R.peerName[0], &incomingData_Buffer[1], 10);
            memcpy(&incomingDataPDU_R.contentName[0], &incomingData_Buffer[11], 10);
            memcpy(&incomingDataPDU_R.address[0], &incomingData_Buffer[21], 10);
            int i = 0;
            for(i = 0; i < sizeof(contentListArray); i++) {
                if (strcmp(incomingDataPDU_R.peerName, contentListArray[i].peerName) == 0) {
                    if (strcmp(incomingDataPDU_R.contentName, contentListArray[i].contentName) == 0) {
                        struct PDU_R temp = contentListArray[contentListArrayPos-1];
                        contentListArray[i] = temp;
                        contentListArrayPos--;
                        struct pdu ackPDU;
                        ackPDU.type = 'A';
                        char errorMsg[] = "Deregistered the content!!!";
                        memcpy(&ackPDU.data[0], &errorMsg[0], 50);
                        (void) sendto(s, &ackPDU, sizeof(ackPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
                        break;
                    }
                }
            }
            struct pdu errorPDU;
            errorPDU.type = 'R';
            char errorMsg[] = "Cannot find that specific content.";
            memcpy(&errorPDU.data[0], &errorMsg[0], 50);
            (void) sendto(s, &errorPDU, sizeof(errorPDU), 0, (struct sockaddr *)&fsin, sizeof(fsin));
        } else if(incomingDataPDU.type == 'O') { //LIST OF content
            int i = 0;
            for(i = 0; i < contentListArrayPos; i++) {
                struct pdu listPDU;
                listPDU.type = 'O';
                memcpy(&listPDU.data[0], &contentListArray[0], 100);
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

    } //End of while loop

} //End of main function