#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../header/pqueue.h"

Node* create_node(char* uname, int score){
    Node* tmp = (Node*)malloc(sizeof(Node));
    strcpy(tmp->uname, uname);
    tmp->score = score;
    tmp->next = NULL;
    return tmp;
}

void add_to_pqueue(Pqueue* pq, char* uname, int score){
    Node* new_node = create_node(uname, score);
    Node* prev;

    if( (*pq) == NULL ){
        new_node->next = NULL;
        (*pq) = new_node;
        return;
    }
    if( (*pq)->score > score ){
        new_node->next = (*pq);
        (*pq) = new_node;
    }
    else{
        prev = (*pq);
        while( prev->next != NULL && prev->next->score < score ){
            prev = prev->next;
        }
        new_node->next = prev->next;
        prev->next = new_node;
    }
}