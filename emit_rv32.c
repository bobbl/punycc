/**********************************************************************
 * Code Generation for RV32IM
 **********************************************************************/


/* constants */
unsigned buf_size;

/* global variables */
char *buf;
unsigned code_pos;
unsigned reg_pos;
unsigned num_globals;
unsigned last_insn;
unsigned num_locals;
unsigned max_locals;
unsigned function_start_pos;
unsigned num_scope;
unsigned num_calls;
unsigned max_reg_pos;
unsigned addsp_list;

unsigned last_branch_target;
    /* Position where the last branch points to.
       Used to determine the length of the last uninterrupted sequences of
       instructions. */



/* helper to write a 32 bit number to a char array */
void set_32bit(char *p, unsigned x)
{
    p[0] = x;
    p[1] = x >> 8;
    p[2] = x >> 16;
    p[3] = x >> 24;
}

/* helper to read 32 bit number from a char array */
unsigned get_32bit(char *p)
{
    return (p[0] & 255) +
          ((p[1] & 255) << 8) +
          ((p[2] & 255) << 16) +
          ((p[3] & 255) << 24);
}



unsigned emit_binary_func(unsigned n, char *s)
{
    unsigned cp = code_pos;
    unsigned i = 0;
    while (i < n) {
        buf[cp + i] = s[i];
        i = i + 1;
    }
    code_pos = cp + n;
    return cp;
}

void emit32(unsigned n)
{
    unsigned cp = code_pos;
    code_pos = cp + 4;
    last_insn = n;
    set_32bit(buf + cp, n);
}

void emit_isdo(unsigned imm, unsigned rs, unsigned rd, unsigned opcode)
{
    emit32((imm << 20) + (rs << 15) + (rd << 7) + opcode);
}

unsigned insn_jal(unsigned rd, unsigned immj)
{
    return
        (((immj>>20) & 1) << 31) |      /* bit  31     = imm[20] */
        (((immj & 2046  )        |      /* bits 30..21 = imm[10..1] */
         ((immj>>11) & 1))<< 20) |      /* bit  20     = imm[11] */
        ((immj & 1044480)      ) |      /* bits 19..12 = imm[19..12] */
        ( rd              <<  7) |      /* bits 11..7  = rd */
        111;                            /* bits  6..0  = 0x6f (jal) */
}

unsigned extract_jal_disp(unsigned insn)
{
    return (((insn >> 20) & 2046) |             /* bits 10..1  = insn[30..21] */
            (((insn >> 20) & 1) << 11) |        /* bit  11     = insn[20]     */
            (insn & 1044480) |                  /* bits 19..12 = insn[19..12]   0xFF000 */
            ((0 - (insn >> 31)) & 4293918720)); /* bits 31..20 = insn[31]       0xFFF00000 */
}

void emit_stype(unsigned opcode, unsigned imm)
{
    emit32(opcode +
        ((imm & 1016   ) << 22) +       /* bits 31..25 = imm[11..5] */
        ((imm & 7      ) <<  9));       /* bits 11..7  = imm[4..0] */
}

void emit_push()
{
    reg_pos = reg_pos + 1;
    if (reg_pos > max_reg_pos) max_reg_pos = reg_pos;
}

void emit_number(unsigned imm)
{
    if ((imm + 2048) < 4096) {
        emit_isdo(imm, 0, reg_pos, 19);
                /* 00000013  ADDI REG[reg_pos], X0, imm */
    }
    else {
        emit32((((imm + 2048) >> 12) << 12) + (reg_pos << 7) + 55);
                /* 00000037  LUI REG[reg_pos], imm */
        if ((imm << 20)) {
            emit_isdo(imm, reg_pos, reg_pos, 19);
                /* 00000013  ADDI REG[reg_pos], REG[reg_pos], imm */
        }
    }
}

void emit_string(unsigned len, char *s)
{
    unsigned aligned_len = (len + 4) & 4294967292;
        /* there are 4 zero bytes appended to s */
    emit32(insn_jal(reg_pos, aligned_len + 4));
        /* JAL REG[reg_pos], align(end_of_string) */
    emit_binary_func(aligned_len, s);
}

void emit_store(unsigned which, unsigned ofs, unsigned deref)
{
    if (deref) {
        emit_isdo(ofs << 2, which, reg_pos, 73859);
            /* LW REG[reg_pos+1], ofs(REG[2+global]) */
        emit_isdo(reg_pos, reg_pos, 0, 40995);
            /* SW reg_pos, 0(REG[reg_pos+1]) */
    }
    else {
        /* reg_pos is always 10 at this point */
        emit_stype(10559523 + (which << 15), ofs);
            /* SW A0, (ofs+1)(REG[2+global]) */
    }
}

