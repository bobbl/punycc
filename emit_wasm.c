/**********************************************************************
 * Code Generation for WebAssembly
 **********************************************************************/

/*
TODO
  * Differenciate between function that return something and not
  * Differenciate between ULEB and SLEB
  * Optimal backward writing of section length and vector length

*/


/* constants */
unsigned int buf_size; /* total size of all buffers */

/* global variables */
unsigned char *buf;
unsigned int code_pos;
unsigned int stack_pos;
unsigned int num_params;
unsigned int last_insn;
    /* Contains the last instruction, but only under special circumstances
       0  = unknown
       15 = return
       26 = drop */

unsigned int block_start_pos;
unsigned int func_no;    /* function number of next function */
unsigned int func_start_pos;
unsigned int num_locals;    /* number for next local variable */

unsigned int data_pos;      /* end of data section */
unsigned int data_ofs;      /* offset in data memory (not section!) */
unsigned int num_globals;   /* number for next global variable */


/* constants (only referred in comments)

    NUM_IMPORTED_FUNCTIONS = 3  number of functions imported from WASI
    LIB_DATA_SIZE = 16          number of bytes at the beginning of the memory
                                that are reserved for library functions
*/

/*

if:
  ...           CONDITION
  45            i32.eqz
  04 40         if
  ...             THEN BRANCH
  05            else
  ...             ELSE BRANCH
  0b            end



while:

  03 40         loop
  02 40           block
  ...               CONDITION
  0d 80 00          br_if 0 ;; -> end of block
  ...               LOOP BODY
  0c 01             br 1 ;; -> top of loop
  0b              end
  0b            end

*/





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


void emit(unsigned int b)
{
    buf[code_pos] = b;
    code_pos = code_pos + 1;
    last_insn = 0;
}


void emit_n(unsigned int n, char *s)
{
    unsigned int i = 0;
    while (i < n) {
        emit(s[i]);
        i = i + 1;
    }
}


void emit32(unsigned int n)
{
    code_pos = code_pos + 4;
    set_32bit(buf + code_pos - 4, n);
    last_insn = 0;
}

void emit_leb(unsigned int n)
{
    unsigned int negative = 0;
    if (n >> 31) { 
        /* large uint32 = negative int32, must be emitted as signed LEB */
        emit(n | 128);
        n = (n >> 7) & 33554431;
        negative = 112;
    }

    while (n > 63) {
        emit(n | 128);
        n = n >> 7;
    }
    emit(n | negative);
}


void set_leb_fix3(unsigned int i, unsigned int x)
{
    buf[i  ] = x       | 128;
    buf[i+1] = (x>>7)  | 128;
    buf[i+2] = (x>>14) & 127;
}


void emit_leb_fix3(unsigned int x)
{
    code_pos = code_pos + 3;
    set_leb_fix3(code_pos - 3, x);
}


void emit_push()
{
}


void emit_number(unsigned int x)
{
    emit(65);                                   /* 41   i32.const */
    emit_leb(x);
    stack_pos = stack_pos + 1;
}


void end_of_block()
{
    emit(92 + (block_start_pos == func_start_pos));
        /* 93 for code block with function head
           92 for code block after a data block */

    emit32(code_pos - block_start_pos);
}


void emit_string(unsigned int len, char *s)
{
    /* code to get address */
    emit_number(data_ofs);
    data_ofs = data_ofs + len + 1;

    /* interrupt code block with data block */
    end_of_block();

    emit_n(len, s);
    emit(0);
    emit(91); /* data block */
    emit32(len + 2);
    block_start_pos = code_pos;
}


void emit_store(unsigned int global, unsigned int ofs)
{
    if (global) {
        emit(36);                               /* 24           global.set */
    }
    else {
        ofs = ofs - 1;
        emit(33);                               /* 21           local.set */
    }
    emit_leb(ofs);
    stack_pos = stack_pos - 1;
}


void emit_load(unsigned int global, unsigned int ofs)
{
    if (global) {
        emit(35);                               /* 23           global.get */
    }
    else {
        ofs = ofs - 1;
        emit(32);                               /* 20           local.get */
    }
    emit_leb(ofs);
    stack_pos = stack_pos + 1;
}


void emit_index_push(unsigned int which, unsigned int ofs)
{
    emit_load(which, ofs);
    emit(106);                                  /* 6A           i32.add */
    stack_pos = stack_pos - 1;
}


void emit_pop_store_array()
{
    emit(58); emit(0); emit(0);                 /* 3A 00 00     i32.store8 */
    stack_pos = stack_pos - 2;
}


