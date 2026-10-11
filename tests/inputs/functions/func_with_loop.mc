int apply_twice(int x, int n) {
    for (int i = 0; i < n; i = i + 1) {
        x = x * 2;
    }
    return x;
}

int main() {
    printf("%d\n", apply_twice(1, 10));
    printf("%d\n", apply_twice(3, 5));
    return 0;
}
