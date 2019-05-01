#include <stdio.h>
int main(int argc, const char **argv)
{
    int a, num;
    scanf("%i", &num);
    for (int i = 0; i < num; i++)
    {
        for (int j = 0; j < num; j++)
            a = i + j;
    }
    printf("%i\n", a);
    return 0;
}
