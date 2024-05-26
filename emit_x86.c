/**********************************************************************
 * Code Generation for x86 (32 bit)
 **********************************************************************
 * Useful links:
 * [x86 Online Assembler and Disassembler](https://defuse.ca/online-x86-assembler.htm)
 * [x86 Instruction Reference](https://www.felixcloutier.com/x86/)
 **********************************************************************/



/* constants */
unsigned buf_size;

/* global variables */
unsigned char *buf;
unsigned code_pos;
unsigned stack_pos;
unsigned num_params;
unsigned num_scope;
unsigned num_regvars;

unsigned last_insn;
unsigned last_load_code_pos;
unsigned last_load_which;
unsigned last_load_ofs;
unsigned last_imm;
unsigned need_return;

/* helper to write a 32 bit number to a char array */
void set_32bit(unsigned char *p, unsigned x)
{
    p[0] = x;
    p[1] = x >> 8;
    p[2] = x >> 16;
    p[3] = x >> 24;
}

/* helper to read 32 bit number from a char array */
unsigned get_32bit(unsigned char *p)
{
    return p[0] +
          (p[1] << 8) +
          (p[2] << 16) +
          (p[3] << 24);
}

void emit(unsigned b)
{
    buf[code_pos] = b;
    code_pos = code_pos + 1;
    last_insn = 0;
}

void emiti(char *s, unsigned i)
{
    emit(s[i]);
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
}

void emit_push()
{
    emit(80);                                   /* 50        push eax */
    stack_pos = stack_pos + 1;
}

void emit_pop(unsigned n)
{
    unsigned m = n << 2;

    if (m == 0) return;
    if (m == 4) {
        emit(89);                               /* 59        pop ecx */
    }
    else if (m == 8) {
        emit(89);                               /* 59        pop ecx */
        emit(89);                               /* 59        pop ecx */
    }
    else if (m < 128) {
        emit(131); emit(196);                   /* 83 C4     add esp, imm7 */
        emit(m);
    }
    else {
        emit(129); emit(196);                   /* 81 C4     add esp, imm32 */
        emit32(m);
    }
}

void emit_number(unsigned x)
{
    last_imm = x;
    if (x) {
        emit(184);                              /* B8        mov eax, imm32 */
        emit32(x);
        last_insn = 31;
    }
    else {
        emit(49); emit(192);                    /* 31 C0     xor eax, eax */
        last_insn = 33;
    }
}

void emit_string(unsigned len, char *s)
{
    emit(232);                                  /* E8        call $+i */
    emit32(len + 1);
    emit_binary_func(len + 1, s);               /* 00        end of string mark */
    emit(88);                                   /* 58        pop eax */
    last_insn = 35;
}


void access_var(unsigned which,
                unsigned ofs_p,
                unsigned modrm,
                unsigned opcode_global,
                unsigned opcode_local)
{
    unsigned ofs = ofs_p; /* smaller code */

    /* global variable */
    if (which) {
        emit(opcode_global);
        if (opcode_global == opcode_local)
             emit(modrm + 5); /* other than mov requires extra byte */
        emit32(ofs);
        return;
    }

    /* local variable in register */
    if (ofs >> 30) {
        if (opcode_local == 137) { /* special case: store regvar */
            emit(137);                          /* 89        mov e??, eax */
            emit((ofs & 7) + 196);
            return;
        }
        if (opcode_local == 255) { /* special case: push regvar */
            emit((ofs & 7) + 84);
            return;
        }
        emit(opcode_local - 2);
        emit(((ofs & 7) << 3) + 224);
        return;
    }

    /* local variable */
    emit(opcode_local);
    if (ofs <= num_params) {
        /* parameters: additional offset for saved registers */
        ofs = ofs - num_regvars;
    }
    ofs = (stack_pos - ofs + num_params + 1) << 2;

    /* local variable with 8-bit offset */
    if (ofs < 128) {
        emit(modrm + 68); emit(36);             /* ?? 44 24  ??? eax, [esp+ofs8] */
        emit(ofs);
        return;
    }

    emit(modrm + 132); emit(36);                /* ?? 84 24  ??? eax, [esp+ofs] */
    emit32(ofs);
}


