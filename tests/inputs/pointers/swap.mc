void swap(int* a, int* b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

int main() {
    int x = 10;
    int y = 20;
    swap(&x, &y);
    printf("%d\n", x);
    printf("%d\n", y);
    return 0;
}
