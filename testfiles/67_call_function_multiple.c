int increment(int x)
{
    return x + 1;
}

int decrement(int x)
{
    return x - 1;
}

int main()
{
    int value = 10;
    int result1 = increment(value);
    int result2 = decrement(value);
    int final = result1 + result2;
    return final;
} 