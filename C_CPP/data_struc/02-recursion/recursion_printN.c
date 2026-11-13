#include <stdio.h>
void printN(int n)
{
    if(n)
    {
        printN(n - 1);
        printf("%d\n", n);
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