/**********************************************************************
 * Code generation for RV32IM
 **********************************************************************
 * Second attempt: fill and spill registers
 *
 * RISC-V Instruction Set Manual:
 * https://github.com/riscv/riscv-isa-manual/
 *
 * RISC-V ABI:
 * https://github.com/riscv-non-isa/riscv-elf-psabi-doc
 *
 * Register usage
 * --------------
 * x0        zero       fixed to
 * x1        ra         return address
 * x2        sp         stack pointer
 * x3        gp         pointer to global variables
 * x4        tp         (thread pointer)
 * x5 ... x7 t0 ... t2  expression stack
 * x8 ... x9 s0 ... s1  callee-saved local variables (including copy of parameters)
 * x10...x17 a0 ... a7  expression stack and function parameters
 * x18...x27 s2 ...s11  callee-saved local variables (including copy of parameters)
 * x28...x31 t3 ... t6  expression stack
 *
 **********************************************************************/



unsigned int buf_size;          /* total size of the buffer */
unsigned char *buf;             /* pointer to the buffer */
unsigned int code_pos;          /* position in the buffer for code generation */
unsigned int num_locals;        /* number of local variables in the current function */
unsigned int num_globals;       /* number of global variables */

unsigned int reg_pos;
unsigned int last_insn;
unsigned int last_insn_type;
    /*  8 push imm12
       10 push uimm32
       11 push reg (used as local variable)
       13 push mem (used as global or loval variable)
       14 arith operation
       15 comparison
       8...15 write into the destination register
    */
unsigned int return_list;
unsigned int max_locals;
unsigned int function_start_pos;
unsigned int last_branch_target;
    /* Position where the last branch points to.
       Used to determine the length of the last uninterrupted sequences of
       instructions. */
unsigned int num_scope;
unsigned int num_calls;
unsigned int max_reg_pos;

char *local_reg;





/* helper to write a 32 bit number to a char array */
void set_32bit(unsigned char *p, unsigned int x)
{
    p[0] = x;
    p[1] = x >> 8;
    p[2] = x >> 16;
    p[3] = x >> 24;
}

/* helper to read 32 bit number from a char array */
unsigned int get_32bit(unsigned char *p)
{
    return p[0] +
          (p[1] << 8) +
          (p[2] << 16) +
          (p[3] << 24);
}


unsigned int emit_binary_func(unsigned int n, char *s)
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
    last_insn = n;
    last_insn_type = 0;
    set_32bit(buf + cp, n);
}

void emit_isdo(unsigned int imm, unsigned int rs, unsigned int rd, unsigned int opcode)
{
    emit32((imm << 20) + (rs << 15) + (rd << 7) + opcode);
}

unsigned int insn_jal(unsigned int rd, unsigned int immj)
{
    return
        (((immj>>20) & 1) << 31) |      /* bit  31     = imm[20] */
        (((immj & 2046  )        |      /* bits 30..21 = imm[10..1] */
         ((immj>>11) & 1))<< 20) |      /* bit  20     = imm[11] */
        ((immj & 1044480)      ) |      /* bits 19..12 = imm[19..12] */
        ( rd              <<  7) |      /* bits 11..7  = rd */
        111;                            /* bits  6..0  = 0x6f (jal) */
}

void emit_push()
{
    reg_pos = reg_pos + 1;
    if (reg_pos > max_reg_pos) max_reg_pos = reg_pos;
}

void emit_number(unsigned int imm)
{
    if (((imm + 2048) >> 12) == 0) {
        emit_isdo(imm, 0, reg_pos, 19);
                /* 00000013  ADDI REG[reg_pos], X0, imm */
        last_insn_type = 8;
    }
    else {
        emit32((((imm + 2048) >> 12) << 12) + (reg_pos << 7) + 55);
                /* 00000037  LUI REG[reg_pos], imm */
        if ((imm << 20) != 0) {
            emit_isdo(imm, reg_pos, reg_pos, 19);
                /* 00000013  ADDI REG[reg_pos], REG[reg_pos], imm */
        }
        last_insn_type = 10;
    }
}

