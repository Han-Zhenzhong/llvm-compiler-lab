// Simple test for TinyDSP backend

int test_add(int a, int b) {
    return a + b;
}

int test_sub(int a, int b) {
    return a - b;
}

int test_mul(int a, int b) {
    return a * b;
}

int test_complex(int x, int y) {
    int z = x + y;
    z = z * 2;
    return z - 1;
}
