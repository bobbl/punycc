/**********************************************************************
 * Standard Library Functions
 **********************************************************************/

void exit(int) _Pragma("PunyC emit \x01\xbc\x01\x27\x00\xdf");
    /*  pop {r0}
        movs r7, #1
        svc  #0 */

int getchar(void) _Pragma("PunyC emit \x81\xb0\x00\x20\x69\x46\x01\x22\x03\x27\x00\xdf\x02\xbc\x00\x28\x02\xd0\x08\x06\x00\x0e\x70\x47\x01\x38\x70\x47");
    /*  81 B0   sub sp, #4
        00 20   movs r0, #0     @ stdin
        69 46   mov r1, sp      @ buffer on stack
        01 22   movs r2, #1     @ 1 byte
        03 27   movs r7, #3     @ sys_read
        00 DF   svc #0
        02 BC   pop {r1}
        00 28   cmp r0, #0
        02 D0   beq 1f
        08 06   lsls r0, r1, #24
        00 0E   lsrs r0, r0, #24
        70 47   bx lr
     1: 01 38   subs r0, #1
        70 47   bx lr */

void *malloc(unsigned long) _Pragma("PunyC emit \x00\x20\x2d\x27\x00\xdf\x01\xb4\x00\x99\x40\x18\x01\xb4\x2d\x27\x00\xdf\x02\xbc\x88\x42\x01\xbc\x00\xd0\x00\x20\x70\x47");
    /*  movs r0, #0
        movs r7, #45    @ sys_brk
        svc #0
        push {r0}
        ldr r1, [sp]
        adds r0, r0, r1
        push {r0}
        movs r7, #45    @ sys_brk
        svc #0
        pop {r1}
        cmp r0, r1
        pop {r0}
        beq 1f
        movs r0, #0
     1: bx lr */

int write(int, char*, int) _Pragma("PunyC emit \x02\x98\x01\x99\x00\x9a\x04\x27\x00\xdf\x70\x47");
    /*  ldr r0, [sp, #8]
        ldr r1, [sp, #4]
        ldr r2, [sp, #0]
        movs r7, #4     @ sys_write
        svc #0
        bx lr */

int read(int, char*, int) _Pragma("PunyC emit \x02\x98\x01\x99\x00\x9a\x03\x27\x00\xdf\x70\x47");
    /*  ldr r0, [sp, #8]
        ldr r1, [sp, #4]
        ldr r2, [sp, #0]
        movs r7, #3     @ sys_read
        svc #0
        bx lr */

int putchar(int) _Pragma("PunyC emit \x01\x20\x69\x46\x01\x22\x04\x27\x00\xdf\x70\x47");
    /*  movs r0, #1
        mov  r1, sp
        movs r2, #1
        movs r7, #4     @ sys_write
        svc #0
        bx lr */
