/**********************************************************************
 * Code Generation for ARM v6-M (Cortex-M0)
 **********************************************************************/

/* constants */
unsigned buf_size;

/* global variables */
unsigned char *buf;
unsigned code_pos;
unsigned syms_head;
unsigned stack_pos;
unsigned num_params;

unsigned char *immpos;
unsigned char *immpool;
unsigned immpos_base;
unsigned immpos_pos;
unsigned immpool_pos;

void set_32bit(unsigned char *p, unsigned x);
unsigned get_32bit(unsigned char *p);
void error(unsigned no);


void emit(unsigned b)
{
    buf[code_pos] = b;
    code_pos = code_pos + 1;
}

void emit_multi(unsigned n, char *s)
{
    unsigned i = 0;
    while (i < n) {
        emit(s[i]);
        i = i + 1;
    }
}

void emit16(unsigned n)
{
    emit(n);
    emit(n >> 8);
}

void emit32(unsigned n)
{
    emit(n);
    emit(n >> 8);
    emit(n >> 16);
    emit(n >> 24);
}

void emit_push()
{
    emit16(46081);                         /* 01 B4   push {r0} */
    stack_pos = stack_pos + 1;
}

void emit_pop(unsigned n)
{
    if (n) {
        emit(n); emit(176);                     /* nn B0   add sp, 4*n */
    }
}

unsigned emit_scope_begin()
{
    return stack_pos;
}

void emit_scope_end(unsigned save)
{
    emit_pop(stack_pos - save);
    stack_pos = save;
}

void empty_immpool()
{
    if (code_pos & 2) { /* align to 4 bytes */
        emit16(0);
    }
    if (code_pos > immpos_base + 1020) exit(200);
        /* too late to empty imm pool: imm pool to far away */

    unsigned code_base = code_pos >> 2;

    /* write pool data */
    unsigned i = 0;
    while (i < immpool_pos) {
        emit(immpool[i]);
        i = i + 1;
    }

    /* fix imm offsets */
    i = 0;
    while (i < immpos_pos) {
        unsigned pos = immpos_base + immpos[i] + (immpos[i + 1] << 8);
        buf[pos] = buf[pos] + code_base - (pos >> 2) - 1;
        i = i + 2;
    }

    immpos_pos = 0;
    immpool_pos = 0;
}

unsigned emit_jump(unsigned destination)
{
    emit16((((destination - code_pos - 4) >> 1) & 2047) + 57344);
    return code_pos - 2;
}

void immpool_ref(unsigned ip)
{
    if (immpos_pos == 0) {
        /* start collection for a new pool */
        immpos_base = code_pos & 4294967292/*~3*/;
    }
    unsigned cp = code_pos - immpos_base;
    immpos[immpos_pos] = cp;
    immpos[immpos_pos + 1] = cp >> 8;
    immpos_pos = immpos_pos + 2;
    emit(ip >> 2);
}

void check_immpool()
{
    if (immpool_pos) {
        if (code_pos >= (immpos_base + 980)) {
            /* 1024 - 980 = 44 => circa 20 instructions before the pool is too
               far away from the first reference */
            emit_jump((code_pos + immpool_pos + 4) & 4294967292 /*~3*/);
            empty_immpool();
        }
    }
}

unsigned immpool_lookup(unsigned imm)
{
    unsigned i = 0;
    while (i < immpool_pos) {
        unsigned v = get_32bit(immpool + i);
        if (v == imm) return i;
        i = i + 4;
    }

    /* not found => add to pool */
    set_32bit(immpool + i, imm);
    immpool_pos = i + 4;
    return i;
}

void e_imm(unsigned reg, unsigned imm)
{
    if (imm < 256) {
        emit(imm); emit(reg + 32);              /* ?? 20   MOVS reg, imm */
    }
    else {
        /* emit index in pool */
        immpool_ref(immpool_lookup(imm));
        emit(reg + 72);                         /* ?? 48   LDR reg, [PC, #?] */
    }
    check_immpool();
}

void emit_number(unsigned x)
{
    e_imm(0, x);
}

void emit_string(unsigned len, char *s)
{
    /* emit index in pool */
    immpool_ref(immpool_pos);
    emit(160);                                  /* ?? A0   ADD R0, [PC, #?] */

    /* write to pool */
    unsigned aligned = (len + 4) & 4294967292; /*~3*/
    unsigned i = 0;
    while (i < aligned) {
        unsigned b = 0;
        if (i < len) b = s[i];
        immpool[immpool_pos + i] = b;
        i = i + 1;
    }
    immpool_pos = immpool_pos + aligned;

    check_immpool();
}

