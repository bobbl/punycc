/*
    Check if correct exit code 70 is returned

        ./make.sh foo compile_native
        cat host_or1k.c porting/test/step07.c \
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
    b = 1;
    a = 70;
    e = 2;
    d = a;
    c = e;
    return d;
}
