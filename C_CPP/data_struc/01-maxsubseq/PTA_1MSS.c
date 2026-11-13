#include <stdio.h>
int main()
{
    int K;
    scanf("%d", &K);

    int ThisSum = 0;
    int MaxSum = 0;
    int num;

    for(int i = 0; i < K; i ++){
        num = 0;
        scanf("%d", &num);
        ThisSum += num;

        if(ThisSum > MaxSum){
            MaxSum = ThisSum;
        }

        if(ThisSum < 0){
            ThisSum = 0;
        }
    }

    printf("%d\n", MaxSum);

    return 0;

}
