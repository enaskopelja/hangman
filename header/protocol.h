#ifndef ZADACA_NOTP_PROTOCOL_H
#define ZADACA_NOTP_PROTOCOL_H

#define GUESS       0
#define TELL        1
#define GAME_OVER   2
#define TRY_AGAIN   3
#define LOGIN       4
#define SCOREBOARD  5
#define OVER        6

#define SUCCESS     1
#define FAIL        0

int recvrequest(int sock, int *rqType, char **rq);
int sendrequest(int sock, int rqType, const char *rq);

void err1(char* mssg);
void err2(char* mssg1, char* mssg2);
void perr(char* mssg);


#endif //ZADACA_NOTP_PROTOCOL_H


