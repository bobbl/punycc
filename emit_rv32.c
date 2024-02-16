/**********************************************************************
 * Code Generation for RV32IM
 **********************************************************************/

/* constants */
unsigned buf_size;

/* global variables */
unsigned char *buf;
unsigned code_pos;
unsigned reg_pos;
unsigned num_globals;
unsigned last_insn;
unsigned num_locals;
unsigned max_locals;
unsigned function_start_pos;

void set_32bit(unsigned char *p, unsigned x);
unsigned get_32bit(unsigned char *p);


void emit(unsigned b)
{
    buf[code_pos] = b;
    code_pos = code_pos + 1;
}

unsigned emit_binary_func(unsigned n, char *s)
{
    unsigned r = code_pos;
    unsigned i = 0;
    while (i < n) {
        emit(s[i]);
        i = i + 1;
    }
    return r;
}

void emit32(unsigned n)
{
    emit(n);
    emit(n >> 8);
    emit(n >> 16);
    emit(n >> 24);
    last_insn = n;
}

unsigned insn_jal(unsigned rd, unsigned immj)
{
    return
        ((immj & 1048576) << 11) |      /* bit  31     = imm[20] */
        ((immj & 2046   ) << 20) |      /* bits 30..21 = imm[10..1] */
        ((immj & 2048   ) <<  9) |      /* bit  20     = imm[11] */
        ((immj & 1044480)      ) |      /* bits 19..12 = imm[19..12] */
        ( rd              <<  7) |      /* bits 11..7  = rd */
        111;                            /* bits  6..0  = 0x6f (jal) */
}

void emit_insn_sw(unsigned rs2, unsigned rs1, unsigned immi)
{
    emit32(
        ((immi & 1016   ) << 22) |      /* bits 31..25 = imm[11..5] */
        ( rs2             << 20) |      /* bits 24..20 = rs2 */
        ( rs1             << 15) |      /* bits 19..15 = rs1 */
        ((immi & 7      ) <<  9) |      /* bits 11..7  = imm[4..0] */
        8227);                          /* bits 14..12 */
                                        /* bits  6..0  = 0x2023 (sw) */
}

void emit_insn_lw(unsigned rd, unsigned rs, unsigned immi)
{
    emit32(
        ( immi            << 22) |      /* bits 31..20 = imm[11..0] */
        ( rs              << 15) |      /* bits 19..15 = rs */
        ( rd              <<  7) |      /* bits 11..7  = rd */
        8195);                          /* bits 14..12 */
                                        /* bits  6..0  = 0x2003 (lw) */
}

void emit_insn_d_s_t(unsigned opcode)
{
    emit32(opcode + (reg_pos << 7) + (reg_pos << 15) + (reg_pos << 20));
}

void emit_insn_d_s(unsigned opcode)
{
    emit32(opcode + (reg_pos << 7) + (reg_pos << 15));
}

void emit_insn_d_t(unsigned opcode)
{
    emit32(opcode + (reg_pos << 7) + (reg_pos << 20));
}

void emit_insn_d(unsigned opcode)
{
    emit32(opcode + (reg_pos << 7));
}

void emit_insn_s_t(unsigned opcode)
/* used only once */
{
    emit32(opcode + (reg_pos << 15) + (reg_pos << 20));
}

void emit_push()
{
    reg_pos = reg_pos + 1;
}

void emit_number(unsigned imm)
{
    if ((imm + 2048) < 4096) {
        emit_insn_d((imm << 20) + 19);
            /* 00000013  addi REG, x0, IMM */
    }
    else {
        emit_insn_d((((imm + 2048) >> 12) << 12) + 55);
            /* 00000037  lui REG, IMM */
        if ((imm << 20)) {
            emit_insn_d_s((imm << 20) + 19);
                /* 00000013  addi REG, REG, IMM */
        }
    }
}

void emit_string(unsigned len, char *s)
{
    emit32(insn_jal(reg_pos, (len + 8) & 4294967292));
        /* jal REGPOS, end_of_string */
    emit_binary_func(len, s);
    emit_binary_func(4 - (len & 3), "\x00\x00\x00\x00");
        /* at least one 0 as end mark and then align */
}

void emit_store(unsigned global, unsigned ofs)
{
    emit_insn_sw(reg_pos, global + 2, ofs + 1);
}

void emit_load(unsigned global, unsigned ofs)
{
    emit_insn_lw(reg_pos, global + 2, ofs + 1);
}

void e_index(unsigned global, unsigned ofs)
{
    emit_insn_lw(reg_pos + 1, global + 2, ofs + 1);
    emit_insn_d_s_t(1048627); 
        /* add REGPOS, REGPOS, REGPOS+1 */
}

