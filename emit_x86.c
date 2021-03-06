/**********************************************************************
 * Code Generation for x86 (32 bit)
 **********************************************************************/

/* constants */
unsigned buf_size;

/* global variables */
unsigned char *buf;
unsigned code_pos;
unsigned syms_head;
unsigned stack_pos;
unsigned num_params;

void set_32bit(unsigned char *p, unsigned x);
unsigned get_32bit(unsigned char *p);


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
    emit(80);                                   /* 50        push eax */
    stack_pos = stack_pos + 1;
}

void emit_pop(unsigned n)
{
    if (n == 0) return;
    if (n == 1) {
        emit(89);                               /* 59        pop ecx */
    }
    else if (n == 2) {
        emit(89);                               /* 59        pop ecx */
        emit(89);                               /* 59        pop ecx */
    }
    else if (n < 32) {
        emit(131); emit(196);                   /* 83 C4     add esp, imm7 */
        emit(n << 2);
    }
    else {
        emit(129); emit(196);                   /* 81 C4     add esp, imm31 */
        emit32(n << 2);
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

void emit_number(unsigned x)
{
    emit(184);                                  /* B8        mov eax, x */
    emit32(x);
}

void emit_string(unsigned len, char *s)
{
    emit(232);                                  /* E8        call $+i */
    emit32(len + 1);
    emit_multi(len, s);
    emit(0);                                    /* 00        end of string mark */
    emit(88);                                   /* 58        pop eax */
}

void access_var(unsigned which, unsigned ofs, unsigned global, unsigned local)
{
    if (which) {                               /* global variable */
        emit(global);
        if (global == local) emit(5); /* other than mov requires extra byte */
        emit32(ofs);
    }
    else {                                      /* local variable */
        emit(local);
        ofs = (stack_pos - ofs + num_params + 1) << 2;

        /* code optimization for 8-bit immediates */
        if (ofs < 128) {
            emit(68); emit(36);                 /* ?? 44 24  ??? eax, [esp+imm8] */
            emit(ofs);
        }
        else {
            emit(132); emit(36);                /* ?? 84 24  ??? eax, [esp+ofs] */
            emit32(ofs);
        }
    }
}

void emit_store(unsigned global, unsigned ofs)
{
    access_var(global, ofs, 163, 137);
    /* global           A3 -- -- -- --          mov [imm32], eax
       local short      89 44 24 --             mov [esp+imm7], eax
       local long       89 84 24 -- -- -- --    mov [esp+imm31], eax */
}

void emit_load(unsigned global, unsigned ofs)
{
    access_var(global, ofs, 161, 139);
    /* global           A1 -- -- -- --          mov eax, [imm32]
       local short      8B 44 24 --             mov eax, [esp+imm7]
       local long       8B 84 24 -- -- -- --    mov eax, [esp+imm31] */
}

void emit_index(unsigned global, unsigned ofs)
{
    access_var(global, ofs, 3, 3);
    /* global           03 05 -- -- -- --       add eax, [imm32]
       local short      03 44 24 --             add eax, [esp+imm7]
       local long       03 84 24 -- -- -- --    add eax, [esp+imm31] */
}

void emit_pop_store_array()
{
    stack_pos = stack_pos - 1;
    emit(89);                                   /* 59        pop ecx */
    emit(136); emit(1);                         /* 88 01     mov [ecx], al */
}

void emit_load_array()
{
    emit(15); emit(182); emit(0);               /* 0F B6 00  movzb eax, [eax] */
}

unsigned emit_pre_call()
{
    return 0;
}

void emit_arg(unsigned i)
{
    emit_push();
}

void emit_call(unsigned defined, unsigned sym, unsigned ofs, unsigned pop,
               unsigned save)
{
    emit(232);                                  /* E8        call */
    if (defined) {                              /* defined function */
        emit32(ofs - code_pos - 4);
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

    /* code optimization for constant immediates */
    if ((buf[code_pos - 6] == 80) & (buf[code_pos-5] == 184)) {
        /* push eax
           mov eax, imm    -->  mov ecx, imm
           pop ecx
           xchg eax, ecx */
        code_pos = code_pos - 6;
        emit(185);                              /* B9       mov ecx, imm */
        emit_multi(4, (char *)buf + code_pos + 1);
    }
    else {
        emit(89);                               /* 59        pop ecx */
        if (op <= 3) emit(145);                 /* 91        xchg eax, ecx */
        if (op >= 9) emit(145);                 /* 91        xchg eax, ecx */
    }

    if (op >= 9) {
        emit32(4059550257);                     /* 31 D2     xor edx, edx */
                                                /* F7 F1     div ecx */
        if (op >= 10) emit(146);                /* 92        xchg eax, edx */
    }
    else {
        /*                <<  >>  -   |   ^   +   &   *   */
        char *code2 = " \xd3\xd3\x29\x09\x31\x01\x21\xf7";
        char *code3 = " \xe0\xe8\xc8\xc8\xc8\xc8\xc8\xe9";

        emit(code2[op]);
        emit(code3[op]);
    }
}

void emit_comp(unsigned t)
{
    char *setcc = "\x94\x95\x92\x93\x97\x96";
    stack_pos = stack_pos - 1;
    emit32(264321369);                          /* 59        pop ecx */
                                                /* 39 C1     cmp ecx, eax */
    emit(setcc[t - 16]);                          /* 0F .. C0  setCC al */
    emit32(3233157056);                         /* 0F B6 C0  movzx eax, al */
}

unsigned emit_branch_if0()
{
    emit32(2215624837);                         /* 85 C0     test eax, eax */
    emit32(0);                                  /* 0F 84     jz ... */
    return code_pos - 4;
}

unsigned emit_branch_if_cond(unsigned t)
{
/* optimization, can be replaced by
    emit_comp(t);
    return emit_branch_if0();
*/
    char *jcc = "\x85\x84\x83\x82\x86\x87";
    stack_pos = stack_pos - 1;
    emit32(264321369);                          /* 59        pop ecx */
                                                /* 39 C1     cmp ecx, eax */
    emit(jcc[t - 16]);                      /* 0F ..     jCC ... */
    emit32(0);
    return code_pos - 4;
}

void emit_fix_branch_here(unsigned insn_pos)
{
    set_32bit(buf + insn_pos, code_pos - insn_pos - 4);
}

void emit_fix_jump_here(unsigned insn_pos)
{
    emit_fix_branch_here(insn_pos);
}

unsigned emit_fix_call_here(unsigned pos)
{
    unsigned next = get_32bit(buf + pos);
    set_32bit(buf + pos, code_pos - pos - 4);
    return next;
}

unsigned emit_jump(unsigned destination)
{
    emit(233);                                  /* E9        jmp */
    emit32(destination - code_pos - 4);
    return code_pos - 4;
}

void emit_enter(unsigned n)
{
    num_params = n;
}

void emit_return()
{
    emit_pop(stack_pos);
    emit(195);                                  /* C3        ret */
}

unsigned emit_local_var()
{
    emit_push();
    return stack_pos + num_params + 1;
}

unsigned emit_global_var()
{
    emit32(0);
    return code_pos + 134512636; /* 0x08048000 - 4 */
        /* global variables need the code offset */
}

unsigned emit_begin()
{
    stack_pos = 0;
    emit_multi(95, "\x7f\x45\x4c\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x03\x00\x01\x00\x00\x00\x54\x80\x04\x08\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x34\x00\x20\x00\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x80\x04\x08\x00\x80\x04\x08........\x07\x00\x00\x00\x00\x10\x00\x00\xe8\x00\x00\x00\x00\x93\x31\xc0\x40\xcd\x80");
/*
elf_header:
    0000 7f 45 4c 46    e_ident         0x7F, "ELF"
    0004 01 01 01 00                    1, 1, 1, 0
    0008 00 00 00 00                    0, 0, 0, 0
    000C 00 00 00 00                    0, 0, 0, 0
    0010 02 00          e_type          2 (executable)
    0012 03 00          e_machine       3 (Intel 80386)
    0014 01 00 00 00    e_version       1
    0018 54 80 04 08    e_entry         0x08048000 + _start
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
    003C 00 80 04 08    p_vaddr         0x08048000 (default)
    0040 00 80 04 08    p_paddr         0x08048000 (default)
    0044 ?? ?? ?? ??    p_filesz
    0048 ?? ?? ?? ??    p_memsz
    004C 07 00 00 00    p_flags         7 (read, write, execute)
    0050 00 10 00 00    p_align         0x1000 (4 KiByte)

_start:
    0054 E8 ?? ?? ?? ?? call main
    0059 93             xchg eax, ebx
    005A 31 C0          xor eax, eax
    005C 40             inc eax
    005D CD 80          int 128
*/

    return 85;
        /* return the address of the call to main() as a forward reference */
}

void emit_end()
{
    set_32bit(buf + 68, code_pos);
    set_32bit(buf + 72, code_pos);
}