void access_last_load(unsigned modrm, unsigned opcode)
{
    access_var(last_load_which, last_load_ofs, modrm, opcode, opcode);
}


void emit_store(unsigned which, unsigned ofs)
{
    access_var(which, ofs, 0, 163, 137);
    /* global           A3 -- -- -- --          mov [imm32], eax
       local register   89 --                   mov e??, eax            !special!
       local short      89 44 24 --             mov [esp+imm7], eax
       local long       89 84 24 -- -- -- --    mov [esp+imm31], eax */
}


void emit_load(unsigned which, unsigned ofs)
{
    last_load_code_pos = code_pos;
    last_load_which = which;
    last_load_ofs = ofs;

    access_var(which, ofs, 0, 161, 139);
    /* global           A1 -- -- -- --          mov eax, [imm32]
       local register   89 --                   mov eax, e??
       local short      8B 44 24 --             mov eax, [esp+imm7]
       local long       8B 84 24 -- -- -- --    mov eax, [esp+imm31] */
    last_insn = 32;
}


void emit_index_push(unsigned which, unsigned ofs)
{
    access_var(which, ofs, 0, 3, 3);
    /* global           03 05 -- -- -- --       add eax, [imm32]
       local reg        01 --                   add eax, e??
       local short      03 44 24 --             add eax, [esp+imm7]
       local long       03 84 24 -- -- -- --    add eax, [esp+imm31] */
    emit_push();
}


void emit_pop_store_array()
{
    stack_pos = stack_pos - 1;
    emit(89);                                   /* 59        pop ecx */
    emit(136); emit(1);                         /* 88 01     mov [ecx], al */
}

void emit_index_load_array(unsigned which, unsigned ofs)
{
    access_var(which, ofs, 0, 3, 3);
    /* global           03 05 -- -- -- --       add eax, [imm32]
       local reg        01 --                   add eax, e??
       local short      03 44 24 --             add eax, [esp+imm7]
       local long       03 84 24 -- -- -- --    add eax, [esp+imm31] */
    emit(15); emit(182); emit(0);               /* 0F B6 00  movzb eax, [eax] */
}


