/*
    Check if correct exit code 120 is returned

        ./make.sh foo compile_native
        cat host_or1k.c porting/test/step12.c \
            | build/punycc_foo.native > exec.foo
        chmod +x exec.foo && qemu-foo exec.foo || echo $?

    expected output:

        foo bar
        foo foo bar
        foo foo bar
        foo foo bar
        foo foo bar

*/

void func01()
{
    int i;
    i = 20;
    return i-19;
}

int func02(int a)
{
    int b;
    int c;
    b = 1;
    c = a;
    return (b & c);
}


int main()
{
    int i;
    i = 1;
    while (i < 10) {
        func01();
        write(1, "foo ", 4);
        if (func02(i)) {
           write(1, "bar\x0d\x0a", 5);
        }
        i = i + 1;
    }

    return i*12;
}
