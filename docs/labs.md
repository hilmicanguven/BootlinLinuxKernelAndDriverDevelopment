
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
- `make help` ile .defconfig dosyaları gözükür. bizim kartımızın family'si "omap2plus_defconfig" olandır.
- `make omap2plus_defconfig`
- `wc -l .config arch/arm/configs/omap2plus_defconfig`
- `make menuconfig`
- `make -j 8`

Terminalde açılan bu menüden konfigürasyon yaparız. 

Örneğin, kartımızda RJ45 ethernet konnektörü olmadığını düşünelim. USB üzerinden Network erişimi sağlamak için default configuration'dan farklı bir ayar yapmamız gerekir. bunun için USB'nin eternet cihazı gibi davranması için Gadget modda çalıştırılır.


Network Ayarları ve BeaglePlay ile Bağlantının Kurulması

1. Fiziksel olarak doğrudan birbirlerine ethernet kablosu ile bağlayıp kendi aralarında lokal bir bağlantı ağı oluşturabiliriz.
2. Statik IP atamaları yapacağız. çalışma/ofis ortamında aynı ağa bağlanama problem oluşturabilir şirketin network policy'leri vb sebepten. Ayrıca yeterli fiziksel port da bulunmuyor. bizim ethernet'imizin bağlı olduğu interface'i bulmalıyız.
- `ip addr` ile network arayüzlerini görebiliriz.
- Ubuntu Host Makinemiz için
    - `ip link` ile mevcut arayüzleri görürüz.
    - `sudo ip addr add 192.168.7.1/24 dev enp3s0` ile ip set edilir
    - `sudo ip link set enp3s0 up` link up hale getirilir
    - `ip addr show dev enp3s0` ip adresinin atandığı kontrol edilir

- BeaglePlay - UBoot için
    - `setenv ipaddr 192.168.7.2`
    - `setenv serverip 192.168.7.1`
    - `saveenv`

3. TFTP kurulumunun yapılmış olması gerekir.

4. Dosya (Basit txt) aktarımı yapmak için gerekli network ayarları yapıldıysa TFTP altına kopyalamak dosyaları `(sudo cp path/to/zImage /srv/tftp/)` ile koyabiliriz.
5. U-Boot terminalinden `tftpboot 0x82000000 hello.txt` ile hello.txt dosyasının belirtilen adrese kopyalar. Bu adrese gerçekten dosyanın geldiğini `md` komutu ile öğreniriz.
```
=> ping 192.168.7.1
link up on port 1, speed 1000, full duplex
Using ethernet@8000000port@1 device
host 192.168.7.1 is alive
=> tftpboot 0x82000000 hello.txt
link up on port 1, speed 1000, full duplex
Using ethernet@8000000port@1 device
TFTP from server 192.168.7.1; our IP address is 192.168.7.2
Filename 'hello.txt'.
Load address: 0x82000000
Loading: #
	 1000 Bytes/s
done
Bytes transferred = 14 (e hex)
=> md 0x82000000
82000000: 6c206968 7720696d 646c726f ffff0a21    hi lmi world!...
82000010: ffffffff ffffffff ffffffff ffffffff    ................
82000020: ffffffff ffffffff ffffffff ffffffff    ................
82000030: ffffffff ffffffff ffffffff ffffffff    ................
82000040: ffffffff ffffffff ffffffff ffffffff    ................
82000050: ffffffff ffffffff ffffffff ffffffff    ................
82000060: ffffffff ffffffff ffffffff ffffffff    ................
82000070: ffffffff ffffffff ffffffff ffffffff    ................
82000080: ffffffff ffffffff ffffffff ffffffff    ................
82000090: ffffffff ffffffff ffffffff ffffffef    ................
820000a0: ffffffff ffffffff ffffffff ffffffff    ................
820000b0: ffffffff ffffff7f ffffffff fffeffff    ................
820000c0: ffffffff ffffffff ffffffff ffffffff    ................
820000d0: ffffffff ffffffff ffffffff ffffffff    ................
820000e0: ffffffff ffffffff ffffffff ffffffff    ................
820000f0: ffffffff ffffffff ffffffff fffdffff    ................
```