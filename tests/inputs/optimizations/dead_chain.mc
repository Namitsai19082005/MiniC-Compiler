int main() {
    int x = 100;
    int y = 200;
    int z = 300;
    // Lots of dead code
    int a = x + y;
    int b = y + z;
    int c = a + b;
    int d = c * 2;
    int e = d - 1;
    // Only f is used
    int f = x + y + z;
    printf("%d\n", f);
    return 0;
}
