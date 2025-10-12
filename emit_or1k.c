/**********************************************************************
 * Code generation for OpenRISC OR1K
 **********************************************************************/




/**********************************************************************
 * Step 1: Preparations for new ISA
 **********************************************************************/

unsigned int buf_size;          /* total size of the buffer */
unsigned char *buf;             /* pointer to the buffer */
unsigned int code_pos;          /* position in the buffer for code generation */
unsigned int stack_pos;         /* current stack pointer */
unsigned int num_params;        /* number of function parameters */
unsigned int num_locals;        /* number of local variables in the current function */
unsigned int num_globals;       /* number of global variables */

unsigned int reg_pos;


static void error(unsigned int no);
/*
201 expression stack exceeded (r3...r11)
*/




/* CAUTION: OpenRISC is big endian by default */

/* write a 32 bit number to a char array */
void set_32bit(unsigned char *p, unsigned int x)
{
    p[3] = x;
    p[2] = x >> 8;
    p[1] = x >> 16;
    p[0] = x >> 24;
}

/* read 32 bit number from a char array */
unsigned int get_32bit(unsigned char *p)
{
    return p[3] +
          (p[2] << 8) +
          (p[1] << 16) +
          (p[0] << 24);
}




/**********************************************************************
 * Step 2: Output valid ELF file
 **********************************************************************/

unsigned int emit_n(unsigned int n, char *s)
{
    unsigned int cp = code_pos;
    unsigned int i = 0;
    while (i < n) {
        buf[cp + i] = s[i];
        i = i + 1;
    }
    code_pos = cp + n;
    return cp;
}

void emit32(unsigned int n)
{
    unsigned int cp = code_pos;
    code_pos = cp + 4;
    set_32bit(buf + cp, n);
}

/* Prepare for code generation
   Return the address of the call to main() as a forward reference */
