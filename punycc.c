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
    operations    conditions    other    reserved words     types
                  50h 'P' ==    28h (    03h if             08h void
    41h 'A' <<    51h 'Q' !=    29h )    04h else           09h char
    42h 'B' >>    52h 'R' <     2Ch ,    05h while          0Ah int
    43h 'C' -     53h 'S' >=    3Bh ;    06h return         0Bh unsigned
    44h 'D' |     54h 'T' >     3Dh =    07h  _Pragma       0Ch long
    45h 'E' ^     55h 'U' <=    5Bh [                       0Dh static
    46h 'F' +                   5Dh ]    special            0Eh const
    47h 'G' &                   7Bh {    00h EOF
    48h 'H' *                   7Dh }    01h string
    49h 'I' /                            0Fh identifier
    4Ah 'J' %                            5Eh '^' number
*/






/**********************************************************************
 * Scanner
 **********************************************************************/

static unsigned int ch;
static unsigned int ch_class;
static unsigned int lineno;
static unsigned int token;
static unsigned int token_int;
static unsigned int token_size;
static char *token_buf;
static unsigned int syms_head;

static void itoa4(unsigned int x)
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

static void error(unsigned int no)
{
    write(2, "Error ", 6);
    itoa4(no);
    write(2, " in line ", 9);
    itoa4(lineno);
    write(2, ".\x0d\x0a", 3);
    exit(no);
}

static unsigned int token_cmp(const char *s, unsigned int n)
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

static unsigned int next_char(void)
{
    const char  *classify  = "         ##  #                  #!!  JG!()HF,C #^^^^^^^^^^ ;R=T  __________________________[ ]E_ __________________________{D}  ";
        /*                    01234567890123456789012345678901234567890123456789
         ! = look at character for further processing
         # = whitespace (9, 10, 13, ' ', '/')
         ^ = digit [0123456789]
         _ = letter or underscore
        */
    ch = getchar();
    if (ch == 10) {
        lineno = lineno + 1;
    }
    ch_class = 32; /* ' ' */
    if (ch < 128) {
        ch_class = classify[ch];
    }
    return ch;
}

static void store_char(void)
{
    token_buf[token_int] = ch;
    token_int = token_int + 1;
    if (token_int >= token_size) {
        error(100); /* Error: buffer overflow */
    }
    (void)next_char();
}

