/* PTA 02-线性结构1 两个有序链表序列的合并 —— 本地可跑版
 *
 * 判题程序里 Read / Print / main 是隐藏的，这里按题面还原出来，
 * 方便本地编译调试。提交 PTA 时只贴最后一节 Merge()。
 *
 * 输入格式（见样例）：先读个数，再读这么多个数；两条链各占两行。
 *   3 / 1 3 5 / 5 / 2 4 6 8 10
 */
#include <stdio.h>
#include <stdlib.h>

typedef int ElementType;
typedef struct Node *PtrToNode;
struct Node {
    ElementType Data;
    PtrToNode   Next;
};
typedef PtrToNode List;

List Merge( List L1, List L2 );

/* ---------- 以下为判题程序隐藏的部分，本地调试用 ---------- */

List Read( void )
{
    List head = (List)malloc(sizeof(struct Node));
    head->Next = NULL;
    List tail = head;

    int n;
    if ( scanf("%d", &n) != 1 ) return head;   /* 没输入就是空表 */

    for ( int i = 0; i < n; i++ ) {
        int x;
        scanf("%d", &x);
        PtrToNode p = (PtrToNode)malloc(sizeof(struct Node));
        p->Data = x;
        p->Next = NULL;
        tail->Next = p;
        tail = p;
    }
    return head;
}

void Print( List L )
{
    List p = L->Next;
    if ( p == NULL ) {          /* 空表 */
        printf("NULL\n");
        return;
    }
    int first = 1;
    while ( p != NULL ) {
        if ( !first ) printf(" ");
        printf("%d", p->Data);
        first = 0;
        p = p->Next;
    }
    printf("\n");
}

int main( void )
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

/* ---------- 以下为要提交的部分 ---------- */

List Merge( List L1, List L2 )
{
    List L = (List)malloc(sizeof(struct Node));   /* 新链表的头结点 */
    List tail = L;                                /* 尾指针：新链表最后一个结点 */
    List p1 = L1->Next;                           /* 跳过各自的头结点 */
    List p2 = L2->Next;

    while ( p1 != NULL && p2 != NULL ) {
        if ( p1->Data <= p2->Data ) {
            tail->Next = p1;                      /* 摘 p1 挂到新链尾 */
            p1 = p1->Next;
        } else {
            tail->Next = p2;                      /* 摘 p2 挂到新链尾 */
            p2 = p2->Next;
        }
        tail = tail->Next;                        /* 尾指针前进，别漏 */
    }

    tail->Next = ( p1 != NULL ) ? p1 : p2;        /* 剩余段整段接上；都空则为 NULL */

    L1->Next = NULL;                              /* 原链表已被掏空 → Print 输出 NULL */
    L2->Next = NULL;

    return L;
}
