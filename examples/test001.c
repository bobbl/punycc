/* Test RV32 correct call address of epilogue when using more than 12 local
 * variables are used. Bug reported by danodus in PR #1.
 *
 * Must be prefixed with architecture-specific code for write().
 *
 * Example for RISC-V:
 *
 *      cat host_rv32.c test001.c > test001_rv32.c
 *      ./punycc_rv32.clang     < test001_rv32.c > test001.rv32
 *      chmod +x test001.rv32
 *      qemu-riscv32 test001.rv32
 */



char *buf;

void print_hex(unsigned int h)
{
    unsigned int i = 0;
    while (i < 8) {
        unsigned int ch = (h >> ((7-i) << 2)) & 15;
        if (ch > 9) {
            ch = ch + 55;
        }
        else {
            ch = ch + 48;
        }
        buf[i] = ch;
        i = i + 1;
    }
    write(1, (char *)buf, 8);
}

unsigned int func01(unsigned int a, unsigned int b)
{
    unsigned int c = 30;
    unsigned int d = 40;
    unsigned int e = 50;
    unsigned int f = 60;
    unsigned int g = 70;
    unsigned int h = 80;
    unsigned int i = 90;
    unsigned int j = 100;
    unsigned int k = 110;
    unsigned int l = 120;
    unsigned int m = c * b;
    unsigned int n = 140;
    unsigned int o = 150;
    unsigned int p = 160;
    unsigned int q = a+b+c+d+e+f+g+h+i+j+k+l+m+n+o+p;
    return q;
}

unsigned int test_compare(unsigned int a, unsigned int b)
{
    return ((a <= b)<<5) | ((a > b)<<4) | ((a >= b)<<3) | ((a < b)<<2) |
           ((a != b)<<1) | (a == b);
}



int main()
{
    unsigned int error = 0;
    buf = malloc(1000);

    /* test number of local variables */
    unsigned int x = func01(1, 2);
    if (x != 1263) error = error | 1;
    print_hex(x); /* should be 000004EF */
    write(1, "\x0d\x0a", 2);

    /* test for expression stack overflow */
    x = 15 - (14 - (13 - (12 - (11 - (10 - (9 - (8 - (7 - (6 - (5 - (4 - (3 - (2 - 1)))))))))))));
    if (x != 8) error = error | 2;
    print_hex(x); /* should be 00000008 */
    write(1, "\x0d\x0a", 2);

    /* test conversion of comparison to int */
    x = test_compare(1, 2);                /* should be 00000026 */
    unsigned int y = test_compare(20, 10); /* should be 0000001A */
    unsigned int z = test_compare(3, 3);   /* should be 00000029 */
    x = (x << 16) | (y << 8) | z;
    if (x != 2497065) error = error | 4;
    print_hex(x); /* should be 00261A29 */
    write(1, "\x0d\x0a", 2);

    return error;
}
