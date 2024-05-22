/* Puny C Compiler
   Limited expressiveness unlimited portability
   Subset of C with a self-compiling compiler

Error return codes

    0100 buffer overflow
    0101 invalid character
    0102 specific token expected
    0103 identifier expected
    0104 unknown identifier
    0105 function redefined
    0106 type expected

Symbol Type

    71 global variable
    72 undefined function
    73 defined function
    74 local variable (or argument)

Token

  operations  conditions    other     reserved words     types
    00h EOF     10h ==      28h (       60h if          65h void
    01h <<      11h !=      29h )       61h else        66h char
    02h >>      12h <       2Ch ,       62h while       67h int
    03h -       13h >=      3Bh ;       63h return      68h unsigned
    04h |       14h >       3Dh =       64h _Pragma     69h long
    05h ^       15h <=      5Bh [
    06h +                   5Dh ]
    07h &                   5Eh number
    08h *                   5Fh identifier
    09h /                   7Bh {
    0Ah %                   7Dh }
 */






/**********************************************************************
 * Scanner
 **********************************************************************/

unsigned ch;
unsigned lineno;
unsigned token;
unsigned token_int;
unsigned token_size;
char *token_buf;
unsigned syms_head;

void itoa4(unsigned x)
{
    char *s = (char *)buf + syms_head - 16;
    x = x * 134218; /* x = x * (((1<<27)/1000) + 1) */
    s[0] = (x>>27) + '0';
    x = (x & 134217727) * 5;  /* 0x07FFFFFF */
    s[1] = (x>>26) + '0';
    x = (x & 67108863) * 5;   /* 0x03FFFFFF */
    s[2] = (x>>25) + '0';
    x = (x & 33554431) * 5;   /* 0x01FFFFFF */
    s[3] = (x>>24) + '0';
    write(2, s, 4);
}

void error(unsigned no)
{
    write(2, "Error ", 6);
    itoa4(no);
    write(2, " in line ", 9);
    itoa4(lineno);
    write(2, ".\x0d\x0a", 3);
    exit(no);
}

int token_cmp(char *s, unsigned n)
{
    unsigned i = 0;
    while (s[i] == token_buf[i]) {
        i = i + 1;
        if (i == n) return 1;
    }
    return 0;
}

unsigned next_char()
{
    ch = getchar();
    if (ch == 10) lineno = lineno + 1;
    return ch;
}

void store_char()
{
    token_buf[token_int] = ch;
    token_int = token_int + 1;
    if (token_int >= token_size)
        error(100); /* Error: buffer overflow */
    next_char();
}

void get_token()
{
    char  *first_map  = " !\x22  \x0a\x07\x27()\x08\x06,\x03 \x09^^^^^^^^^^ ;\x12=\x14  __________________________[ ]\x05_ __________________________{\x04}  ";

    token_size = syms_head - code_pos;
    if (token_size < 1024)
        error(100); /* Error: buffer overflow */
    token_size = token_size - 512;
    token_buf = (char *)buf + code_pos + 256;
    token_int = 0;
    token = 0;

    while ((ch == ' ') | (ch == 9) | (ch == 10) | (ch == '/'))
    {
        if (ch == '/') {
            if (next_char() != '*') {
                token = 9; /* 9 '/' */
                return;
            }
            while (next_char() != '/') {
                while (ch != '*') {
                    next_char();
                }
            }
        }
        next_char();
    }
    if (ch > 255) return;

    if ((ch - 32) >= 96) error(101); /* Error: invalid character */
    token = first_map[ch - 32]; /* mask signed char */
    if (token == ' ') error(101); /* Error: invalid character */

    if (ch == 39) {                  /* ' */
        token = '^'; /* number */
        token_int = next_char();
        while (next_char() != 39) {}
        next_char();
    }
    else if (token == '"') { /* string */
        next_char();
        while (ch != 34) {
            if (ch == 92) { /* \ */
                if (next_char() == 'x') {
                    unsigned h = next_char() - 48; /* 0 */
                    if (h > 9) h = h - 39;
                    unsigned l = next_char() - 48; /* 0 */
                    if (l > 9) l = l - 39;
                    ch = (h << 4) + l;
                }
            }
            store_char();
        }
        next_char();
    }
    else if (token == '^') { /* number */
        while ((ch >= '0') & (ch <= '9')) {
            token_int = (10 * token_int) + ch - '0';
            next_char();
        }
    }
    else if (token == '_') { /* identifier */

        /* store identifier in space between code and symbol table */
        unsigned j = 1;
        while (j) {
            store_char();
            if ((ch - 32) < 96)
                j = ((first_map[ch - 32] & 254) == 94); /* '^' or '_' */
            else j = 0;
        }
        token_buf[token_int] = 0;

        /* search keyword */
        char *keywords = "2if4else5while6return7_Pragma4void4char3int8unsigned4long0";
        unsigned i = 0;
        unsigned len = 2;
        token = 96;
        while (len) {
            if (len == token_int) {
                if (token_cmp(keywords + i + 1, token_int)) return;
            }
            token = token + 1;
            i = i + len + 1;
            len = keywords[i] - '0';
        }
        token = '_'; /* identifier */
    }
    else if (ch == '!') {
        if (next_char() == '=') {
            next_char();
            token = 17; /* 0x11 ne */
        }
        else error(101); /* Error: invalid character */
    }
    else if (ch == '<') {
        if (next_char() == '<') {
            next_char();
            token = 1; /* 1 << */
        }
        else if (ch == '=') {
            next_char();
            token = 21; /* 0x15 le */
        }
    }
    else if (ch == '=') {
        if (next_char() == '=') {
            next_char();
            token = 16; /* 0x10 eq */
        }
    }
    else if (ch == '>') {
        if (next_char() == '=') {
            next_char();
            token = 19; /* 0x13 setae */
        }
        else if (ch == '>') {
            next_char();
            token = 2; /* 2 >> */
        }
    }
    else {
        next_char();
    }
}





