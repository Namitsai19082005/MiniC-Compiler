int main() {
    int x = 10;
    int y = 20;
    int z = 30;
    int* p = &x;
    printf("%d\n", *p);
    p = &y;
    printf("%d\n", *p);
    p = &z;
    printf("%d\n", *p);
    return 0;
}
