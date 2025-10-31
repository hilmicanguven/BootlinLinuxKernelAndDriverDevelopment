
Exercise - Lab Environment

- linux source kod clone edilir. içerisinde 
    - git remote add stable https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux
    - git fetch stable
- `make kernelversion` ile hangi versiyonda olduğunu gözlemleyebiliriz.
- kurs ile uyumlu gitmek adına `git checkout -b 6.7.bootlin stable/linux-6.7.y` 


Practical Lab - Kernel Compiling and Booting

- İlk olarak Bootloader'ımızı konfigüre edip, host bilgisayarımızdan derlenen dosyaları alabilmek için TFTP'yi kullanacağız.
- Doğrudan host bilgisayardan kernel'deki bir dosyayı yükleyip boot edebileceğiz. NFS (Network File System sayesinde) çok daha hızlı bir şekilde çalışma ortamımızdaki dosyalara erişebiliriz.
- Kernel'imizi ARM platform için cross-compile şeklinde derleyeceğiz.
- Daha önce kernel kodlarımızı clone'lamıştık. Bu repository'yi kullanacağız.

linux-kernel-beagleplay-labs.pdf dosyasında belirtilen setup ve cross-compiling toolchain setup adımlarını yapalım
- $HOME/.../linux linux source kod dizinine gidelim
- `sudo apt install libssl-dev bison flex`
- `sudo apt install gcc-aarch64-linux-gnu` ile cross-compile derleyici kurulur
- `dpkg -L gcc-aarch64-linux-gnu` ile bir üst adımda kurulan dosyalar gözlemlenebilir
- `export ARCH=arm64` ile ortam değişkeni ekledik. bu linux source kodlarında doğru path'e ulaşmak için kullanılacak
- `export CROSS_COMPILE=/usr/bin/aarch64-linux-gnu-`
- `make help` ile .defconfig dosyalarıgözükür. bizim kartımızın family'si "omap2plus_defconfig" olandır.
- `make omap2plus_defconfig`
- `wc -l .config arch/arm/configs/omap2plus_defconfig`
- `make menuconfig`

Terminalde açılan bu menüden konfigürasyon yaparız. 

Örneğin, kartımızda RJ45 ethernet konnektörü olmadığını düşünelim. USB üzerinden Network erişimi sağlamak için default configuration'dan farklı bir ayar yapmamız gerekir. bunun için USB'nin eternet cihazı gibi davranması için Gadget modda çalıştırılır.