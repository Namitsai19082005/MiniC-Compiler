int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int main() {
    printf("%d\n", gcd(48, 18));
    printf("%d\n", gcd(100, 75));
    return 0;
}
