The "assemble.sh" shell script provided is used for assembling and linking an
assembly program. It takes as input a name (which we will refer to with $1),
and it requires a linker script named "$1.ld" as well as an assembly file
named "$1.s" to both exist. It will then use the assembly source file and
linker script to generate an object file ("$1.o") and an ELF file ("$1.elf").
Note that, for the object file and ELF file, if files with these names already
exist, it will overwrite them.

Example Usage:

source assemble.sh syscall_test/syscall_test

The above command would require syscall_test/syscall_test.s as well as
syscall_test/syscall_test.ld to both exist. It would then use these to create
syscall_test/syscall_test.o and syscall_test/syscall_test.elf.

