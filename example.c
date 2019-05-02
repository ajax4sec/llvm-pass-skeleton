#include <stdio.h>
#include <stdlib.h>

int cal(int * array, int num){
    int i = 1, sum = 0;
    while(i < num){
        sum = sum + array[i] + 2;
        i++;
    }
    return sum;
}

int main(int argc, const char **argv)
{
    int num;
    int * buffer;
    scanf("%i", &num);
    for (int i = 1; i < num; i++)
    {
        buffer = (int *)malloc(num * sizeof(int));
        for (int j = 0; j < 4; j++)
        {
            buffer[i-1] = i + j;
        }
    }
    printf("%i\n", cal(buffer, num));
    return 0;
}
