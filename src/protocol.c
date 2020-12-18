#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "../header/protocol.h"

int recvrequest(int sock, int *rqType, char **rq){
    int recvd = 0, lastRecvd;
    int rqLen, b_rqType;

    if(recv(sock, &rqLen, sizeof(rqLen), 0) != sizeof(rqLen)){
        return FAIL;
    }
    rqLen = ntohl(rqLen);

    if(recv(sock, &b_rqType, sizeof(b_rqType), 0) != sizeof(b_rqType)){
        return FAIL;
    }
    *rqType = ntohl(b_rqType);

    *rq = (char *)malloc((rqLen + 1) * sizeof(char));

    while(recvd != rqLen){
        lastRecvd = recv(sock, *rq + recvd, rqLen - recvd, 0);
        if( lastRecvd == -1 || lastRecvd == 0 ){
            return FAIL;
        }
        recvd += lastRecvd;
    }
    (*rq)[rqLen] = '\0';
    return SUCCESS;
}

int sendrequest(int sock, int rqType, const char *rq){
    unsigned int rqLen = strlen(rq);
    int b_rqLen = htonl(rqLen);

    if(send(sock, &b_rqLen, sizeof(b_rqLen), 0) != sizeof(b_rqLen)){
        return FAIL;
    }

    rqType = htonl(rqType);
    if(send(sock, &rqType, sizeof(rqType), 0) != sizeof(rqType)){
        return FAIL;
    }


    int sent, lastSent;
    for(sent = 0 ; sent < rqLen; sent += lastSent){
        lastSent = send(sock, rq + sent, strlen(rq) - sent, 0);
        if(lastSent == 0 || lastSent == -1){
            return FAIL;
        }
    }
    return SUCCESS;
}



void err1(char* mssg){
    printf("%s\n",mssg);
    exit(0);
}

void err2(char* mssg1, char* mssg2) {
    printf("%s %s\n",mssg1, mssg2);
    exit(0);
}

void perr(char* mssg){
    perror(mssg);
    exit(0);
}