unsigned int emit_begin()
{
    code_pos = 0;
    num_globals = 0;
    reg_pos = 3;

    emit_n(312, "\x7f\x45\x4c\x46\x01\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x5c\x00\x00\x00\x01\x00\x00\x20\x54\x00\x00\x00\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x34\x00\x20\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x20\x00........\x00\x00\x00\x07\x00\x00\x10\x00\x18\x00\x00\x00\x18\x40\x00\x00\xa8\x42\x00\x00\x00\x00\x00\x00\x15\x00\x00\x00\xa9\x60\x00\x5d\x20\x00\x00\x00\x9c\x21\xff\xb0\xd4\x01\x60\x00\xd4\x01\x68\x04\xd4\x01\x70\x08\xd4\x01\x78\x0c\xd4\x01\x80\x10\xd4\x01\x88\x14\xd4\x01\x90\x18\xd4\x01\x98\x1c\xd4\x01\xa0\x20\xd4\x01\xa8\x24\xd4\x01\xb0\x28\xd4\x01\xb8\x2c\xd4\x01\xc0\x30\xd4\x01\xc8\x34\xd4\x01\xd0\x38\xd4\x01\xd8\x3c\xd4\x01\xe0\x40\xd4\x01\xe8\x44\xd4\x01\xf0\x48\xd4\x01\xf8\x4c\xe1\xa3\x18\x04\xe1\xc4\x20\x04\xe1\xe5\x28\x04\xe2\x06\x30\x04\xe2\x27\x38\x04\xe2\x48\x40\x04\x44\x00\x48\x00\x85\x21\x00\x00\x85\xa1\x00\x04\x85\xc1\x00\x08\x85\xe1\x00\x0c\x86\x01\x00\x10\x86\x21\x00\x14\x86\x41\x00\x18\x86\x61\x00\x1c\x86\x81\x00\x20\x86\xa1\x00\x24\x86\xc1\x00\x28\x86\xe1\x00\x2c\x87\x01\x00\x30\x87\x21\x00\x34\x87\x41\x00\x38\x87\x61\x00\x3c\x87\x81\x00\x40\x87\xa1\x00\x44\x87\xc1\x00\x48\x87\xe1\x00\x4c\x9c\x21\x00\x50\x44\x00\x48\x00"
);

/*
elf_header:
    0000 7f 45 4c 46    e_ident         0x7F, "ELF"
    0004 01 02 01 00                    1, 2, 1, 0  (2=bigendian)
    0008 00 00 00 00                    0, 0, 0, 0
    000C 00 00 00 00                    0, 0, 0, 0
    0010 00 02          e_type          2 (executable)
    0012 00 5C          e_machine       92 (OpenRISC)
    0014 00 00 00 01    e_version       1
    0018 00 00 20 54    e_entry         0x00002000 + _start
    001C 00 00 00 34    e_phoff         program_header_table
    0020 00 00 00 00    e_shoff         0
    0024 00 00 00 00    e_flags         0
    0028 00 34          e_ehsize        52 (program_header_table)
    002A 00 20          e_phentsize     32 (start - program_header_table)
    002C 00 01          e_phnum         1
    002E 00 00          e_shentsize     0
    0030 00 00          e_shnum         0
    0032 00 00          e_shstrndx      0

program_header_table:
    0034 00 00 00 01    p_type          1 (load)
    0038 00 00 00 00    p_offset        0
    003C 00 00 20 00    p_vaddr         0x00002000
    0040 00 00 20 00    p_paddr         0x00002000
    0044 ?? ?? ?? ??    p_filesz
    0048 ?? ?? ?? ??    p_memsz
    004C 00 00 00 07    p_flags         7 (read, write, execute)
    0050 00 00 10 00    p_align         0x1000 (4 KiByte)

_start:
    0054 18 00 00 00    l.movhi r0, 0
    0058 18 40 ?? ??    l.movhi r2, ?   r2 points to the start of the data section
    005C A8 42 ?? ??    l.ori r2, r2, ?
    0060 00 00 00 00    l.jal main      but must be 0 for emit_fix_call()
    0064 15 00 00 00    l.nop 0
    006C A9 60 00 5D    l.ori r11, r0, 93
    0070 20 00 00 00    l.sys 0

_function_prolog:
   0:	9c 21 ff b0 	l.addi r1,r1,-80
   4:	d4 01 60 00 	l.sw 0(r1),r12
   8:	d4 01 68 04 	l.sw 4(r1),r13
   c:	d4 01 70 08 	l.sw 8(r1),r14
  10:	d4 01 78 0c 	l.sw 12(r1),r15
  14:	d4 01 80 10 	l.sw 16(r1),r16
  18:	d4 01 88 14 	l.sw 20(r1),r17
  1c:	d4 01 90 18 	l.sw 24(r1),r18
  20:	d4 01 98 1c 	l.sw 28(r1),r19
  24:	d4 01 a0 20 	l.sw 32(r1),r20
  28:	d4 01 a8 24 	l.sw 36(r1),r21
  2c:	d4 01 b0 28 	l.sw 40(r1),r22
  30:	d4 01 b8 2c 	l.sw 44(r1),r23
  34:	d4 01 c0 30 	l.sw 48(r1),r24
  38:	d4 01 c8 34 	l.sw 52(r1),r25
  3c:	d4 01 d0 38 	l.sw 56(r1),r26
  40:	d4 01 d8 3c 	l.sw 60(r1),r27
  44:	d4 01 e0 40 	l.sw 64(r1),r28
  48:	d4 01 e8 44 	l.sw 68(r1),r29
  4c:	d4 01 f0 48 	l.sw 72(r1),r30
  50:	d4 01 f8 4c 	l.sw 76(r1),r31
  54:	e1 a3 18 04 	l.or r13,r3,r3
  58:	e1 c4 20 04 	l.or r14,r4,r4
  5c:	e1 e5 28 04 	l.or r15,r5,r5
  60:	e2 06 30 04 	l.or r16,r6,r6
  64:	e2 27 38 04 	l.or r17,r7,r7
  68:	e2 48 40 04 	l.or r18,r8,r8
  6c:	44 00 48 00 	l.jr r9

_function_epilog:
  70:	85 21 00 00 	l.lwz r9,0(r1)
  74:	85 a1 00 04 	l.lwz r13,4(r1)
  78:	85 c1 00 08 	l.lwz r14,8(r1)
  7c:	85 e1 00 0c 	l.lwz r15,12(r1)
  80:	86 01 00 10 	l.lwz r16,16(r1)
  84:	86 21 00 14 	l.lwz r17,20(r1)
  88:	86 41 00 18 	l.lwz r18,24(r1)
  8c:	86 61 00 1c 	l.lwz r19,28(r1)
  90:	86 81 00 20 	l.lwz r20,32(r1)
  94:	86 a1 00 24 	l.lwz r21,36(r1)
  98:	86 c1 00 28 	l.lwz r22,40(r1)
  9c:	86 e1 00 2c 	l.lwz r23,44(r1)
  a0:	87 01 00 30 	l.lwz r24,48(r1)
  a4:	87 21 00 34 	l.lwz r25,52(r1)
  a8:	87 41 00 38 	l.lwz r26,56(r1)
  ac:	87 61 00 3c 	l.lwz r27,60(r1)
  b0:	87 81 00 40 	l.lwz r28,64(r1)
  b4:	87 a1 00 44 	l.lwz r29,68(r1)
  b8:	87 c1 00 48 	l.lwz r30,72(r1)
  bc:	87 e1 00 4c 	l.lwz r31,76(r1)
  c0:	9c 21 00 50 	l.addi r1,r1,80
  c4:	44 00 48 00 	l.jr r9



first_function:
*/

    return 96;
        /* return the address of the call to main() as a forward reference */
}

