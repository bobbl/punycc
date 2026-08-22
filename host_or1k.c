/**********************************************************************
 * Standard Library Functions
 **********************************************************************/

void exit(int) _Pragma("PunyC emit \xa9\x60\x00\x5d\x20\x00\x00\x00");
    /*  l.ori r11, r0, 93       # sys_exit
        l.sys 0 */

int getchar(void) _Pragma("PunyC emit \x9c\x21\xff\xc0\xd4\x01\x60\x0c\xd4\x01\x68\x10\xd4\x01\x78\x14\xd4\x01\x88\x18\xd4\x01\x98\x1c\xd4\x01\xa8\x20\xd4\x01\xb8\x24\xd4\x01\xc8\x28\xd4\x01\xd8\x2c\xd4\x01\xe8\x30\xd4\x01\xf8\x34\x18\x60\x00\x00\xe0\x81\x08\x04\xa8\xa0\x00\x01\xa9\x60\x00\x3f\x20\x00\x00\x00\x8c\x61\x00\x00\x9c\x80\xff\xff\xbc\x0b\x00\x00\xe0\x64\x18\x0e\x85\x81\x00\x0c\x85\xa1\x00\x10\x85\xe1\x00\x14\x86\x21\x00\x18\x86\x61\x00\x1c\x86\xa1\x00\x20\x86\xe1\x00\x24\x87\x21\x00\x28\x87\x61\x00\x2c\x87\xa1\x00\x30\x87\xe1\x00\x34\x9c\x21\x00\x40\x44\x00\x48\x00\x15\x00\x00\x00");
    /*  l.addi r1, r1, -64
        l.sw 12(r1), r12
        l.sw 16(r1), r13
        l.sw 20(r1), r15
        l.sw 24(r1), r17
        l.sw 28(r1), r19
        l.sw 32(r1), r21
        l.sw 36(r1), r23
        l.sw 40(r1), r25
        l.sw 44(r1), r27
        l.sw 48(r1), r29
        l.sw 52(r1), r31

        l.movhi r3, 0           # arg 1: stdin
        l.or r4, r1, r1         # arg 2: top of stack
        l.ori r5, r0, 1         # arg 3: 1 byte
        l.ori r11, r0, 63       # sys_read
        l.sys 0
        l.lbz r3, 0(r1)
        l.addi r4, r0, -1
        l.sfeqi r11, 0
        l.cmov r3, r4, r3      # return r11==0 ? -1 : [r1]

        l.lwz r12, 12(r1)
        l.lwz r13, 16(r1)
        l.lwz r15, 20(r1)
        l.lwz r17, 24(r1)
        l.lwz r19, 28(r1)
        l.lwz r21, 32(r1)
        l.lwz r23, 36(r1)
        l.lwz r25, 40(r1)
        l.lwz r27, 44(r1)
        l.lwz r29, 48(r1)
        l.lwz r31, 52(r1)
        l.addi r1, r1, 64
        l.jr r9
        l.nop 0 */

void *malloc(unsigned long) _Pragma("PunyC emit \x9c\x21\xff\xc0\xd4\x01\x60\x0c\xd4\x01\x68\x10\xd4\x01\x78\x14\xd4\x01\x88\x18\xd4\x01\x98\x1c\xd4\x01\xa8\x20\xd4\x01\xb8\x24\xd4\x01\xc8\x28\xd4\x01\xd8\x2c\xd4\x01\xe8\x30\xd4\x01\xf8\x34\xd4\x01\x18\x00\x18\x60\x00\x00\xa9\x60\x00\xd6\x20\x00\x00\x00\x84\x61\x00\x00\xd4\x01\x58\x04\xe0\x63\x58\x00\xd4\x01\x18\x00\xa9\x60\x00\xd6\x20\x00\x00\x00\x84\x61\x00\x00\x84\x81\x00\x04\xe4\x0b\x18\x00\xe0\x64\x00\x0e\x85\x81\x00\x0c\x85\xa1\x00\x10\x85\xe1\x00\x14\x86\x21\x00\x18\x86\x61\x00\x1c\x86\xa1\x00\x20\x86\xe1\x00\x24\x87\x21\x00\x28\x87\x61\x00\x2c\x87\xa1\x00\x30\x87\xe1\x00\x34\x9c\x21\x00\x40\x44\x00\x48\x00\x15\x00\x00\x00");
    /*  l.addi r1, r1, -64
        l.sw 12(r1), r12
        l.sw 16(r1), r13
        l.sw 20(r1), r15
        l.sw 24(r1), r17
        l.sw 28(r1), r19
        l.sw 32(r1), r21
        l.sw 36(r1), r23
        l.sw 40(r1), r25
        l.sw 44(r1), r27
        l.sw 48(r1), r29
        l.sw 52(r1), r31

        l.sw 0(r1), r3          # size
        l.movhi r3, 0
        l.ori r11, r0, 214      # sys_brk(0)
        l.sys 0

        l.lwz r3, 0(r1)         # size
        l.sw 4(r1), r11         # end
        l.add r3, r3, r11
        l.sw 0(r1), r3          # end + size
        l.ori r11, r0, 214      # sys_brk(end+size)
        l.sys 0

        l.lwz r3, 0(r1)         # end + size
        l.lwz r4, 4(r1)         # (old) end
        l.sfeq r11, r3
        l.cmov r3, r4, r0       # return r11==(end+size) ? end : 0

        l.lwz r12, 12(r1)
        l.lwz r13, 16(r1)
        l.lwz r15, 20(r1)
        l.lwz r17, 24(r1)
        l.lwz r19, 28(r1)
        l.lwz r21, 32(r1)
        l.lwz r23, 36(r1)
        l.lwz r25, 40(r1)
        l.lwz r27, 44(r1)
        l.lwz r29, 48(r1)
        l.lwz r31, 52(r1)
        l.addi r1, r1, 64
        l.jr r9
        l.nop 0 */

