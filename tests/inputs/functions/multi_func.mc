int max(int a, int b) {
    if (a > b) return a;
    return b;
}

int min(int a, int b) {
    if (a < b) return a;
    return b;
}

int clamp(int val, int lo, int hi) {
    return max(lo, min(val, hi));
}

int main() {
    printf("%d\n", clamp(15, 0, 10));
    printf("%d\n", clamp(-5, 0, 10));
    printf("%d\n", clamp(5, 0, 10));
    return 0;
}