void emit_index_load_array(unsigned int which, unsigned int ofs)
{
    emit_index_push(which, ofs);
    emit(45); emit(0); emit(0);                 /* 2D 00 00     i32.load8_u */
}


void emit_operation(unsigned int op)
{
    /*               <<  >>  -   |   ^   +   &   *   /   %  */
    /* signed:     \x74\x75\x6b\x72\x73\x6a\x71\x6c\x6d\x6f */
    char *code = " \x74\x76\x6b\x72\x73\x6a\x71\x6c\x6e\x70";
    emit(code[op]);
    stack_pos = stack_pos - 1;
}


void emit_comp(unsigned int op)
{
    /*              ==  !=  <   >=  >   <=  */
    /* signed:    \x46\x47\x48\x4e\x4a\x4c  */
    char *code = "\x46\x47\x49\x4f\x4b\x4d";
    emit(code[op - 16]);
    stack_pos = stack_pos - 1;
}


unsigned int emit_pre_while()
{
    emit32(1073889283);                         /* 03 40        loop */
                                                /* 02 40        block */
    return 1;
        /* This value must only be different from 0.
           A value of 0 tells emit_jump_and_fix_branch_here() that it is
           called at the beginning of an else branch. Another value tells
           that it is at the end of a while loop. */
}


unsigned int emit_if(unsigned int condition)
{
    if (condition) {
        emit_comp(condition);
    }

    emit(4); emit(64);                          /* 04 40        if (no result) */
    emit(1);                                    /* 01           nop */
        /* nop is neccessary, because the 3 bytes will be replaced later by
                45     i32.eqz
                0D 00  br_if 0
            if this is emitted after a while condition. 
        */
    stack_pos = stack_pos - 1;
    return code_pos;
}


void emit_fix_branch_here(unsigned int insn_pos)
{
    /* end of then branch without else */
    emit(11);                                   /* 0B           end */
}


void emit_fix_jump_here(unsigned int insn_pos)
{
    /* end of else branch */
    emit(11);                                   /* 0B           end */
}


unsigned int emit_jump_and_fix_branch_here(unsigned int destination, unsigned int insn_pos)
{
    /* destination==0: beginning of else branch
       destination!=0: end of while loop */

    if (destination) {
        /* At the end of loop add: */
        emit32(185270540);                      /* 0C 01        br 1 */
                                                /* 0B           end */
                                                /* 0B           end */

        /* The code for the condition and for the loop body, is still separated
           by the bytecode for if (01 04 40). Fix it now. */
        buf[insn_pos-3] = 69;                   /* 45           i32.eqz */
        buf[insn_pos-2] = 13;                   /* 0D 00        br_if 0 */
        buf[insn_pos-1] = 0;                    /* jump to end of block */
    }
    else {
        emit(5);                                /* 05           else */
    }

    return 0; /* does not mind in any case */
}


unsigned int emit_pre_call()
{
    return 0;
}


void emit_arg(unsigned int i)
{
}


unsigned int emit_call(unsigned int ofs, unsigned int pop, unsigned int save)
{
    unsigned int r = code_pos;

    /* The code must be at least 4 bytes, because if the function is not yet
       defined, the 4 bytes are overwritten by a link. The link is part of the
       linked list of the calls to the not yet defined function. */
    emit(16);
    emit_leb_fix3(get_32bit(buf + ofs));

    stack_pos = stack_pos - pop + 1;
    return r;
}


unsigned int emit_fix_call(unsigned int from, unsigned int to)
{
    /* Called only at the beginning of the code generation of the function
       where `to` points to. Therefore func_no has the correct value.
       Otherwise the number must be read from the address `to` points at:
            f = get_32bits(buf + to) */
    return (((func_no & 2080768) << 10)
          | ((func_no & 16256) << 9)
          | ((func_no & 127) << 8)
          | 8421392);
/*
    return (((func_no & 0x1FC000) << 10)
          | ((func_no & 0x003F80) << 9)
          | ((func_no & 0x00007F) << 8)
          | 0x00808010);
*/
}


unsigned int emit_local_var(unsigned int init)
{
    num_locals = num_locals + 1;

    if (init) {
        emit_store(0, num_params + num_locals);
    }

    return num_params + num_locals; /* one higher than will be emitted */
}


unsigned int emit_global_var()
{
    num_globals = num_globals + 1;
    return num_globals - 1;
}


void emit_return()
{
    /* Optimization: if last instruction is a return, don't emit anything */
    if (last_insn != 15) {

        /* if nothing is on the stack push 0 to always return something */
        if (stack_pos == 0) {

            /* Optimization: if last instruction is a drop, just remove it */
            if (last_insn == 26) {
                code_pos = code_pos - 1;
            }
            else {
                emit(65); emit(42);             /* 41 2A        i32.const 42 */
            }
            stack_pos = 1;
        }

        emit(15);                               /* 0F           return */
        last_insn = 15;
        stack_pos = stack_pos - 1;
    }
}


