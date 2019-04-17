void swapArray(int *a, int *b, int n)
{
    int i;
    int temp;
    for (i=0; i < n; i++)
    {
        temp = a[i];
        a[i] = b[i];
        b[i] = temp;
    }

}
