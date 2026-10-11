int digit_sum(int n) {
    int sum = 0;
    while (n > 0) {
        sum = sum + n % 10;
        n = n / 10;
    }
    return sum;
}

int main() {
    printf("%d\n", digit_sum(12345));
    printf("%d\n", digit_sum(99999));
    return 0;
}
