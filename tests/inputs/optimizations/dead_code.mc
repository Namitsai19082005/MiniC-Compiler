int main() {
    int a = 1;
    int b = 2;
    int c = a + b;
    // d is dead — never used after assignment
    int d = c * 2;
    printf("%d\n", c);
    return 0;
}
