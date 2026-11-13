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
    Node* n1 = malloc(sizeof(Node));
    Node* n2 = malloc(sizeof(Node));
    Node* n3 = malloc(sizeof(Node));
   
    if(n1 == NULL || n2 == NULL || n3 == NULL){
        printf("failed malloc\n");
        free(n1);
        free(n2);
        free(n3);

        return 1;
    }

    n1 -> data = 3; n1 -> next = n2;
    n2 -> data = 4; n2 -> next = n3;
    n3 -> data = 5; n3 -> next = NULL;

    Node* p = n1;
    while (p != NULL) {
        printf("%d\n",p -> data);
        p = p -> next;
    }

    free(n1);
    free(n2);
    free(n3);

    return 0;
}