void emit_load(unsigned which, unsigned ofs, unsigned deref)
{
/*  Accumulate the constant adds to the arguments in the opcode:
    emit_insn_lw(reg_pos, global + 2, ofs + 1);
    emit_isdo((ofs + 1)<<2, global + 2, reg_pos, 8195);
*/
    emit_isdo(ofs << 2, which, reg_pos, 73731);
        /* LW reg_pos, ofs(REG[2+global]) */

    if (deref) {
        emit_isdo(0, reg_pos, reg_pos, 8195);
            /* LW REG[reg_pos], 0(REG[reg_pos]) */
    }
}

void emit_index_push(unsigned global, unsigned ofs)
{
    emit_isdo(ofs << 2, global, reg_pos, 73859);
        /* LW REG[reg_pos+1], ofs(REG[2+global]) */

    emit_isdo(reg_pos, reg_pos, reg_pos, 1048627);
        /* ADD REG[reg_pos], REG[reg_pos], REG[reg_pos+1] */

    emit_push();
}

void emit_pop_store_array()
{
    /* reg_pos is always 11 at this point */
    reg_pos = 10;
    emit32(11862051);
        /* 00B50023  SB A1,0(A0) */
}

void emit_index_load_array(unsigned which, unsigned ofs)
{
    emit_index_push(which, ofs);
    reg_pos = reg_pos - 1;

    emit_isdo(0, reg_pos, reg_pos, 16387);
        /* LBU REG[reg_pos], 0(REG[reg_pos]) */
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
            unsigned imm = last_insn >> 20;
            if (t == 3) imm = 0 - imm;
                /* 00000013  addi REG, REG, -IMM 
                   imm is always positive, therefore -(-2048)=2048
                   cannot happen */
            code_pos = code_pos - 4;
            emit_isdo(imm, reg_pos, reg_pos, (code_func[t] << 12) + 19);
            return;
        }
    }

    char *code_arith = "    \x33\x10\x10\x00\x33\x50\x10\x00\x33\x00\x10\x40\x33\x60\x10\x00\x33\x40\x10\x00\x33\x00\x10\x00\x33\x70\x10\x00\x33\x00\x10\x02\x33\x50\x10\x02\x33\x70\x10\x02"; 
    emit_isdo(reg_pos, reg_pos, reg_pos, get_32bit(code_arith + (t<<2)));
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
            emit_isdo(reg_pos, reg_pos, reg_pos, 1074790451);
                /* sub REG, REG, REG+1 */
        }
        if (t == 16) emit_isdo(0, reg_pos, reg_pos, 1060883);
            /* sltiu REG, REG, 1        == */
        else emit_isdo(reg_pos, 0, reg_pos, 12339);
            /* sltu REG, x0, REG        != */
    }
    else {
        unsigned o = 45107;
            /* 0000B033  sltu REG, REG+1, REG     > or <= */
        if (t < 20) o = 1060915;
            /* 00103033  sltu REG, REG, REG+1     < or >= */
        emit_isdo(reg_pos, reg_pos, reg_pos, o);
        if (t & 1)  emit_isdo(0, reg_pos, reg_pos, 1064979);
            /* xori REG, REG, 1         >= or <= */
    }
}

unsigned emit_pre_while()
{
    return code_pos;
}

