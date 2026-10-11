int main() {
    int x = 5;
    int y = x;
    int z = y;
    int w = z;
    // Should propagate all the way back to x=5
    printf("%d\n", w);
    return 0;
}
