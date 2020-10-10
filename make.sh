#!/bin/sh

HOSTS="x86 rv32"
TARGETS="x86 rv32"

QEMU_RV32=${QEMU_RV32:-qemu-riscv32}


if [ $# -eq 0 ] 
then
    echo "Usage: $0 <action> ..."
    echo
    echo "  clang                       Compile all targets with clang (host x86)"
    echo "  test-cc500                  Compile original with compiled oroginal"
    echo "  clang test-x86 test-cc500   Compile punycc_x86 with clang"
    echo "                              Compile punycc_x86 with punycc_x86 to self"
    echo "                              Compile cc500mod with self"
    echo "                              Compile cc500 with cc500mod and check against original"
    echo "  clang test-rv32 test-cc500  Compile punycc_rv32 with clang"
    echo "                              Compile punycc_x86 with punycc_rv32 to self"
    echo "                              Compile cc500mod with self"
    echo "                              Compile cc500 with cc500mod and check against original"
    echo
    echo "  disasm-x86 <elf>            Disassemble raw x86 ELF"
    echo "  disasm-rv32 <elf>           Disassemble raw rv32 ELF"
    echo "  asm-riscv <code>            Assemble to rv32 machine code"
    echo "                              Surround by \" and separate commands by ;"
    echo
    echo "Use 'QEMU_RV32=path/qemu-riscv32 $0' if qemu is not in the search path"
    echo
    echo "Supported targets: $TARGETS"
    exit 1
fi




while [ $# -ne 0 ]
do
    case $1 in
        clang)
            mkdir -p build
            cd build
            for target in $TARGETS
            do
                cat ../host_$target.c ../emit_$target.c ../punycc.c > punycc_$target.c
                clang -O2 -o punycc_$target.clang punycc_$target.c
            done
            cd ..
            ;;

        test-x86)
            mkdir -p build
            cd build
            ./punycc_x86.clang < punycc_x86.c > punycc_x86.x86
            errorlevel=$?
            if [ $errorlevel -ne 0 ]
            then
                echo "Error $errorlevel"
                exit $errorlevel
            fi
            echo "-------------------------------------------------------------"

            # use the compiled binary to compile the modified original
            chmod 775 punycc_x86.x86
            cat ../host_x86.c ../cc500/cc500_mod.c | ./punycc_x86.x86 > cc500_mod.x86
            echo compiler size: $(wc -c < punycc_x86.x86)
            cd ..
            ;;

        test-rv32)
            mkdir -p build
            cd build

            # use the compiled rv32 binary to compile itself to rv32
            cat ../host_rv32.c ../emit_rv32.c ../punycc.c > punycc_rv32.rv32.c
            ./punycc_rv32.clang < punycc_rv32.rv32.c > punycc_rv32.rv32
            errorlevel=$?
            if [ $errorlevel -ne 0 ]
            then
                echo "Error $errorlevel"
                exit $errorlevel
            fi
            echo rv32 compiler size: $(wc -c < punycc_rv32.rv32)
            echo "-------------------------------------------------------------"

            cat ../host_rv32.c ../emit_x86.c ../punycc.c > punycc_x86.rv32.c
#            ./punycc_rv32.clang < punycc_x86.rv32.c > punycc_x86.rv32
            chmod 775 punycc_rv32.rv32
            $QEMU_RV32 punycc_rv32.rv32 < punycc_x86.rv32.c > punycc_x86.rv32
            errorlevel=$?
            if [ $errorlevel -ne 0 ]
            then
                echo "Error $errorlevel"
                exit $errorlevel
            fi
            echo x86 compiler size: $(wc -c < punycc_x86.rv32)
            echo "-------------------------------------------------------------"

            # use the compiled binary to compile the modified original
            chmod 775 punycc_x86.rv32
            cat ../host_x86.c ../cc500/cc500_mod.c \
                | $QEMU_RV32 punycc_x86.rv32 > cc500_mod.x86
            cd ..
            ;;

        test-cc500)
            # use the compiled (modified) original to compile the original
            mkdir -p build
            cd build
            chmod 775 cc500_mod.x86
            ./cc500_mod.x86 < ../cc500/cc500.c > cc500.cc500
            cmp cc500.cc500 ../cc500/cc500.correct.x86

            echo original size: $(wc -c < cc500_mod.x86) instead of 16458
            cd ..
            ;;

        disasm-x86)
            objdump -b binary -m i386 -M intel -D "$2"
            shift
            ;;

        disasm-rv32)
            riscv64-linux-gnu-objdump -b binary -m riscv -D "$2"
            shift
            ;;

        asm-riscv)
            echo "$2" | riscv64-linux-gnu-as - -o tmp.elf
            riscv64-linux-gnu-objdump -Mnumeric -d tmp.elf | tail -n +8 > tmp.dump
            cat tmp.dump
            sed -n -e 's/....:.\(..\)\(..\)\(..\)\(..\).*/\\x\4\\x\3\\x\2\\x\1/p' tmp.dump \
                | tr -d '\n'
            echo
            printf "%d\n" 0x$(cut -f 2 tmp.dump)
            rm tmp.elf tmp.dump
            shift
            ;;

        asm-riscv-file)
            cat "$2" | riscv64-linux-gnu-as - -o tmp.elf
            riscv64-linux-gnu-objdump -Mnumeric -d tmp.elf | tail -n +8 > tmp.dump
            cat tmp.dump
            sed -n -e 's/....:.\(..\)\(..\)\(..\)\(..\).*/\\x\4\\x\3\\x\2\\x\1/p' tmp.dump \
                | tr -d '\n'
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



# SPDX-License-Identifier: ISC