void emit_index_push(unsigned global, unsigned ofs)
{
    e_index(global, ofs);
    emit_push();
}

void emit_pop_store_array()
{
    reg_pos = reg_pos - 1;
    emit_insn_s_t(1048611);
        /* 00000023  sb REG+1, 0(REG) */
}

void emit_index_load_array(unsigned global, unsigned ofs)
{
    e_index(global, ofs);
    emit_insn_d_s(16387);
        /* lbu REG, 0(REG) */
}




void emit_operation(unsigned t)
{
    reg_pos = reg_pos - 1;

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

    /* code optimization for constant immediates */

    if (t < 8) {
        if ((last_insn & 1044607) == 19) {
            /* 0xFF07F  addi ?, x0, ?
               register need not be checked
               if (((last_insn & 1048575) == (19 + ((reg_pos + 11) << 7))) { */
            char *code_func = " \x01\x05\x00\x06\x05\x00\x07";
            unsigned imm = (last_insn >> 20) << 20;
            code_pos = code_pos - 4;
            if (t == 3) {
                emit_insn_d_s(19 - imm);
                    /* 00000013  addi REG, REG, -IMM 
                       imm is always positive, therefore -(-2048)=2048
                       cannot happen */
            }
            else emit_insn_d_s((code_func[t]<<12) | 19 | imm);
            return;
        }
    }

    char *code_arith = "    \x33\x10\x10\x00\x33\x50\x10\x00\x33\x00\x10\x40\x33\x60\x10\x00\x33\x40\x10\x00\x33\x00\x10\x00\x33\x70\x10\x00\x33\x00\x10\x02\x33\x50\x10\x02\x33\x70\x10\x02"; 
    emit_insn_d_s_t(get_32bit((unsigned char *)code_arith + (t<<2)));
}

void emit_comp(unsigned t)
{
    reg_pos = reg_pos - 1;

    if (t < 18) {
        if ((last_insn & 4294963327) == 19) {
            /* 0xFFFFF07F optimization if compared with 0 */
            code_pos = code_pos - 4;
        }
        else {
            emit_insn_d_s_t(1074790451);
                /* sub REG, REG, REG+1 */
        }
        if (t == 16) emit_insn_d_s(1060883);
            /* sltiu REG, REG, 1        == */
        else         emit_insn_d_t(12339);
            /* sltu REG, x0, REG        != */
    }
    else {
        if (t < 20) emit_insn_d_s_t(1060915);
            /* sltu REG, REG, REG+1     < or >= */
        else        emit_insn_d_s_t(45107);
            /* sltu REG, REG+1, REG     > or <= */
        if (t & 1)  emit_insn_d_s(1064979);
            /* xori REG, REG, 1         >= or <= */
    }
}

unsigned emit_pre_while()
{
    return code_pos;
}

unsigned emit_if(unsigned condition)
{
    unsigned o = 327779;
        /* in this case reg_pos must be 10 */
        /* 00050063     beqz a0, ... */

    if (condition) {
        /* reg_pos must be 11 */
        reg_pos = 10;

        o = 10874979;
        if (condition < 18) {
            o = 11866211;
            if (last_insn == 1427) { /* 00000593  addi a1, x0, 0 */
                /* optimization if compared with 0 */
                code_pos = code_pos - 4;
                o = 331875; /* 00051063  bnez a0, +0 */
            }
        }
        else if (condition < 20) o = 11890787;
        o = o - ((condition & 1) << 12);
            /* t==16 00B51063     bne a0, a1, +0    ==
               t==17 00B50063     beq a0, a1, +0    !=
               t==18 00B57063     bgeu a0, a1, +0   <
               t==19 00B56063     bltu a0, a1, +0   >=
               t==20 00A5F063     bgeu a1, a0, +0   >
               t==21 00A5E063     bltu a1, a0, +0   <=      */
    }
    emit32(o);
    return code_pos - 4;
}

void emit_fix_branch_here(unsigned insn_pos)
{
    unsigned immb = code_pos - insn_pos;
    immb = 
        ((immb & 4096) << 19) |     /* bit  31     = immb[12] */
        ((immb & 2016) << 20) |     /* bits 30..25 = immb[10..5] */
        ((immb &   30) <<  7) |     /* bits 11..8  = immb[4..1] */
        ((immb & 2048) >>  4);      /* bit  7      = immb[11] */
    set_32bit(buf + insn_pos, get_32bit(buf + insn_pos) | immb);
}

void emit_fix_jump_here(unsigned insn_pos)
{
    set_32bit(buf + insn_pos, insn_jal(0, code_pos - insn_pos));
}

