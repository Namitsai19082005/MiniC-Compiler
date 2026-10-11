int sum(int n) {
    int s = 0;
    for (int i = 1; i <= n; i = i + 1) {
        s = s + i;
    }
    return s;
}

int main() {
    printf("%d\n", sum(100));
    return 0;
}
