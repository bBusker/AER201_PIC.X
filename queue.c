#include <xc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "configBits.h"
#include "queue.h"

void initQueue(int *q, int head, int tail) {
   
}

void enqueue(int *q, int *head, int *tail, int x){
    q[tail] = x;
    *tail += 1;
    *tail = *tail%10;            
}

int dequeue(int *q, int *head, int *tail){
    int res = q[*head];
    *head += 1;
    *head = *head%10;
}
void enqueue(Queue* q, int x) {
	Node* temp = (Node*)malloc(sizeof(Node));
	temp->data = x; 
	temp->next = NULL;

    if(q->sizeOfQueue == 0) {
        q->head = q->tail = temp;
    }
    else {
        q->tail->next = temp;
        q->tail = temp;
    }
    q->sizeOfQueue++;
    return;
}

int dequeue(Queue *q) {
    if(q->sizeOfQueue > 0)
    {
        Node *temp = q->head;
        int res = temp->data;
        
        if(q->sizeOfQueue > 1) q->head = q->head->next;
        else q->head = q->tail = NULL;
        
        q->sizeOfQueue--;
        
        free(temp);
        return res;
    }
}

//void clearQueue(Queue *q)
//{
//  Node *temp;
//
//  while(q->sizeOfQueue > 0)
//  {
//      temp = q->head;
//      q->head = temp->next;
//      free(temp->data);
//      free(temp);
//      q->sizeOfQueue--;   
//  }
//
//  q->head = q->tail = NULL;
//}

int getQueueSize(Queue *q)
{
    return q->sizeOfQueue;
}