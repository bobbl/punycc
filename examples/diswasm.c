/* Tolerant disassembler for WebAssembly */

unsigned ch;
unsigned filepos;
unsigned dumppos;
unsigned remaining;
unsigned indent;

char *tmp_buf; /* buffer for 256 bytes */

unsigned strlen_u(char *s)
{
    unsigned l = 0;
    while (s[l]) l = l + 1;
    return l;
}

void print_keyword(char *s, unsigned n)
{
    unsigned i = 0;
    while (n) {
        while (s[i] != '|') i = i + 1;
        i = i + 1;
        n = n - 1;
    }
    unsigned len = 0;
    while (s[i+len] != '|') len = len + 1;
    write(1, s + i, len);
}


void prints(char *s)
{
    unsigned len = strlen_u(s);
    write(1, s, len);
}

unsigned hex(unsigned u)
{
    if (u > 9)
        return u + 'A' - 10;
    return u + '0';
}

void printx(unsigned u, unsigned digits)
{
    unsigned i = 0;
    while (i < 8) {
        tmp_buf[7 - i] = hex(u & 15);
        u = u >> 4;
        i = i + 1;
    }
    write(1, tmp_buf - digits + 8, digits);
}

void println()
{
    dumppos = 0;
    write(1, "\x0d\x0a", 2);
    printx(filepos, 4);
    write(1, " ", 1);
}

void printu(unsigned u)
{
    if (u == 0) {
        write(1, "0", 1);
    }
    else {
        unsigned width = 0;
        while (u) {
            width = width + 1;
            tmp_buf[10 - width] = (u % 10) + '0';
            u = u / 10;
        }
        write(1, tmp_buf + 10 - width, width);
    }
}

void printi(int i)
{
    if (i >> 31) {
        write(1, "-", 1);
        i = 0 - i;
    }
    printu(i);
}

void printbyte(unsigned b)
{
    printx(b, 2);
    write(1, " ", 1);
}

void tab()
{
    while (dumppos < 10) {
        prints("   ");
        dumppos = dumppos + 1;
    }
}

void tab_indent()
{
    tab();
    unsigned i = 0;
    while (i < indent) {
        prints("  ");
        i = i + 1;
    }
}

void error(char *msg)
{
    write(2, "Error: ", 7);
    write(2, msg, strlen_u(msg));
    write(2, " at offset line ", 16);
    write(2, ".\x0d\x0a", 3);
    exit(1);
}

unsigned next_char()
{
    ch = getchar();
    remaining = remaining - 1;
    filepos = filepos + 1;
    dumppos = dumppos + 1;
    if (dumppos > 8) {
        println();
        dumppos = 1;
    }
    printbyte(ch);
    return ch;
}

void skip_bytes(unsigned n)
{
    while (n) {
        next_char();
        n = n - 1;
    }
}

unsigned read_uleb()
{
    unsigned x = 0;
    unsigned shift = 0;
    ch = next_char();
    while (ch > 127) {
        x = x + ((ch-128) << shift);
        shift = shift + 7;
        ch = next_char();
    }
    return x + (ch << shift);
}

unsigned read_sleb()
{
    unsigned x = 0;
    unsigned shift = 0;
    ch = next_char();
    while (ch > 127) {
        x = x + ((ch-128) << shift);
        shift = shift + 7;
        ch = next_char();
    }
    if (ch >= 64) {
        ch = ch + 4294967232; /* 0xFFFFFFC0 */
    }
    return x + (ch << shift);
}



unsigned parse_header()
{
    next_char();
    if (ch != 0) return 0;
    next_char();
    if (ch != 'a') return 0;
    next_char();
    if (ch != 's') return 0;
    next_char();
    if (ch != 'm') return 0;
    next_char();
    if (ch != 1) return 0;
    next_char();
    if (ch != 0) return 0;
    next_char();
    if (ch != 0) return 0;
    next_char();
    if (ch != 0) return 0;
    tab();
    prints("WASM header");
    println();
    return 1;
}

