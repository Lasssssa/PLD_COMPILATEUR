int inner(int x)
{
    return x * 2;
}

int outer(int y)
{
    return inner(y) + 1;
}

int main()
{
    int value = 7;
    int result = outer(value);
    return result;
} 