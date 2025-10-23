# void exit(int) _Pragma("PunyC emit \xa9\x60\x00\x5d\x20\x00\x00\x00");

exit:
        l.ori r11, r0, 93       # sys_exit
        l.sys 0



# int getchar(void) _Pragma("PunyC emit \x13\x01\xc1\xff\x13\x05\x00\x00\x93\x05\x01\x00\x13\x06\x10\x00\x93\x08\xf0\x03\x73\x00\x00\x00\x93\x05\x05\x00\x03\x45\x01\x00\x13\x01\x41\x00\x63\x44\xb0\x00\x13\x05\xf0\xff\x67\x80\x00\x00");
getchar:
        l.addi r1, r1, -64
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
        l.nop 0




# void *malloc(unsigned long) _Pragma("PunyC emit \x13\x01\x41\xff\x23\x24\xa1\x00\x13\x05\x00\x00\x93\x08\x60\x0d\x73\x00\x00\x00\x23\x20\xa1\x00\x83\x28\x81\x00\x33\x05\x15\x01\x23\x22\xa1\x00\x93\x08\x60\x0d\x73\x00\x00\x00\x83\x28\x41\x00\x63\x08\x15\x01\x13\x05\x00\x00\x13\x01\xc1\x00\x67\x80\x00\x00\x03\x25\x01\x00\x13\x01\xc1\x00\x67\x80\x00\x00");
malloc:
        l.addi r1, r1, -64
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
        l.nop 0



# int write(int, char*, int) _Pragma("PunyC emit \x93\x08\x00\x04\x73\x00\x00\x00\x67\x80\x00\x00");
write:
        l.addi r1, r1, -64
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
        l.nop 0





# int read(int, char*, int) _Pragma("PunyC emit \x93\x08\xf0\x03\x73\x00\x00\x00\x67\x80\x00\x00");
read:
        l.addi r1, r1, -64
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
        l.nop 0