void emit_operation(unsigned op)
{
    stack_pos = stack_pos - 1;

    /* div or mod */
    if (op >= 9) {
        if (last_insn == 32) {
            code_pos = last_load_code_pos - 1;
            emit(49); emit(210);
            access_last_load(48, 247);          /* F7 30     div [] */
        }
        else {
            if (last_insn == 31) {
                code_pos = code_pos - 6;
                /* push eax
                   mov eax, imm    -->  mov ecx, imm
                   pop ecx
                   xchg eax, ecx */
                emit(185);                      /* B9        mov ecx, imm32 */
                emit32(last_imm);
            }
            else {
                emit(89);                       /* 59        pop ecx */
                emit(145);                      /* 91        xchg eax, ecx */
            }
            emit32(4059550257);                 /* 31 D2     xor edx, edx */
                                                /* F7 F1     div ecx */
        }
        if (op >= 10) emit(146);                /* 92        xchg eax, edx */
        return;
    }

    /* code optimization for constant immediates */
    if (last_insn == 31) {
        code_pos = code_pos - 6;
        if (op < 3) {
            emit(193);                          /* C1 E0 ..  shl eax, imm8 */
            emit((op << 3) + 216);              /* C1 E8 ..  shr eax, imm8 */
            emit(last_imm);
            return;
        }
        if (last_imm == 1) {
            if (op == 3) {
                emit(72);                       /* 48        dec eax */
                return;
            }
            if (op == 6) {
                emit(64);                       /* 40        inc eax */
                return;
            }
        }

        /*          -   |   ^   +   &   *    */
        emiti("   \x2d\x0d\x35\x05\x25\x69", op);
        if (op == 8) emit(192);                 /* 69 C0 ..  imul eax, eax, imm32 */
        emit32(last_imm);
        return;
    }

    /* code optimization for single load */
    if (last_insn == 32) {
        code_pos = last_load_code_pos - 1;

            /*               <<  >>  -   |   ^   +   &   *      */
        char *op2code  = " \x8b\x8b\x2b\x0b\x33\x03\x23\xf7 \x08\x08\x00\x00\x00\x00\x00\x28";
            /* translate operation to x86 opcode
               1  <<  8B 08  mov ecx, []
               2  >>  8B 08  mov ecx, []
               3  -   2B 00  sub eax, []
               4  |   0B 00  or  eax, []
               5  ^   33 00  xor eax, []
               6  +   03 00  add eax, []
               7  &   23 00  and eax, []
               8  *   F7 28  imul eax, [] */
        access_last_load(op2code[op + 9], op2code[op]);

        if (op >= 3) return;
        /* For SHL and SHR only `mov ecx, []` was emitted. The remaining
           code is emitted at the end of this function. */
    }

    /* general case */
    else {
        emit(89);                               /* 59        pop ecx */
        if (op <= 3) emit(145);                 /* 91        xchg eax, ecx */
    }

    /*        <<  >>  -   |   ^   +   &   *   */
    emiti(" \xd3\xd3\x29\x09\x31\x01\x21\xf7", op);
    emiti(" \xe0\xe8\xc8\xc8\xc8\xc8\xc8\xe9", op);
}

void compare_with_imm()
{
    stack_pos = stack_pos - 1;

    /* code optimization for constant immediates */
    if (last_insn == 31) {
        code_pos = code_pos - 6;
        emit(61);                               /* 3D        cmp eax, imm32 */
        emit32(last_imm);
        emit(15);                               /* 0F */
        return;
    }

    /* compare with 0 (xor eax, eax) */
    if (last_insn == 33) {
        code_pos = code_pos - 3;
        emit(133); emit(192);                   /* 85 C0     test eax, eax */
        emit(15);                               /* 0F */
        return;
    }

    /* code optimization for single load */
    if (last_insn == 32) {
        code_pos = last_load_code_pos - 1;
        access_last_load(0, 59);
                                                /* 3B ...    cmp eax, []   */
        emit(15);                               /* 0F */
        return;
    }

    emit32(264321369);                          /* 59        pop ecx */
                                                /* 39 C1     cmp ecx, eax */
                                                /* 0F */
}

void emit_comp(unsigned condition)
{
    compare_with_imm();
    emiti("\x94\x95\x92\x93\x97\x96", condition - 16);
                                                /* 0F .. C0  setCC al */
    emit32(3233157056);                         /* 0F B6 C0  movzx eax, al */
}

unsigned emit_pre_while()
{
    return code_pos;
}

