/**********************************************************************
 * Standard Library Functions
 **********************************************************************/

void exit(int) _Pragma("PunyC emit \x58\x5b\x31\xc0\x40\xcd\x80");
    /*  pop eax
        pop ebx
        xor eax, eax
        inc eax
        int 128 */

int getchar(void) _Pragma("PunyC emit \x53\xb8\x03\x00\x00\x00\x31\xdb\x53\x89\xe1\x31\xd2\x42\xcd\x80\x85\xc0\x58\x75\x01\x48\x5b\xc3");
    /*  push ebx
        mov eax, 3
        xor ebx, ebx
        push ebx
        mov ecx, esp
        xor edx, edx
        inc edx
        int 128
        test eax, eax
        pop eax
        jnz 1f
        dec eax
     1: pop ebx
        ret */

void *malloc(unsigned long) _Pragma("PunyC emit \x53\x31\xdb\xb8\x2d\x00\x00\x00\xcd\x80\x8b\x5c\x24\x08\x01\xc3\x50\x53\xb8\x2d\x00\x00\x00\xcd\x80\x5b\x39\xc3\x58\x74\x02\x31\xc0\x5b\xc2\x04\x00");
    /*  push ebx
        xor ebx, ebx
        mov eax, 45
        int 128
        mov ebx, [esp+8]
        add ebx, eax
        push eax
        push ebx
        mov eax, 45

        int 128
        pop ebx
        cmp eax, ebx
        pop eax
        je 1f
        xor eax, eax
     1: pop ebx
        ret 4 */

int putchar(int) _Pragma("PunyC emit \x53\xb8\x04\x00\x00\x00\x31\xdb\x43\x8d\x4c\x24\x08\x89\xda\xcd\x80\x5b\xc2\x04\x00");
    /*  push ebx
        mov eax, 4
        xor ebx, ebx
        inc ebx
        lea ecx, [esp+8]
        mov edx, ebx
        int 128
        ret 4 */

int write(int, char*, int) _Pragma("PunyC emit \x53\x8b\x54\x24\x08\x8b\x4c\x24\x0c\x8b\x5c\x24\x10\xb8\x04\x00\x00\x00\xcd\x80\x5b\xc2\x0c\x00");
    /*  push ebx
        mov edx, [esp+8]
        mov ecx, [esp+12]
        mov ebx, [esp+16]
        mov eax, 4
        int 128
        pop ebx
        ret 12 */

