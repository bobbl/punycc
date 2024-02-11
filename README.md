Puny C
======

> *Limited expressiveness unlimited portability*

Very small cross compiler for a subset of C.

Features
  * Supported target and host architectures: **RISC-V**, **ARM-Thumb-2** and **x86**.
  * Valid source code for Puny C is also valid C99 and can be written in a way
    that gcc or clang compile it without any warning.
  * Code generation is designed to be easily portable to other target
    architectures.
  * Fast compilation, small code size.

Inspired by
  * [cc500](https://github.com/8l/cc500) -
    a tiny self-hosting C compiler by Edmund Grimley Evans
  * [Obfuscated Tiny C Compiler](https://bellard.org/otcc/) -
    very small self compiling C compiler by Fabrice Bellard
  * [Tiny C Compiler](https://savannah.nongnu.org/projects/tinycc) -
    a small but hyper fast C compiler.
  * [Compiler Construction](https://people.inf.ethz.ch/wirth/CompilerConstruction/index.html) -
    brief but comprehensive book by Niklaus Wirth.

Language restrictions
---------------------

  * No linker.
  * No preprocessor.
  * No standard library.
  * No `typedef`.
  * No type checking. Variable types are always `unsigned int`, except if
    indexed with `[]` then the type is `char *`.
  * Any combination of `unsigned`, `long` `int`, `char`, `void` and `*` is
    accepted as valid type.
  * Type casts are allowed, but ignored.
  * Constants: only decimal, character and string without backslash escape
  * Statements: `if`, `while`, `return`.
  * Variable declaration: C99-style statements.
  * Operators: no unary, ternary, extended assignment.
  * Operator precedence: simplified, use parentheses instead.

| level | operator             | description             |
| ----- | -------------------- | ----------------------- |
|   1   | [] ()                | array and function call |
|   2   | + - << >> & ^ &#124; | binary operation        |
|   3   | < <= > >= == !=      | comparison              |
|   4   | =                    | assignment              |



Low-Level Functions
-------------------

There is no inline assembler for functions that directly access the operating
system (e.g. file I/O). But code can be written in pure binary:

    void exit(int) _Pragma("PunyC emit \x58\x5b\x31\xc0\x40\xcd\x80");
    /*  58      pop eax
        5b      pop ebx
        31 c0   xor eax, eax
        40      inc eax
        cd 80   int 128 */

Other compilers ignore the `_Pragma` statement, which turns the line into a
forward declaration where libc can be linked against.



Usage
-----

To build punycc for all target architectures use

    ./make.sh clang

The executables are named `build/punycc_ARCH.clang`.
They read C source code from stdin and write an executable to stdout:

    ./punycc_x86.clang < foo.c > foo.x86

To execute `foo` it must be made executable:

    chmod 775 foo.x86
    ./foo.x86

A cross compiled executable can be emulated with qemu:

    ./punycc_rv32.clang < foo.c > foo.rv32
    chmod 775 foo.rv32
    qemu-riscv32 foo.rv32

There is no standard library or standard include files. Everything must be in
the single source code file. The `host_ARCH.c` files have some rudimentary
implementations of standard functions that are needed for the compiler.
Use them by concatenating files:

    cat host_rv32.c hello.c | ./punycc_rv32.clang > hello.rv32





Implementation Details
----------------------

Each compiler consists of three parts:

 1. Host-specific standard functions for i/o in `host_ARCH.c`
 2. Target-specific code generation in `emit_ARCH.C`
 3. Architecture independent compiler parts (scanner, parser and symbol table)

Concatenate the three files and compile it, for example

    cat host_x86.c emit_x86.c punycc.c | ./punycc_x86.clang > punycc_x86.x86

Cross compilers can be built by using a different `ARCH` for `host_` and `emit_`:

    cat host_x86.c emit_armv6m._c punycc.c | ./punycc_x86.clang > punycc_armv6m.x86


### Memory Management

There is only one buffer `buf`.
The code grows from 0 upwards, the symbol table grows from the top downwards.
The token buffer for strings and identifiers is dynamically allocated in the
space between them:

    0   code_pos     code_pos+256         sym_head-256      sym_head   buf_size
                       token_buf      token_buf+token_size
    +------+---------------+-------------------+---------------+--------------+
    | code |   256 bytes   | identifier/string |   256 bytes   | symbol table |
    +------+---------------+-------------------+---------------+--------------+


### Symbol Table

The symbol table starts at sym_head at ends at the end of the buffer. It is the
concatination of symbol entries with the following format:

| offset | size    | description             |
| ------:| -------:| ----------------------- |
|     0  | 4 bytes | address (little endian) |
|     4  | 1 byte  | symbol type             |
|     5  | 1 byte  | n: length of name       |
|     6  | n bytes | name                    |


### Code Generation

The functions prefixed by `emit_` are used to generate the machine code. The
template in [emit_template.c](emit_template.c) documents all functions and can be used as
starting point for a new architecture backend.
