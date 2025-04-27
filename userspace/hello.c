#include <unistd.h>
#include <string.h>

int main(void)
{
    const char msg[] = "hello world\n";
    write(STDOUT_FILENO, msg, sizeof(msg)-1);
    _exit(0);
}
