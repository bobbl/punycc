/*
    Check if correct exit code 40 is returned

        ./make.sh foo compile_native
        cat porting/test/step04.c | build/punycc_foo.native > exec.foo
        chmod +x exec.foo && qemu-foo exec.foo || echo $?

*/

int main()
{
    return 40;
}
