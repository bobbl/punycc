#!/bin/sh

or1k-elf-as host.s -o host.o
or1k-elf-objdump -d host.o > tmp.dump

sed -n -e 's/....:.\(..\) \(..\) \(..\) \(..\).*/\\x\1\\x\2\\x\3\\x\4/p;s/<\(.*\)>:/\1/p' \
    tmp.dump > tmp.words

tr -d '\n' < tmp.words
