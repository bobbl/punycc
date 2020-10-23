#!/bin/sh

QEMU_RV32=${QEMU_RV32:-qemu-riscv32}
QEMU_ARM=${QEMU_ARM:-qemu-arm}


if [ $# -eq 0 ] 
then
    echo "Usage: $0 <action> ..."
    echo
    echo "  x86         Set ISA to Intel x86 32 bit (default)"
    echo "  rv32        Set ISA to RISC-V RV32IM"
    echo "  armv6m      Set ISA to ARMv6-M (minimal thumb ISA of Cortex-M0)"
    echo
    echo "  clang       Use clang to build cross compiler for current ISA (host x86)"
    echo "  gcc         Use gcc to build cross compiler for current ISA (host x86)"
    echo "  test-self   Build compiler for ISA"
    echo "  test-tox86  Use ISA compiler to build cross compiler from ISA to x86"
    echo "  test-cc500  Compile cc500 with cross compiler and check against original"
    echo
    echo "  disasm <elf>        Disassemble raw ELF with current ISA"
    echo "  asm-riscv <code>    Assemble to rv32 machine code"
    echo "                      Surround by \" and separate commands by ;"
    echo
    echo "Use 'QEMU_RV32=path/qemu-riscv32 $0' if qemu is not in the search path"
    echo "    'QEMU_ARM=path/qemu-arm $0'"
    exit 1
fi



qemu () {
    case $arch in
        x86)    ;;
        rv32)   echo "$QEMU_RV32" ;;
        armv6m) echo "$QEMU_ARM" ;;
    esac
}



arch="x86"

mkdir -p build
cd build

while [ $# -ne 0 ]
do
    case $1 in
        x86)    arch="x86" ;;
        rv32)   arch="rv32" ;;
        armv6m) arch="armv6m" ;;

        clang)
            cat ../host_$arch.c ../emit_$arch.c ../punycc.c > punycc_$arch.c
            clang -O2 -o punycc_$arch.clang punycc_$arch.c
            ;;

        gcc)
            cat ../host_$arch.c ../emit_$arch.c ../punycc.c > punycc_$arch.c
            gcc -O2 -o punycc_$arch.clang punycc_$arch.c
            ;;

        test-self)
            # use the clang cross compiler to build a compiler for the current
            # arch
            cat ../host_$arch.c ../emit_$arch.c ../punycc.c > punycc_$arch.$arch.c
            ./punycc_$arch.clang < punycc_$arch.$arch.c > punycc_$arch.$arch
            errorlevel=$?
            if [ $errorlevel -ne 0 ]
            then
                echo "Error $errorlevel"
                exit $errorlevel
            fi
            chmod 775 punycc_$arch.$arch
            echo $arch compiler size: $(wc -c < punycc_$arch.$arch)
            ;;

        test-tox86)
            # use the compiler for the current arch to compile a cross compiler
            # from this arch to x86
            cat ../host_$arch.c ../emit_x86.c ../punycc.c > punycc_tox86.$arch.c
            $(qemu) ./punycc_$arch.$arch < punycc_tox86.$arch.c > punycc_tox86.$arch
            errorlevel=$?
            if [ $errorlevel -ne 0 ]
            then
                echo "Error $errorlevel"
                exit $errorlevel
            fi
            chmod 775 punycc_tox86.$arch
            echo cross compiler $arch to x86 size: $(wc -c < punycc_tox86.$arch)
            ;;

        test-cc500)
            # use the compiled binary to compile the modified original
            cat ../host_x86.c ../cc500/cc500_mod.c > tmp.c
            $(qemu) ./punycc_tox86.$arch < tmp.c > cc500_mod.x86
            echo "-------------------------------------------------------------"

            # use the compiled (modified) original to compile the original
            chmod 775 cc500_mod.x86
            ./cc500_mod.x86 < ../cc500/cc500.c > cc500.cc500
            cmp cc500.cc500 ../cc500/cc500.correct.x86
            echo original size: $(wc -c < cc500_mod.x86) instead of 16458
            ;;

        disasm)
            cd ..
            case $arch in
                x86)    objdump -b binary -m i386 -M intel -D "$2"
                        ;;
                rv32)   riscv64-linux-gnu-objdump -b binary -m riscv -D "$2"
                        ;;
                armv6m) arm-none-eabi-objdump -b binary -m arm -M force-thumb -D "$2"
                        ;;
            esac
            shift
            cd build
            ;;

        asm-riscv)
            echo "$2" | riscv64-linux-gnu-as - -o tmp.elf
            riscv64-linux-gnu-objdump -Mnumeric -d tmp.elf | tail -n +8 > tmp.dump
            cat tmp.dump
            sed -n -e 's/....:.\(..\)\(..\)\(..\)\(..\).*/\\x\4\\x\3\\x\2\\x\1/p' \
                tmp.dump | tr -d '\n'
            echo
            printf "%d\n" 0x$(cut -f 2 tmp.dump)
            rm tmp.elf tmp.dump
            shift
            ;;

        asm-riscv-file)
            cat "$2" | riscv64-linux-gnu-as - -o tmp.elf
            riscv64-linux-gnu-objdump -Mnumeric -d tmp.elf | tail -n +8 > tmp.dump
            cat tmp.dump
            sed -n -e 's/....:.\(..\)\(..\)\(..\)\(..\).*/\\x\4\\x\3\\x\2\\x\1/p' \
                tmp.dump | tr -d '\n'
            echo
            rm tmp.elf tmp.dump
            shift
            ;;

        *)
            echo "Unknown target $1. Stop."
            exit 1
            ;;
    esac
    echo "============================================================="
    shift
done

cd ..


# SPDX-License-Identifier: ISC

