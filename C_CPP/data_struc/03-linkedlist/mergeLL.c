#include<stdio.h>
#include<stdlib.h>

typedef int ElementType;
typedef struct Node* PtrToNode;
struct Node{
    ElementType Data;
    PtrToNode   Next;
};
typedef PtrToNode List;

List Read();
void Print( List L );

List Merge( List L1, List L2 )
{
    List L = (List)malloc(sizeof(struct Node));//?
    List tail = L;
    List p1 = L1 -> Next;
    List p2 = L2 -> Next;

    while(p1 != NULL && p2 != NULL){
        if(p1 -> Data <= p2 -> Data){
            tail -> Next = p1;
            p1 = p1 -> Next;
        }else{
            tail -> Next = p2;
            p2 = p2 -> Next;
        }
        tail -> Next = (p1 != NULL)? p1 : p2 ;
        L1 -> Next = NULL;
        L2 -> Next = NULL;
    }
        return L;
}

int main()
{
    List L1, L2, L;
    L1 = Read();
    L2 = Read();
    L = Merge(L1, L2);
    Print(L);
    Print(L1);
    Print(L2);

    return 0;
}
