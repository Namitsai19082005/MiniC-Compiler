int main() {
    int x = 10;
    int* p = &x;
    int** pp = &p;
    printf("%d\n", **pp);
    return 0;
}
