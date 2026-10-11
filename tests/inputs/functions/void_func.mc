void print_range(int start, int end) {
    for (int i = start; i <= end; i = i + 1) {
        printf("%d\n", i);
    }
}

int main() {
    print_range(1, 5);
    return 0;
}