void emit_string(unsigned int len, char *s)
{
    unsigned int aligned_len = (len + 4) & 4294967292;
        /* there are 4 zero bytes appended to s */
    emit32(insn_jal(reg_pos, aligned_len + 4));
        /* JAL REG[reg_pos], align(end_of_string) */
    emit_binary_func(aligned_len, s);
}

void emit_store(unsigned int global, unsigned int ofs)
{
    /* When called from punycc.c, reg_pos is always 10.
       But it is called from emit_pre_call() (via emit_local_var())
       to save the parameter stack. In the latter case, reg_pos
       can be higher. */

    if (global == 0) {
        if (ofs < 13) {
            if (last_insn_type > 7) {
                code_pos = code_pos - 4;
                emit32((last_insn & 4294963327) | (local_reg[ofs] << 7));
                    /*              0xFFFFF07F */
            }
            else {
                emit_isdo(0, reg_pos, local_reg[ofs], 19);
                    /* ADDI REG[local_reg[ofs]], REG[reg_pos], 0 */
            }
            return;
        }
    }
    emit32(73763 +
        (global << 15) +
        (reg_pos << 20) +
        ((ofs & 1016   ) << 22) +       /* bits 31..25 = ofs[9..3] */
        ((ofs & 7      ) <<  9));       /* bits 11..7  = ofs[2..0] 0 0  */
        /* SW REG[reg_pos], (ofs+1)(REG[2+global]) */
}

void emit_load(unsigned int global, unsigned int ofs)
{
    if (global == 0) {
        if (ofs < 13) {
            emit_isdo(0, local_reg[ofs], reg_pos, 19);
                /* ADDI REG[reg_pos], REG[local_reg[ofs]], 0 */
            last_insn_type = 11; /* push reg */
            return;
        }
    }
    else {
        emit_isdo(ofs << 2, global, reg_pos, 73731);
            /* LW reg_pos, ofs(REG[2+global]) */
        last_insn_type = 13; /* push mem */
    }
}

/* Same as emit_isdo(), but rs=reg_pos and if REG[reg_pos] is loaded from
   a local variable in the previous instruction, fuse them. */
void emit_irdo(unsigned int imm, unsigned int rd, unsigned int opcode)
{
    unsigned int rs = reg_pos;
    unsigned int prev_insn = get_32bit(buf + code_pos - 4);
    if ((prev_insn & 4293947519) == 19) {
        /*           0xfff0707f) == 0x13)    ADDI REG[reg_pos], REG[local], 0 */
        if (((prev_insn >> 7) & 31) == reg_pos) { /* really necessary? */
            /* load local*/
            rs = (prev_insn >> 15) & 31;
            code_pos = code_pos - 4;
        }
    }
    emit_isdo(imm, rs, rd, opcode);
    last_insn_type = 14; /* arith operation */
}

void emit_operation(unsigned int operation)
{
/*
    1  << sll  00001033
    2  >> srl  00005033
    3  -  sub  40000033
    4  |  or   00006033
    5  ^  xor  00004033
    6  +  add  00000033
    7  &  and  00007033
    8  *  mul  02000033
    9  / divu  02005033
    10 % remu  02007033
*/

    unsigned int imm = reg_pos;
    reg_pos = imm - 1;

    unsigned int shift = operation + operation + operation - 3;
    unsigned int op = (((1025264681 >> shift) & 7) << 12) + 51;
        /* octal: 75'0704'6051 */
    if (operation > 7) op = op + 33554432; /* 0x0200'0000 */
    if (operation == 3) op = 1073741875;  /* 0x4000'0033 */

    /* code optimisation if second operand is a load from register */
    if (last_insn_type == 11) { /* push reg */
        imm = (last_insn >> 15) & 31;
        code_pos = code_pos - 4;
    }

    /* code optimisation for constant immediates */
    if (operation < 8) {
        if ((last_insn & 1044607) == 19) {
            /* 0xFF07F  ADDI ?, X0, ?
               register need not be checked
               if (((last_insn & 1048575) == (19 + ((reg_pos + 11) << 7))) { */
            imm = last_insn >> 20;
            if (operation == 3) imm = 0 - imm;
                /* 00000013  ADDI reg, reg, -imm
                   imm is always positive, therefore -(-2048)=2048
                   cannot happen */
            code_pos = code_pos - 4;
            op = (((1854505 >> shift) & 7) << 12) + 19;
                /* octal: 704'6051 */
        }
    }
    emit_irdo(imm, reg_pos, op);
}

