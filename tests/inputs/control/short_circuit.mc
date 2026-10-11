int main() {
    int x = 10;
    if (x > 5 && x < 20) {
        printf("%d\n", 1);
    }
    if (x > 5 && x < 8) {
        printf("%d\n", 0);
    }
    if (x < 5 || x > 8) {
        printf("%d\n", 1);
    }
    return 0;
}
