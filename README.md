# dingOS

## About

A distributed OS with a microkernel design that aims to emulate Linux. dingOS is event-driven and binary-compatible.

Written in C++ and uses ARM64 architecture. We will run it on a Raspberry Pi 3B. We will have access to two.

## Build

Using the Makefile, to build with no debug print statements:

```sh
make qemu
```

With debug print statements:

```sh
make qemu DEBUG_ENABLED=1
```

To clean the build:

```sh
make clean
```

To quit QEMU, `Ctrl` + `A`, then `X`.

## Logfile

To specify a logfile, use the argument `QEMU_LOG=kernel_output.log`, for example:

```sh
make qemu DEBUG_ENABLED=1 QEMU_LOG=output.log
```

## Debug

Use the Make target

```sh
make debug
```

and follow the instructions from there.

To help simplify debugging, use `debug.sh`, which will build the kernel with debug symbols and attach the GDB script, `gdb-init-script.gdb`.

The script `run_until_unexpected.sh` will continuously run the kernel until the last line of kernel output is different than expected. You will need to change the variables at the top of the script to your desired values.
