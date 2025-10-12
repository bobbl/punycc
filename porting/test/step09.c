/*
    Check if correct exit code 90 is returned

        ./make.sh foo compile_native
        cat host_or1k.c porting/test/step09.c \
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
    d = (c > 6); /* == 0 */
    e = (a < b); /* == 1 */
    c = (c >= b); /* == 1 */
    return (d + (e << 1) + (c << 2) + ((9 <= a) << 3)) * 15;
}
