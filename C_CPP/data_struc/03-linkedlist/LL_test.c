#include <stdio.h>
 struct node{
        int data;
        struct node * next;
    };

int main()
{
    struct node n3 = {5, NULL};
    struct node n2 = {4, &n3};
    struct node n1 = {3, &n2};

    struct node * p = &n1;
    while(p != NULL)
    {
        printf("%d\n", p -> data);
        p = p -> next;
    }

    return 0;
    
}