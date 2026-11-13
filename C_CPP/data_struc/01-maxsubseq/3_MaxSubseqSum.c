//分而治之递归求和，复杂度nlogn
#include <stdio.h>
#define Num 10
int Max3(int A , int B , int C)
{   //返回三个数的最大值
    return A > B ? (A > C ? A : C ): (B > C ? B : C);
}

int DivideAndConquer(int List[], int left, int right )
{   //分而治之求List[left]到List[right]的最大系列和
    int MaxLeftSum, MaxrightSum;//存放左右子问题的解
    int MaxLeftBorderSum, MaxRightBorderSum;//存放分界线的结果

    int LeftBorderSum, RightBorderSum;
    int center, i;

    if( left == right )//递归终止条件：子列只有1个数字
    {
        if( List[left] > 0 ) return List[left];
        else return 0;
    }
    //下面是‘分’的过程
    center = (left + right) / 2;
    //递归求得两边子列最大和
    MaxLeftSum = DivideAndConquer(List, left, center);
    MaxrightSum = DivideAndConquer(List, center+1, right);

    //下面求跨分界线的最大子列和
    MaxLeftBorderSum = 0; LeftBorderSum = 0;
    for(i = center; i >= left; i --)//从中线向左扫描
    {
        LeftBorderSum += List[i];
        if(LeftBorderSum > MaxLeftBorderSum){
            MaxLeftBorderSum = LeftBorderSum;
        }
    }//左边扫描结束

    MaxRightBorderSum = 0; RightBorderSum = 0;
    for(i = center + 1; i <= right; i ++)//中线向右扫描
    {
        RightBorderSum += List[i];
        if(RightBorderSum > MaxRightBorderSum){
            MaxRightBorderSum = RightBorderSum;
        }
    }//右边扫描结束

    //下面是‘治’的结果
    return Max3(MaxLeftSum, MaxrightSum, MaxLeftBorderSum + MaxRightBorderSum);
}

int MaxSubseqSum3(int List[], int N)
{
    if (List == NULL || N <= 0)
        return 0;                       // 空数组：按"允许空子列"约定返回 0
    return DivideAndConquer(List, 0, N - 1);
    
}

int main()
{
    int A[Num] = {-2, 1, -3, 4, -1, 2, 1, -5, 4, 0};
    int maxsum= MaxSubseqSum3(A, Num);
    printf("%d\n", maxsum);

    return 0;
}
