#include "kernel.h"

int testMain()
{
	int test = 1;
	int test2 = 2;
	int test3 = test * test2;
	return test3;
}

extern "C" void kernelMain()
{
	while (1) {
		testMain();
		}
}
