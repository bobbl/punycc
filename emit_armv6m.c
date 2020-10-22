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

void emit32(unsigned n)
{
    emit(n);
    emit(n >> 8);
    emit(n >> 16);
    emit(n >> 24);
}

void emit_push()
{
    emit(1); emit(180);                         /* 01 B4   push {r0} */
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
        emit(170);
        emit(170);
    }
    if (code_pos > immpos_base + 1020) exit(200);
        /* too late to empty imm pool: imm pool to far away */

    unsigned delta = 255 - ((code_pos - immpos_base) >> 2);

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
        buf[pos] = buf[pos] - delta;
        i = i + 2;
    }

    immpos_pos = 0;
    immpool_pos = 0;
}

unsigned emit_jump(unsigned destination)
{
    unsigned disp = (destination - code_pos - 4) >> 1;
    emit(disp);
    emit(((disp >> 8) & 7) + 224);
    unsigned r = code_pos - 2;

/* Not so simple: first branch must jump over pool
    if (immpool_pos) empty_immpool();
*/

    return r;
}


void ofs_from_immpool()
{
    if (immpos_pos == 0) {
        /* start collection for a new pool */
        immpos_base = code_pos & 4294967292; /* ~3 */
    }
    unsigned pos = code_pos - immpos_base;

    emit(((immpos_base + immpool_pos - (code_pos & 4294967292/*~3*/))
         >> 2) + 254);

    immpos[immpos_pos] = pos;
    immpos[immpos_pos + 1] = pos >> 8;
    immpos_pos = immpos_pos + 2;
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

void emit_imm(unsigned reg, unsigned imm)
{
    if (imm < 256) {
        emit(imm); emit(32 + reg);              /* ?? 20   MOVS reg, imm */
    }
    else {
        /* get index in pool */
        ofs_from_immpool();
        emit(72 + reg);                         /* ?? 48   LDR reg, [PC, #?] */

        /* write to pool */
        set_32bit(immpool + immpool_pos, imm);
        immpool_pos = immpool_pos + 4;
    }
    check_immpool();
}

void emit_number(unsigned x)
{
    emit_imm(0, x);
}

void emit_string(unsigned len, char *s)
{
    /* get index in pool */
    ofs_from_immpool();
    emit(160);                                  /* ?? A0   ADD R0, [PC, #?] */

    /* write to pool */
    unsigned i = 0;
    while (i < len) {
        immpool[immpool_pos + i] = s[i];
        i = i + 1;
    }
    len = (len + 4) & 4294967292; /*~3*/
    while (i < len) {
        immpool[immpool_pos + i] = 0;
        i = i + 1;
    }
    immpool_pos = immpool_pos + len;

    check_immpool();
}

void emit_store(unsigned global, unsigned ofs)
{
    if (global) {
        emit_imm(1, ofs);
        emit(8); emit(96);                      /* 08 60   STR R0, [R1] */
    }
    else {
        ofs = stack_pos - ofs + num_params + 1;
        emit(ofs); emit(144);                   /* ?? 90   STR R0, [SP, #ofs] */
    }
}

void emit_load(unsigned global, unsigned ofs)
{
    if (global) {
        emit_imm(0, ofs);
        emit(0); emit(104);                     /* 08 68   LDR R0, [R0] */
    }
    else {
        ofs = stack_pos - ofs + num_params + 1;
        emit(ofs); emit(152);                   /* ?? 98   LDR R0, [SP, #ofs] */
    }
}

void emit_index(unsigned global, unsigned ofs)
{
    if (global) {
        emit_imm(1, ofs);
        emit(9); emit(104);                     /* 09 68   LDR R1, [R1] */
    }
    else {
        ofs = stack_pos - ofs + num_params + 1;
        emit(ofs); emit(153);                   /* ?? 99   LDR R1, [SP, #ofs] */
    }
    emit(8); emit(68);                          /* 08 44   ADD R0, R1 */
}

void emit_pop_store_array()
{
    stack_pos = stack_pos - 1;
    emit(2); emit(188);                         /* 02 BC   POP {R1} */
    emit(8); emit(112);                         /* 08 70   STRB R0, [R1] */
}

void emit_load_array()
{
    emit(0); emit(120);                         /* 00 78   LDRB R0, [R0] */
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

    return
        3489722368 +
        ((disp >> 12) & 1023) +
        (s << 10) +

        (((disp >> 1) & 2047) << 16) +
        ((((disp >> 23) & 1) ^ s ^ 1) << 29) +
        ((((disp >> 22) & 1) ^ s ^ 1) << 27);
}

void emit_call(unsigned defined, unsigned sym, unsigned ofs, unsigned pop,
               unsigned save)
{
    if (defined) {                              /* defined function */
        unsigned disp = ofs - code_pos - 4;
        emit32(insn_bl(disp));
    }
    else {                                      /* undefined function */
        set_32bit(buf + sym, code_pos);
        emit32(ofs);
    }
    emit_pop(pop);
    stack_pos = stack_pos - pop;
}

void emit_operation(unsigned op)
{
    stack_pos = stack_pos - 1;
    emit(2); emit(188);                         /* 02 BC   POP {R1} */

    if (op == 1) {
        emit(129); emit(64);                    /* 81 40   LSLS R1, R0 */
        emit(8); emit(70);                      /* 08 46   MOV R0, R1 */
    }
    else if (op == 2) {
        emit(193); emit(64);                    /* C1 40   LSRS R1, R0 */
        emit(8); emit(70);                      /* 08 46   MOV R0, R1 */
    }
    else if (op == 3) {
        emit(8); emit(26);                      /* 08 1A   SUB R0, R1, R0 */
    }
    else if (op == 4) {
        emit(8); emit(67);                      /* 08 43   ORRS R0, R1 */
    }
    else if (op == 5) {
        emit(72); emit(64);                     /* 48 40   EORS R0, R1 */
    }
    else if (op == 6) {
        emit(8); emit(68);                      /* 08 44   ADD R0, R1 */
    }
    else if (op == 7) {
        emit(8); emit(64);                      /* 08 40   AND R0, R1 */
    }
    else if (op == 8) {
        emit(72); emit(67);                     /* 48 43   MULS R0, R1 */
    }
    /* TODO: div and rem */
}

void emit_comp(unsigned op)
{
    stack_pos = stack_pos - 1;
    emit(2); emit(188);                         /* 02 BC   POP {R1} */

    if (op == 16) {                             /* == */
        emit(64); emit(26);                     /* 40 1A   SUBS R0, R0, R1 */
        emit(65); emit(66);                     /* 41 42   RSBS R1, R0, #0 */
        emit(72); emit(65);                     /* 48 41   ADCS R0, R1 */
    }
    else if (op == 17) {                        /* != */
        emit(64); emit(26);                     /* 40 1A   SUBS R0, R0, R1 */
        emit(65); emit(66);                     /* 41 42   RSBS R1, R0, #0 */
        emit(136); emit(65);                    /* 88 41   SBCS R0, R1 */
    }
    else if (op == 18) {                        /* < */
        emit_multi(6, "\x81\x42\x80\x41\x40\x42");
            /* 81 42   CMP R1, R0 */
            /* 80 41   SBCS R0, R0 */
            /* 40 42   RSBS R0, R0, #0 */
    }
    else if (op == 21) {                        /* >= */
        emit(0); emit(34);                      /* 00 22   MOVS R2, #0 */
        emit(136); emit(66);                    /* 88 42   CMP R0, R1 */
        emit(82); emit(65);                     /* 52 41   ADCS R2, R2 */
        emit(16); emit(0);                      /* 10 00   MOVS R0, R2 */
    }
    else if (op == 20) {                        /* > */
        emit_multi(6, "\x88\x42\x80\x41\x40\x42");
            /* 88 42   CMP R0, R1 */
            /* 80 41   SBCS R0, R0 */
            /* 40 42   RSBS R0, R0, #0 */
    }
    else /*if (op == 21)*/ {                    /* <= */
        emit(0); emit(34);                      /* 00 22   MOVS R2, #0 */
        emit(129); emit(66);                    /* 81 42   CMP R1, R0 */
        emit(82); emit(65);                     /* 52 41   ADCS R2, R2 */
        emit(16); emit(0);                      /* 10 00   MOVS R0, R2 */
    }
}

unsigned emit_branch_if0()
{
    emit(0); emit(40);                          /* 00 28   CMP R0, #0 */
    emit(0); emit(209);                         /* 00 D1   BNE ... */
    emit_jump(0 /* dont care */);
    return code_pos - 2;
}

unsigned emit_branch_if_cond(unsigned op)
{
/* optimization, can be replaced by
    emit_comp(t);
    return emit_branch_if0();
*/
    stack_pos = stack_pos - 1;
    emit(2); emit(188);                         /* 02 BC   POP {R1} */
    emit(129); emit(66);                        /* 81 42   CMP R1, R0 */

    char *bcc = "\xd1\xd0\xd2\xd3\xd9\xd8";
    emit(0);
    emit(bcc[(op ^ 1) - 16]);
    emit_jump(0 /* dont care */);
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
/*
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
    emit(0); emit(181);                         /* 00 B5   PUSH {LR} */
    num_params = n;
}

void emit_return()
{
    emit_pop(stack_pos);
    emit(0); emit(189);                         /* 00 BD   POP {PC} */
    if (immpool_pos) empty_immpool();
}

unsigned emit_local_var()
{
    emit_push();
    return stack_pos + num_params + 1;
}

unsigned emit_global_var()
{
    if (code_pos & 2) {
        emit(0); emit(0);
    }
    emit32(0);
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
