int main()
{
    int a;
    int b = 5;
    int c = 10;
    int d;

    a = b;
    d = c;
    b = d;
    c = a;

    return c;
}