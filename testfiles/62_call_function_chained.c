int add(int a, int b)
{
    return a + b;
}

int multiply(int a, int b)
{
    return a * b;
}

int main()
{
    int x = 3;
    int y = 4;
    int z = 5;
    int result = multiply(add(x, y), z);
    return result;
} 