void emit_comp(unsigned int condition)
{
    reg_pos = reg_pos - 1;

    if (condition < 2) {
        if ((last_insn & 4294963327) == 19) {
            /* 0xFFFFF07F optimization if compared with 0 */
            code_pos = code_pos - 4;
        }
        else {
            emit_isdo(reg_pos, reg_pos, reg_pos, 1074790451);
                /* sub REG, REG, REG+1 */
        }
        if (condition == 0) {
            emit_isdo(0, reg_pos, reg_pos, 1060883);
                /* sltiu REG, REG, 1        == */
        }
        else {
            emit_isdo(reg_pos, 0, reg_pos, 12339);
                /* sltu REG, x0, REG        != */
        }
    }
    else {
        unsigned int o = 45107;
            /* 0000B033  sltu REG, REG+1, REG     > or <= */
        if (condition < 4) o = 1060915;
            /* 00103033  sltu REG, REG, REG+1     < or >= */
        emit_isdo(reg_pos, reg_pos, reg_pos, o);
        if ((condition & 1) != 0) {
            emit_isdo(0, reg_pos, reg_pos, 1064979);
                /* xori REG, REG, 1         >= or <= */
        }
    }
    last_insn_type = 13; /* arith comparison */
}

void emit_index_push(unsigned int global, unsigned int ofs)
{
    emit_push();
    emit_load(global, ofs);
    emit_operation(6); /* add */
    emit_push();
    last_insn_type = 14; /* arith operation */
}

void emit_pop_store_array()
{
    /* reg_pos is always 11 at this point */
    reg_pos = 10;
    emit32(11862051);
        /* 00B50023  SB A1,0(A0) */
}

void emit_index_load_array(unsigned int global, unsigned int ofs)
{
    unsigned int imm = 0;
    unsigned int rs = reg_pos;
    if (last_insn_type == 8) { /* push imm12 */
        imm = last_insn >> 20;
        code_pos = code_pos - 4;
        rs = local_reg[ofs];
        if ((global != 0) | (ofs >= 13)) {
            emit_load(global, ofs);
            rs = reg_pos;
        }
    }
    else {
        emit_index_push(global, ofs);
        reg_pos = reg_pos - 1;
    }
    emit_isdo(imm, rs, reg_pos, 16387);
        /* LBU REG[reg_pos], 0(REG[reg_pos]) */
}

unsigned int emit_pre_while()
{
    return code_pos;
}

unsigned int emit_if(unsigned int condition)
{
    /* at function entry reg_pos is always 11 */
    unsigned int rs = 10;
    unsigned int rt = 11;

    /* code optimisation if first operand is a local register */
    unsigned int prev_insn = get_32bit(buf + code_pos - 8);
    if ((prev_insn & 4293951487) == 1299) {
        /*           0xfff07fff) == 0x513     ADDI A0, x?, 0 */
        rs = (prev_insn >> 15) & 31;
        code_pos = code_pos - 8;
        emit32(last_insn);
    }

    /* code optimisation if second operand is a local register */
    if (last_insn_type == 11) { /* push reg */
        rt = (last_insn >> 15) & 31;
        code_pos = code_pos - 4;
    }

    /* code optimisation if second operand is 0 */
    if (last_insn == 1427) { /* 00000593  ADDI A1, X0, 0 */
        rt = 0;
        code_pos = code_pos - 4;
    }

    /* 0  00001063     BNE  s, t, +0    ==
       1  00000063     BEQ  s, t, +0    !=
       2  00007063     BGEU s, t, +0   <
       3  00006063     BLTU s, t, +0   >=
       4  00007063     BGEU t, s, +0   >
       5  00006063     BLTU t, s, +0   <= */
    if (condition > 3) {
        unsigned h = rs;
        rs = rt;
        rt = h;
        condition = condition - 2;
    }
    emit_isdo(rt, rs, 0,
        ((condition & 2) << 13) |
        (((condition & 3) ^ 1) << 12) | 99);
/*
    emit32(
        (rt << 20) |
        (rs << 15) |
        ((condition & 2) << 13) |
        (((condition & 3) ^ 1) << 12) |
        99);
*/
    reg_pos = 10;
    return code_pos - 4;
}

