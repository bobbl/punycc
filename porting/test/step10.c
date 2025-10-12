/*
    Check if correct exit code 100 is returned

        ./make.sh foo compile_native
        cat host_or1k.c porting/test/step10.c \
            | build/punycc_foo.native > exec.foo
        chmod +x exec.foo && qemu-foo exec.foo || echo $?

    expected output:

        if ok
        three times
        three times
        three times

*/

unsigned int a;
unsigned int b;

int main()
{
    a = 1;
    b = 4;

    if (0) {
        exit(1);
    }
    else {
        if (1) {
            if (b > 7) {
                exit(2);
            }
            if (a == 1) {
                write(1, "if ok\x0d\x0a", 7);
            }
            else {
                exit(3);
            }
        }
        else {
            exit(4);
        }
    }

    while (a <= 3) {
        write(1, "three times\x0d\x0a", 13);
        a = a + 1;
    }

    exit(100);
        /* return doesn't work because the call to write() corrupts the return
           address in r9 */
}
