#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <pthread.h>

#include "../header/protocol.h"
#include "../header/pqueue.h"

//------------- threads --------------
#define MAX_THREADS  3
#define INACTIVE    0
#define ACTIVE      1
#define DONE        2
pthread_t threads[MAX_THREADS];
int threadState[MAX_THREADS] = {INACTIVE};

typedef struct {
    int commSocket;
    int threadIndex;
}threadParam;
threadParam thread_params[MAX_THREADS];

pthread_mutex_t m_activeThreads;

//----------- scoreboard -------------
Pqueue scoreboard = NULL;
pthread_mutex_t m_scoreboard;

//-------------- word ----------------
#define MAX_ATTEMPTS 8
char word[30];

int send_progress(int commSocket, char* uncovered_word, int attempts){
    char* message = (char*) malloc((strlen(uncovered_word) + 30) * sizeof(char));
    sprintf(message, "%d %s#", attempts, uncovered_word);

    return sendrequest(commSocket, TELL, message);
}

void send_scoreboard(int commSocket){
    Node* iter = scoreboard;
    char message[50];

    while(iter != NULL){
        sprintf(message, "%s\t\t%d", iter->uname, iter->score);
        sendrequest(commSocket, SCOREBOARD,message);
        iter = iter->next;
    }


    sendrequest(commSocket, OVER, "");
}

void game_over(int commSocket, int score, unsigned int letters_to_guess, char* uname){
    char* m1 = letters_to_guess == 0 ? "1" : "0";
    char* message = (char*) malloc((strlen(word) + 5) * sizeof(char));
    sprintf(message, "%s %s#", m1, word);

    if(sendrequest(commSocket, GAME_OVER, message) == FAIL){
        err1("Error communicating with client.\n");
    }

    pthread_mutex_lock(&m_scoreboard);
    add_to_pqueue(&scoreboard, uname, score + letters_to_guess);
    send_scoreboard(commSocket);
    pthread_mutex_unlock(&m_scoreboard);

}

void already_tried(int commSocket, char c){
    char mssg[100];
    sprintf(mssg, "You already tried guessing %c", c);
    sendrequest(commSocket, TRY_AGAIN, mssg);
}

void not_alpha(int commSocket){
    sendrequest(commSocket, TRY_AGAIN, "Try guessing a letter");
}

static void* process_client( void *paramsVP ){
    threadParam *params = (threadParam *) paramsVP;
    int commSocket = params->commSocket;

    int rqType;
    char* rq;
    //--------------------- set up uncovered word -------------------------------
    char* uncovered_word = (char*) malloc((strlen(word)+1) * sizeof(char));
    memset(uncovered_word, '_', strlen(word));
    uncovered_word[strlen(word)] = '\0';

    //-------------------------- set up guesses ---------------------------------
    unsigned long letters_to_guess = 0;
    bool tried_letter[26] = {false};
    int remaining_attempts = MAX_ATTEMPTS;
    int i;

    for(i=0; word[i]; i++) {
        if(word[i] != ' ') {
            letters_to_guess ++;
        }
        else{
            uncovered_word[i] = ' ';
        }
    }


    //-----------------------------  get uname  ---------------------------------
    if(recvrequest(commSocket, &rqType, &rq) == FAIL){
        err1("Error communicating with client.\n");
    }
    if(rqType != LOGIN)
        err1("Got wrong request (not LOGIN)");

    char uname[20];
    sscanf(rq, "%s", uname);
    free(rq);
    printf("----- uname: %s\n", uname);


    //-------------------------------- play -------------------------------------
    char letter;
    bool guessed_letter;
    int mistakes = 0;

    while(remaining_attempts > 0){
        guessed_letter = false;

        if(recvrequest(commSocket, &rqType, &rq) == FAIL){
            err1("Error communicating with client.\n");
        }
        if(rqType != GUESS)
            err1("Got wrong request");

        sscanf(rq, "%c", &letter);
        free(rq);

        if(!isalpha(letter)){
            not_alpha(commSocket);
            continue;
        }

        letter = (char)toupper(letter);
        if(tried_letter[(int)letter - (int)'A']){
            already_tried(commSocket, letter);
            continue;
        }else{
            tried_letter[(int)letter - (int)'A'] = true;
        }

        for( i = 0; i < strlen(word); i++ ){
            if(word[i] == letter){
                uncovered_word[i] = letter;
                letters_to_guess -= 1;
                guessed_letter = true;
            }
        }
        if(!guessed_letter){
            remaining_attempts--;
            mistakes ++;
        }

        if( remaining_attempts == 0 || letters_to_guess == 0 ){
            break;
        }
        if(send_progress(commSocket, uncovered_word, remaining_attempts) == FAIL){
            err1("Error communicating with client.\n");
        }
    }
    game_over(commSocket, mistakes, letters_to_guess, uname);

    //------------------------- close thread -----------------------
    pthread_mutex_lock( &m_activeThreads );
    threadState[params->threadIndex] = DONE;
    pthread_mutex_unlock( &m_activeThreads );

    close(commSocket);

    return NULL;
}

int main( int argc, char **argv ){
    if(argc != 3){
        printf( "Usage: %s port word\n", argv[0] );
        exit( 0 );
    }
    int port;
    sscanf(argv[1], "%d %s", &port, word);

    int listenerSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(listenerSocket == -1)
        perror( "socket" );

    struct sockaddr_in servAddr = {
            .sin_family      = AF_INET,
            .sin_port        = htons(port),
            .sin_addr.s_addr = INADDR_ANY
    };

    memset(servAddr.sin_zero, '\0', 8);

    if(bind(listenerSocket,
            (struct sockaddr *) &servAddr,
            sizeof(servAddr)) == -1){
        perror( "bind" );
    }

    if(listen(listenerSocket, 10) == -1)
        perror("listen");


    while(1){
        struct sockaddr_in cliAddr;
        unsigned int lenAddr = sizeof(cliAddr);
        int commSocket = accept( listenerSocket, (struct sockaddr *) &cliAddr, &lenAddr );

        if( commSocket == -1 ){
            perr("accept");
        }

        char *decIP = inet_ntoa(cliAddr.sin_addr);
        printf("Accepted connection from: %s\n", decIP);

        pthread_mutex_lock( &m_activeThreads );
        int inactiveThread = -1;
        int i;
        for( i = 0; i < MAX_THREADS; i++ ) {
            if (threadState[i] == INACTIVE) {
                inactiveThread = i;
            } else if (threadState[i] == DONE) {
                pthread_join(threads[i], NULL);
                threadState[i] = DONE;
                inactiveThread = i;
            }
        }

        if( inactiveThread == -1 ){
            close( commSocket ); // nemam vise dretvi...
            printf( "--> no available threads, connection closed.\n" );
        }
        else{
            threadState[inactiveThread] = ACTIVE;
            thread_params[inactiveThread].commSocket = commSocket;
            thread_params[inactiveThread].threadIndex = inactiveThread;
            printf( "--> using thread %d.\n", inactiveThread );

            pthread_create( &threads[inactiveThread], NULL, process_client, &thread_params[inactiveThread] );
        }
        pthread_mutex_unlock( &m_activeThreads );

    }

}