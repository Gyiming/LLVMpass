void indexincre(int *A, int *B, int n)
{
    int i,j;
    for (i=0;i<n;i++)
        for (j=0;j<n;j++)
    {
        A[i+1]=A[j];
    }
}
