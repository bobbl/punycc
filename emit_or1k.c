/**********************************************************************
 * Code generation for OpenRISC OR1K
 **********************************************************************
 *
 * OpenRISC 1000 Architecture Manual:
 * https://raw.githubusercontent.com/openrisc/doc/master/openrisc-arch-1.4-rev0.pdf
 *
 * Register usage
 * --------------
 * r0         fixed to
 * r1         stack pointer
 * r2         pointer to global variables
 * r3         expression stack and function parameters and return value
 * r4 ... r8  expression stack and function parameters
 * r9         expression stack and return address
 * r10...r12  expression stack
 * r13...r31  callee-saved local variables (including copy of parameters)
 *
 **********************************************************************/



unsigned int buf_size;          /* total size of the buffer */
unsigned char *buf;             /* pointer to the buffer */
unsigned int code_pos;          /* position in the buffer for code generation */
unsigned int stack_pos;         /* current stack pointer */
unsigned int num_params;        /* number of function parameters */
unsigned int num_locals;        /* number of local variables in the current function */
unsigned int num_globals;       /* number of global variables */

unsigned int reg_pos;
unsigned int last_insn;
unsigned int last_insn_type;
    /*  1 pop and store
        8 push uimm15
        9 push uimm16
       10 push uimm32
       12 push local
       13 push global
       14 operation
       15 comparison
       8...15 write into the destination register
    */
unsigned int return_list;
    /* Linked list of return statements within a function.
       Needed, because the number of local variables */
unsigned int max_locals;
    /* How many callee-saved registers where used in this function */
unsigned int function_start_pos;
    /* Start of the current function */
unsigned int last_branch_target;
    /* Last address that was the target of a jump instruction */


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
    last_insn = n;
    last_insn_type = 0;
}

/* Prepare for code generation
   Return the address of the call to main() as a forward reference */
