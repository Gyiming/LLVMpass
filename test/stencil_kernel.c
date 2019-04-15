#define R (2)
#define R1 (R + 1)
#define WSize (2 * R + 1)
int main() {
    float weight[WSize][WSize];
    for (int jj = 1; jj <= 2; jj++) {
        float element = (1.0/(4.0*jj*(2.0*jj-1)*R));
        for (int ii = R1+(-jj+1); ii < R1+jj; ii++) {
            weight[ ii][R1+jj] = element;
            weight[ ii][R1-jj] = -element;
            weight[R1+jj][ ii] = element;
            weight[R1-jj][ ii] = -element;
        }
        weight[R1+jj][R1+jj] = (1.0/(4.0*jj*R));
        weight[R1-jj][R1-jj] = -(1.0/(4.0*jj*R));
    }
}
