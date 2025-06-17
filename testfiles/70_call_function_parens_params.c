int calculate(int a, int b, int c)
{
    return a + b * c;
}

int main()
{
    int x = 2;
    int y = 3;
    int z = 4;
    int result = calculate(x, (y + 1), z);
    return result;
} 