/* Finish code generation and return the size of the compiled binary */
unsigned int emit_end()
{
    /* set pointer to memory section for global variables */
    unsigned int addr = code_pos + 8192;
    set_32bit(buf + 88, 406847488 + (addr >> 16));
        /* 18 40 ?? ??  l.movhi r2, HI */
    set_32bit(buf + 92, 2822897664 + (addr & 65535));
        /* A8 42 ?? ??  l.ori r2, r2, LO */
    unsigned int i = 0;
    while (i < num_globals) {
        emit32(0);
        i = i + 1;
    }

    /* set file and memory size in ELF header */
    set_32bit(buf + 68, code_pos);
    set_32bit(buf + 72, code_pos);
    return code_pos;
}




/**********************************************************************
 * Step 3: Call binary main function
 **********************************************************************/

void emit_odabi(unsigned int o, unsigned int d, unsigned int a, unsigned int b, unsigned int i)
{
    emit32((o << 26) | (d << 21) | (a << 16) | (b << 11) | i);
}

void emit_mv(unsigned int d, unsigned int s)
{
    emit_odabi(56, d, s, s, 4);
}

unsigned int insn_disp26(unsigned int op, unsigned int disp)
{
    return ((disp >> 2) & 67108863)    | (op << 26);
        /*              & 0x03ff'ffff) */
}

unsigned int insn_call(unsigned int disp)
{
    return insn_disp26(1, disp);
}

/* Emit a function that consists of the given binary machine code.
   The parameters are length and pointer to the machine code.
   Return the address of the beginning of the function. */
unsigned int emit_binary_func(unsigned int n, char *s)
{
    unsigned int function_begin = code_pos;
    emit_n(n, s);
    return function_begin;
}

/* Call the function at address `ofs`.
   Then remove `pop` values from the stack.
   `save` is the return value from the corresponding emit_pre_call(). It can be
   used to restore the registers that were saved by emit_pre_call().
   Return the address of the call instruction. It must be at least 4 bytes long,
   because it might be overwritten by a pointer if the address of the function
   is not yet known. emit_fix_call() will overwrite this pointer later */
