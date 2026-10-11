int is_palindrome(int n) {
    int reversed = 0;
    int original = n;
    while (n > 0) {
        reversed = reversed * 10 + n % 10;
        n = n / 10;
    }
    if (reversed == original) return 1;
    return 0;
}

int main() {
    printf("%d\n", is_palindrome(12321));
    printf("%d\n", is_palindrome(12345));
    return 0;
}