void access_var(unsigned global, unsigned ofs, unsigned store, unsigned index)
{
    if (global) {
        e_imm(1, ofs);
        emit(8 + index); emit(104 - store);
    }
    else {
        emit(stack_pos - ofs + num_params + 1);
        emit(152 - store + index);
    }
}

void emit_store(unsigned global, unsigned ofs)
{
    access_var(global, ofs, 8, 0);
    /* global   -- -- 08 60     LDR R1, imm32 ; STR R0, [R1]
       local    -- 90           STR R0, [SP, #ofs] */
}

void emit_load(unsigned global, unsigned ofs)
{
    access_var(global, ofs, 0, 0);
    /* global   -- -- 08 68     LDR R1, imm32 ; LDR R0, [R1]
       local    -- 98           LDR R0, [SP, #ofs] */
}

void emit_index(unsigned global, unsigned ofs)
{
    access_var(global, ofs, 0, 1);
    /* global   -- -- 09 68     LDR R1, imm32 ; LDR R1, [R1]
       local    -- 99           LDR R1, [SP, #ofs] */
    emit16(17416);
    /*          08 44           ADD R0, R1 */
}

void emit_load_array()
{
    /* optimisation */
    if (buf[code_pos - 2] == 8)          /* 08 44   ADD R0, R1 */
        if (buf[code_pos - 1] == 68) {
            code_pos = code_pos - 2;
            emit16(23616);               /* 40 5C   LDRB R0, [R0, R1] */
            return;
    }

    emit16(30720);                       /* 00 78   LDRB R0, [R0] */
}

unsigned emit_pre_call()
{
    return 0;
}

void emit_arg(unsigned i)
{
    emit_push();
}

unsigned insn_bl(unsigned disp)
{
    unsigned s = (disp >> 24) & 1;

    return 4160811008
        + ((disp >> 12) & 1023)
        + (s << 10)
        + ((disp & 4095) << 15)
        - ((((disp >> 23) & 1) ^ s) << 29)
        - ((((disp >> 22) & 1) ^ s) << 27);
}

void emit_call(unsigned defined, unsigned sym, unsigned ofs, unsigned pop,
               unsigned save)
{
    if (defined) {                              /* defined function */
        emit32(insn_bl(ofs - code_pos - 4));
    }
    else {                                      /* undefined function */
        set_32bit(buf + sym, code_pos);
        emit32(ofs);
    }
    emit_pop(pop);
    stack_pos = stack_pos - pop;
}

/* move the ldr pc instruction 2 bytes forward and adjust immpool reference */
void adjust_immpool_load()
{
    immpos_pos = immpos_pos - 2;
    immpool_ref(buf[code_pos + 2] << 2);
    emit(73);
}

/* optimise if there are only a few instructions between push and pop */
/* return 1 if R0 and R1 are swapped */
unsigned swap_or_pop()
{
    stack_pos = stack_pos - 1;

    unsigned b2 = buf[code_pos - 2];
    unsigned b1 = buf[code_pos - 1];

    if ((buf[code_pos - 4] == 1)        /* 01 B4   PUSH {R0} */
        & (buf[code_pos - 3] == 180))
    {
        if (b1 == 32) {                 /* ?? 20   MOVS R0, imm */
            code_pos = code_pos - 4;
            emit(b2);                   /* ?? 21   MOVS R1, #?? */
            emit(33);
            return 1;
        }
        if (b1 == 152) {                /* ?? 98   LDR R0, [SP, #??] */
            code_pos = code_pos - 4;
            emit(b2 - 1);               /* ?? 99   LDR R1, [SP, #??-1] */
            emit(153);
            return 1;
        }
        if (b1 == 72) {                 /* ?? 48   LDR R0, [PC, #??] */
            code_pos = code_pos - 4;
            adjust_immpool_load();
            return 1;
        }
    }
    if ((buf[code_pos - 6] == 1)        /* 01 B4   PUSH {R0} */
        & (buf[code_pos - 5] == 180)
        & (buf[code_pos - 3] == 73)
        & (b2 == 8)
        & (b1 == 104))
    {
        code_pos = code_pos - 6;
        adjust_immpool_load();
        emit16(26633);                  /* 09 68   LDR R1, [R1] */
        return 1;
    }
    emit16(48130);                      /* 02 BC   POP {R1} */
    return 0;
}