/**********************************************************************
 * Symbol Management
 **********************************************************************/


unsigned sym_lookup()
{
    if (token != '_')
        error(103); /* Error: identifier expected */

    unsigned s = syms_head;
    while (s < buf_size) {
        unsigned len = buf[s + 5];
        if (len == token_int) {
            if (token_cmp((char *)buf + s + 6, token_int)) return s;
        }
        s = s + len + 6;
    }
    return 0;
}

void sym_append(unsigned addr, unsigned type)
{
    syms_head = syms_head - token_int - 6;
    set_32bit(buf + syms_head, addr);
    buf[syms_head+4] = type;
    buf[syms_head+5] = token_int;

    /* copy backwards in case the token and the symbol table overlap */
    while (token_int != 0) {
        token_int = token_int - 1;
        buf[syms_head + 6 + token_int] = token_buf[token_int];
    }
    get_token();
}


void sym_fix(unsigned sym, unsigned func_pos)
{
    if (buf[sym+4] != 72)
        error(105); /* Error: function redefined */

    unsigned i = get_32bit(buf + sym);
    unsigned next;
    while (i) {
        next = get_32bit(buf + i);
        set_32bit(buf + i, emit_fix_call(i, func_pos));
        i = next;
    }
    set_32bit(buf + sym, func_pos);
    buf[sym + 4] = 73;
}





/**********************************************************************
 * Parser
 **********************************************************************/

unsigned accept(unsigned ch)
/* parameter named `ch` to check if name scopes work */
{
    if (token == ch) {
        get_token();
        return 1;
    }
    return 0;
}

void expect(unsigned t)
{
    if (accept(t) == 0)
        error(102); /* Error: specific token expected */
}

unsigned accept_type_id()
{
    if (token < 101) return 0;
    if (token > 105) return 0;
    get_token();
    return 1;
}

unsigned accept_type()
{
    if (accept_type_id()) {
        while (accept_type_id()) {}
        while (accept(8)) {} /* multiple * */
        return 1;
    }
    return 0;
}

void expect_type()
{
    if (accept_type() == 0)
        error(106); /* Error: type expected */
}

void parse_expression();


/* assign    = identifier , "=" , expression ;
 * array     = identifier , "[" , expression , "]" ;
 * aasign    = identifier , "[" , expression , "]" , "=" , expression ;
 * call      = identifier , "(" , params , ")" ;
 * lvalue    = identifier | assign | array | aassign | call ;
 * bracketed = "(" , expression , ")" ;
 * factor    = number | string | bracketed | lvalue ;
 */
void parse_factor()
{
    while (token == '(') { /* '(' */
        get_token();

        /* expression in brackets */
        if (accept_type() == 0) {
            parse_expression();
            expect(')');
            return;
        }

        /* ignore type cast */
        expect(')');
    }

    if (token == '^') { /* number */
        emit_number(token_int);
        get_token();
    }
    else if (token == '"') { /* string */
        set_32bit((unsigned char *)token_buf + token_int, 0);
            /* append 4 zero bytes to simplify alignment in the backends */
        emit_string(token_int, token_buf);
        get_token();
    }
    else { /* identifier */
        unsigned sym = sym_lookup();
        if (sym == 0)
            error(104); /* Error: unknown identifier */
        unsigned type = buf[sym + 4];
        unsigned ofs = get_32bit(buf + sym);
        get_token();

        if ((type == 72) | (type == 73)) { /* function */
            expect('(');
            unsigned argno = 0;
            unsigned save = emit_pre_call();
            if (accept(')') == 0) {
                parse_expression();
                emit_arg(0);
                argno = argno + 1;
                while (accept(',')) {
                    parse_expression();
                    emit_arg(argno);
                    argno = argno + 1;
                }
                expect(')');
            }
            unsigned link = emit_call(ofs, argno, save);
            if (type == 72) {
                set_32bit(buf + link, ofs); 
                    /* overwrite the call to an undefined address with a link
                       to the rest of the linked list of calls to this not yet
                       defined function */
                set_32bit(buf + sym, link);
            }
        }
        else if (accept('[')) { /* array */
            parse_expression();
            expect(']');
            if (accept('=')) {
                emit_index_push(type & 1, ofs);
                parse_expression();
                emit_pop_store_array();
            }
            else emit_index_load_array(type & 1, ofs);
        }
        else { /* variable */
            if (accept('=')) {
                parse_expression();
                emit_store(type & 1, ofs);
            }
            else emit_load(type & 1, ofs);
        }
    }
}