void emit_then_end(unsigned int insn_pos)
{
    unsigned int immb = code_pos - insn_pos;
    immb = 
        ((immb & 4096) << 19) |     /* bit  31     = immb[12] */
        ((immb & 2016) << 20) |     /* bits 30..25 = immb[10..5] */
        ((immb &   30) <<  7) |     /* bits 11..8  = immb[4..1] */
        ((immb & 2048) >>  4);      /* bit  7      = immb[11] */
    set_32bit(buf + insn_pos, (get_32bit(buf + insn_pos) & 33550463) | immb);
    last_branch_target = code_pos;
}

void emit_else_end(unsigned int insn_pos)
{
    set_32bit(buf + insn_pos, insn_jal(0, code_pos - insn_pos));
    last_branch_target = code_pos;
}

static unsigned int emit_then_else(unsigned int insn_pos)
{
    emit32(0);
    emit_then_end(insn_pos);
    return code_pos - 4;
}

static void emit_loop(unsigned int destination, unsigned int insn_pos)
{
    emit32(insn_jal(0, destination - code_pos));
    emit_then_end(insn_pos);
}

unsigned int emit_local_var(unsigned int init)
{
    unsigned int n = num_locals + 1;
    num_locals = n;
    if (n > max_locals) max_locals = n;

    if (init != 0) {                                 /* set initial value */
        emit_store(0, n);
    }

    return n;
}

unsigned int emit_global_var()
{
    num_globals = num_globals + 1;
    return num_globals - 513;
}

unsigned int emit_pre_call()
{
    /* save expression stack it it is not empty */
    unsigned int r = reg_pos;
    if (r > 10) {
        /* save currently used expression stack registers */
        while (reg_pos > 10) {
            reg_pos = reg_pos - 1;
            emit_local_var(1);
        }
    }
    reg_pos = 10;
    return r;
}

void emit_arg()
{
    emit_push();
}

unsigned int emit_call(unsigned int ofs, unsigned int pop, unsigned int save)
{
    unsigned int r = code_pos;
    emit32(insn_jal(1, ofs - code_pos));
    num_calls = num_calls + 1;

    if (save > 10) {
        /* restore previously saved expression stack registers */
        emit32((save << 7) + 327699);
            /* 000500513  MV REG[reg_pos], A0 */

        reg_pos = 10;
        while (reg_pos < save) {
            emit_load(0, num_locals);
            reg_pos = reg_pos + 1;
            num_locals = num_locals - 1;
        }
    }
    last_insn_type = 0; /* avoid fusion with next instruction */

    reg_pos = save;
    return r;
}

void emit_fix_call(unsigned int from, unsigned int to)
{
    set_32bit(buf + from, insn_jal(1, to - from));
}

unsigned int emit_func_begin(unsigned int n)
{
    unsigned int cp0 = code_pos;
    unsigned int cp8 = cp0 + 8;
    function_start_pos = cp0;
    reg_pos = 10;
    max_reg_pos = 10;
    num_locals = n;
    max_locals = n;
    num_scope = 0;
    num_calls = 0;
    return_list = 0;

    last_branch_target = cp8;
    code_pos = cp8;
        /* The first two instructions will be written by emit_func_end,
           when the number of local variables is known. */

    return cp0;
}

