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

unsigned int ch;
unsigned int ch_class;
unsigned int lineno;
unsigned int token;
unsigned int token_int;
unsigned int token_size;
char *token_buf;
unsigned int syms_head;

void itoa4(unsigned int x)
{
    unsigned int i = x * 134218; /* x = x * (((1<<27)/1000) + 1) */
    char *s = (char *)buf + syms_head - 16;
    s[0] = (i>>27) + '0';
    i = (i & 134217727) * 5;  /* 0x07FFFFFF */
    s[1] = (i>>26) + '0';
    i = (i & 67108863) * 5;   /* 0x03FFFFFF */
    s[2] = (i>>25) + '0';
    i = (i & 33554431) * 5;   /* 0x01FFFFFF */
    s[3] = (i>>24) + '0';
    write(2, s, 4);
}

void error(unsigned int no)
{
    write(2, "Error ", 6);
    itoa4(no);
    write(2, " in line ", 9);
    itoa4(lineno);
    write(2, ".\x0d\x0a", 3);
    exit(no);
}

int token_cmp(char *s, unsigned int n)
{
    unsigned int i = 0;
    while (s[i] == token_buf[i]) {
        i = i + 1;
        if (i == n) {
            return 1;
        }
    }
    return 0;
}

unsigned int next_char()
{
    char  *classify  = " !\x22  \x0a\x07\x27()\x08\x06,\x03 \x09^^^^^^^^^^ ;\x12=\x14  __________________________[ ]\x05_ __________________________{\x04}  ";

    ch = getchar();
    if (ch == 10) {
        lineno = lineno + 1;
    }
    ch_class = ' ';
    if ((ch - 32) < 96) {
        ch_class = classify[ch - 32];
    }
    return ch;
}

void store_char()
{
    token_buf[token_int] = ch;
    token_int = token_int + 1;
    if (token_int >= token_size) {
        error(100); /* Error: buffer overflow */
    }
    next_char();
}

