/* Test RV32 correct call address of epilogue when using more than 12 local
 * variables are used. Bug reported by danous in PR #1.
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

void func01(unsigned int a, unsigned int b)
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
}



int main()
{
    buf = malloc(1000);

    print_hex(func01(1, 2)); /* should be 000004EF */
    write(1, "\x0d\x0a", 2);
    return 0;
}