unsigned emit_if(unsigned condition)
{
    /* in this case reg_pos must be 10 */
    unsigned o = 10485859;  
        /* 00A00063     BEQ ZERO, A0, +0 */
        /* Don't use the recommended `BEQ A0, ZERO, +0` which is abbreviated
           as `BNEZ A0, +0`.
           Reason: In refactor() a0 will be replaced by a register and this way
           A0 remains at the same position in the opcode and no special case is
           needed. */

    if (condition) {
        /* reg_pos must be 11 */
        reg_pos = 10;

        o = 10874979;
        if (condition < 18) {
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
    set_32bit(buf + insn_pos, (get_32bit(buf + insn_pos) & 33550463) | immb);
    last_branch_target = code_pos;
}

void emit_fix_jump_here(unsigned insn_pos)
{
    set_32bit(buf + insn_pos, insn_jal(0, code_pos - insn_pos));
    last_branch_target = code_pos;
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
        /* save currently used temporary registers */
        unsigned i = 10;
        while (i < r) {
            emit_stype(73763 + (i << 20), num_locals + i - 9);
                /* SW REG[i], (num_locals+i+1-10)(SP) */
            i = i + 1;
        }
    }
    reg_pos = 10;
    return r;
}

void emit_arg(unsigned i)
{
    emit_push();
}

void update_locals(unsigned n)
{
    if (n > max_locals) max_locals = n;

}

unsigned emit_call(unsigned ofs, unsigned pop, unsigned save)
{
    unsigned r = code_pos;
    emit32(insn_jal(1, ofs - code_pos));
    reg_pos = save;
    num_calls = num_calls + 1;

    update_locals(num_locals + reg_pos - 10);
    if (reg_pos > 10) {
        /* restore previously saved temporary registers */
        emit32((reg_pos << 7) + 327699);
            /* 000500513  MV REG[reg_pos], A0 */
        unsigned i = 10;
        while (i < reg_pos) {
            emit_isdo((num_locals+i)<<2, 2, i, 4257226755);
                /* LW REG[i], (num_locals+1+i-10)(SP) */
            i = i + 1;
        }
    }
    return r;
}

unsigned emit_fix_call(unsigned from, unsigned to)
{
    return insn_jal(1, to - from);
}

unsigned emit_local_var(unsigned init)
{
    num_locals = num_locals + 1;
    update_locals(num_locals);

    if (init) {                                 /* set initial value */
        emit_stype(10559523, num_locals);
        /* SW A0, num_locals(SP) */
    }

    return num_locals;
}

unsigned emit_global_var()
{
    num_globals = num_globals + 1;
    return num_globals - 513;
}

unsigned emit_func_begin(unsigned n)
{
    function_start_pos = code_pos;
    reg_pos = 10;
    max_reg_pos = 10;
    num_locals = n;
    max_locals = 0;
    num_scope = 0;
    num_calls = 0;
    addsp_list = 0;

    emit32(0);                  /* 00010113  ADD SP, SP, max_locals+1 */
    emit32(1122339);            /* 00112023  SW  RA, 0(SP) */
    update_locals(n); 
        /* if there is at least one parameter, overwrite ADD SP, SP, -8 */

    /* Store arguments from registers to stack
       IMPORTANT: The registers are stored in reverse order (a2, a1, a0).
       Not an issue when storing them to the stack. But when the code is
       refactored and the arguments are stored in registers, the register
       numbers might overlap (e.g. in emit_isdo()).
     */
    while (n) {
        emit_stype(9510947 + (n << 20), n);
            /* SW REG[n-1+10], (4*n)(SP) */
        n = n - 1;
    }

    return function_start_pos;
}

void restore_sp()
{
    emit32(73859);              /* 00412083  lw ra, 0(sp) */
    emit32(addsp_list);         /* 00010113  add sp, sp, MAX_LOCALS+1 */
    addsp_list = code_pos - 4;
}

void emit_return()
{
    unsigned prev_insn = get_32bit(buf + code_pos - 4);

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
            unsigned disp = extract_jal_disp(prev_insn);
            emit32(insn_jal(0, disp - 8));

            num_calls = num_calls - 1;
                /* enables more "locals in variable" optimisations */
            return;
        }
    }
    restore_sp();
    emit32(32871);          /* 00008067  RET */
}

void refactor(unsigned start_pos, unsigned end_pos)
{
    unsigned i = start_pos;
    while (i < end_pos) {
        unsigned insn = get_32bit(buf + i);
        if (insn == 73859) {                  /* 00412083 LW RA,0(SP) */
            /* Next instruction is always
                00010113  ADD SP, SP, max_locals+1
               which must be removed, too */
            i = i + 4;
            insn = get_32bit(buf + i + 4);

            /* adjust target if tail call */
            if ((insn & 4095) == 111) {    /* xxxxx06F  JAL X0, xxxxx*/
                i = i + 4;
                unsigned disp = extract_jal_disp(insn) + i - code_pos;
                emit32(insn_jal(0, disp));
            }
        }
        else if ((insn & 1044575) == 73731) { 
            /*         & 0xFF05F) == 0x12003 */

            /* load or store local variable */
            if (insn & 32) {
                /* store */
                unsigned reg_for_var = (((insn >> 9) & 7) + ((insn >> 26) << 3))
                                        + max_reg_pos;
                unsigned prev_insn = get_32bit(buf + code_pos - 4);
                unsigned prev_opcode = prev_insn & 127;

                if ((((prev_insn >> 7) & 31) == 10) &
                    ((prev_opcode == 19) |        /* arith with immediate */
                     (prev_opcode == 51)))        /* arith with reg */
                {
                    code_pos = code_pos - 4;
                    emit32((prev_insn & 4294963327) | (reg_for_var << 7));
                        /* FFFFF07F */
                }
                else {
                    emit32((insn & 32505856) | (reg_for_var << 7) | 16435);
                        /*       & 0x01F0'0000                    | 0x4033 */
                        /* XOR REG[ofs], X0, REG */
                }
            }
            else {
                /* load */
                unsigned reg_for_var = (insn >> 22) + max_reg_pos;
                unsigned next_insn = get_32bit(buf + i + 4);
                unsigned next_opcode = next_insn & 127;
                unsigned rd = (insn >> 7) & 31;

                if ((next_insn == 11862051) |   /* 00B50023  SB A1,0(A0) */
                    (next_opcode == 51) |       /* arith with reg */
                    (next_opcode == 99))        /* branch */
                {
                    emit32((next_insn & 4262461439) | (reg_for_var << 20));
                        /*            & 0xFE0FFFFF */
                    i = i + 4;
                }
                else if ((next_opcode == 19) & 
                         (((next_insn >> 15) & 31) == rd)) /* 1st.rd == 2nd.rs1 */
                {
                    /* arith with immediate */
                    emit32((next_insn & 4293951487) | (reg_for_var << 15));
                        /*            & 0xFFF07FFF */
                    i = i + 4;
                }
                else {
                    emit32((insn & 3968) | (reg_for_var << 15) | 16435);
                        /*       & 0x0F80                      | 0x4033 */
                        /* XOR REG, REG[ofs], X0 */
                }
            }
        }
        else {
            emit32(insn);
        }
        i = i + 4;
    }
}

