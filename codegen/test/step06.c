/*
    Check if "Hello, world!" appears and the correct exit code 60 is returned

        ./make.sh foo compile_native
        cat host_or1k.c porting/test/step06.c \
            | build/punycc_foo.native > exec.foo
        chmod +x exec.foo && qemu-foo exec.foo || echo $?

*/

int main()
{
    write(1, "Hello, world!\x0d\x0a", 15);
    exit(60);
}
