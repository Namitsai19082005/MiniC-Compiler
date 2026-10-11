int power(int base, int exp) {
    int result = 1;
    for (int i = 0; i < exp; i = i + 1) {
        result = result * base;
    }
    return result;
}

int main() {
    printf("%d\n", power(2, 10));
    printf("%d\n", power(3, 5));
    return 0;
}