void walk_through(unsigned start_pos, unsigned end_pos)
{
    unsigned branch_pos = start_pos;

    /* search next branch */
    while (branch_pos < end_pos) {
        unsigned insn = get_32bit(buf + branch_pos);
        branch_pos = branch_pos + 4;
        if ((insn & 127) == 99) { /* & 0x7f) == 0x63  branch*/
            unsigned branch_target =  ((insn & 128) << 4) +
                                      ((insn >> 20) & 2016) +
                                      ((insn >> 7) & 28) +
                                        branch_pos - 4;
            insn = get_32bit(buf + branch_target - 4);

            if ((insn & 4095) == 111) { /* JAL X0, */
                unsigned jump_target = extract_jal_disp(insn) + branch_target - 4;

                if (insn >> 31) { /* jump backward => while loop */
                    refactor(start_pos, jump_target);
                    unsigned new_loop_pos = code_pos;
                    refactor(jump_target, branch_pos);
                    unsigned new_exit_pos = code_pos - 4;
                    walk_through(branch_pos, branch_target - 4);
                    emit_jump_and_fix_branch_here(new_loop_pos, new_exit_pos);
                    branch_pos = branch_target;
                }
                else { /* jump forward => else branch */
                    refactor(start_pos, branch_pos);
                    unsigned new_branch_pos = code_pos - 4;
                    walk_through(branch_pos, branch_target - 4);
                    unsigned new_not_else_pos = code_pos;
                    emit_jump_and_fix_branch_here(0, new_branch_pos);
                    walk_through(branch_target, jump_target);
                    emit_fix_jump_here(new_not_else_pos);
                    branch_pos = jump_target;
                }
            }
            else {
                refactor(start_pos, branch_pos);
                unsigned new_branch_pos = code_pos - 4;
                walk_through(branch_pos, branch_target);
                emit_fix_branch_here(new_branch_pos);
                branch_pos = branch_target;
            }
            start_pos = branch_pos;
        }
    }
    refactor(start_pos, end_pos);
}

void emit_func_end()
{
    emit_return();

    if ((num_calls == 0) & (max_reg_pos + num_locals < 32)) {
        unsigned function_end_pos = code_pos;
        code_pos = function_start_pos;
        walk_through(function_start_pos + 8, function_end_pos);
    }
    else {
        set_32bit(buf + function_start_pos, 4290838803 - (max_locals << 22));
            /* FFC10113  ADD SP, SP, -4*(max_locals+1) */

        unsigned insn = (max_locals << 22) + 4260115;
            /* 00010113  ADD SP, SP, 4*(max_locals+1) */
        unsigned next = addsp_list;
        while (next) {
            char *p = buf + next;
            next = get_32bit(p);
            set_32bit(p, insn);
        }
    }
}

unsigned emit_scope_begin()
{
    num_scope = num_scope + 1;
    return num_locals;
}

void emit_scope_end(unsigned save)
{
    num_locals = save;
    num_scope = num_scope - 1;
}

unsigned emit_begin()
{
    code_pos = 0;
    num_globals = 0;
    last_branch_target = 0;
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
        /* 00000197  AUIPC GP, hi(addr) */
    set_32bit(buf + 88, (addr << 20) + 98707);
        /* 00018193  ADDI GP, GP, lo(addr) */
    unsigned i = 0;
    while (i < num_globals) {
        emit32(0);
        i = i + 1;
    }

    set_32bit(buf + 68, code_pos);
    set_32bit(buf + 72, code_pos);
    return code_pos;
}
