/*
    Check if correct exit code 120 is returned

        ./make.sh foo compile_native
        cat host_or1k.c porting/test/step12.c \
            | build/punycc_foo.native > exec.foo
        chmod +x exec.foo && qemu-foo exec.foo || echo $?

    expected output:

        My name is Jonas

*/

int func01(char *s, int r)
{
    return r;
}

int func02(char *s, unsigned int l, int r, int x)
{
    write(1, s, l);
    write(1, "Jonas\x0d\x0a", 7);
    return r;
}

int main()
{
    return func02("My name is ", 11, func01("Weezer", 130), 99);
}
