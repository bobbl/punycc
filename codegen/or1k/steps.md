Porting PunyCC to OpenRISC OR1K
===============================



Step 1: Preparations for new ISA
--------------------------------

  * Install simulator `qemu-or1k` and C compiler toolchain `gcc-or1k-elf`
    for OpenRISC

        sudo apt install qemu-or1k gcc-or1k-elf

  * Add or1k target to `make.sh`.
  * Copy `emit_steps.c` to `emit_or1k.c`.
  * Adjust endianess in `set_32bit()` and `get_32bit()` to big endian.
  * Create empty file `host_or1k.c`.

Tests for correctness

  * Check if a compiler is build for the native host without errors

        ./make.sh or1k compile_native

  * Check if the compiler runs without errors on a valid C input file

        echo "int main(){}" | ./build/punycc_or1k.native > exec.or1k

  * Check if the output (`exec.or1k`) is always an empty file.



Step 2: Output valid ELF file
-----------------------------

  * Modify `emit_begin()` to output a valid ELF file that executes some
    machine instructions to do an exit syscall with argument 20.
  * `emit_end()` is called at the end of the compilation and can be used
    to write values that are not known earlier (e.g. file size).

Check if correct exit code 20 is returned

    ./make.sh or1k compile_native
    echo "int main(){}" | ./build/punycc_or1k.native > exec.or1k
    chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?



Step 3: Call binary main function
---------------------------------

  * Modify the startup code to do a call to the `main()` function and use the
    return value of this call as argument to the exit syscall.
  * Write an assembly function that returns 30. Translate it to binary and
    write a C program `return30.c` with the help of the PunyCC pragma extension

        int main() _Pragma("PunyC emit \x00...\x00");

  * Implement `emit_binary_func()` to support the pragma machine code
    generation
  * Implement `emit_fix_call()` to set the address of the main function in the
    startup code.
  * As `emit_call()` is closely related to `emit_fix_call()` it might be a good
    idea to implement it now. But it won't be tested before step 5.

Check if correct exit code 30 is returned by the C program

    ./make.sh or1k compile_native
    ./build/punycc_or1k.native < return30.c > exec.or1k
    chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?



Step 4: Main function with return 40
------------------------------------

Goal of this step is to compile a simple source code file that returns 40 in
the main routine.

  * Implement `emit_number()` to set the accumulator to a value (40 here)
  * Implement `emit_return()` to return from a function
  * Although `emit_push()` is not required at this stage, it might be a
    good idea to implement it now.

Check if correct exit code 40 is returned

    ./make.sh or1k compile_native
    echo "int main(){return 40;}" | ./build/punycc_or1k.native > exec.or1k
    chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?



Step 5: Call library function to exit
-------------------------------------

  * Write the machine code of the `exit()` library function to `host_or1k.c`
  * Implement `emit_call()` if it wasn't implemented yet.
  * Implement `emit_arg()` to pass the exit code to the library function.

Check if correct exit code 50 is returned

    ./make.sh or1k compile_native
    cat host_or1k.c porting/test/step05.c \
        | ./build/punycc_or1k.native > exec.or1k
    chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?



Step 6: String constants
------------------------

  * Write the machine code of the remaining required standard library functions
    to `host_or1k.c`: `getchar()`, `malloc()`, `write()`, `read()`.
    Only `write()` will be tested now.
  * Implement `emit_string()` to put the address of a string constant in the
    accumulator.

Check if the hello word program works

    ./make.sh or1k compile_native
    cat host_or1k.c test06.c | ./build/punycc_or1k.native > exec.or1k
    chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?



Step 7: Global variables
------------------------

  * Implement `emit_global_var()`
  * Implement `emit_store()`
  * Implement `emit_load()`
  * Depending on architecture: modify `emit_end()` to reserve memory for global
    variables

Check

    ./make.sh or1k compile_native
    cat host_or1k.c test07.c | ./build/punycc_or1k.native > exec.or1k
    chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?



Step 8: Arithmetic operations
-----------------------------

  * Implement `emit_push()` if it wasn't implemented yet.
  * Implement `emit_operation()`

Check

    ./make.sh or1k compile_native
    cat host_or1k.c test08.c | ./build/punycc_or1k.native > exec.or1k
    chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?



Step 9: Compare operations
--------------------------

  * Implement `emit_comp()`

Check

    ./make.sh or1k compile_native
    cat host_or1k.c test09.c | ./build/punycc_or1k.native > exec.or1k
    chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?



Step 10: if and while
---------------------

  * Implement `emit_pre_while()`
  * Implement `emit_if()`
  * Implement `emit_then_end()`
  * Implement `emit_else_end()`
  * Implement `emit_then_else()`
  * Implement `emit_loop()`

Check

    ./make.sh or1k compile_native
    cat host_or1k.c test10.c | ./build/punycc_or1k.native > exec.or1k
    chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?



Step 11: Access function arguments
----------------------------------

  * Modify `emit_load()` to read function arguments
  * Modify `emit_store()` to write function arguments
  * Implement `emit_func_begin()` to create a stack frame and save registers
  * Implement `emit_func_end()` to remove the stack frame and restore registers

Check

    ./make.sh or1k compile_native
    cat host_or1k.c test11.c | ./build/punycc_or1k.native > exec.or1k
    chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?



Step 12: Local variables
------------------------

  * Modify `emit_load()` to read local variables
  * Modify `emit_store()` to write local variables
  * Implement `emit_local_var()` to save space for a local variable
  * Implement `emit_scope_begin()`
  * Implement `emit_scope_end()` to free no more used variables

Check

    ./make.sh or1k compile_native
    cat host_or1k.c test12.c | ./build/punycc_or1k.native > exec.or1k
    chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?



Step 13: Function result as argument
------------------------------------

If a function is called within an argument list of another function:

    func_outer(1, func_inner(5, 6), 3);

and the arguments are passed in registers, the inner arguments overwrite
the outer arguments because the same registers are used. There is no
problem if the arguments are passed on a stack in memory.

To solve the problem, implement `emit_pre_call()` to save the registers
with the outer arguments and restore them in a modified `emit_call()`.

Check

    ./make.sh or1k compile_native
    cat host_or1k.c test13.c | ./build/punycc_or1k.native > exec.or1k
    chmod +x exec.or1k && qemu-or1k exec.or1k || echo $?



Step 14: Arrays
---------------

  * Implement `emit_index_push`
  * Implement `emit_pop_store_array`
  * Implement `emit_index_load_array`

Check

    ./make.sh or1k test_self test_tox86 test_cc500
    ./make.sh test_full
    ./make.sh compile_all stats
