/*
    OpenRISC assembly code

    Check if correct exit code 30 is returned

        ./make.sh foo compile_native
        cat porting/test/step03.or1k.c | build/punycc_foo.native > exec.or1k
        chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?

*/

int main() _Pragma("PunyC emit \xa8\x60\x00\x1e\x44\x00\x48\x00\x15\x00\x00\x00");
    /*  l.ori r3, r0, 30
        l.jr  r9
        l.nop 0 */
