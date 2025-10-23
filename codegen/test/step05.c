/*
    Check if correct exit code 50 is returned

        ./make.sh foo compile_native
        cat host_or1k.c porting/test/step05.c \
            | build/punycc_foo.native > exec.foo
        chmod +x exec.foo && qemu-foo exec.foo || echo $?

*/

int main()
{
    exit(50);
}