unsigned int emit_call(unsigned int ofs, unsigned int pop, unsigned int save)
{
    unsigned int cp = code_pos;
    emit32(insn_call(ofs - code_pos));
        /* 04 ?? ?? ??  l.jal ? */
    emit32(352321536);
        /* 15 00 00 00  l.nop 0 */
    reg_pos = 3;
    return cp;
}

/* Write a call instruction at the address from. The target address of the call
   is to. This is used to fix a previous emit_call() where 4 bytes are
   reserved for this call. */
void emit_fix_call(unsigned int from, unsigned int to)
{
    set_32bit(buf + from, insn_call(to - from));
        /* 04 ?? ?? ??  l.jal ? */
}




/**********************************************************************
 * Step 4: Main function with return 40
 **********************************************************************/

/* push value in the accumulator to the value stack */
void emit_push()
{
    reg_pos = reg_pos + 1;
    if (reg_pos > 11) error(201); /* r3 ... r11 can be used */
}

/* set accumulator to a unsigend 32 bit number */
void emit_number(unsigned int x)
{
    /* TODO: optimize for 16 bit values */

    emit_odabi(6, reg_pos, 0, 0, (x >> 16) & 65535);
        /* l.movhi REG, HI(x) */
    emit_odabi(42, reg_pos, reg_pos, 0, x & 65535);
        /* l.ori REG, REG, LO(x) */
}

/* Exit from the current function and return the value of the accumulator to
   the caller. Only called on explicit return statements, not at the end of a
   function. */
void emit_return()
{
    emit32(insn_call(224 - code_pos));
        /* 04 ?? ?? ??  l.jal ? */
    emit32(352321536);
        /* 15 00 00 00  l.nop 0 */
}




/**********************************************************************
 * Step 5: Call library function to exit
 **********************************************************************/

/* Emit code to put the value from the accumulator at the right location for
   an function argument. That could be a stack or a register. It is called in
   order of the arguments. */
void emit_arg()
{
    emit_push();
}




/**********************************************************************
 * Step 6: String constants
 **********************************************************************/

/* Store string somewhere in the binary and set accumulator to its address.
   To simplify the alignment code here, it is guaranteed that 4 zero bytes
   are following after the end of s. */
void emit_string(unsigned int len, char *s)
{
    unsigned int aligned_len = (len + 4) & 4294967292;
        /* there are 4 zero bytes appended to s */

    emit32(3783870468);
        /* E1 89 48 04  l.or r12,r9,r9          # save r9 */
    emit32(insn_call(aligned_len + 8));
        /* 04 ?? ?? ??  l.jal ? */
    emit32(352321536);
        /* 15 00 00 00  l.nop 0 */
    emit_binary_func(aligned_len, s);

    emit_odabi(56, reg_pos, 9, 9, 4);
        /*              l.or REG, r9, r9 */
    emit_odabi(56, 9, 12, 12, 4);
        /*              l.or r9, r12, r12         # restore r9*/

    /* TODO: optimize code */
}




/**********************************************************************
 * Step 7: Global variables
 **********************************************************************/

/* Reserve memory for a global variable and return its address */
unsigned int emit_global_var()
{
    num_globals = num_globals + 1;
    return num_globals - 1;
}

/* Store accumulator in a global(1) or local(0) variable with address `ofs`
   Function arguments have global=0 and ofs=1,2,3,... For other variables,
   ofs is the return value of emit_local_var() or emit_global_var() */
void emit_store(unsigned int global, unsigned int ofs)
{
    if (global) {
        emit_odabi(53, (ofs >> 9) & 31, 2, reg_pos, (ofs << 2) & 2047);
            /* l.sw OFS(r2), REG */
    }
    else {
        emit_mv(ofs+12, reg_pos);
            /* l.ori REG[ofs+12], REG[reg_pos], REG[reg_pos] */
    }
}

/* Load accumulator from a global(1) or local(0) variable with address `ofs`
   Function arguments have global=0 and ofs=1,2,3,... For other variables,
   ofs is the return value of emit_local_var() or emit_global_var() */
