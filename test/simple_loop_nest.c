int main(void) {
    int X = 1;
    int A[10][10];
    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++) {
            A[i+1][j] = A[i][j] + X;
        }
    }
}
