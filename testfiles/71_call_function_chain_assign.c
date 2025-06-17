int setValue(int x)
{
    return x;
}

int main()
{
    int a = 5;
    int b = 10;
    int result = setValue(a = b);
    return result;
} 