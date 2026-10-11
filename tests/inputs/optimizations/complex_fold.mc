int main() {
    int a = 1;
    int b = 2;
    int c = 3;
    // Complex expression with many sub-expressions
    int result = (a + b) * (c - a) + (b * c) - (a + b + c) * 2 + a;
    printf("%d\n", result);
    return 0;
}
