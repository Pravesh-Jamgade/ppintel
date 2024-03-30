#include <iostream>


int SIZE = 1e7;
int *a = (int*) malloc(sizeof(int) * SIZE);

void LOADS()
{
    int sum = 0;
    for(int i=0; i< SIZE; i++)
    {
        sum += i;
        a[i] = sum;
    }

    std::cout << "ignore me " << sum << '\n';
}

int main()
{

    for(int i=0; i< 1; i++)
        LOADS();
    return  0;
}