static void get_token(void)
{
    unsigned int i;
    unsigned int len;

    token_size = syms_head - code_pos;
    if (token_size < 1024) {
        error(100); /* Error: buffer overflow */
    }
    token_size = token_size - 512;
    token_buf = (char *)buf + code_pos + 256;
    token_int = 0;
    token = 0;

    while (ch_class == '#') { /* ch = 9,10,13,' ','/' */
        if (ch == '/') {
            if (next_char() != '*') {
                token = 73; /* 'I' /  */
                return;
            }
            while (next_char() != '/') {
                while (ch != '*') {
                    (void)next_char();
                }
            }
        }
        (void)next_char();
    }

    if (ch > 255) {
        return;
    }
    if (ch_class == ' ') {
        error(101); /* Error: invalid character */
    }

    token = ch_class;
    if (ch == 39) {                  /* ' */
        token_int = next_char();
        while (next_char() != 39) {}
        (void)next_char();
        token = 94; /* '^' 5Eh number */
    }
    else if (ch == '"') { /* string */
        (void)next_char();
        while (ch != 34) {
            if (ch == 92) { /* \ */
                if (next_char() == 'x') {
                    i = next_char() - 48; /* '0' */
                    if (i > 9) { i = i - 39; }
                    len = next_char() - 48; /* '0' */
                    if (len > 9) { len = len - 39; }
                    ch = (i << 4) + len;
                }
            }
            store_char();
        }
        (void)next_char();
        token = 1; /* 0x01 string */
    }
    else if (ch_class == '^') { /* digit 0...9 */
        while (ch_class == '^') {
            token_int = (10 * token_int) + ch - 48; /* '0' */
            (void)next_char();
        }
        /* token = '^' 0x5E number */
    }
    else if (ch_class == '_') { /* letter or underscore */
        /* store identifier in space between code and symbol table */
        while ((ch_class & 254) == 94) { /* ==94 if  '^' or '_' */
            store_char();
        }
        token_buf[token_int] = 0;

        /* search keyword */
        const char *keywords = "2if4else5while6return7_Pragma4void4char3int8unsigned4long6static5const0";
        i = 0;
        len = 2;
        token = 3;
        while (len != 0) {
            if (len == token_int) {
                if (token_cmp(keywords + i + 1, token_int) != 0) {
                    return;
                }
            }
            token = token + 1;
            i = i + len + 1;
            len = keywords[i] - '0';
        }
        /* token = 15 0x0F identifier */
    }
    else if (ch == '!') {
        if (next_char() != '=') {
            error(101); /* Error: invalid character */
        }
        (void)next_char();
        token = 81; /* 'Q' 0x51 != */
    }
    else if (ch == '<') {
        if (next_char() == '<') {
            (void)next_char();
            token = 65; /* 'A' 0x41 << */
        }
        else {
            if (ch == '=') {
                (void)next_char();
                token = 85; /* 'U' 0x55 <= */
            }
        }
        /* token = 'R' 0x52 < */
    }
    else if (ch == '=') {
        if (next_char() == '=') {
            (void)next_char();
            token = 80; /* 'P' 0x50 == */
        }
        /* token = '=' 0x3D = (assignment) */
    }
    else if (ch == '>') {
        if (next_char() == '=') {
            (void)next_char();
            token = 83; /* 'S' 0x53 >= */
        }
        else {
            if (ch == '>') {
                (void)next_char();
                token = 66; /* 'B' 0x42 >> */
            }
        }
        /* token = 'T' 0x54 > */
    }
    else {
        /* case for (),;[]{} */
        (void)next_char();
    }
}





/**********************************************************************
 * Symbol Management
 **********************************************************************/


static unsigned int sym_lookup(void)
{
    if (token != 15) {
        error(103); /* Error: identifier expected */
    }
    unsigned int s = syms_head;
    while (s < buf_size) {
        unsigned int len = buf[s + 5];
        if (len == token_int) {
            if (token_cmp((char *)buf + s + 6, token_int) != 0) {
                return s;
            }
        }
        s = s + len + 6;
    }
    return 0;
}

static void sym_append(unsigned int addr, unsigned int type)
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


static void sym_fix(unsigned int sym, unsigned int func_pos)
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

static unsigned int accept(unsigned int ch)
/* parameter named `ch` to check if name scopes work */
{
    if (token == ch) {
        get_token();
        return 1;
    }
    return 0;
}

static void expect(unsigned int t)
{
    if (accept(t) == 0) {
        error(102); /* Error: specific token expected */
    }
}

static unsigned int accept_type(void)
{
    unsigned int accepted = 0;
    while ((token - 8) < 7) { /* 8...14 */
        get_token();
        accepted = 1;
    }
    if (accepted != 0) {
        while (accept('H') != 0) {} /* multiple stars */
    }
    return accepted;
}

static void expect_type(void)
{
    if (accept_type() == 0) {
        error(106); /* Error: type expected */
    }
}

static void parse_factor(void);

/* bin_op = "<<" | ">>" | "&" | "|" | "^" | "+" | "-" ;
 * operation = factor , { bin_op , factor } ;
 */
static void parse_operation(void)
{
    parse_factor();
    while ((token & 240) == 64) {
        emit_push();
        unsigned int op = token & 15;
        get_token();
        parse_factor();
        emit_operation(op);
    }
}

/* cmp_op     = "<" | "<=" | "==" | "!=" | ">=" | ">" ;
 * comparison = binary , [ shift_op , binary ] ;
 */
static void parse_expression(void)
{
    parse_operation();
    if ((token & 248) == 80) { /* (token & 0xF8) == 0x50 */
        emit_push();
        unsigned int op = token & 15;
        get_token();
        parse_operation();
        emit_comp(op);
    }
}

