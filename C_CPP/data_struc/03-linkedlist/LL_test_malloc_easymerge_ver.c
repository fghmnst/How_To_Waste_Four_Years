
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

typedef struct node Node ;
struct node{
    int   data;
    Node* next;
};

int main(void)
{
    Node* a1 = malloc(sizeof(Node));
    Node* a2 = malloc(sizeof(Node));
    Node* a3 = malloc(sizeof(Node));
   
    if(a1 == NULL || a2 == NULL || a3 == NULL){
        printf("failed malloc\n");
        free(a1);
        free(a2);
        free(a3);

        return 1;
    }

    a1 -> data = 3; a1 -> next = a2;
    a2 -> data = 4; a2 -> next = a3;
    a3 -> data = 5; a3 -> next = NULL;
//above define a_linkedlist.
    Node* b1 = malloc(sizeof(Node));
    Node* b2 = malloc(sizeof(Node));
    Node* b3 = malloc(sizeof(Node));
   
    if(b1 == NULL || b2 == NULL || b3 == NULL){
        printf("failed malloc\n");
        free(b1);
        free(b2);
        free(b3);

        return 1;
    }

    b1 -> data = 6; b1 -> next = b2;
    b2 -> data = 7; b2 -> next = b3;
    b3 -> data = 8; b3 -> next = NULL;
//above define b_linkedlist.
//then merge a and b in a simple way.
    Node* headA = a1;
    Node* headB = b1;
    Node* tail = headA;

    if(tail == NULL){
        tail = headB;
    }else {
        while (tail -> next != NULL) {
            tail = tail -> next;
        }
        tail -> next = headB;
    }
//merge complete.now print it.
    Node* p = headA;
    while (p != NULL) {
        printf("%d\n", p -> data);
        p = p -> next;
    }
//free all.
    p = headA;
    while (p != NULL) {
        Node* next_temp = p -> next;
        free(p);
        p = next_temp; 
    }

    return 0;
}