void get_token()
{
    unsigned int i;
    unsigned int j;
    unsigned int len;


    token_size = syms_head - code_pos;
    if (token_size < 1024) {
        error(100); /* Error: buffer overflow */
    }
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
    if (ch > 255) {
        return;
    }

    token = ch_class;
    if (token == ' ') {
        error(101); /* Error: invalid character */
    }

    if (ch == 39) {                  /* ' */
        token = 94; /* '^' number */
        token_int = next_char();
        while (next_char() != 39) {}
        next_char();
    }
    else if (token == '"') { /* string */
        next_char();
        while (ch != 34) {
            if (ch == 92) { /* \ */
                if (next_char() == 'x') {
                    i = next_char() - 48; /* 0 */
                    if (i > 9) { i = i - 39; }
                    j = next_char() - 48; /* 0 */
                    if (j > 9) { j = j - 39; }
                    ch = (i << 4) + j;
                }
            }
            store_char();
        }
        next_char();
    }
    else if (token == '^') { /* number */
        while ((ch >= '0') & (ch <= '9')) {
            token_int = (10 * token_int) + ch - 48; /* '0' */
            next_char();
        }
    }
    else if (token == '_') { /* identifier */

        /* store identifier in space between code and symbol table */
        ch_class = 94;
        while ((ch_class & 254) == 94) { /* ==94 if  '^' or '_' */
            store_char();
        }
        token_buf[token_int] = 0;

        /* search keyword */
        char *keywords = "2if4else5while6return7_Pragma4void4char3int8unsigned4long0";
        i = 0;
        len = 2;
        token = 96;
        while (len) {
            if (len == token_int) {
                if (token_cmp(keywords + i + 1, token_int)) {
                    return;
                }
            }
            token = token + 1;
            i = i + len + 1;
            len = keywords[i] - '0';
        }
        token = 95; /* '_' identifier */
    }
    else if (ch == '!') {
        if (next_char() != '=') {
            error(101); /* Error: invalid character */
        }
        next_char();
        token = 17; /* 0x11 ne */
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


unsigned int sym_lookup()
{
    if (token != '_') {
        error(103); /* Error: identifier expected */
    }
    unsigned int s = syms_head;
    while (s < buf_size) {
        unsigned int len = buf[s + 5];
        if (len == token_int) {
            if (token_cmp((char *)buf + s + 6, token_int)) {
                return s;
            }
        }
        s = s + len + 6;
    }
    return 0;
}

void sym_append(unsigned int addr, unsigned int type)
{
    unsigned int i = token_int;
    syms_head = syms_head - token_int - 6;
    unsigned char *s = buf + syms_head;

    set_32bit(s, addr);
    s[4] = type;
    s[5] = i;

    /* copy backwards in case the token and the symbol table overlap */
    while (i != 0) {
        i = i - 1;
        s[6 + i] = token_buf[i];
    }
    get_token();
}


void sym_fix(unsigned int sym, unsigned int func_pos)
{
    unsigned char *s = buf + sym;
    unsigned int i = get_32bit(buf + sym);
    unsigned int next;

    if (s[4] != 72) {
        error(105); /* Error: function redefined */
    }

    while (i != 0) {
        next = get_32bit(buf + i);
        set_32bit(buf + i, emit_fix_call(i, func_pos));
        i = next;
    }
    set_32bit(s, func_pos);
    s[4] = 73;
}





/**********************************************************************
 * Parser
 **********************************************************************/

unsigned int accept(unsigned int ch)
/* parameter named `ch` to check if name scopes work */
{
    if (token == ch) {
        get_token();
        return 1;
    }
    return 0;
}

void expect(unsigned int t)
{
    if (accept(t) == 0) {
        error(102); /* Error: specific token expected */
    }
}

unsigned int accept_type_id()
{
    if (token < 101) {
        return 0;
    }
    if (token > 105) {
        return 0;
    }
    get_token();
    return 1;
}

unsigned int accept_type()
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
    if (accept_type() == 0) {
        error(106); /* Error: type expected */
    }
}

void parse_factor();

/* bin_op = "<<" | ">>" | "&" | "|" | "^" | "+" | "-" ;
 * operation = factor , { bin_op , factor } ;
 */
void parse_operation()
{
    parse_factor();
    while (token < 16) {
        emit_push();
        unsigned int op = token;
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
        unsigned int op = token;
        get_token();
        parse_operation();
        emit_comp(op);
    }
}

/* condition = "(" , operation , [ cmp_op , operation ] , ")" ;
 */
unsigned int parse_condition()
{
    unsigned int cond = 0;
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
    unsigned int sym;
    unsigned int type;
    unsigned int ofs;

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
        sym = sym_lookup();
        if (sym == 0) {
            error(104); /* Error: unknown identifier */
        }
        type = buf[sym + 4];
        ofs = get_32bit(buf + sym);
        get_token();

        if ((type | 1) == 73) { /* type 72 or 73: function */
            expect('(');
            unsigned int argno = 0;
            unsigned int save = emit_pre_call();
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
            unsigned int link = emit_call(ofs, argno, save);
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
            else {
                emit_index_load_array(type & 1, ofs);
            }
        }
        else { /* variable */
            if (accept('=')) {
                parse_expression();
                emit_store(type & 1, ofs);
            }
            else {
                emit_load(type & 1, ofs);
            }
        }
    }
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
    unsigned int h;
    unsigned int s;

    if (accept('{')) {
        h = syms_head;
        s = emit_scope_begin();
        while (accept('}') == 0) {
            parse_statement();
        }
        emit_scope_end(s);
        syms_head = h;
    }
    else if (accept(96)) { /* if */
        h = parse_condition();
        parse_statement();
        if (accept(97)) { /* else */
            s = emit_jump_and_fix_branch_here(0, h);
            parse_statement();
            emit_fix_jump_here(s);
        }
        else {
            emit_fix_branch_here(h);
        }
    }
    else if (accept(98)) { /* while */
        h = emit_pre_while();
        s = parse_condition();
        parse_statement();
        emit_jump_and_fix_branch_here(h, s);
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
        s = 0;
        if (accept('=')) {
            parse_expression();
            s = 1;
        }
        expect(';');
        set_32bit(buf + syms_head, emit_local_var(s));
    }
    else {
        s = emit_scope_begin();
        parse_expression();
        emit_scope_end(s);
        expect(';');
    }
}

/* params   = { type , [ identifier ] , [ "," ] } ;
 * body     = statement | ";" ;
 * function = type , identifier , "(" , params ,  ")" , body ;
 */
void parse_function(unsigned int sym)
{
    expect('(');
    unsigned int restore_head = syms_head;

    unsigned int n = 0;
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
        unsigned int sym = sym_lookup();
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
