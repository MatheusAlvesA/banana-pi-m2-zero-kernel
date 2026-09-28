# Banana Pi M2 Zero até o Kernel

O objetivo desse repositório ensinar sobre os requisitos e a stack de softwares necessários para  
realizar o boot do Kernel linux na placa Banana Pi M2 Zero. Esse tutorial assume que você está usando uma distro linux com apt, como o Ubuntu.

# U-Boot

O bootloader utilizado é o [U-Boot](https://docs.u-boot-project.org/en/v2026.07/build/index.html)  
Um dos bootloaders mais populares em SoCs arm.

Faça download do código via git:  
```sh
git clone https://source.denx.de/u-boot/u-boot.git
```

Selecione a branch de um release recente do u-boot, nesse tutorial vamos usar a 2026.07
```sh
cd u-boot
git checkout v2026.07
```

Para fazer a build, certifique-se que seu sistema tem os requisitos da build
```sh
sudo apt-get install bc bison build-essential coccinelle \
  device-tree-compiler dfu-util efitools flex gdisk graphviz imagemagick \
  libgnutls28-dev libguestfs-tools libncurses-dev \
  libpython3-dev libsdl2-dev libssl-dev lz4 lzma lzma-alone openssl \
  pkg-config python3 python3-asteval python3-coverage python3-filelock \
  python3-pkg-resources python3-pycryptodome python3-pyelftools \
  python3-pytest python3-pytest-xdist python3-sphinxcontrib.apidoc \
  python3-sphinx-rtd-theme python3-subunit python3-testtools \
  python3-venv swig uuid-dev
```

Com o sistema pronto, configure a build para executar específicamente a build da placa Banana Pi  
U-Boot tem suporte nativo a placa. Execute:
```sh
make bananapi_m2_zero_defconfig
```

Para compilar o projeto, é necessário o cross compilador para a arquitetura ARM
```sh
sudo apt install gcc-arm-linux-gnueabihf \
    build-essential bison flex libssl-dev \
    device-tree-compiler python3-pyelftools swig
```

Agora a build do projeto:
```sh
make CROSS_COMPILE=arm-linux-gnueabihf- -j$(nproc)
```

Após compilado, o arquivo `u-boot-sunxi-with-spl.bin` contém nosso bootloadter já pronto para executar.  

Vamos formatar, e deixar pronto o SDCard para receber nosso boot loader:
```sh
sudo parted --script /dev/sdX \
  mklabel msdos \
  mkpart primary ext4 100MiB 100% \
  set 1 boot on

sudo partprobe /dev/sdX
sudo udevadm settle
sudo mkfs.ext4 -L main /dev/sdXp1

sudo mount -o remount,rw /dev/sdXp1
sudo chown user:user /run/media/user/main
```
Substitua o `sdX` pela label do dispositivo na sua máquina. e `user` pelo seu usuário.  
Esse processo deve fazer com que seu SDCard agora tenha cerca de 100MB não utilizados no inicio, seguido de uma única partição bootavel ext4.  
Confira se ficou correto, experimente executar os comandos 1 a 1 para garantir.  

O processador do Banana Pi, Allwinner H3, procura pelo bootloader no SDCard,  
específicamente no endereço 8000, ou seja a partir do oitavo kilo byte.  
Vamos gravar o bootloader nesse endereço diretamente, por isso reservamos os 100mb.  
```sh
sudo dd if=u-boot-sunxi-with-spl.bin \
    of=/dev/sdX \
    bs=1k seek=8
```

Agora copie a pasta `boot` desse projeto para a partição `main` do SDCard  
essa pasta contém a device tree e também o kernel linux compilado.  
Também copie a pasta `sbin` desse projeto para a partição `main` do SDCard  
essa pasta contém o programa que o Kernel linux vai executar.  

Com isso feito, basta colocar o SDCard e observe a placa dar boot no Kernel linux.

# Boot Script

O arquivo `boot/boot.scr` é versão compilada de `boot/boot.cmd`, um script executado pelo U-boot automaticamente para executar o boot do Kernel.  
Se deseja customizar isso, edite o `boot.cmd` e execute:
```sh
./u-boot/tools/mkimage -C none -A arm -T script -d boot/boot.cmd boot/boot.scr
```
Com isso, o script de boot agora é a versão customizada que você criou

A raiz é selecionada com `root=PARTUUID=...`, obtido da primeira partição
do dispositivo usado pelo U-Boot. Isso evita depender da correspondência
entre os números MMC do U-Boot e do Linux. O projeto mantém `/boot` e
`/sbin/init` nessa mesma partição.

O script também informa `capacity-dmips-mhz = <1024>` para os quatro
Cortex-A7 da Banana Pi M2 Zero no device tree em memória. Esse valor representa
capacidade relativa igual entre os núcleos, não uma frequência em MHz.

Para a GPU, o script liga `mali-supply` ao regulador fixo `vcc1v2` e informa
`opp-microvolt = <1200000>` nos quatro pontos de operação. Isso corrige a
mensagem do Lima `error -ENODEV: _opp_set_regulators: no regulator (mali) found`
sem desabilitar o controle de frequência da GPU. A alimentação de 1,2 V está
descrita no [esquema da placa, página 3](https://linux-sunxi.org/images/2/2f/BPi-M2-Zero-schematic_V1_0-R.pdf).

# O Kernel

Essa etapa é opcional, pois a pasta `boot/` do projeto já contém uma imagem do kernel e a device tree. Para compilar sua própria versão, baixe o código do Linux a partir da raiz deste projeto e configure a arquitetura ARM:

```sh
git clone https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
cd linux/
git checkout v7.2
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabihf-
make sunxi_defconfig
```

Para exibir as mensagens do kernel pelo HDMI, habilite a emulação de framebuffer do DRM e o console de texto:

```sh
./scripts/config --enable DRM_FBDEV_EMULATION \
                 --enable FB_DEVICE \
                 --enable FRAMEBUFFER_CONSOLE \
                 --enable DRM_CLIENT_DEFAULT_FBDEV

make olddefconfig
```

Agora compile o kernel e a device tree da placa, mantendo as variáveis `ARCH` e `CROSS_COMPILE` exportadas acima. Não execute `make sunxi_defconfig` novamente depois dos ajustes, pois isso substituiria a configuração:

```sh
make -j"$(nproc)" zImage allwinner/sun8i-h2-plus-bananapi-m2-zero.dtb
```

Ainda dentro da pasta `linux/`, copie os arquivos gerados para a pasta `boot/` do projeto:

```sh
cp arch/arm/boot/zImage ../boot/zImage
cp arch/arm/boot/dts/allwinner/sun8i-h2-plus-bananapi-m2-zero.dtb ../boot/dtb/
```

Esses comandos substituem a imagem e a device tree fornecidas no projeto. Depois, copie os arquivos atualizados para os mesmos locais no SDCard usados pelo script de boot.


# Init Program

O Kernel busca por um executável em /sbin/init, se achar executa ele com PDID=1, esse programa nunca pode retornar, se o fizer vai causar Kernel Panic.  
Você pode editar e recompilar o init desse projeto, para isso entre na pasta `sbin` modifique o init.c e execute:
```sh
arm-linux-gnueabihf-gcc -static -Os -s init.c -o init
```
Perceba que o `-static` está presente, ele é necessário, já que nosso sistema de arquivos não tem nenhuma lib disponível para usar.


# Start Program

Na pasta `bin` existe um programa chamado start.c. O programa init automaticamente procura por um executável em /bin/start e o executa se encontrar.  
Dessa forma, se quiser escrever um programa customizado sem precisar lidar com a inicialização do linux, apenas modifique, faça a build e então  
copie a pasta bin para o cartão de memória. Essa etapa é totalmente opcional.

O exemplo limpa a tela após 5 segundos e passa a piscar o LED vermelho da placa,
com 1 segundo aceso e 1 segundo apagado. Ele escreve em
`/sys/class/leds/bananapi-m2-zero:red:pwr/brightness`, usando o driver de LEDs
do Linux. O `init` já monta `/sys` antes de executar o programa. A configuração
local do kernel habilita `CONFIG_LEDS_GPIO=y` e `CONFIG_LEDS_CLASS=y`.
