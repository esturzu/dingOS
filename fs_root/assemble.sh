#!/bin/bash
SCRIPT_GNU_PATH="/u/gheith/public/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin"
"${SCRIPT_GNU_PATH}/aarch64-none-linux-gnu-as" $1.s -o $1.o
"${SCRIPT_GNU_PATH}/aarch64-none-linux-gnu-ld" -T $1.ld $1.o -o $1.elf
