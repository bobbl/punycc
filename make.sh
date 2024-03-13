#!/bin/sh


help () {
    echo "Usage: $0 <action> ..."
    echo
    echo "  x86         Set ISA to Intel x86 32 bit (default)"
    echo "  rv32        Set ISA to RISC-V RV32IM"
    echo "  armv6m      Set ISA to ARMv6-M (minimal thumb ISA of Cortex-M0)"
    echo
    echo "  clang       Use clang to build cross compiler for current ISA (host x86)"
    echo "  gcc         Use gcc to build cross compiler for current ISA (host x86)"
    echo "  test_self   Build compiler for ISA"
    echo "  test_tox86  Use ISA compiler to build cross compiler from ISA to x86"
    echo "  test_cc500  Compile cc500 with cross compiler and check against original"
    echo
    echo "  all         Compile all targets with gcc, then all cross combinations"
    echo "  stats       Binary sizes of all self-compiled cross compilers"
    echo
    echo "  disasm <elf>        Disassemble raw ELF with current ISA"
    echo "  asm-riscv <code>    Assemble to rv32 machine code"
    echo "                      Surround by \" and separate commands by ;"
    echo
    echo "Use 'QEMU_RV32=path/qemu-riscv32 $0' if qemu is not in the search path"
    echo "    'QEMU_ARM=path/qemu-arm $0'"
}

if [ $# -eq 0 ] 
then
    help
    exit 1
fi




QEMU_RV32=${QEMU_RV32:-qemu-riscv32}
QEMU_ARM=${QEMU_ARM:-qemu-arm}

arch_list="armv6m	rv32	x86	wasm"



qemu () {
    case $arch in
        x86)    ;;
        rv32)   echo "$QEMU_RV32" ;;
        armv6m) echo "$QEMU_ARM" ;;
        wasm)   echo "wasmtime" ;;
    esac
}



# build punycc with the compiler given as parameter
compile () {
    cat ../host_$arch.c ../emit_$arch.c ../punycc.c > punycc_$arch.c
    $1 -O2 -o punycc_$arch.clang punycc_$arch.c
}



# use the clang cross compiler to build a compiler for the current arch
test_self () {
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
}



# use the compiler for the current arch to compile a cross compiler
# from this arch to x86
test_tox86 () {
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
}



test_cc500 () {
    # use the compiled binary to compile the modified original
    cat ../host_x86.c ../cc500/cc500_mod.c > tmp.c
    $(qemu) ./punycc_tox86.$arch < tmp.c > cc500_mod.x86
    echo "-------------------------------------------------------------"

    # use the compiled (modified) original to compile the original
    chmod 775 cc500_mod.x86
    ./cc500_mod.x86 < ../cc500/cc500.c > cc500.cc500
    cmp cc500.cc500 ../cc500/cc500.correct.x86
    echo original size: $(wc -c < cc500_mod.x86) instead of 16458
}



test_all () {
    for arch in $arch_list
    do
        compile clang
        echo "============================================================="
        test_self
        echo "============================================================="
        test_tox86
        echo "============================================================="
        test_cc500
        echo "============================================================="
    done
}



all () {
    for host in $arch_list
    do
        cat ../host_$host.c ../emit_$host.c ../punycc.c > punycc_$host.c
        gcc -O2 -o punycc_$host.gcc punycc_$host.c

        for target in $arch_list
        do
            cat ../host_$host.c ../emit_$target.c ../punycc.c > punycc_$target.$host.c
            $(qemu) ./punycc_$host.gcc < punycc_$target.$host.c > punycc_$target.$host
        done
    done
}



stats () {
    all
    echo "        hosts"
    echo "target  $arch_list"
    for target in $arch_list
    do
        printf "$target\t"
        for host in $arch_list
        do
            filesize=$(stat -c '%s' punycc_$target.$host)
            printf "$filesize\t"
        done
        echo
    done
}



arch="x86"

mkdir -p build
cd build

while [ $# -ne 0 ]
do
    case $1 in
        help)           help ;;

        x86)            arch="x86" ;;
        rv32)           arch="rv32" ;;
        armv6m)         arch="armv6m" ;;
        wasm)           arch="wasm" ;;

        clang)          compile clang ;;
        gcc)            compile gcc ;;

        test_self)      test_self ;;
        test_tox86)     test_tox86 ;;
        test_cc500)     test_cc500 ;;
        test_all)       test_all ;;

        all)            all ;;
        stats)          stats ;;

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

        asm_riscv)
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

        asm_riscv_file)
            cat "$2" | riscv64-linux-gnu-as - -o tmp.elf
            riscv64-linux-gnu-objdump -Mnumeric -d tmp.elf | tail -n +8 > tmp.dump
            cat tmp.dump
            sed -n -e 's/....:.\(..\)\(..\)\(..\)\(..\).*/\\x\4\\x\3\\x\2\\x\1/p' \
                tmp.dump | tr -d '\n'
            echo
            rm tmp.elf tmp.dump
            shift
            ;;

        clean)
            rm -f *
            ;;

        *)
            echo "Unknown target $1. Stop."
            exit 1
            ;;
    esac
    shift
done

cd ..


# SPDX-License-Identifier: ISC

