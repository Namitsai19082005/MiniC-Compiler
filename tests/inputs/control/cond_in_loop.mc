int main() {
    int sum = 0;
    for (int i = 0; i < 100; i = i + 1) {
        if (i % 2 == 0) {
            sum = sum + i;
        }
    }
    printf("%d\n", sum);
    return 0;
}
