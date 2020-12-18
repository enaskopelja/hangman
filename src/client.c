#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>

#include "../header/protocol.h"

char HANG_STATES[7][10 * 9] =
{
        "             +         +----     +----     +----     +----     +----     +----     +----  ",
        "             |         |         |   O     |   O     |   O     |   O     |   O     |   O  ",
        "             |         |         |         |   +     | --+     | --+--   | --+--   | --+--",
        "             |         |         |         |   |     |   |     |   |     |   |     |   |  ",
        "             |         |         |         |         |         |         |  /      |  / \\ ",
        "             |         |         |         |         |         |         |         |      ",
        "/*****\\   /*****\\   /*****\\   /*****\\   /*****\\   /*****\\   /*****\\   /*****\\   /*****\\   "
};

bool guess( int commSocket, char letter ){
    if(sendrequest(commSocket, GUESS, &letter) == FAIL){
        err1("Error communicating with server.\n");
    }
    int rqType;
    char* rq;
    if(recvrequest(commSocket, &rqType, &rq) == FAIL){
        err1("Error communicating with server.\n");
    }

    int attempts = 8;
    int won;
    char* word = (char*)malloc(strlen(rq)*sizeof(char));
    if(rqType == TELL){
        sscanf(rq, "%d %[^#]s", &attempts, word);
    } else if(rqType == GAME_OVER){
        sscanf(rq, "%d %[^#]s", &won, word);
    }

    int hangstate = 8-attempts;
    if(rqType == GAME_OVER && !won){
        hangstate = 8;
    }

    printf("\033[2J\033[1;1H");
    fflush(stdout);
    int i;
    if(rqType == TRY_AGAIN){
        printf("%s\n\n", rq);
    }
    else{
        for (i = 0; i < 7; i++) {
            printf("%.10s\n", &HANG_STATES[i][hangstate * 10]);
        }
        printf("\n");

        if (rqType == TELL){
            printf("%s\n\n", word);
        }
        else{
            printf("%s\n"
                       "The word was: %s\n", won ? "CONGRATULATIONS!": "GAME OVER!", word);

        }
    }


    free(rq);
    free(word);

    return rqType == GAME_OVER;
}

int main( int argc, char **argv ){
    if( argc != 3 ){
        char mssg[50];
        sprintf(mssg,"Usage: %s IP port\n", argv[0]);
        err1(mssg);
    }

    // ------------------------ connect ----------------------------
    char dekadskiIP[20];
    strcpy(dekadskiIP, argv[1]);
    int port;
    sscanf(argv[2], "%d", &port);

    int commSocket = socket( PF_INET, SOCK_STREAM, 0 );
    if( commSocket == -1 )
        perr( "socket" );

    struct sockaddr_in servAddr = {
            .sin_family = AF_INET,
            .sin_port = htons(port)
    };

    if( inet_aton( dekadskiIP, &servAddr.sin_addr ) == 0 )
        err2( "%s invalid IP address!\n", dekadskiIP );

    memset( servAddr.sin_zero, '\0', 8 );

    if(connect(commSocket,
               (struct sockaddr *) &servAddr,
               sizeof(servAddr)
    ) == -1) perr( "connect" );

    // ------------------------ play ----------------------------
    bool gameOver = false;
    char uname[20];

    printf("User name: ");
    scanf(" %s", uname);

    if(sendrequest(commSocket, LOGIN, uname) == FAIL){
        err1("Error communicating with server.\n"
             "Could not send user name\n");
    }

    printf("Starting game....\n");
    char letter;
    while (!gameOver){
        printf("Guess a letter: ");
        scanf(" %c", &letter);
        gameOver = guess(commSocket, letter);
    }

    int rqType;
    char* rq;
    printf("SCOREBOARD:\n");
    do{
        if(recvrequest(commSocket, &rqType, &rq) == FAIL){
            err1("Error communicating with server.\n");
        }

        printf("%s\n", rq);
    }while (rqType!=OVER);

    return 0;
}