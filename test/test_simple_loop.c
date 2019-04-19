void simple(int *A)
{
    int i,n;
    n=10;
    for (i=0;i<n;i++)
    {
        A[i+1] = A[i];
    }
}
