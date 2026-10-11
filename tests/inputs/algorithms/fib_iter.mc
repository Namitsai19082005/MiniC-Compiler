int main() {
    int n = 20;
    int a = 0;
    int b = 1;
    for (int i = 0; i < n; i = i + 1) {
        printf("%d\n", a);
        int temp = a + b;
        a = b;
        b = temp;
    }
    return 0;
}
