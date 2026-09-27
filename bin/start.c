#include <stdio.h>
#include <unistd.h>

#define LED_PATH "/sys/class/leds/bananapi-m2-zero:red:pwr"

static int write_led(const char *path, const char *value)
{
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        perror(path);
        return -1;
    }
    if (fputs(value, file) == EOF) {
        perror(path);
        fclose(file);
        return -1;
    }
    /* fclose tambem verifica a escrita dos dados armazenados no buffer. */
    if (fclose(file) == EOF) {
        perror(path);
        return -1;
    }
    return 0;
}

int main(void)
{
    puts("Eu sou o programa customizado!");
    puts("Vou limpar o stdout em 5 segundos...!\n");
    sleep(5);
    printf("\033[2J\033[H");
    fflush(stdout);
    puts("Limpei!");

    /* Desativa qualquer controle automatico antes de alternar o brilho. */
    if (write_led(LED_PATH "/trigger", "none\n") == -1)
        return 1;

    puts("Piscando o LED: 1 segundo aceso, 1 segundo apagado.");
    fflush(stdout);
    for (;;) {
        /* O driver trata a polaridade ativa em nivel baixo do pino PL10. */
        if (write_led(LED_PATH "/brightness", "1\n") == -1)
            return 1;
        sleep(1);
        if (write_led(LED_PATH "/brightness", "0\n") == -1)
            return 1;
        sleep(1);
    }
}
