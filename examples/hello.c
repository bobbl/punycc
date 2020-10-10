/* Hello world example.
 * Must be prefixed with architecture-specific code for write().
 *
 * Example for RISC-V:
 *
 *      cat host_rv32.c hello.c > hello_rv32.c
 *      ./punycc_rv32.clang     < hello_rv32.c > hello.rv32
 *      chmod 775 hello.rv32
 *      qemu-riscv32 hello.rv32
 */


int main()
{
    write(1, "Hello World\x0d\x0a", 13);
      /* 1 is stdin, "\x0d\x0a" is a newline, 13 is the length of the string */
}