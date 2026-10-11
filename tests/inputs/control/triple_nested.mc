int main() {
    for (int i = 0; i < 3; i = i + 1) {
        for (int j = 0; j < 3; j = j + 1) {
            for (int k = 0; k < 3; k = k + 1) {
                if (i + j + k == 3) {
                    printf("%d %d %d\n", i, j, k);
                }
            }
        }
    }
    return 0;
}
