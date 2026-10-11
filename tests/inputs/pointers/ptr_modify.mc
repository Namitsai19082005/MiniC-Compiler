void modify(int* p) {
    *p = *p + 10;
}

int main() {
    int x = 5;
    modify(&x);
    printf("%d\n", x);
    modify(&x);
    printf("%d\n", x);
    return 0;
}
