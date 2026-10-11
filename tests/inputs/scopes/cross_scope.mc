int main() {
    int x = 1;
    {
        int y = 2;
        {
            int z = 3;
            {
                int w = 4;
                printf("%d\n", x + y + z + w);
            }
        }
    }
    return 0;
}