int write(int, char*, int) _Pragma("PunyC emit \x9c\x21\xff\xc0\xd4\x01\x60\x0c\xd4\x01\x68\x10\xd4\x01\x78\x14\xd4\x01\x88\x18\xd4\x01\x98\x1c\xd4\x01\xa8\x20\xd4\x01\xb8\x24\xd4\x01\xc8\x28\xd4\x01\xd8\x2c\xd4\x01\xe8\x30\xd4\x01\xf8\x34\xa9\x60\x00\x40\x20\x00\x00\x00\xe0\x6b\x58\x04\x85\x81\x00\x0c\x85\xa1\x00\x10\x85\xe1\x00\x14\x86\x21\x00\x18\x86\x61\x00\x1c\x86\xa1\x00\x20\x86\xe1\x00\x24\x87\x21\x00\x28\x87\x61\x00\x2c\x87\xa1\x00\x30\x87\xe1\x00\x34\x9c\x21\x00\x40\x44\x00\x48\x00\x15\x00\x00\x00");
    /*  l.addi r1, r1, -64
        l.sw 12(r1), r12
        l.sw 16(r1), r13
        l.sw 20(r1), r15
        l.sw 24(r1), r17
        l.sw 28(r1), r19
        l.sw 32(r1), r21
        l.sw 36(r1), r23
        l.sw 40(r1), r25
        l.sw 44(r1), r27
        l.sw 48(r1), r29
        l.sw 52(r1), r31

        l.ori r11, r0, 64       # sys_write
        l.sys 0
        l.or r3, r11, r11

        l.lwz r12, 12(r1)
        l.lwz r13, 16(r1)
        l.lwz r15, 20(r1)
        l.lwz r17, 24(r1)
        l.lwz r19, 28(r1)
        l.lwz r21, 32(r1)
        l.lwz r23, 36(r1)
        l.lwz r25, 40(r1)
        l.lwz r27, 44(r1)
        l.lwz r29, 48(r1)
        l.lwz r31, 52(r1)
        l.addi r1, r1, 64
        l.jr r9
        l.nop 0 */

/*
int read(int, char*, int) _Pragma("PunyC emit \x9c\x21\xff\xc0\xd4\x01\x60\x0c\xd4\x01\x68\x10\xd4\x01\x78\x14\xd4\x01\x88\x18\xd4\x01\x98\x1c\xd4\x01\xa8\x20\xd4\x01\xb8\x24\xd4\x01\xc8\x28\xd4\x01\xd8\x2c\xd4\x01\xe8\x30\xd4\x01\xf8\x34\xa9\x60\x00\x3f\x20\x00\x00\x00\xe0\x6b\x58\x04\x85\x81\x00\x0c\x85\xa1\x00\x10\x85\xe1\x00\x14\x86\x21\x00\x18\x86\x61\x00\x1c\x86\xa1\x00\x20\x86\xe1\x00\x24\x87\x21\x00\x28\x87\x61\x00\x2c\x87\xa1\x00\x30\x87\xe1\x00\x34\x9c\x21\x00\x40\x44\x00\x48\x00\x15\x00\x00\x00");
*/
    /*  l.addi r1, r1, -64
        l.sw 12(r1), r12
        l.sw 16(r1), r13
        l.sw 20(r1), r15
        l.sw 24(r1), r17
        l.sw 28(r1), r19
        l.sw 32(r1), r21
        l.sw 36(r1), r23
        l.sw 40(r1), r25
        l.sw 44(r1), r27
        l.sw 48(r1), r29
        l.sw 52(r1), r31

        l.ori r11, r0, 63       # sys_read
        l.sys 0
        l.or r3, r11, r11

        l.lwz r12, 12(r1)
        l.lwz r13, 16(r1)
        l.lwz r15, 20(r1)
        l.lwz r17, 24(r1)
        l.lwz r19, 28(r1)
        l.lwz r21, 32(r1)
        l.lwz r23, 36(r1)
        l.lwz r25, 40(r1)
        l.lwz r27, 44(r1)
        l.lwz r29, 48(r1)
        l.lwz r31, 52(r1)
        l.addi r1, r1, 64
        l.jr r9
        l.nop 0 */