void print_valtype(unsigned v)
{
    if (v==111) prints("externref");
    else if (v==112) prints("funcref");
    else if (v==123) prints("v128");
    else if (v==124) prints("f64");
    else if (v==125) prints("f32");
    else if (v==126) prints("i64");
    else if (v==127) prints("i32");
    else {
        prints("unknown valtype ");
        printu(v);
    }
}

void print_blocktype(unsigned blocktype)
{
    if (blocktype > 4294967232) { /* 0xFFFFFFC0 */
        blocktype = blocktype & 127;
        if (blocktype != 64) {
            print_valtype(blocktype);
        }
    }
    else {
        prints("$type");
        printu(blocktype);
    }
}

void print_code_simple()
{
    while (remaining > 0) {
        ch = next_char();
    }
    println();
}


/* 0x00 ... 0x11 */
void print_insn_control(unsigned insn)
{
    if (insn == 0) {
        tab_indent();
        prints("unreachable");
    }
    else if (insn == 1)  {
        tab_indent();
        prints("nop");
    }
    else if (insn <= 4) {
        unsigned blocktype = read_sleb();
        tab_indent();
        if (insn == 2) prints("block ");
        else if (insn == 3) prints("loop ");
        else prints("if ");
        print_blocktype(blocktype);
        indent = indent + 1;
    }
    else if (insn == 5) {
        indent = indent - 1;
        tab_indent();
        prints("else");
        indent = indent + 1;
    }
    else if (insn == 11) {
        indent = indent - 1;
        tab_indent();
        prints("end");
    }
    else if ((insn == 12) | (insn == 13)) {
        unsigned label = read_uleb();
        tab_indent();
        if (insn == 12) prints("br ");
        else prints("br_if ");
        printu(label);
    }
    else if (insn == 15) {
        tab_indent();
        prints("return");
    }
    else if (insn == 16) {
        unsigned funcno = read_uleb();
        tab_indent();
        prints("call $func");
        printu(funcno);
    }
    /* TODO: missing 14=br_table and 17=call_indirect */
    else {
        tab_indent();
        prints("???");
    }
    println();
}

/* 0x12 ... 0x1F */
void print_insn_parametric(unsigned insn)
{
    tab_indent();
    if (insn == 26) prints("drop");
    else if (insn == 27) prints("select");
    else prints("unknown parametric instructionn");
    println();
}


/* 0x20 ... 0x24 */
void print_insn_variable(unsigned insn)
{
    unsigned no = read_uleb();
    tab_indent();
    if (insn == 32) prints("local.get $local");
    else if (insn == 33) prints("local.set $local");
    else if (insn == 34) prints("local.tee $local");
    else if (insn == 35) prints("global.get $global");
    else prints("local.set $global");
    printu(no);
    println();
}

/* 0x25 ... 0x40 */
void print_insn_memory(unsigned insn)
{
    if (insn < 40) {
        tab_indent();
        prints("unknown memory instruction");
    }
    else if (insn >= 63) {
        ch = next_char();
        tab_indent();
        if (insn == 63) prints("memory.size ");
        else prints("memory.grow ");
        printu(ch);
    } else {
        unsigned align = read_uleb();
        unsigned ofs = read_uleb();
        tab_indent();
        print_keyword("i32.load|i64.load|f32.load|f64.load|i32.load8_s|i32.load8_u|i32.load16_s|i32.load16_u|i64.load8_s|i64.load8_u|i64.load16_s|i64.load16_u|i64.load32_s|i64.load32_u|i32.store|i64.store|f32.store|f64.store|i32.store8|i32.store16|i64.store8|i64.store16|i64.store32|",
            insn - 40);
        prints(" ");
        printu(align);
        prints(" ");
        printu(ofs);
    }
    println();
}

