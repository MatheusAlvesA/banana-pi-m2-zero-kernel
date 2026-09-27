#include <unistd.h>

int main(void)
{
    const char msg[] = "Seu Banana pi, completou o boot do Kernel Linux!\n";

    auto _ = write(STDOUT_FILENO, msg, sizeof(msg) - 1);

    for (;;) {
        pause();
    }
}