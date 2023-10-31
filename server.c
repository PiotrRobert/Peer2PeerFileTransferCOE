/* time_server.c - main */

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

/*------------------------------------------------------------------------
 * main - Iterative UDP server for TIME service
 *------------------------------------------------------------------------
 */

struct pdu {
    char type;
    char data[100];
};

int
main(int argc, char *argv[])
{
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

    /* Allocate a socket */
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0)
        fprintf(stderr, "can't creat socket\n");

    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        fprintf(stderr, "can't bind to %d port\n",port);
    listen(s, 5);
    alen = sizeof(fsin);

    while (1) {

        int lstat_returncode;
        int sendfileloop_counter;
        int readamnt_file;

        int has_file = 0;

        struct pdu returndata_pdu;
        struct stat file_stats;
        FILE* file;
        struct pdu filename_pdu;

        do{
            char	buf[100];		/* "input" buffer; any size > 0	*/
            int readamnt_filename;
            printf("wait for filename\n");
            //if (readamnt_filename = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&fsin, &alen) < 0)
            //	fprintf(stderr, "recvfrom error\n");
            readamnt_filename = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&fsin, &alen);
            printf("READ %d bytes from",readamnt_filename);
            filename_pdu.type = 'C';//buf[0];
            strncpy(filename_pdu.data, &buf[1], readamnt_filename);
            filename_pdu.data[readamnt_filename-2]='\0';
            printf("check pdu data\n");
            if (filename_pdu.type != 'C'){
                printf("Buffer 0 is: %c\n",buf[0]);
                printf("Error, didn't get filename, got %c\n",filename_pdu.type);
                printf("Error, filename is %s\n",filename_pdu.data);
                struct pdu filenameerror_pdu;
                filenameerror_pdu.type = 'E';
                strncpy(filenameerror_pdu.data, "Server Error", sizeof("Server Error"));
                (void) sendto(s, &filename_pdu, sizeof(filename_pdu), 0, (struct sockaddr *)&fsin, sizeof(fsin));
            }else{
                file = fopen(filename_pdu.data,"rb");
                // If file doesn't exist, notify client and exit
                if (file == NULL){
                    printf("PDU DATA %s DOES NOT EXIST\n",filename_pdu.data);
                    printf("Buffer DATA %s ???\n",buf);
                    struct pdu filenameerror_pdu;
                    filenameerror_pdu.type = 'E';
                    strncpy(filenameerror_pdu.data, "File doesn't exist", sizeof("File doesn't exist"));
                    (void) sendto(s, &filenameerror_pdu, sizeof(filenameerror_pdu), 0, (struct sockaddr *)&fsin, sizeof(fsin));
                }else{
                    has_file = 1;
                    printf("WE GOT THE FILE. filename indicator is now %d \n",has_file);
                }
            }

        }while(has_file == 0);

        lstat_returncode = lstat(filename_pdu.data, &file_stats);
        file = fopen(filename_pdu.data,"rb");

        for (sendfileloop_counter=0;sendfileloop_counter<(file_stats.st_size)/99;sendfileloop_counter++){
            returndata_pdu.type = 'D';
            printf("Sizeof pdu data %d, arraysize pdu data %d \n\n",sizeof(*returndata_pdu.data),ARRAY_SIZE(returndata_pdu.data));
            readamnt_file = fread(returndata_pdu.data,sizeof(*returndata_pdu.data),ARRAY_SIZE(returndata_pdu.data)-1,	file);
            returndata_pdu.data[readamnt_file] = '\0';
            printf("Sizeof pdu data %d, arraysize pdu data %d, readmant %d \n\n",sizeof(*returndata_pdu.data),ARRAY_SIZE(returndata_pdu.data),readamnt_file);
            printf("sending packet: TYPE %c \n DATA: %s\n",returndata_pdu.type, returndata_pdu.data);
            (void) sendto(s, &returndata_pdu, readamnt_file+2, 0, (struct sockaddr *)&fsin, sizeof(fsin));
            memset(returndata_pdu.data, 0, sizeof(returndata_pdu.data));
            // sleep(1);
        }
        returndata_pdu.type = 'F';
        readamnt_file = fread(returndata_pdu.data,sizeof(*returndata_pdu.data),ARRAY_SIZE(returndata_pdu.data)-1,	file);
        returndata_pdu.data[readamnt_file] = '\0';
        printf("sending packet: TYPE %c \n DATA: %s",returndata_pdu.type, returndata_pdu.data);
        (void) sendto(s, &returndata_pdu, readamnt_file+2, 0, (struct sockaddr *)&fsin, sizeof(fsin));
    }
}