unsigned int emit_binary_func(unsigned int n, char *s)
{
    func_no = func_no + 1;

    /* emit length of previous block
       better: do this when parsing the end of a function, but no emit_
               routine is called then so far */

    func_start_pos = code_pos;
    block_start_pos = code_pos;

    emit32(func_no);
    emit_n(n, s); /* first byte must be the number of parameters,
                     then code including return but no end */

    end_of_block();

    return func_start_pos;
}


unsigned int emit_func_begin(unsigned int n)
{
    func_no = func_no + 1;
    func_start_pos = code_pos;
    block_start_pos = code_pos;
    num_params = n;
    num_locals = 0;

    emit32(func_no);
    emit(n); /* number of parameters */

    /* vector length 0 encoded in 3 bytes
       will be overwritten by   01 <number of locals> 7E
       whenever a new local variable is declared */
    emit(128);
    emit(128);
    emit(0);
    return func_start_pos;
}


void emit_func_end()
{
    if (num_locals) {
        /* overwrite number of locals at the beginning of the code */
        buf[func_start_pos+5] = 1;          /* vector length 1 */
        buf[func_start_pos+6] = num_locals; /* number of local vars */
        buf[func_start_pos+7] = 127;        /* type: i32 */
    }

    emit_return();
    end_of_block();
}


unsigned int emit_scope_begin()
{
    return stack_pos;
}


void emit_scope_end(unsigned int save)
{
    unsigned int drop = stack_pos - save;

    while (drop != 0) {
        emit(26);                               /* 1A   drop */
        last_insn = 26;
        stack_pos = stack_pos - 1;
        drop = drop - 1;
    }
}


unsigned int emit_begin()
{
    code_pos            = 0;
    block_start_pos     = 0;
    func_no             = 2;    /* 2  = NUM_IMPORTED_FUNCTIONS - 1 */
    data_ofs            = 16;   /* 16 = LIB_DATA_SIZE */
        /* The first bytes of the memory are reserved for library functions */

    stack_pos = 0;

    /* Add _start function that has a forward reference to main() */
    emit_binary_func(7, "\xfe\x00\x00\x00\x00\x00\x1a");
    /* FE               trick: -2 parameters means functype 0 which is ()->()
       00               no locals
       00 00 00 00      placeholder for call to main()
       1A               drop
    */

    return code_pos - 10;
        /* return the address of the call to main() as a forward reference */
}


