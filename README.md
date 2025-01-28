# dingOS


______:
$ make qemu
to exit
ctrl + a then x 

gdb stuff: 

$ make clean debug
then run ⬇ in another terminal
$ /u/hill/Coursework/CS378/tools/arm-gnu-toolchain/bin/aarch64-none-elf-gdb -x gdb-init-script.gdb kernel/build/kernel.elf

