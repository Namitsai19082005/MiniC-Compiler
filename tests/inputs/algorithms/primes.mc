int main() {
    int n = 100;
    int count = 0;
    for (int i = 2; i <= n; i = i + 1) {
        int is_prime = 1;
        for (int j = 2; j * j <= i; j = j + 1) {
            if (i % j == 0) {
                is_prime = 0;
            }
        }
        if (is_prime) {
            count = count + 1;
        }
    }
    printf("%d\n", count);
    return 0;
}
