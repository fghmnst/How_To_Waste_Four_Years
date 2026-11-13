#include<stdio.h>
#include<time.h>
#include<math.h>
#define MAXN 10
#define MAXK 1e7
#define X_VAL 6.66
double duration;
clock_t start,stop;

double f1(int n , double a[] , double x)
{
    double s = 0.0;
    for(int i = 0 ; i <= n ; i ++){
        s += a[i] * pow(x,i);
    }

    return s;
}
double f2(int n , double a[] , double x)
{
    double s = a[n];
    for(int i = n; i > 0 ; i --){
        s = a[i - 1] + x * s;
    }

    return s;
}
int main()
{   
    double a[MAXN] = {0};
    for(int i = 0; i < 10; i ++){
        a[i] = i;
    }

    //fun1
    start = clock();
    for(int k = 0; k < MAXK; k ++){
    f1(MAXN - 1 , a , X_VAL);
    }
    stop = clock();

    duration = (double)(stop - start) / CLOCKS_PER_SEC / MAXK;
    
    printf("ticks1 = %.2lf\n", (double)(stop - start));
    printf("duration1 = %.2e\n", duration);

    //fun2
    start = clock();
    for(int k = 0; k < MAXK; k ++){
    f2(MAXN - 1 , a , X_VAL);
    }
    stop= clock();
    
    duration = (double)(stop - start) / CLOCKS_PER_SEC / MAXK;

    printf("ticks2 = %.2lf\n", (double)(stop - start));
    printf("duration2 = %.2e\n", duration);

    return 0;
}
