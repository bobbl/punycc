/*
    Check if correct exit code 100 is returned

        ./make.sh foo compile_native
        cat host_or1k.c porting/test/step10.c \
            | build/punycc_foo.native > exec.foo
        chmod +x exec.foo && qemu-foo exec.foo || echo $?

    expected output:

        foo bar
        foo foo bar
        foo foo bar
        foo foo bar
        foo foo bar

*/

int func01(int a)
{
    return (a & 1);
}


int func02(int i)
{
    while (i < 10) {
        write(1, "foo ", 4);
        if (func01(i)) {
           write(1, "bar\x0d\x0a", 5);
        }
        i = i + 1;
    }

    return i*11;
}

int main()
{
    return func02(1);
}