void emit_return()
{
    emit32(return_list);
        /* will be overwritten by a jump to the end of the function */
    return_list = code_pos - 4;
}

void emit_func_end()
{
    unsigned int m = max_locals;

    /* Set stack reservation at start of function.
       Stack pointer must be a multiple of 16.
       Shift by 20 is an optimisation to save the imm field shift */
    unsigned int stack_size = ((m + 4) >> 2) << 24;
    set_32bit(buf + function_start_pos, 65811 - stack_size);
        /* 00010113  ADD SP, SP, 0-stack_size */

    /* entry to prologue depends on number of local variables */
    unsigned int entry = 100 - function_start_pos;
    if (m < 9) {
        entry = entry + 80 - (m << 3);
    } else if (m < 12) {
        entry = entry + 48 - (m << 2);
    }
    entry = insn_jal(5, entry);
        /* J _prologue */
    set_32bit(buf + function_start_pos + 4, entry);

    /* go throught list of return statements */
    unsigned int cp = code_pos;
    unsigned int next = return_list;
    if (next == (cp - 4)) {
        /* remove last jump to following instruction */
        cp = cp - 4;
        code_pos = cp;
    }
    while (next != 0) {
        unsigned int pos = next;
        next = get_32bit(buf + pos);
        set_32bit(buf + pos, insn_jal(0, cp - pos));
    }

    /* emit jump to epilogue */
    emit32(stack_size + 659);
        /* 00000593  ADDI X5, X0, stack_size */
    emit32(insn_jal(0, 236 - (m << 2) - cp));
        /* J _epilogue + 4*(12-num_locals) */
}

unsigned int emit_scope_begin()
{
    num_scope = num_scope + 1;
    return num_locals;
}

void emit_scope_end(unsigned int save)
{
    num_locals = save;
    num_scope = num_scope - 1;
}

