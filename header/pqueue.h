#ifndef ZADACA_PQUEUE_H
#define ZADACA_PQUEUE_H

typedef struct node {
    char uname[20];
    int score;
    struct node* next;
} Node;

typedef Node* Pqueue;
Node* create_node(char* uname, int score);
void add_to_pqueue(Pqueue* pq, char* uname, int score);

#endif //ZADACA_PQUEUE_H