/* 0x41 ... 0xC4 */
void print_insn_numeric(unsigned insn)
{
    if (insn <= 68) {
        if (insn == 65) {
            unsigned i = read_sleb();
            tab_indent();
            prints("i32.const ");
            printi(i);
        }
        else if (insn == 66) {
            unsigned i = read_sleb();
            tab_indent();
            prints("i64.const ");
            printi(i);
        }
        else if (insn == 67) {
            skip_bytes(4);
            tab_indent();
            prints("f32.const ?");
            /* TODO: read and print f32 */
        }
        else {
            skip_bytes(8);
            tab_indent();
            prints("f64.const ");
            /* TODO: read and print f32 */
        }
    }
    else if (insn < 91) {
        if (insn < 80) {
            tab_indent();
            prints("i32.");
        }
        else {
            tab_indent();
            prints("i64.");
            insn = insn - 11;
        }
        print_keyword("eqz|eq|ne|lt_s|lt_u|gt_s|gt_u|le_s|le_u|ge_s|ge_u|",
            insn - 69);
    }
    else if (insn < 103) {
        if (insn < 97) {
            tab_indent();
            prints("f32.");
        }
        else {
            tab_indent();
            prints("f64.");
            insn = insn - 6;
        }
        print_keyword("eq|ne|lt|gt|le|ge|",
            insn - 91);
    }
    else if (insn < 139) {
        if (insn < 121) {
            tab_indent();
            prints("i32.");
        }
        else {
            tab_indent();
            prints("i64.");
            insn = insn - 18;
        }
        print_keyword("clz|ctz|popcnt|add|sub|mul|div_s|div_u|rem_s|rem_u|and|or|xor|shl|shr_s|shr_u|rotl|rotr|",
            insn - 103);
    }
    else if (insn < 167) {
        if (insn < 153) {
            tab_indent();
            prints("f32.");
        }
        else {
            tab_indent();
            prints("f64.");
            insn = insn - 14;
        }
        print_keyword("abs|neg|ceil|floor|trunc|nearest|sqrt|dd|sub|mul|div|min|max|copysign|",
            insn - 139);
    }

    
    else {
        tab_indent();
        prints("unknown parametric instructionn");
    }
    println();
}



void print_code()
{
    indent = 3;
    while (remaining > 0) {
        ch = next_char();
        if (ch <= 17) print_insn_control(ch);
        else if (ch <= 31) print_insn_parametric(ch);
        else if (ch <= 36) print_insn_variable(ch);
        else if (ch <= 64) print_insn_memory(ch);
        else if (ch <= 196) print_insn_numeric(ch);
        else {
            tab_indent();
            prints("not yet");
            println();
        }
    }
    println();
}

void section_code(unsigned section_size)
{
    prints(" CODE SECTION");
    println();

    unsigned entries = read_uleb();
    tab();
    prints("entries: ");
    printu(entries);
    println();

    unsigned e = 0;
    while (e < entries) {
        unsigned code_size = read_uleb();
        tab();
        prints("  $func");
        printu(e);
        prints(" size=");
        printu(code_size);
        println();
        remaining = code_size;

        unsigned locals = read_uleb();
        tab();
        prints("    locals: ");
        printu(locals);
        println();

        unsigned no = 0;
        unsigned l = 0;
        while (l < locals) {
            unsigned n = read_uleb();
            unsigned valtype = read_uleb();
            tab();
            if (n==1) {
                prints("      $var");
                printu(no);
                no = no + 1;
            }
            else {
                prints("      $var");
                printu(no);
                no = no + n;
                prints("...$var");
                printu(no-1);
            }
            prints(" : ");
            print_valtype(valtype);
            println();
            l = l + 1;
        }
        tab();
        prints("    code");
        println();
        print_code();

        e = e + 1;
    }
}


int main()
{
    filepos = 0;
    dumppos = 0;
    indent = 0;
    tmp_buf = malloc(256);

    if (parse_header() == 0) {
        error("No WASM header\x0d\x0a.");
        exit(1);
    }

    ch = next_char();
    while (ch < 256) {
        unsigned section_id = ch;
        unsigned size = read_uleb();

        tab();
        prints("section id=");
        printu(section_id);
        prints(" size=");
        printu(size);
        remaining = size;

        if (section_id==10) {
            section_code(size);
        } else {
            println();

            unsigned i = 0;
            while (i < size) {
                ch = next_char();
                i = i + 1;
            }
            tab();
            println();
        }
        ch = next_char();
    }

    println();
    return 0;
}