/* bin_op = "<<" | ">>" | "&" | "|" | "^" | "+" | "-" ;
 * operation = factor , { bin_op , factor } ;
 */
void parse_operation()
{
    parse_factor();
    while (token < 16) {
        emit_push();
        unsigned op = token;
        get_token();
        parse_factor();
        emit_operation(op);
    }
}

/* cmp_op     = "<" | "<=" | "==" | "!=" | ">=" | ">" ;
 * comparison = binary , [ shift_op , binary ] ;
 */
void parse_expression()
{
    parse_operation();
    if ((token & 240) == 16) { /* (token & 0xF0) == 0x10 */
        emit_push();
        unsigned op = token;
        get_token();
        parse_operation();
        emit_comp(op);
    }
}

/* condition = "(" , operation , [ cmp_op , operation ] , ")" ;
 */
unsigned parse_condition()
{
    unsigned cond = 0;
    expect('(');
    parse_operation();
    if ((token & 240) == 16) { /* (token & 0xF0) == 0x10 */
        emit_push();
        cond = token;
        get_token();
        parse_operation();
    }
    expect(')');
    return emit_if(cond);
}

/* block     = "{" , statement , "}" ;
 * if        = "if" , condition , statement , [ "else" , statement ] ;
 * while     = "while" , condition , statement ;
 * return    = "return" , [ expression ] ;
 * local     = type , identifier , [ "=" , expression ] , ";" ;
 * statement = block | if | while | return | local | expression ;
 */
void parse_statement()
{
    if (accept('{')) {
        unsigned h = syms_head;
        unsigned s = emit_scope_begin();
        while (accept('}') == 0) parse_statement();
        emit_scope_end(s);
        syms_head = h;
    }
    else if (accept(96)) { /* if */
        unsigned branch_pos = parse_condition();
        parse_statement();
        if (accept(97)) { /* else */
            unsigned not_else_pos = emit_jump_and_fix_branch_here(0, branch_pos);
            parse_statement();
            emit_fix_jump_here(not_else_pos);
        }
        else emit_fix_branch_here(branch_pos);
    }
    else if (accept(98)) { /* while */
        unsigned loop = emit_pre_while();
        unsigned exit_pos = parse_condition();
        parse_statement();
        emit_jump_and_fix_branch_here(loop, exit_pos);
    }
    else if (accept(99)) { /* return */
        if (accept(';') == 0) {
            parse_expression();
            expect(';');
        }
        emit_return();
    }
    else if (accept_type()) { /* variable declaration */
        sym_append(0 /* don't care */, 74); /* local variable */
        if (accept('=')) parse_expression();
        expect(';');
        set_32bit(buf + syms_head, emit_local_var());
    }
    else {
        unsigned s = emit_scope_begin();
        parse_expression();
        emit_scope_end(s);
        expect(';');
    }
}

/* params   = { type , [ identifier ] , [ "," ] } ;
 * body     = statement | ";" ;
 * function = type , identifier , "(" , params ,  ")" , body ;
 */
void parse_function(unsigned sym)
{
    expect('(');
    unsigned restore_head = syms_head;

    unsigned n = 0;
    while (accept(')') == 0) {
        n = n + 1;
        expect_type();
        if (token == '_') { /* identifier */
            sym_append(n, 74);
        }
        accept(','); /* ignore trailing comma */
    }

    if (accept(100)) { /* _Pragma */
        expect('(');
        while (token != ')') {
            if (token == '"') {
                if (token_cmp("PunyC emit ", 11)) {
                    sym_fix(sym, emit_binary_func(token_int-11, token_buf+11));
                }
            }
            get_token();
        }
        get_token();
        expect(';');
    }
    else if (accept(';') == 0) {
        sym_fix(sym, emit_func_begin(n));
        parse_statement();
        emit_func_end();
    }
    syms_head = restore_head;
}

/* global  = type , identifier , ";" ;
 * program = { global | function } ;
 */
void parse_program()
{
    while (token) {
        expect_type();
        unsigned sym = sym_lookup();
        if (sym) { 
            get_token();
            parse_function(sym);
        }
        else { /* unknown identifier */
            sym_append(0, 72); /* undefined function */
            if (accept(';')) {
                set_32bit(buf + syms_head, emit_global_var());
                buf[syms_head + 4] = 71; /* global variable */
            }
            else {
                parse_function(syms_head);
            }
        }
    }
}

int main()
{
    buf_size  = 65536;
    buf       = malloc(buf_size);
    syms_head = buf_size;
    lineno    = 1;

    next_char();
    token_int = 4;
    token_buf = "main";
    sym_append(emit_begin(), 72); /* implicit get_token() */
    parse_program();
    write(1, (char *)buf, emit_end());

    return 0;
}