unsigned emit_if(unsigned condition)
{
    if (condition) {
        compare_with_imm();
        emiti("\x85\x84\x83\x82\x86\x87", condition - 16);
                                                /* 0F ..     jCC ... */
    }
    else {
        emit32(2215624837);                     /* 85 C0     test eax, eax */
    }                                           /* 0F 84     jz ... */
    emit32(0);
    last_insn = 34;
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

void imm8_if_possible(unsigned opcode, unsigned imm8, unsigned imm32)
{
    if (((imm8 + 128) >> 8) == 0) {
        emit(opcode + 2);
        emit(imm8);
    }
    else {
        emit(opcode);
        emit32(imm32);
    }
}

unsigned emit_jump_and_fix_branch_here(unsigned destination, unsigned insn_pos)
{
    unsigned disp = destination - code_pos - 2;
    if (destination == 0) {
        disp = 256; /* no short optimization if destination is unknown */
    }
    imm8_if_possible(233, disp, disp - 3);
       /* EB        jmp imm8 */
       /* E9        jmp */

    emit_fix_branch_here(insn_pos);
    return code_pos - 4; /* does not care for short jump */
}

unsigned emit_pre_call()
{
    return 0;
}

void emit_arg(unsigned i)
{
    /* push immediate */
    if (last_insn == 31) {
        code_pos = code_pos - 5;
        imm8_if_possible(104, last_imm, last_imm);
    }

    /* push 0 */
    else if (last_insn == 33) {
        code_pos = code_pos - 2;
        emit(106);
        emit(0);
    }

    /* push variable */
    else if (last_insn == 32) {
        code_pos = last_load_code_pos;
        access_last_load(48, 255);
    }

    /* push string */
    else if (last_insn == 35) {
        code_pos = code_pos - 1;
    }

    else {
        emit(80);                               /* 50        push eax */
    }
    stack_pos = stack_pos + 1;
}

unsigned emit_call(unsigned ofs, unsigned pop, unsigned save)
{
    emit(232);                                  /* E8           call */
    emit32(ofs - code_pos - 4);
    stack_pos = stack_pos - pop;
    return code_pos - 4;
}

unsigned emit_fix_call(unsigned from, unsigned to)
{
    return to - from - 4;
}

unsigned emit_local_var(unsigned init)
{
    unsigned r;

    if (num_scope == 1) {
        if (num_regvars < 3) {
            num_regvars = num_regvars + 1;
            emiti("\x53\x55\x56\x57", num_regvars);
            r = num_regvars + 1073741824;
            if (init) {
                emit_store(0, r);
            }
            return r;
        }
    }
    emit_arg(0); /* more optimized code than emit_push() */
    return stack_pos + num_params + 1;
}

unsigned emit_global_var()
{
    emit32(0);
    return code_pos + 134512636; /* 0x08048000 - 4 */
        /* global variables need the code offset */
}

void emit_return()
{
    /* If there is a return on the lowest scope and the last statement was
       neither if nor while, program execution will stop here.
       Therefore no implicit return at the end of the function is required. */
    if (num_scope == 1) {
        if (last_insn != 34) {
            need_return = 0;
        }
    }

    /* do NOT change stack_pos! */
    emit_pop(stack_pos);

    char *pop_code = "\x5f\x5e\x5d";
    emit_binary_func(num_regvars, pop_code + 3 - num_regvars);
                                                /* 5F           pop edi */
                                                /* 5E           pop esi */
                                                /* 5D           pop ebp */

    if (num_params) {
        emit(194);                              /* C2 nn nn     ret n */
        emit(num_params << 2); /* FIXME: num_params > 63 */
        emit(0);
        return;
    }
    emit(195);                                  /* C3   ret */
}

unsigned emit_func_begin(unsigned n)
{
    num_params = n;
    num_scope = 0;
    num_regvars = 0;
    stack_pos = 0;
    need_return = 1;
    return code_pos;
}

void emit_func_end()
{
    if (need_return) {
        emit_return();
    }
}

unsigned emit_scope_begin()
{
    num_scope = num_scope + 1;
    return stack_pos;
}

void emit_scope_end(unsigned save)
{
    emit_pop(stack_pos - save);
    stack_pos = save;
    num_scope = num_scope - 1;
}

unsigned emit_begin()
{
    code_pos = 0;
    emit_binary_func(95, "\x7f\x45\x4c\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x03\x00\x01\x00\x00\x00\x54\x80\x04\x08\x34\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x34\x00\x20\x00\x01\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x80\x04\x08\x00\x80\x04\x08........\x07\x00\x00\x00\x00\x10\x00\x00\xe8\x00\x00\x00\x00\x93\x31\xc0\x40\xcd\x80");
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

unsigned emit_end()
{
    set_32bit(buf + 68, code_pos);
    set_32bit(buf + 72, code_pos);
    return code_pos;
}
