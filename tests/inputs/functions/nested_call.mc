int identity(int x) {
    return x;
}

int main() {
    printf("%d\n", identity(identity(identity(42))));
    return 0;
}