void emit_pop_store_array()
{
    if (swap_or_pop()) emit16(28673);   /* 01 70   STRB R1, [R0] */
                  else emit16(28680);   /* 08 70   STRB R0, [R1] */
}

void emit_operation(unsigned op)
{
    char *code1 = " \x81\xc1\x08\x08\x48\x08\x08\x48";
    char *code2 = " \x40\x40\x1a\x43\x40\x44\x40\x43";
    char *code3 = " \x88\xc8\x40\x08\x48\x08\x08\x48";

    if (swap_or_pop()) {
        if (buf[code_pos - 1] == 33) {
            /* optimization for 8 bit immm */
            if (op < 3) { /* LSL or LSR */
                code_pos = code_pos - 2;
                emit16(((buf[code_pos + 2] & 31) << 6)
                    + ((op - 1) << 11));    /* LSLS/LSRS R0, R0, (imm & 31) */
                return;
            }
            if (op == 3) {
                buf[code_pos - 1] = 56;         /* ?? 38   SUBS R0, imm */
                return;
            }
            if (op == 6) {
                buf[code_pos - 1] = 48;         /* ?? 30   ADDS R0. imm */
                return;
            }
        }
        /* emit alternate code for non-commutative ops */
        emit(code3[op]);
        emit(code2[op]);
            /* 88 40   LSLS R0, R1          <<  op=1 */
            /* C8 40   LSRS R0, R1          >>  op=2 */
            /* 40 1A   SUBS R0, R0, R1      -   op=3 */
            /* the remaining ops don't change because they are communtative
               (R0 and R1 are interchangeable */
    }
    else {
        emit(code1[op]);
        emit(code2[op]);
            /* 81 40   LSLS R1, R0          <<*/
            /* C1 40   LSRS R1, R0          >>*/
            /* 08 1A   SUBS R0, R1, R0      - */
            /* 08 43   ORRS R0, R1          | */
            /* 48 40   EORS R0, R1          ^ */
            /* 08 44   ADD R0, R1           + */
            /* 08 40   ANDS R0, R1          & */
            /* 48 43   MULS R0, R1          * */
            /* TODO: / and % */

        if (op <= 2) emit16(17928);             /* 08 46   MOV R0, R1 */
    }
}

void emit_comp(unsigned op)
{
    /* TODO: swap_or_pop() */
    stack_pos = stack_pos - 1;
    emit16(48130);                              /* 02 BC   POP {R1} */

    char *code = "\x40\x1a\x41\x42\x48\x41\x00\x00\x40\x1a\x41\x42\x88\x41\x00\x00\x81\x42\x80\x41\x40\x42\x00\x00\x00\x22\x81\x42\x52\x41\x10\x00\x88\x42\x80\x41\x40\x42\x00\x00\x00\x22\x88\x42\x52\x41\x10\x00";
    unsigned len = 6;
    if (op == 19) len = 8;
    if (op == 21) len = 8;
    emit_multi(len, code + (((op - 16) << 3)));

        /* == */
        /* 40 1A   SUBS R0, R0, R1 */
        /* 41 42   RSBS R1, R0, #0 */
        /* 48 41   ADCS R0, R1 */

        /* != */
        /* 40 1A   SUBS R0, R0, R1 */
        /* 41 42   RSBS R1, R0, #0 */
        /* 88 41   SBCS R0, R1 */

        /* < */
        /* 81 42   CMP R1, R0 */
        /* 80 41   SBCS R0, R0 */
        /* 40 42   RSBS R0, R0, #0 */

        /* >= */
        /* 00 22   MOVS R2, #0 */
        /* 81 42   CMP R1, R0 */
        /* 52 41   ADCS R2, R2 */
        /* 10 00   MOVS R0, R2 */

        /* > */
        /* 88 42   CMP R0, R1 */
        /* 80 41   SBCS R0, R0 */
        /* 40 42   RSBS R0, R0, #0 */

        /* <= */
        /* 00 22   MOVS R2, #0 */
        /* 88 42   CMP R0, R1 */
        /* 52 41   ADCS R2, R2 */
        /* 10 00   MOVS R0, R2 */
}