unsigned emit_jump_and_fix_branch_here(unsigned destination, unsigned insn_pos)
{
    emit32(insn_jal(0, destination - code_pos));
    emit_fix_branch_here(insn_pos);
    return code_pos - 4;
}

unsigned emit_pre_call()
/* save temporary registers and return how many */
{
    unsigned r = reg_pos;
    if (r > 10) {
        /* emit32(16435);    xor zero, zero, zero */
        /* save currently used temporary registers */
        unsigned i = 10;
        while (i < r) {
            emit_insn_sw(i, 2, num_locals + i - 8);
            i = i + 1;
        }
    }
    reg_pos = 10;
    return r;
}

void emit_arg(unsigned i)
{
    reg_pos = reg_pos + 1;
}

void update_locals(unsigned n)
{
    if (n > max_locals) {
        max_locals = n;
        set_32bit(buf + function_start_pos + 4, 65811 + ((0 - 2 - max_locals) << 22));
            /* add sp, sp, -4*maxlocals-8 */
    }

}

unsigned emit_call(unsigned ofs, unsigned pop, unsigned save)
{
    unsigned r = code_pos;
    emit32(insn_jal(1, ofs - code_pos));
    reg_pos = save;

    update_locals(num_locals + reg_pos - 10);
    if (reg_pos > 10) {
        /* restore previously saved temporary registers */
        emit_insn_d(327699);
            /* 000500513  mv REG, a0 */
        unsigned i = 10;
        while (i < reg_pos) {
            emit_insn_lw(i, 2, num_locals + i - 8);
            i = i + 1;
        }
    }
    return r;
}

unsigned emit_fix_call(unsigned from, unsigned to)
{
    return insn_jal(1, to - from);
}

unsigned emit_local_var()
{
    num_locals = num_locals + 1;
    update_locals(num_locals);
    emit_insn_sw(10, 2, num_locals + 1); /* set initial value */
    return num_locals;
}

unsigned emit_global_var()
{
    num_globals = num_globals + 1;
    return num_globals - 514;
}

unsigned emit_enter(unsigned n)
{
    function_start_pos = code_pos;
    reg_pos = 10;
    num_locals = n;
    max_locals = 0;

    emit_binary_func(16, "\x13\x02\x01\x00\x13\x01\x81\xff\x23\x20\x41\x00\x23\x22\x11\x00");
        /*  00010213  mv  tp, sp
            ff810113  add sp, sp, -8
            00412023  sw  tp, 0(sp)
            00112223  sw  ra, 4(sp) */
    update_locals(n); 
        /* if there is at least one parameter, overwrite add sp, sp, -8 */

    unsigned i = 0;
    while (i < n) {
        emit_insn_sw(i + 10, 2, i + 2);
        i = i + 1;
    }
    return function_start_pos;
}

void emit_return()
{
    emit_binary_func(12, "\x83\x20\x41\x00\x03\x21\x01\x00\x67\x80\x00\x00");
        /*  00412083  lw ra, 4(sp)
            00012103  lw sp, 0(sp)
            00008067  ret */
}

unsigned emit_scope_begin()
{
    return num_locals;
}

void emit_scope_end(unsigned save)
{
    num_locals = save;
}

unsigned emit_begin()
{
    code_pos = 0;
    num_globals = 0;
    emit_binary_func(104, "\x7f\x45\x4c\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\xf3\x00\x01\x00\x00\x00\x54\x00\x01\x00\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x34\x00\x20\x00\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x01\x00........\x07\x00\x00\x00\x00\x10\x00\x00\x13\x00\x00\x00\x13\x00\x00\x00\x00\x00\x00\x00\x93\x68\xd0\x05\x73\x00\x00\x00");
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
    0054 
    0058 
    005C 00 00 00 00    jal x1, main
    0064 93 68 D0 05    or x17, x0, 93
    0068 73 00 00 00    ecall
*/

    return 92;
        /* return the address of the call to main() as a forward reference */
}

unsigned emit_end()
{
    unsigned addr = code_pos + 1964; /* 2048 - 84 */
    set_32bit(buf + 84, (((addr + 2048) >> 12) << 12) + 407);
        /* 00000197  auipc gp, ADDR_HI */
    set_32bit(buf + 88, (addr << 20) + 98707);
        /* 00018193  addi gp, gp, ADDR_LO */
    unsigned i = 0;
    while (i < num_globals) {
        emit32(0);
        i = i + 1;
    }

    set_32bit(buf + 68, code_pos);
    set_32bit(buf + 72, code_pos);
    return code_pos;
}