unsigned int emit_end()
{
    /* go backwards through the immediate code */
    unsigned int func_section_begin = buf_size - func_no + 2;
    unsigned int data_section_begin = func_section_begin - data_ofs + 16;
        /* 16 = LIB_DATA_SIZE */

    unsigned int func_section_pos = buf_size;
    unsigned int data_section_pos = func_section_begin;
    unsigned int code_section_pos = data_section_begin;


    unsigned int func_end_pos = code_section_pos;
    code_section_pos = code_section_pos - 1;
    buf[code_section_pos] = 11; /* end bytecode at end of function */

    unsigned int i = code_pos;
    while (i) {
        unsigned int len = get_32bit(buf + i - 4);
        unsigned int blocktype = buf[i-5];
        i = i - len - 4;
        len = len - 1;

        if (blocktype == 91) { /* data block, copy to data section */
            data_section_pos = data_section_pos - len;
            while (len) {
                len = len - 1;
                buf[data_section_pos + len] = buf[i + len];
            }
        }
        else { /* code block */
            code_section_pos = code_section_pos - len;
            while (len) {
                len = len - 1;
                buf[code_section_pos + len] = buf[i + len];
            }

            if (blocktype == 93) { /* convert function head */

                /* write type of function (~= #parameters) to func section */
                func_section_pos = func_section_pos - 1;
                buf[func_section_pos] = buf[code_section_pos + 4] + 2;

                /* Replace the 5 byte header (func_no and parameter count) by a 3-byte
                   LEB with the length of the code. */
                code_section_pos = code_section_pos + 2;
                set_leb_fix3(code_section_pos,
                    func_end_pos - code_section_pos - 3);
                func_end_pos = code_section_pos;
                code_section_pos = code_section_pos - 1;
                buf[code_section_pos] = 11; /* end bytecode at end of function */
            }
        }
    }
    code_section_pos = code_section_pos + 1; /* remove last end opcode */

    /* number of functions */
    code_section_pos = code_section_pos - 3;
    set_leb_fix3(code_section_pos, func_no - 2); /* 2 = NUM_IMPORTED_FUNCTIONS - 1 */

    /* forward generate final code */
    code_pos = 0;
    emit_n(163, "\x00\x61\x73\x6d\x01\x00\x00\x00\x01\x2f\x08\x60\x00\x00\x60\x01\x7f\x00\x60\x00\x01\x7f\x60\x01\x7f\x01\x7f\x60\x02\x7f\x7f\x01\x7f\x60\x03\x7f\x7f\x7f\x01\x7f\x60\x04\x7f\x7f\x7f\x7f\x01\x7f\x60\x05\x7f\x7f\x7f\x7f\x7f\x01\x7f\x02\x67\x03\x16wasi_snapshot_preview1\x07\x66\x64_read\x00\x06\x16wasi_snapshot_preview1\x08\x66\x64_write\x00\x06\x16wasi_snapshot_preview1\x09proc_exit\x00\x01\x03");
/*
    0000 00 61 73 6D                    0x00, "asm"
    0004 01 00 00 00                    version 1

    0008 01 2F 08                       type section: length 47, 8 entries
    000B 60 00 00                       functype 0: ()->()
    000E 60 01 7f 00                    functype 1: (i32)->()
    0012 60 00 01 7f                    functype 2: ()->(i32)
    0014 60 01 7f 01 7f                 functype 3: (i32)->(i32)
    001B 60 02 7f 7f 01 7f              functype 4: (i32,i32)->(i32)
    0021 60 03 7f 7f 7f 01 7f           functype 5: (i32,i32,i32)->(i32)
    0028 60 04 7f 7f 7f 7f 01 7f        functype 6: (i32,i32,i32,i32)->(i32)
    0030 60 05 7f 7f 7f 7f 7f 01 7f     functype 7: (i32,i32,i32,i32,i32)->(i32)

    0039 02 67                          import section: length 103
         03                             3 entries (3 = NUM_IMPORTED_FUNCTIONS)
         16 77 61 73 69 5f 73 6e
         61 70 73 68 6f 74 5f 70
         72 65 76 69 65 77 31           module="wasi_snapshot_preview1"
         07 66 64 5f 72 65 61 64        name="fd_read"
         00 06                          function with type 6=(i32,i32,i32,i32)->(i32)

    005D 16 77 61 73 69 5f 73 6e
         61 70 73 68 6f 74 5f 70
         72 65 76 69 65 77 31           module="wasi_snapshot_preview1"
         08 66 64 5f 77 72 69 74
         65                             name="fd_write"
         00 06                          function with type 6=(i32,i32,i32,i32)->(i32)

    007F 16 77 61 73 69 5f 73 6e
         61 70 73 68 6f 74 5f 70
         72 65 76 69 65 77 31           module="wasi_snapshot_preview1"
         09 70 72 6f 63 5f 65 78
         69 74                          name="proc_exit"
         00 01                          function with type 1=(i32)->()

    00A2 03                             function section
*/

    unsigned int len = func_no - 2; /* 2 = NUM_IMPORTED_FUNCTIONS - 1 */
    emit_leb(len+3); /* bytes */
    emit_leb_fix3(len); /*entries */
    emit_n(buf_size - func_section_begin, (char *)buf + func_section_begin);

    /* memory section */
    emit_n(6, "\x05\x03\x01\x00\x01\x06");

    /* global section */
    len = num_globals * 5 + 3;
    emit_leb(len);
    emit_leb_fix3(num_globals);
    i = num_globals;
    while (i != 0) {
        emit_n(5, "\x7f\x01\x41\x00\x0b");
        /* 7F           data type i32
           01           mutable variable, not a constant
           41 00        i32.const 0: init to 0
           0B           end (of initialisation expression)
         */
        i = i - 1;
    }

    /* export section
        07              section id
        13              length 19 bytes
        02              2 entries
        06 "memory"
        02 00           memory 0
        06 "_start"
        00 03           function 3 that calls main()
     */
    emit_n(22, "\x07\x13\x02\x06memory\x02\x00\x06_start\x00\x03\x0a");

    /* code section */
    len = data_section_begin - code_section_pos;
    emit_leb(len);
    emit_n(len, (char *)buf + code_section_pos);

    /* data section 
        0B              section id
        <len>           length of the section
        01              1 entry
        00              active memory 0
        41              i32.const
        10              offset for const data (16 = LIB_DATA_SIZE)
        0B              end
    */
    len = func_section_begin - data_section_begin;
    emit(11);
    emit_leb(len+8);
    emit_n(5, "\x01\x00\x41\x10\x0b");
    emit_leb_fix3(len);
    emit_n(len, (char *)buf + data_section_begin);
    return code_pos;
}