unsigned emit_branch_if0()
{
    emit_multi(6, "\x00\x28\x00\xd1\x00\xe0");
        /* 00 28   CMP R0, #0 */
        /* 00 D1   BNE $+4 */
        /* ?? E?   B ?          will be fixed later*/
    return code_pos - 2;
}

unsigned emit_branch_if_cond(unsigned op)
{
/* optimization, can be replaced by
    emit_comp(t);
    return emit_branch_if0();
*/
    if (swap_or_pop()) {
        if (buf[code_pos - 1] == 33) {
            buf[code_pos - 1] = 40;             /* ?? 28   CMP R0, #imm */
        }
        else emit16(17032);                     /* 88 42   CMP R0, R1 */
    }
    else emit16(17025);                         /* 81 42   CMP R1, R0 */
    emit(0);

    /* branch over next instruction which is the real jump */
    char *bcc = "\xd0\xd1\xd3\xd2\xd8\xd9";
    emit(bcc[op - 16]);
    /* pure arithmetic alternative:
        emit((((op >> 1) & 1) ^ op) + (op & 4) + 192);
    */
    emit16(0); /* don't care, will be fixed later to a jump */
    return code_pos - 2;
}

void emit_fix_jump_here(unsigned insn_pos)
{
    unsigned disp = code_pos - insn_pos - 4;
    buf[insn_pos] = disp >> 1;
    buf[insn_pos + 1] = ((disp >> 9) & 7) + 224;
}

void emit_fix_branch_here(unsigned insn_pos)
{
/* short branch:
    unsigned disp = code_pos - insn_pos - 4;
    if ((disp + 256) >= 512) error (110);
    buf[insn_pos] = disp >> 1;
*/
    emit_fix_jump_here(insn_pos);
}

unsigned emit_fix_call_here(unsigned pos)
{
    unsigned next = get_32bit(buf + pos);
    set_32bit(buf + pos, insn_bl(code_pos - pos - 4));
    return next;
}

void emit_enter(unsigned n)
{
    emit16(46336);                              /* 00 B5   PUSH {LR} */
    num_params = n;
}

void emit_return()
{
    emit_pop(stack_pos);
    emit16(48384);                              /* 00 BD   POP {PC} */
    if (immpool_pos) empty_immpool();
}

unsigned emit_local_var()
{
    emit_push();
    return stack_pos + num_params + 1;
}

unsigned emit_global_var()
{
    emit_multi(4 + (code_pos & 2), "\x00\x00\x00\x00\x00\x00");
    return code_pos + 65532; /* 0x10000 - 4 */
        /* global variables need the code offset */
}

unsigned emit_begin()
{
    immpos = malloc(1024);
    immpool = malloc(1024);
    immpos_pos = 0;
    immpos_base = 0;
    immpool_pos = 0;

    stack_pos = 0;
    emit_multi(92, "\x7f\x45\x4c\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x28\x00\x01\x00\x00\x00\x55\x00\x01\x00\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x34\x00\x20\x00\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x01\x00........\x07\x00\x00\x00\x00\x10\x00\x00\x00\x00\x00\x00\x01\x27\x00\xdf");
/* \x04\x00\x8f\xe2\x01\x00\x80\xe3\x30\xff\x2f\xe1
elf_header:
    0000 7f 45 4c 46    e_ident         0x7F, "ELF"
    0004 01 01 01 00                    1, 1, 1, 0
    0008 00 00 00 00                    0, 0, 0, 0
    000C 00 00 00 00                    0, 0, 0, 0
    0010 02 00          e_type          2 (executable)
    0012 28 00          e_machine       40 (ARM 32 bit)
    0014 01 00 00 00    e_version       1
    0018 55 00 01 00    e_entry         0x00010000 + _start + thumb
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
    0054 04 00 8F E2    ADD R0, PC, #4  (arm)
    0058 01 00 80 E3    ORR R0, #1      (arm)
    005C 30 FF 2F E1    BLX R0          (arm)
    0054 ?? ?? ?? ??    BL main         (thumb)
    0058 01 27          MOVS R7, #1     (thumb) exit
    005A 00 DF          SVC #0          (thumb)
*/

    return 84;
        /* return the address of the call to main() as a forward reference */
}

void emit_end()
{
    set_32bit(buf + 68, code_pos);
    set_32bit(buf + 72, code_pos);
}
