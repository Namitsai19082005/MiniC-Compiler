int abs_val(int x) {
    if (x < 0) return -x;
    return x;
}

int main() {
    printf("%d\n", abs_val(-42));
    printf("%d\n", abs_val(42));
    return 0;
}