unsigned int emit_begin()
{
    code_pos = 0;
    num_globals = 0;
    reg_pos = 3;
    last_branch_target = 0;

    emit_n(308, "\x7f\x45\x4c\x46\x01\x02\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x5c\x00\x00\x00\x01\x00\x00\x20\x54\x00\x00\x00\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x34\x00\x20\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x20\x00\x00\x00\x20\x00........\x00\x00\x00\x07\x00\x00\x10\x00\x18\x00\x00\x00\x18\x40\x00\x00\xa8\x42\x00\x00\x00\x00\x00\x00\x15\x00\x00\x00\xa9\x60\x00\x5d\x20\x00\x00\x00\xd4\x01\xf8\x4c\xd4\x01\xf0\x48\xd4\x01\xe8\x44\xd4\x01\xe0\x40\xd4\x01\xd8\x3c\xd4\x01\xd0\x38\xd4\x01\xc8\x34\xd4\x01\xc0\x30\xd4\x01\xb8\x2c\xd4\x01\xb0\x28\xd4\x01\xa8\x24\xd4\x01\xa0\x20\xd4\x01\x98\x1c\xd4\x01\x90\x18\xe2\x48\x40\x04\xd4\x01\x88\x14\xe2\x27\x38\x04\xd4\x01\x80\x10\xe2\x06\x30\x04\xd4\x01\x78\x0c\xe1\xe5\x28\x04\xd4\x01\x70\x08\xe1\xc4\x20\x04\xd4\x01\x68\x04\xe1\xa3\x18\x04\x44\x00\x60\x00\xd4\x01\x48\x00\x87\xe1\x00\x4c\x87\xc1\x00\x48\x87\xa1\x00\x44\x87\x81\x00\x40\x87\x61\x00\x3c\x87\x41\x00\x38\x87\x21\x00\x34\x87\x01\x00\x30\x86\xe1\x00\x2c\x86\xc1\x00\x28\x86\xa1\x00\x24\x86\x81\x00\x20\x86\x61\x00\x1c\x86\x41\x00\x18\x86\x21\x00\x14\x86\x01\x00\x10\x85\xe1\x00\x0c\x85\xc1\x00\x08\x85\xa1\x00\x04\x85\x21\x00\x00\x44\x00\x48\x00\xe0\x21\x60\x00");

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
    005C a8 42 ?? ??    l.ori r2, r2, ?
    0060 00 00 00 00    l.jal main      but must be 0 for emit_fix_call()
    0064 15 00 00 00    l.nop 0
    006c a9 60 00 5D    l.ori r11, r0, 93
    0070 20 00 00 00    l.sys 0

_function_prolog:
    0074 d4 01 f8 4c    l.sw 76(r1), r31
    0078 d4 01 f0 48    l.sw 72(r1), r30
    007C d4 01 e8 44    l.sw 68(r1), r29
    0080 d4 01 e0 40    l.sw 64(r1), r28

  40:	d4 01 d8 3c 	l.sw 60(r1),r27
  3c:	d4 01 d0 38 	l.sw 56(r1),r26
  38:	d4 01 c8 34 	l.sw 52(r1),r25
  34:	d4 01 c0 30 	l.sw 48(r1),r24
  30:	d4 01 b8 2c 	l.sw 44(r1),r23
  2c:	d4 01 b0 28 	l.sw 40(r1),r22
  28:	d4 01 a8 24 	l.sw 36(r1),r21
  24:	d4 01 a0 20 	l.sw 32(r1),r20
  20:	d4 01 98 1c 	l.sw 28(r1),r19
  1c:	d4 01 90 18 	l.sw 24(r1),r18
  68:	e2 48 40 04 	l.or r18,r8,r8
  18:	d4 01 88 14 	l.sw 20(r1),r17
  64:	e2 27 38 04 	l.or r17,r7,r7
  14:	d4 01 80 10 	l.sw 16(r1),r16
  60:	e2 06 30 04 	l.or r16,r6,r6
  10:	d4 01 78 0c 	l.sw 12(r1),r15
  5c:	e1 e5 28 04 	l.or r15,r5,r5
   c:	d4 01 70 08 	l.sw 8(r1),r14
  58:	e1 c4 20 04 	l.or r14,r4,r4

    00CC d4 01 68 04    l.sw 4(r1), r13
    00D0 e1 a3 18 04    l.or r13, r3, r3
    00D4 44 00 60 00    l.jr r12
    00D8 d4 01 48 00    l.sw 0(r1), r9

_function_epilog:
    00DC 87 e1 00 4c    l.lwz r31, 76(r1)
    00E0 87 c1 00 48    l.lwz r30, 72(r1)
    00E4 87 a1 00 44    l.lwz r29, 68(r1)
    00E8 87 81 00 40    l.lwz r28, 64(r1)

  ac:	87 61 00 3c 	l.lwz r27,60(r1)
  a8:	87 41 00 38 	l.lwz r26,56(r1)
  a4:	87 21 00 34 	l.lwz r25,52(r1)
  a0:	87 01 00 30 	l.lwz r24,48(r1)
  9c:	86 e1 00 2c 	l.lwz r23,44(r1)
  98:	86 c1 00 28 	l.lwz r22,40(r1)
  94:	86 a1 00 24 	l.lwz r21,36(r1)
  90:	86 81 00 20 	l.lwz r20,32(r1)
  8c:	86 61 00 1c 	l.lwz r19,28(r1)
  88:	86 41 00 18 	l.lwz r18,24(r1)
  84:	86 21 00 14 	l.lwz r17,20(r1)
  80:	86 01 00 10 	l.lwz r16,16(r1)
  7c:	85 e1 00 0c 	l.lwz r15,12(r1)
  78:	85 c1 00 08 	l.lwz r14,8(r1)

    0124 85 a1 00 04    l.lwz r13, 4(r1)
    0128 85 21 00 00    l.lwz r9, 0(r1)
    012c 44 00 48 00    l.jr r9
    0130 e0 21 60 00    l.add r1, r1, r12

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

void emit_odai(unsigned int o, unsigned int d, unsigned int a, unsigned int i)
{
    emit32((o << 26) | (d << 21) | (a << 16) | i);
}

/* Do a operation on the top register (with an immediate or another register
   that is encoded in the immediate).
   If previous instruction is a load of a local variable, fuse the instructions */
void emit_odri(unsigned int o, unsigned int d, unsigned int i)
{
    unsigned int prev_insn = get_32bit(buf + code_pos - 4);
    unsigned int a = reg_pos;

    if (code_pos > last_branch_target) {
        if ((prev_insn & 4292935679) == ((a << 21) | 3758096388)) {
            /*           0xffe0ffff                  0xe0000004
                                 or REG[reg_pos], REG[a], r0 */
            a = (prev_insn >> 16) & 31;
            code_pos = code_pos - 4;
        }
    }
    emit_odai(o, d, a, i);
}

/* If last instruction is a load from a local variable to reg, return the
   register number of the local variable instead of reg. */
unsigned int fuse_load_local(unsigned int reg)
{
    unsigned int b = reg << 11;
    if (last_insn_type == 12 /* push local */) {
        if (code_pos > last_branch_target) {
            code_pos = code_pos - 4;
            b = (last_insn >> 5) & 63488; /* 0xf800 */
        }
    }
    return b;
}

/* Operation on the two topmost elements of the expression stack.
   Optimise, if the second operand is a load from a local variable. */
void emit_odrri(unsigned int o, unsigned int d, unsigned int i)
{
    unsigned int b = fuse_load_local(reg_pos + 1);
    emit_odri(o, d, b | i);
}

unsigned int insn_disp26(unsigned int op, unsigned int disp)
/* op 0x0000'0000         0  l.j
      0x0400'0000  67108864  l.jal
      0x0C00'0000 201326592  l.bnf */
{
    return ((disp >> 2) & 67108863) | op;
        /*              & 0x03ff'ffff) */
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

/* Emit a jump or call and put the last instruction in the branch delay slot
   if possible. */
void fill_delay_slot(unsigned int op, unsigned int destination)
{
    unsigned int cp = code_pos;
    unsigned int slot_insn = 352321537; /* 15 00 00 01  l.nop 1 */

    if (last_branch_target < cp) {
        cp = cp - 4;
        slot_insn = last_insn;
        code_pos = cp;
    }

    emit32(insn_disp26(op, destination - cp));
        /* l.j / l.jal */
    emit32(slot_insn);
    last_branch_target = code_pos;
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
    fill_delay_slot(67108864, ofs); /* l.jal */

    reg_pos = save;
    if (save > 3) {

        /* set the appropriate parameter register to the function result */
        emit_odai(56, save, 3, 4);
            /* l.or REG[save], r3, r0 */

        /* restore previously saved temporary registers */
        unsigned int r = 3;
        while (r < save) {
            emit_odai(56, r, num_locals + 12, 4);
                /* l.or REG[r], REG[num_locals + 12], r0 */
            num_locals = num_locals - 1;
            r = r + 1;
        }
    }
    return code_pos - 8;
}

/* Write a call instruction at the address from. The target address of the call
   is to. This is used to fix a previous emit_call() where 4 bytes are
   reserved for this call. */
void emit_fix_call(unsigned int from, unsigned int to)
{
    set_32bit(buf + from, insn_disp26(67108864, to - from)); /* l.jal */
}

/* push value in the accumulator to the value stack */
void emit_push()
{
    reg_pos = reg_pos + 1;
    if (reg_pos > 11) error(201); /* r3 ... r11 can be used */
}

/* set accumulator to a unsigend 32 bit number */
void emit_number(unsigned int x)
{
    if ((x >> 16) != 0) {
        emit_odai(6, reg_pos, 0, x >> 16);
            /* l.movhi REG, HI(x) */
        x = x & 65535;
        if (x != 0) {
            emit_odai(42, reg_pos, reg_pos, x);
            /* l.ori REG, REG, LO(x) */
        }
        last_insn_type = 10; /* push uimm32 */
    }
    else {
        emit_odai(42, reg_pos, 0, x);
            /* l.ori REG, r0, LO(x) */
        last_insn_type = (x > 32767) + 8; /* push uimm15 / push uimm16 */
    }
}

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

    emit32(insn_disp26(0, aligned_len + 8));
        /* 00 ?? ?? ??  l.j after_string? */
    emit_odai(42, reg_pos, 0, code_pos + 8196);
        /* l.ori REG, r0, $+4 + 0x2000 */
        /* Don't forget to add the offset 0x2000 from the ELF header here */
    emit_binary_func(aligned_len, s);
    last_branch_target = code_pos;
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


void pop_to_reg(unsigned int dest_reg)
{
    if (last_insn_type > 7) {
        code_pos = code_pos - 4;
        emit32((last_insn & 4229955583) | (dest_reg << 21));
            /* (last_insn & 0xfc1fffff) | (dest_reg << 21)); */
    } else {
        emit_odai(56, dest_reg, reg_pos, 4);
            /* l.ori REG[dest_reg], REG[reg_pos], r0 */
    }
}

/* Store accumulator in a global(1) or local(0) variable with address `ofs`
   Function arguments have global=0 and ofs=1,2,3,... For other variables,
   ofs is the return value of emit_local_var() or emit_global_var() */
void emit_store(unsigned int global, unsigned int ofs)
{
    if (global) {
        unsigned int i = fuse_load_local(reg_pos) | ((ofs << 2) & 2047);
        emit_odai(53, (ofs >> 9) & 31, 2, i);
            /* l.sw OFS(r2), REG */
    }
    else {
        pop_to_reg(ofs + 12);
    }
    last_insn_type = 1; /* pop and store */
}

/* Load accumulator from a global(1) or local(0) variable with address `ofs`
   Function arguments have global=0 and ofs=1,2,3,... For other variables,
   ofs is the return value of emit_local_var() or emit_global_var() */
void emit_load(unsigned int global, unsigned int ofs)
{
    if (global) {
        emit_odai(33, reg_pos, 2, ofs << 2);
            /* l.lwz REG, OFS(r2) */
        last_insn_type = 13; /* push global */
    }
    else {
        emit_odai(56, reg_pos, ofs + 12, 4);
            /* l.ori REG[reg_pos], REG[ofs+12], r0 */
        last_insn_type = 12; /* push local */
    }
}




/**********************************************************************
 * Step 8: Arithmetic operations
 **********************************************************************/

/* Pop one value from the stack.
   Combine it with the accumulator and store the result in the accumulator.

   operation                    rrr     rri
      1  <<  l.sll              008     2e*
      2  >>  l.srl     t        048     2e*
      3  -   l.sub              002     27-
      4  |   l.or               004     2a
      5  ^   l.xor              005     2b
      6  +   l.add              000     27+
      7  &   l.and              003     29
      8  *   l.mulu             30b     2c      l.muli
      9  /   l.divu             30a
     10  %   modulo
*/
void emit_operation(unsigned int operation)
{
    reg_pos = reg_pos - 1;
    if (operation < 9) {

        /* optimisation: second operand is an immediate */
        if (last_insn_type == 8 /* push uimm15 */) {
            /* set accu to immediate < 32K
               In some caseses immediates < 64K would also be possible, but
               checking the special cases is too difficult */

            unsigned int op = ((3380324334 >> ((operation-1) << 2)) & 15) + 32;
                    /* 1 1110 2e l.slli
                       2 1110 2e l.slri
                       3 0111 27 l.addi
                       4 1010 2a l.ori
                       5 1011 2b l.xori
                       6 0111 27 l.addi
                       7 1001 29 l.andi
                       8 1100 2c l.muli
                      (0xc97b'a7ee >> (((operation-1) << 4) & 15)) + 32 */
            unsigned int imm = last_insn & 65535;
            code_pos = code_pos - 4;
            if (operation < 3) {
                imm = ((operation-1)<<6) + (imm & 31);
                  /* l.slli r, r, imm */
                  /* l.srli r, r, imm */
            }
            else if (operation == 3) {
                imm = (0 - imm) & 65535;
                  /* l.addi r, r, imm */
            }
            emit_odri(op, reg_pos, imm);
        }
        else {
            char *operation_code = " \x08\x48\x02\x04\x05\x00\x03\x0b";
            emit_odrri(56, reg_pos, ((operation & 8)*96) + operation_code[operation]);
        }
    }
    else if (operation == 9) {
        emit_odrri(56, reg_pos, 778); /* l.divu */
    }
    else { /* a % b = a - ((a / b) * b) */

        unsigned int rrr = (reg_pos << 21) | (reg_pos << 16) | (reg_pos << 11);
        emit32(3762293514 + rrr);
            /* E? ?? ?3 0A  l.divu r+2, r,   r+1    0xe040'0b0a+rrr */
        emit32(3760263947 + rrr);
            /* E? ?? ?3 0B  l.mulu r+1, r+1, r+2    0xe021'130b+rrr */
        emit32(3758098434 + rrr);
            /* E? ?? ?0 02  l.sub  r,   r,   r+1    0xe000'0802+rrr */
    }
    last_insn_type = 14; /* pop operation */
}




/**********************************************************************
 * Step 9: Compare operations
 **********************************************************************/

void set_flag(unsigned int condition)
{
    reg_pos = reg_pos - 1;
/*
    0  0000  l.sfeq  0x720  l.sfeqi  0x5e0 simm16
    1  0001  l.sfne  0x721  l.sfnei  0x5e1 simm16
    2  0100  l.sfltu 0x724  l.sfltui 0x5e4 simm16
    3  0011  l.sfgeu 0x723  l.sfgeui 0x5e3 simm16
    4  0010  l.sfgeu 0x722  l.sfgeui 0x5e2 simm16
    5  0101  l.sfleu 0x725  l.sfleui 0x5e5 simm16
    unsigned int cond = 0x523410 >> (condition << 2)) & 15;
*/
    unsigned int cond = (5387280 >> (condition << 2)) & 15;

    if (last_insn_type == 8) {
        code_pos = code_pos - 4;
        emit_odri(47, cond, last_insn & 32767);
    }
    else {
        emit_odrri(57, cond, 0);
    }
    last_insn_type = 15; /* pop comparision */
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

    emit_odai(42, reg_pos, 0, 1);
        /* l.ori REG[regpos], r0, 1 */
    emit_odai(56, reg_pos, reg_pos, 14);
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
    emit32(0);
        /* emit_then_end() will overwrite this instruction with l.bnf */
    emit32(352321536);
        /* 15 00 00 00  l.nop 0         # branch delay slot */
        /* TODO: fill this delay slot */
    last_branch_target = code_pos;
    return code_pos - 8;
}

/* Called at the end of a then branch without an else branch.
   Used to fix the branch instruction at the end of the then branch. It should
   point to the current address.
   The address of the branch instruction is given by the parameter.
*/
void emit_then_end(unsigned int insn_pos)
{
    set_32bit(buf + insn_pos, insn_disp26(201326592, code_pos - insn_pos));
        /* l.bnf */
    last_branch_target = code_pos;
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
    last_branch_target = code_pos;
}

/* Called between then and else branch of an if statement.
   insn_pos points to the branch instruction that should be fixed to point to
   the current address.
   Return address where the jump target address will be written later  */
unsigned int emit_then_else(unsigned int insn_pos)
{
    fill_delay_slot(0, 0); 
        /* branch target doesn't care, because emit_else_end() will overwrite
           this instruction with l.j */
    emit_then_end(insn_pos);
    return code_pos - 8;
}

/* Called at the end of a while loop with the value returned by
   emit_pre_while(). Typically it is the address of the top of the loop.
   insn_pos points to the branch instruction that should be fixed to point to
   the current address. */
void emit_loop(unsigned int destination, unsigned int insn_pos)
{
    fill_delay_slot(0, destination);
    emit_then_end(insn_pos);
}




/**********************************************************************
 * Function prologue and epilogue
 **********************************************************************
 * Easy solution (not used):
 * Allways call the same prologue and epilogue routine that saves/restores
 * all 19 registers and the return address to the stack
 *
 * Better solution (saves stack space and runs faster, but compiling is more
 *                  difficult):
 * Count the really used local variables in |max_locals| and save/restore
 * only the used registers by entering the prolog/epiloge later.
 **********************************************************************/


/* Emit code at the beginning of a function.
   The parameter is the number of arguments for the function.
   The return value is the address of the function entry point. */
unsigned int emit_func_begin(unsigned int n)
{
    reg_pos = 3;
    num_locals = n;
    max_locals = n;
    return_list = 0;
    function_start_pos = code_pos;
    code_pos = code_pos + 12;
    last_branch_target = code_pos;
    return function_start_pos;
}

/* Exit from the current function and return the value of the accumulator to
   the caller. Only called on explicit return statements, not at the end of a
   function. */
void emit_return()
{
    emit32(return_list);
    emit32(0);
    return_list = code_pos - 8;
}

/* Emit code at the end of a function.
   Typically simply emit_return() is called here. */
void emit_func_end()
{
    unsigned int stack_frame_size = max_locals << 2;

    /* Write call to prologe at function start.
       Separate scope to re-use registers for local variables later. */
    {
        unsigned int fsp = function_start_pos;
        unsigned int prolog_entry = 184 - stack_frame_size;
        /* For the lowest 6 registers, the parameters are copied.
           Don't forget them  */
        if (max_locals < 6) {
            prolog_entry = prolog_entry - stack_frame_size + 24;
        }

        set_32bit(buf + fsp, 2619473916 - stack_frame_size);
            /* 9c 21 ff b0  l.addi r1, r1, 0-stack_frame_size-4 */
            /* Save space on the stack for local variables */

        set_32bit(buf + fsp + 4, insn_disp26(0, prolog_entry - fsp));
            /* 00 ?? ?? ??  l.j _function_prolog */
            /* Jump to the prologue to not destroy the return adress of the
               function in r9. */

        set_32bit(buf + fsp + 8, 2843746316 + fsp);
            /* a9 8c ?? ??  l.ori r12, r0, code_pos + 4 + 0x2000 */
            /* Fill the delay slot with an explicit write of the return address
               to r12. Don't forget to add 0x2000, the offset from the ELF
               header */
    }

    unsigned int pos = code_pos - 8;
    unsigned int next;

    /* add an return in case there was no final return */
    if ((return_list != pos) | (last_branch_target == code_pos)) {
        pos = code_pos;
        emit_return();
    }

    /* go through linked list of return statements */
    while (pos != 0) {
        next = get_32bit(buf + pos);
        set_32bit(buf + pos, insn_disp26(0, 296 - pos - stack_frame_size));
            /* 00 ?? ?? ??  l.j _function_epilog */
        set_32bit(buf + pos + 4, 2843738116 + stack_frame_size);
            /* a9 80 00 50  l.ori r12, r0, stack_frame_size+4 */
            /* Use the delay slot to write the size of the stack frame to r12 */
        pos = next;
    }
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
    unsigned int n = num_locals + 1;
    num_locals = n;
    if (n > max_locals) {
        max_locals = n;
    }
    if (init != 0) {
        pop_to_reg(n + 12);
            /* l.or REG[num_locals+12], REG[reg_pos], r0 
               or fuse with previous instruction */
    }
    return n;
}

/* Called at the beginning of a scope (`{`).
   The value that is returned will be the parameter for the corresponding
   emit_scope_end().
   Typically the stack position is returned to remove temporary variables
   from the stack at the end of the scope. */
unsigned int emit_scope_begin()
{
    return num_locals;
}

/* Called at end of a scope (`}`).
   The parameter is the return value of the corresponding emit_scope_begin(). */
void emit_scope_end(unsigned int stack_pos)
{
    num_locals = stack_pos;
}




/**********************************************************************
 * Function call within function parameters
 **********************************************************************
 * If the parameter stack is not empty (reg_pos>3) when a function is called,
 * this means that the result of this function will be used as parameter to
 * another function (and it is not the first parameter).
 * Therefore the parameter stack with all the parameters that were computed
 * so far must be saved to local variables.
 * emit_call() is responsible for restoring the parameters of the outer
 * function after the call to the inner function.
 **********************************************************************/

/* Called before pushing the arguments of a function call.
   The return value is forwarded to emit_call(). */
unsigned int emit_pre_call()
{
    /* save currently used temporary registers in local variables */
    unsigned int r = reg_pos;
    while (reg_pos > 3) {
        reg_pos = reg_pos - 1;
        emit_local_var(1);
    }
    return r;
}




/**********************************************************************
 * Step 14: Arrays
 **********************************************************************/

/* input:  global(1) or local(0) variable used as a pointer
           accumulator contains the index into the array the pointer points to
   output: push pointer to the array element */
void emit_index_push(unsigned int global, unsigned int ofs)
{
    unsigned int op = 56;
    unsigned int b = ofs + 12;
    unsigned int imm = reg_pos << 11;

    if (last_insn_type == 8) { /* push uimm15 */
        op = 39;
        imm = last_insn & 32767;
        code_pos = code_pos - 4;
    }
    else if (last_insn_type == 12) { /* push local */
        imm = ((last_insn >> 16) & 31) << 11;
        code_pos = code_pos - 4;
    }

    if (global) {
        emit_odai(33, reg_pos+1, 2, ofs << 2);
            /* l.lwz REG[reg_pos+1], OFS(r2) */
        b = reg_pos + 1;
    }
    emit_odai(op, reg_pos, b, imm);

    last_insn_type = 14; /* pop operation */
    emit_push();
}

/* pop address (from emit_index_push()) and store lowest byte of accumulator
   there */
void emit_pop_store_array()
{
    /* reg_pos is always 4 at this point */
    reg_pos = 3;
    emit32(3624083456);
        /* D8 03 20 00  l.sb 0(r3), r4 */
}

/* Read global(1) or local(0) variable with address `ofs` as base pointer.
   Add index (in accumulator) to the base pointer.
   Load byte from the computed address and store in accumulator.  */
void emit_index_load_array(unsigned int global, unsigned int ofs)
{
    emit_index_push(global, ofs);
    reg_pos = reg_pos - 1;
    emit_odri(35, reg_pos, 0);
        /* l.lbz REG[reg_pos], 0(REG[reg_pos]) */
}



