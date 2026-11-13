//暴力穷举，复杂度n三次方
#include <stdio.h>
#define Num 10
int MaxSubseqSum1(int A[], int N)
{
    int ThisSum = 0, MaxSum = 0;
    int i, j, k ;
    for(i = 0; i < N ; i ++)
    {
        for(j = i; j < N; j ++)
        {
            ThisSum = 0;
            for(k = i; k < j; k ++){
                ThisSum += A[k];
            }
            if(ThisSum > MaxSum){
                MaxSum = ThisSum;
            }
        }
    }
            
    return MaxSum;
}

int main()
{
    int A[Num] = {-2, 1, -3, 4, -1, 2, 1, -5, 4, 0};
    int maxsum= MaxSubseqSum1(A, Num);
    printf("%d\n", maxsum);

    return 0;
}