unsigned int emit_begin()
{
    code_pos = 0;
    num_globals = 0;
    last_branch_target = 0;
    local_reg = " \x08\x09\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b";
    emit_binary_func(252, "\x7f\x45\x4c\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\xf3\x00\x01\x00\x00\x00\x54\x00\x01\x00\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x34\x00\x20\x00\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x01\x00........\x07\x00\x00\x00\x00\x10\x00\x00\x13\x00\x00\x00\x13\x00\x00\x00\x00\x00\x00\x00\x93\x68\xd0\x05\x73\x00\x00\x00\x23\x28\xb1\x03\x23\x26\xa1\x03\x23\x24\x91\x03\x23\x22\x81\x03\x23\x20\x71\x03\x93\x8b\x08\x00\x23\x2e\x61\x01\x13\x0b\x08\x00\x23\x2c\x51\x01\x93\x8a\x07\x00\x23\x2a\x41\x01\x13\x0a\x07\x00\x23\x28\x31\x01\x93\x89\x06\x00\x23\x26\x21\x01\x13\x09\x06\x00\x23\x24\x91\x00\x93\x84\x05\x00\x23\x22\x81\x00\x13\x04\x05\x00\x23\x20\x11\x00\x67\x80\x02\x00\x83\x2d\x01\x03\x03\x2d\xc1\x02\x83\x2c\x81\x02\x03\x2c\x41\x02\x83\x2b\x01\x02\x03\x2b\xc1\x01\x83\x2a\x81\x01\x03\x2a\x41\x01\x83\x29\x01\x01\x03\x29\xc1\x00\x83\x24\x81\x00\x03\x24\x41\x00\x83\x20\x01\x00\x33\x01\x51\x00\x67\x80\x00\x00");
/*
elf_header:
    0000 7f 45 4c 46    e_ident         0x7F, "ELF"
    0004 01 01 01 00                    1, 1, 1, 0
    0008 00 00 00 00                    0, 0, 0, 0
    000C 00 00 00 00                    0, 0, 0, 0
    0010 02 00          e_type          2 (executable)
    0012 03 00          e_machine       0xF3 (RISC-V)
    0014 01 00 00 00    e_version       1
    0018 00 00 01 00    e_entry         0x00010000 + _start
    001C 34 00 00 00    e_phoff         program_header_table
    0020 00 00 00 00    e_shoff         0
    0024 00 00 00 00    e_flags         0
    0028 34 00          e_ehsize        52 (program_header_table)
    002A 20 00          e_phentsize     32 (start - program_header_table)
    002C 01 00          e_phnum         1
    002E 00 00          e_shentsize     0
    0030 00 00          e_shnum         0
    0032 00 00          e_shstrndx      0

program_header_table:
    0034 01 00 00 00    p_type          1 (load)
    0038 00 00 00 00    p_offset        0
    003C 00 80 04 08    p_vaddr         0x00010000 (default)
    0040 00 80 04 08    p_paddr         0x00010000 (default)
    0044 ?? ?? ?? ??    p_filesz
    0048 ?? ?? ?? ??    p_memsz
    004C 07 00 00 00    p_flags         7 (read, write, execute)
    0050 00 10 00 00    p_align         0x1000 (4 KiByte)

_start:
    0054 ?? ?? ?? ??                    set gp register
    0058 ?? ?? ?? ??
    005C 00 00 00 00    jal x1, main
    0060 93 68 D0 05    or x17, x0, 93
    0064 73 00 00 00    ecall

_prologue:
    0068 23 28 b1 03    sw      s11,48(sp)
   4:   03a12623                sw      s10,44(sp)
   8:   03912423                sw      s9,40(sp)
   c:   03812223                sw      s8,36(sp)
  10:   03712023                sw      s7,32(sp)
  14:   00088b93                mv      s7,a7
  18:   01612e23                sw      s6,28(sp)
  1c:   00080b13                mv      s6,a6
  20:   01512c23                sw      s5,24(sp)
  24:   00078a93                mv      s5,a5
  28:   01412a23                sw      s4,20(sp)
  2c:   00070a13                mv      s4,a4
  30:   01312823                sw      s3,16(sp)
  34:   00068993                mv      s3,a3
  38:   01212623                sw      s2,12(sp)
  3c:   00060913                mv      s2,a2
  40:   00912423                sw      s1,8(sp)
  44:   00058493                mv      s1,a1
  48:   00812223                sw      s0,4(sp)
  4c:   00050413                mv      s0,a0
  50:   00112023                sw      ra,0(sp)
    00BC 67 80 02 00    jr      t0

_epilogue:
    00C0 83 2d 01 03    lw      s11,48(sp)
  5c:   02c12d03                lw      s10,44(sp)
  60:   02812c83                lw      s9,40(sp)
  64:   02412c03                lw      s8,36(sp)
  68:   02012b83                lw      s7,32(sp)
  6c:   01c12b03                lw      s6,28(sp)
  70:   01812a83                lw      s5,24(sp)
  74:   01412a03                lw      s4,20(sp)
  78:   01012983                lw      s3,16(sp)
  7c:   00c12903                lw      s2,12(sp)
  80:   00812483                lw      s1,8(sp)
  84:   00412403                lw      s0,4(sp)
  88:   00012083                lw      ra,0(sp)
  8c:   00510133                add     sp,sp,t0
    00F8 67 80 00 00    ret
*/

    return 92;
        /* return the address of the call to main() as a forward reference */
}

unsigned int emit_end()
{
    unsigned int addr = code_pos + 1964; /* 2048 - 84 */
    set_32bit(buf + 84, (((addr + 2048) >> 12) << 12) + 407);
        /* 00000197  AUIPC GP, hi(addr) */
    set_32bit(buf + 88, (addr << 20) + 98707);
        /* 00018193  ADDI GP, GP, lo(addr) */
    unsigned int i = 0;
    while (i < num_globals) {
        emit32(0);
        i = i + 1;
    }

    set_32bit(buf + 68, code_pos);
    set_32bit(buf + 72, code_pos);
    return code_pos;
}