/* condition = "(" , operation , [ cmp_op , operation ] , ")" ;
 */
static unsigned int parse_condition(void)
{
    unsigned int cond = 1;
    expect('(');
    parse_operation();
    emit_push();
    if ((token & 248) == 80) { /* (token & 0xF8) == 0x50 */
        cond = token & 15;
        get_token();
        parse_operation();
    }

    /* implicit "!= 0" for compatibility with cc500 */
    else {
        emit_number(0);
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
static void parse_factor(void)
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
    else if (token == 1) { /* string */
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
                emit_arg();
                argno = argno + 1;
                while (accept(',') != 0) {
                    parse_expression();
                    emit_arg();
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
        else if (accept('[') != 0) { /* array */
            parse_expression();
            expect(']');
            if (accept('=') != 0) {
                emit_index_push(type & 1, ofs);
                parse_expression();
                emit_pop_store_array();
            }
            else {
                emit_index_load_array(type & 1, ofs);
            }
        }
        else { /* variable */
            if (accept('=') != 0) {
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
static void parse_statement(void)
{
    unsigned int h;
    unsigned int s;

    if (accept('{') != 0) {
        h = syms_head;
        s = emit_scope_begin();
        while (accept('}') == 0) {
            parse_statement();
        }
        emit_scope_end(s);
        syms_head = h;
    }
    else if (accept(3) != 0) { /* if */
        h = parse_condition();
        parse_statement();
        if (accept(4) != 0) { /* else */
            s = emit_then_else(h);
            parse_statement();
            emit_else_end(s);
        }
        else {
            emit_then_end(h);
        }
    }
    else if (accept(5) != 0) { /* while */
        h = emit_pre_while();
        s = parse_condition();
        parse_statement();
        emit_loop(h, s);
    }
    else if (accept(6) != 0) { /* return */
        if (accept(';') == 0) {
            parse_expression();
            expect(';');
        }
        emit_return();
    }
    else if (accept_type() != 0) { /* variable declaration */
        sym_append(0 /* don't care */, 74); /* local variable */
        s = 0;
        if (accept('=') != 0) {
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
static void parse_function(unsigned int sym)
{
    unsigned int restore_head = syms_head;
    unsigned int n = 0;

    expect('(');
    (void)accept(8); /* accept foo(void) instead of foo() */
    while (accept(')') == 0) {
        n = n + 1;
        expect_type();
        if (token == 15) { /* identifier */
            sym_append(n, 74);
        }
        (void)accept(','); /* ignore trailing comma */
    }

    if (accept(7) != 0) { /* _Pragma */
        expect('(');
        while (token != ')') {
            if (token == 1) { /* string */
                if (token_cmp("PunyC emit ", 11) != 0) {
                    sym_fix(sym, emit_binary_func(token_int-11, token_buf+11));
                }
            }
            get_token();
        }
        get_token();
        expect(';');
    }
    else {
        if (accept(';') == 0) {
            sym_fix(sym, emit_func_begin(n));
            parse_statement();
            emit_func_end();
        }
    }
    syms_head = restore_head;
}

/* global  = type , identifier , ";" ;
 * program = { global | function } ;
 */
static void parse_program(void)
{
    while (token != 0) {
        expect_type();
        unsigned int sym = sym_lookup();
        if (sym != 0) { 
            get_token();
            parse_function(sym);
        }
        else { /* unknown identifier */
            sym_append(0, 72); /* undefined function */
            if (accept(';') != 0) {
                set_32bit(buf + syms_head, emit_global_var());
                buf[syms_head + 4] = 71; /* global variable */
            }
            else {
                parse_function(syms_head);
            }
        }
    }
}

int main(void)
{
    buf_size  = 65536;
    buf       = malloc(buf_size);
    syms_head = buf_size;
    lineno    = 1;

    (void)next_char();
    token_int = 4;
    token_buf = "main";
    sym_append(emit_begin(), 72); /* implicit get_token() */
    parse_program();
    write(1, (char *)buf, emit_end());

    return 0;
}
