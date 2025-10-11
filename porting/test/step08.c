/*
    Check if correct exit code 80 is returned

        ./make.sh foo compile_native
        cat host_or1k.c porting/test/step08.c \
            | build/punycc_foo.native > exec.foo
        chmod +x exec.foo && qemu-foo exec.foo || echo $?

*/

unsigned int a;
unsigned int b;
unsigned int c;
unsigned int d;
unsigned int e;

int main()
{
    a = 3;
    b = 4;
    c = 5;
    d = ((a << 1) + b) - 5; /* == 5 */
    e = 128 >> (1 & c) ; /* == 64 */
    return d + 11 + e;
}
