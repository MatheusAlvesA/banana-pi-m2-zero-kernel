# Banana Pi M2 Zero até o Kernel

O objetivo desse repositório ensinar sobre os requisitos e a stack de softwares necessários para  
realizar o boot do Kernel linux na placa Banana Pi M2 Zero. Esse tutorial assume que você está usando uma distro linux com apt, como o Ubuntu.

# U-Boot

O bootloader utilizado é o [https://docs.u-boot-project.org/en/v2026.07/build/index.html](U-Boot)  
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
```
Substitua o sdX pela label do dispositivo na sua máquina.  

O processador do Banana Pi, Allwinner H3, procura pelo bootloader no SDCard,  
específicamente no endereço 8000, ou seja a partir do oitavo kilo byte.
```sh
sudo dd if=u-boot-sunxi-with-spl.bin \
    of=/dev/sdX \
    bs=1k seek=8
```

Agora copie o conteúdo da pasta boot desse projeto para a partição `main` do SDCard  
essa pasta contém a device tree e também o kernel linux compilado.  

Com isso feito, basta colocar o SDCard e observe a placa dar boot no Kernel linux.

# Boot Script

O arquivo `boot/boot.scr` é versão compilada de `boot/boot.cmd`, um script executado pelo U-boot automaticamente para executar o boot do Kernel.  
Se deseja customizar isso, edite o `boot.cmd` e execute:
```sh
./u-boot/tools/mkimage -C none -A arm -T script -d boot/boot.cmd boot/boot.scr
```
Com isso, o script de boot agora é a versão customizada que você criou


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

