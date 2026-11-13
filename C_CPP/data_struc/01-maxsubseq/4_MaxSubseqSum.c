//在线处理算法，复杂度为惊人的n（TODO）
#include <stdio.h>
#define Num 10
int MaxSubseqSum4(int A[], int N)
{
    int i;
    int ThisSum ,MaxSum;
    ThisSum = MaxSum = 0;
    for(i = 0; i < N ; i ++){
        ThisSum += A[i];
        if(ThisSum > MaxSum){
            MaxSum = ThisSum;
        }
        else if (ThisSum < 0){
            ThisSum = 0;
        }
    }
    return MaxSum;
}

int main()
{
    int A[Num] = {-2, 1, -3, 4, -1, 2, 1, -5, 4, 0};
    int maxsum= MaxSubseqSum4(A, Num);
    printf("%d\n", maxsum);

    return 0;
}
