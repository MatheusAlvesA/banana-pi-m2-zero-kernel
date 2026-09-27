#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <unistd.h>

static void mount_virtual_fs(const char *path, const char *type, mode_t mode)
{
    if (mkdir(path, mode) == -1 && errno != EEXIST) {
        perror(path);
        return;
    }

    if (mount(type, path, type, 0, NULL) == -1 && errno != EBUSY)
        perror(path);
}

static void init_base_virtual_fs(void)
{
    /* O kernel monta a raiz como somente leitura por padrao. */
    if (mount(NULL, "/", NULL, MS_REMOUNT, NULL) == -1) {
        perror("remount /");
    }

    mount_virtual_fs("/proc", "proc", 0755);
    mount_virtual_fs("/sys", "sysfs", 0755);
    mount_virtual_fs("/dev", "devtmpfs", 0755);
    mount_virtual_fs("/tmp", "tmpfs", 01777);
}

static int read_number(const char *path, long *value)
{
    FILE *file = fopen(path, "r");
    int success;

    if (file == NULL) {
        return 0;
    }

    success = fscanf(file, "%ld", value) == 1;
    fclose(file);
    return success;
}

static void print_cpu(void)
{
    FILE *file = fopen("/proc/cpuinfo", "r");
    char line[512];
    int found = 0;

    if (file == NULL) {
        perror("/proc/cpuinfo");
        puts("CPU: indisponivel");
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        char *colon = strchr(line, ':');
        char *value;
        size_t key_length;
        if (colon == NULL)
            continue;
        // Cortando a string, apenas o que tem antes do : importa para determinar label
        *colon = '\0';
        // Removendo espaços em branco e tabs
        key_length = strlen(line);
        while (
                key_length > 0 &&
               (line[key_length - 1] == ' ' || line[key_length - 1] == '\t')
            ) {
            line[--key_length] = '\0';
        }
        // Se não é a label que procuramos, pule
        if (strcmp(line, "Processor") != 0 && strcmp(line, "model name") != 0)
            continue;
        // Encontramos a label correta, obtendo valor
        value = colon + 1;
        value += strspn(value, "\t ");
        value[strcspn(value, "\r\n")] = '\0';
        if (*value != '\0') {
            printf("CPU: %s\n", value);
            found = 1;
            break;
        }
    }
    fclose(file);

    if (!found)
        puts("CPU: indisponivel");
}

static void print_hardware_info(void)
{
    struct utsname kernel;
    struct sysinfo memory;
    long value;
    long cores = sysconf(_SC_NPROCESSORS_ONLN);

    if (uname(&kernel) == 0) {
        printf("Arquitetura: %s\n", kernel.machine);
        printf("Kernel: %s %s\n", kernel.sysname, kernel.release);
    } else {
        perror("uname");
    }

    print_cpu();
    printf("Nucleos online: %ld\n", cores);

    if (read_number("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", &value)
        && value > 0)
        printf("Frequencia da CPU (cpu0): %.1f MHz\n", value / 1000.0);
    else
        puts("Frequencia da CPU: indisponivel");

    if (sysinfo(&memory) == 0) {
        double unit_mib = memory.mem_unit / (1024.0 * 1024.0);
        printf("RAM reconhecida pelo kernel: %.1f MiB\n",
               memory.totalram * unit_mib);
        printf("RAM livre (exclui cache): %.1f MiB\n",
               memory.freeram * unit_mib);
    } else {
        perror("sysinfo");
    }
    puts("------------------------------\n");
}

static void print_random_number(void)
{
    FILE *file = fopen("/dev/random", "rb");
    unsigned int number;

    if (file == NULL) {
        perror("/dev/random");
        return;
    }

    if (fread(&number, sizeof(number), 1, file) == 1)
        printf("Numero aleatorio: %u\n", number);
    else
        fputs("Falha ao ler /dev/random\n", stderr);

    fclose(file);
}

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);
    puts("Seu Banana Pi completou o boot do Kernel Linux!");

    init_base_virtual_fs();
    print_hardware_info();
    print_random_number();

    for (;;) {
        pause();
    }
}