void emit_load(unsigned int global, unsigned int ofs)
{
    if (global) {
        emit_odabi(33, reg_pos, 2, 0, ofs << 2);
            /* l.lwz REG, OFS(r2) */
    }
    else {
        emit_mv(reg_pos, ofs+12);
            /* l.ori REG[reg_pos], REG[ofs+12], REG[ofs+12] */
    }
}




/**********************************************************************
 * Step 8: Arithmetic operations
 **********************************************************************/

/* Pop one value from the stack.
   Combine it with the accumulator and store the result in the accumulator.

   operation
      1  <<  shift left
      2  >>  shift right
      3  -   subtract
      4  |   or
      5  ^   xor
      6  +   add
      7  &   and
      8  *   multiply
      9  /   divide
     10  %   modulo
*/
void emit_operation(unsigned int operation)
{
    reg_pos = reg_pos - 1;
    if (operation == 10) { /* a % b = a - ((a / b) * b) */
        emit_odabi(56, reg_pos+2, reg_pos,   reg_pos+1, 778); /* l.div r+2, r,   r+1 */
        emit_odabi(56, reg_pos+1, reg_pos+1, reg_pos+2, 779); /* l.mul r+1, r+1, r+2 */
        emit_odabi(56, reg_pos,   reg_pos,   reg_pos+1, 2);   /* l.sub r,   r,   r+1 */
    }
    else {
        char *operation_code = " \x08\x48\x02\x04\x05\x00\x03\x0b\x0a";
        unsigned int op = operation_code[operation];
        if (operation > 7) op = op + 768;
        emit_odabi(56, reg_pos, reg_pos, reg_pos+1, op);
    }
}




/**********************************************************************
 * Step 9: Compare operations
 **********************************************************************/

void set_flag(unsigned int condition)
{
    reg_pos = reg_pos - 1;
    char *condition_code = "\x00\x01\x04\x03\x02\x05";
    unsigned int cond = condition_code[condition];
    emit_odabi(57, cond, reg_pos, reg_pos+1, 0);
}

/* Pop one value from the stack and compare it with the accumulator.
   Store 1 in the accumulator if the comparison is true, otherwise 0.

   condition (same as in emit_if()
     0  tos == accu
     1  tos != accu
     2  tos <  accu
     3  tos >= accu
     4  tos >  accu
     5  tos <= accu
*/
void emit_comp(unsigned int condition)
{
    set_flag(condition);

    emit_odabi(42, reg_pos, 0, 0, 1);
        /* l.ori REG[regpos], r0, 1 */
    emit_odabi(56, reg_pos, reg_pos, 0, 14);
        /* l.cmov REG[regpos], REG[regpos], r0 */
}




/**********************************************************************
 * Step 10: if and while
 **********************************************************************/

/* Called at the beginning of a while loop, before the condition.
   The return value is forwarded to emit_jump_and_fix_jump_here() at the end of
   the loop. It must not be 0, because that means emit_jump_and_fix_jump_here()
   is called between a then and else branch of a if statement. Typically the
   return value is code_pos, the address of the top of the loop.
*/
unsigned int emit_pre_while()
{
    return code_pos;
}

/* Called after a if or while condition.
   Emit code for a branch if condition is NOT true.
   Return address where the branch target address will be written later.

   condition (same as in emit_comp()
     0  tos == accu
     1  tos != accu
     2  tos <  accu
     3  tos >= accu
     4  tos >  accu
     5  tos <= accu
*/
unsigned int emit_if(unsigned int condition)
{
    set_flag(condition);
    emit32(352321536);
        /* 15 00 00 00  l.nop 0 */
        /* l.bnf 0 # disp will later be fixed */
    emit32(352321536);
        /* 15 00 00 00  l.nop 0 */
    return code_pos - 8;
}

/* Called at the end of a then branch without an else branch.
   Used to fix the branch instruction at the end of the then branch. It should
   point to the current address.
   The address of the branch instruction is given by the parameter.
*/
void emit_then_end(unsigned int insn_pos)
{
    set_32bit(buf + insn_pos, insn_disp26(3, code_pos - insn_pos));
        /* l.bnf */
}

