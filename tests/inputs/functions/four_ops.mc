int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int mul(int a, int b) { return a * b; }
int div_i(int a, int b) { return a / b; }

int main() {
    int x = 10;
    int y = 3;
    printf("%d\n", add(x, y));
    printf("%d\n", sub(x, y));
    printf("%d\n", mul(x, y));
    printf("%d\n", div_i(x, y));
    return 0;
}
