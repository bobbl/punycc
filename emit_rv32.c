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

unsigned int extract_jal_disp(unsigned int insn)
{
    return (((insn >> 20) & 2046) |             /* bits 10..1  = insn[30..21] */
            (((insn >> 20) & 1) << 11) |        /* bit  11     = insn[20]     */
            (insn & 1044480) |                  /* bits 19..12 = insn[19..12]   0xFF000 */
            ((0 - ((insn >> 31) & 1)) & 4293918720)); /* bits 31..20 = insn[31]       0xFFF00000 */
 /*           ^^^^^ FIXME: signed integer */
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

void emit_index_push(unsigned int global, unsigned int ofs)
{
    emit_push();
    emit_load(global, ofs);
    emit_isdo(reg_pos, reg_pos-1, reg_pos-1, 51);
        /* ADD REG[reg_pos-1], REG[reg_pos-1], REG[reg_pos] */
    last_insn_type = 14; /* arith operation */

}

void emit_pop_store_array()
{
    /* reg_pos is always 11 at this point */
    reg_pos = 10;
    emit32(11862051);
        /* 00B50023  SB A1,0(A0) */
}

void emit_index_load_array(unsigned int which, unsigned int ofs)
{
    emit_index_push(which, ofs);
    reg_pos = reg_pos - 1;

    emit_isdo(0, reg_pos, reg_pos, 16387);
        /* LBU REG[reg_pos], 0(REG[reg_pos]) */
}


/* Same as emit_isdo(), but rs=reg_pos and if REG[reg_pos] is loaded from
   a local varaible in the previous instruction, fuse them. */
void emit_irdo(unsigned int imm, unsigned int rd, unsigned int opcode)
{
    unsigned int reg_s = reg_pos;
    unsigned int prev_insn = get_32bit(buf + code_pos - 4);
    if ((prev_insn & 4293947519) == 19) {
        /*           0xfff0707f) == 0x13) { */
        if (((prev_insn >> 7) & 31) == reg_pos) { /* really necessary? */
            /* load local*/
            reg_s = (prev_insn >> 15) & 31;
            code_pos = code_pos - 4;
        }
    }
    emit_isdo(imm, reg_s, rd, opcode);
    last_insn_type = 14; /* arith operation */
}


void emit_operation(unsigned int operation)
{
    unsigned int reg_t = reg_pos;
    unsigned int reg_s = reg_t - 1;
    reg_pos = reg_s;

/*
    if (t == 1)       o = 4147;         / * << sll  00101033 * /
    else if (t == 2)  o = 20531;        / * >> srl  00105033 * /
    else if (t == 3)  o = 1073741875;   / * -  sub  40100033 * /
    else if (t == 4)  o = 24627;        / * |  or   00106033 * /
    else if (t == 5)  o = 16435;        / * ^  xor  00104033 * /
    else if (t == 6)  o = 51;           / * +  add  00100033 * /
    else if (t == 7)  o = 28723;        / * &  and  00107033 * /
    else if (t == 8)  o = 33554483 + 1048576;     / * *  mul  02100033 * /
    else if (t == 9)  o = 33574963 + 1048576;     / * / divu  02105033 * /
    else if (t == 10) o = 33583155 + 1048576;     / * % remu  02107033 * /
*/

    unsigned int imm = reg_pos;
        /*    reg_pos+1      But +1 is added to opcode */
    char *code_arith = "    \x33\x10\x10\x00\x33\x50\x10\x00\x33\x00\x10\x40\x33\x60\x10\x00\x33\x40\x10\x00\x33\x00\x10\x00\x33\x70\x10\x00\x33\x00\x10\x02\x33\x50\x10\x02\x33\x70\x10\x02"; 
    unsigned int op = get_32bit((unsigned char *)code_arith + (operation<<2));

    /* code optimization if second operand is a load from register */
    if (last_insn_type == 11) { /* push reg */
        imm = ((last_insn >> 15) & 31) - 1;
        code_pos = code_pos - 4;
    }
    

    /* code optimization for constant immediates */
    if (operation < 8) {
        if ((last_insn & 1044607) == 19) {
            /* 0xFF07F  addi ?, x0, ?
               register need not be checked
               if (((last_insn & 1048575) == (19 + ((reg_pos + 11) << 7))) { */
            imm = last_insn >> 20;
            if (operation == 3) imm = 0 - imm;
                /* 00000013  addi REG, REG, -IMM 
                   imm is always positive, therefore -(-2048)=2048
                   cannot happen */
            code_pos = code_pos - 4;
            op = (((1884685584 >> (operation << 2)) & 15) << 12) + 19;
                /* 0x70560510 */
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

unsigned int emit_pre_while()
{
    return code_pos;
}

unsigned int emit_if(unsigned int condition)
{
    /* in this case reg_pos must be 11 */
    reg_pos = 10;

    unsigned int o = 10874979;
    if (condition < 2) {
        o = 11866211;
        if (last_insn == 1427) { /* 00000593  addi a1, x0, 0 */
            /* optimization if compared with 0 */
            code_pos = code_pos - 4;
            o = 10489955;  /* 00A01063 bne zero, a0, +0 */
                /* Don't use the recommended `bne a0, zero, +0` which is
                   abbreviated as `bnez a0, +0`.
                   Reason: in refactor() a0 will be replaced by a register
                   and this way a0 remains at the same position in the
                   opcode and no special case is needed. */
        }
    }
    else if (condition < 4) {
        o = 11890787;
    }
    emit32(o - ((condition & 1) << 12));
            /* 0  00B51063     bne a0, a1, +0    ==
               1  00B50063     beq a0, a1, +0    !=
               2  00B57063     bgeu a0, a1, +0   <
               3  00B56063     bltu a0, a1, +0   >=
               4  00A5F063     bgeu a1, a0, +0   >
               5  00A5E063     bltu a1, a0, +0   <= */
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
    /* save parameter stack it it is not empty */
    unsigned int r = reg_pos;
    if (r > 10) {
        /* save currently used parameter registers */
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
        /* restore previously saved parameter registers */
        emit32((save << 7) + 327699);
            /* 000500513  MV REG[reg_pos], A0 */

        reg_pos = 10;
        while (reg_pos < save) {
            emit_load(0, num_locals);
            reg_pos = reg_pos + 1;
            num_locals = num_locals - 1;
        }
    }

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

void restore_sp()
{
    emit32(73859);              /* 00412083  lw ra, 0(sp) */
    emit32(return_list);         /* 00010113  add sp, sp, MAX_LOCALS+1 */
    return_list = code_pos - 4;
}

void emit_return()
{
    emit32(return_list);         /* 00010113  add sp, sp, MAX_LOCALS+1 */
    return_list = code_pos - 4;
    emit32(insn_jal(0, 192 - code_pos));
        /* j _epilogue */
    return;


    /* TODO */
    unsigned int prev_insn = get_32bit(buf + code_pos - 4);
    if (last_branch_target != code_pos) {
        /* precondition: no if or else branch to the current position */

        /* Do not generate anything if the last instruction was a return */
        if (prev_insn == 32871) return;

        /* Tail call optimisation:
           If the last instruction was a (already resolved) call */
        if ((prev_insn & 4095) == 239)
                    /* & 0xFFF) == 0x0EF JAL RA,*/
        {
            code_pos = code_pos - 4;
            restore_sp();
            unsigned int disp = extract_jal_disp(prev_insn);
            emit32(insn_jal(0, disp - 8));

            num_calls = num_calls - 1;
                /* enables more "locals in variable" optimisations */
            return;
        }
    }
    restore_sp();
    emit32(32871);          /* 00008067  RET */

}

void emit_func_end()
{
    unsigned int n = ((max_locals + 4) >> 2) << 2;
        /* stack pointer must be a multiple of 16 */

    emit_return();


    /* set stack reservation at start of function */
    set_32bit(buf + function_start_pos, 4290838803 - ((n-1) << 22));
        /* FFC10113  ADD SP, SP, -4*n */

    /* entry to prologue depends on number of local variables */
    unsigned int entry = 100 - function_start_pos;
    if (max_locals < 9) {
        entry = entry + 80 - (max_locals << 3);
    } else if (num_locals < 12) {
        entry = entry + 48 - (max_locals << 2);
    }
    unsigned int insn_jal5 = insn_jal(5, entry);
        /* J _prologue */
    set_32bit(buf + function_start_pos + 4, insn_jal5);



    /* go throught list of return statements */
    unsigned int insn_li = (n << 22) + 659;
        /* 00000593  ADDI X5, X0, 4*n */
    unsigned int next = return_list;
    while (next != 0) {
        unsigned char *p = buf + next;
        unsigned int insn_j = insn_jal(0, 236 - (max_locals << 2) - next);
            /* J _epilogue + 4*(12-num_locals) */

        next = get_32bit(p);
        set_32bit(p, insn_li);
        set_32bit(p+4, insn_j);
    }
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
