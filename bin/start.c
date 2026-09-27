#include <stdio.h>
#include <unistd.h>

int main(void)
{
    puts("Eu sou o programa customizado!");
    puts("Vou limpar o stdout em 5 segundos...!\n");
    sleep(5);
    printf("\033[2J\033[H");
    fflush(stdout);
    puts("Limpei!");
    return 0;
}
