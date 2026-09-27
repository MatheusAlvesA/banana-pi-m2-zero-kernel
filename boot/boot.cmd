# ENV Config
setenv verbosity "4"
setenv rootfstype "ext4"
setenv devnum "0"

echo "Iniciando Boot Script!"

# Carregando id da partição, numero e caminho da partição de boot
part uuid mmc ${devnum}:1 partuuid;
setenv devnum ${mmc_bootdev}
setenv rootdev "/dev/mmcblk${mmc_bootdev}p1"

setenv consoleargs "console=ttyS0,115200 console=tty1"
setenv bootargs "root=${rootdev} rootwait rootfstype=${rootfstype} ${consoleargs} consoleblank=0 loglevel=${verbosity} ubootpart=${partuuid} ubootsource=${devtype} cma=128M"

echo "Carregando Kernel em memória..."
load ${devtype} ${devnum} ${kernel_addr_r} ${prefix}zImage

echo "Carregando Device tree"
load ${devtype} ${devnum} ${fdt_addr_r} ${prefix}dtb/${fdtfile}
fdt addr ${fdt_addr_r}

# Capacidade relativa igual nos quatro Cortex-A7 (1024 nao e frequencia).
fdt resize 0x100 || exit
setenv dt_cpu_index
for dt_cpu_index in 0 1 2 3; do
    fdt set /cpus/cpu@${dt_cpu_index} capacity-dmips-mhz <0x400> || exit
done

echo "Iniciando boot do Kernel..."
bootz ${kernel_addr_r} - ${fdt_addr_r}