/* Called at the end of an else branch.
   Used to fix the jump instruction at the end of the then branch. It should
   point to the current address.
   The address of the jump instruction is given by the parameter.
*/
void emit_else_end(unsigned int insn_pos)
{
    set_32bit(buf + insn_pos, insn_disp26(0, code_pos - insn_pos));
        /* l.j */
}

/* Called between then and else branch of an if statement.
   insn_pos points to the branch instruction that should be fixed to point to
   the current address.
   Return address where the jump target address will be written later  */
unsigned int emit_then_else(unsigned int insn_pos)
{
    emit32(0); /* fix to l.j */
    emit32(352321536);
        /* 15 00 00 00  l.nop 0 */
    emit_then_end(insn_pos);
    return code_pos - 8;
}

/* Called at the end of a while loop with the value returned by
   emit_pre_while(). Typically it is the address of the top of the loop.
   insn_pos points to the branch instruction that should be fixed to point to
   the current address. */
void emit_loop(unsigned int destination, unsigned int insn_pos)
{
    emit32(insn_disp26(0, destination - code_pos));
    emit32(352321536);
        /* 15 00 00 00  l.nop 0 */
    emit_then_end(insn_pos);
}




/**********************************************************************
 * Step 11: Access function arguments
 **********************************************************************/

/* Emit code at the beginning of a function.
   The parameter is the number of arguments for the function.
   The return value is the address of the function entry point. */
unsigned int emit_func_begin(unsigned int n)
{
    reg_pos = 3;
    num_locals = n;

    emit32(3783870468);
        /* E1 89 48 04  l.or r12,r9,r9 */
    emit32(insn_call(112 - code_pos));
        /* 04 ?? ?? ??  l.jal _function_prolog */
    emit32(352321536);
        /* 15 00 00 00  l.nop 0 */

    return code_pos - 12;
}

/* Emit code at the end of a function.
   Typically simply emit_return() is called here. */
void emit_func_end()
{
    emit_return();
}




/**********************************************************************
 * Step 12: Local variables
 **********************************************************************/

/* Reserve memory for a local variable and return its address.
   Function arguments have the addresses 1, 2, 3, ... Therefore use an address
   scheme that fits this.
   If `init` is not 0, an initial value is in the accumulator and the variable
   must be set accordingly. */
unsigned int emit_local_var(unsigned int init)
{
    num_locals = num_locals + 1;
    return num_locals;
}

/* Called at the beginning of a scope (`{`).
   The value that is returned will be the parameter for the corresponding
   emit_scope_end().
   Typically the stack position is returned to remove temporary variables
   from the stack at the end of the scope. */
unsigned int emit_scope_begin()
{
    return 0;
}

/* Called at end of a scope (`}`).
   The parameter is the return value of the corresponding emit_scope_begin(). */
void emit_scope_end(unsigned int stack_pos)
{
}




/**********************************************************************
 * Step 13: Function result as argument
 **********************************************************************/

/* Called before pushing the arguments of a function call.
   The return value is forwarded to emit_call().
   Used in emit_rv32.c to save registers before using them for the function 
   arguments. The return value tells emit_call() how many registers have to
   be restored. */
unsigned int emit_pre_call()
{
    return 0;
}




/**********************************************************************
 * Step 14: Arrays
 **********************************************************************/

/* input:  global(1) or local(0) variable used as a pointer
           accumulator contains the index into the array the pointer points to
   output: push pointer to the array element */
void emit_index_push(unsigned int global, unsigned int ofs)
{
}

/* pop address (from emit_index_push()) and store lowest byte of accumulator
   there */
void emit_pop_store_array()
{
}

/* Read global(1) or local(0) variable with address `ofs` as base pointer.
   Add index (in accumulator) to the base pointer.
   Load byte from the computed address and store in accumulator.  */
void emit_index_load_array(unsigned int global, unsigned int ofs)
{
}



