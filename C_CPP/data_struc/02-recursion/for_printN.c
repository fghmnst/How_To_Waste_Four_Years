#include <stdio.h>
void printN(int n)
{
    for(int i = 1; i <= n; i ++)
    {
        printf("%d\n",i);
    }
    return;
}

int main()
{
    int N = 0;
    scanf("%d",&N);
    printN(N);
    return 0;
}