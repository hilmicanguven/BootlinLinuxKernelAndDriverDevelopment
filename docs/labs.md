
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



LAB: Describing Hardware Devices

mevcutta bulunan .dtb dosyasını(k3-am625-beagleplay.dts) kopyalayıp kendi dtb dosyamızı oluşturup aynı directory de bulunan Makefile a derenmesi için ekleriz. Sonrasında uboot da bootcmd args kısmından yeni dtb dosyasını seçerek kernel'i bu dtb ile ilklendiririz. mevcut dtb yi include edip çalıştırdığımızda tüm property'lerin inherit edildiği gözlemlenebilir. örneğin, ilklenme esnasında terminale bastırılan mesajlarda kartın adı/vendor vb bilgileri görürüz. buna benzer şekilde farklı property'ler de override edilebiliriz. kernel ilklendikten sonra `cat /sys/firmware/devicetree/base` path'i içerisinde devicetree ye ait bilgilere de erişebiliriz. ./model içerisinde modelimize dts dosyası içerisinde verdiğimiz model adı ne ise onu görürüz.

- bazı LED'lerin default-state ini override ederek default on yapacağız. ek olarak trigger da belirtilip belirli bir olay olduğunda sürülecek şekilde konfigüre edebiliriz. default halinde de her led için tanımlı bir trigger action'ı olur. örneğin "heartbeat" ile on olan bir led.
- nunchuk kullanmak için i2c kullanacağız. `i2cdetect` komutu ile kernel de i2c bus'ları gözlemleriz. buradaki tüm bus'lardaki cihazları ve slave adreslerini görmeliyiz. eğer bir cihaz var ve kernel bunu sürüyorsa "UU" olarak gözükür. I2C1 bus'ını konfigüre edeceğiz. bu komutu yazdığımızda bu bus'ı göremeyebiliriz. örneğin I2C0 ve I2C2 görebiliriz. lab örneğinde halihazırda bu iki bus active. biz ise I2C1 kullanıp işleri biraz daha zorlaştıracağız. linux kernel'deki I2C bus sırası ile datasheet uyumlu olmayabilir çünkü kernel probe sırasına göre device'ları oluşturur. sırasını belirlemek için `ls -l /sys/bus/i2c/devices/i2c-*` komutu çağırılarak peripheral base adreslerine bakılarak karar verebiliriz hangi sıra probe olan datasheet e göre hangi bus'a tekabül ediyor.
    - I2C1 i enable etmek için öncelikle devicetree içerisinde buluruz. elbette bunun bulunduğu yerde değilde kendi custom device tree'mizde referans node vererek status=okay olarak değiştiririz.
    - nunchuck için yazacağımız sürücüyü i2c bus'ında bir cihaz olacağı için device tree dosyası içerisinde child node olarak ekleriz.


LAB: Configuring Pin Muxing

olduğu hali ile nunchuk'ı denemeye kalkıtığımızda cihazı bulamadığını görebiliriz (`i2cdetect -r 1` ile probe olmayı deneyebiliriz) bunun nedeni scl ve sda pinlerinin sürekli low'da kaldığı için timeout oluşmasıdır çünkü bu hatların external pull up resistorlar ile bağlanması gerekir. sanıyorum controlcünün bunu yapmaması farklı pinmux seçenekleri olduğu içindir. i2c1 bus'ı için reference manuel e (bazen datasheet de de yer alabilir. beaglebone black için datasheet de yer alıyor) bakıp sda ve scl pinlerinin nereye (hangi pin'lere) bağlı olduğuna, alternatif fonksiyonlara (o pin'i i2c olarak kullanmak için hangi modu seçmeliyiz bunu belirleriz) vb bakmak gerekir. bu pinlerin pull-up ya da pull-down, receiver enable-disable, fast-slow slew rate gibi konfigürasyonlarını içeren register'ları buluyor olacağız. beaglebone için "table 4.2 pin attributes" altında görebiliriz. bu konu SoC özelinde ve biraz daha detaylı olduğundan derinlemesine araştırma yapmak gerekir. bu ayarları genellikle pinMuxConfiguration benzeri register'lar(conf_<module>_<pin> Register) aracılığı ile yaparız. bu pinmux işlemini sağlayacak şekilde dts içerisinde tanımlamalarını yapmamız gerekir. 

```
mikrobus_i2c_pins_default: mikrobus-i2c-default-pins {
pinctrl-single,pins = <
    AM62X_IOPAD(0x01d0, PIN_INPUT_PULLUP, 2) /* (A15) UART0_CTSn.I2C3_SCL */
    AM62X_IOPAD(0x01d4, PIN_INPUT_PULLUP, 2) /* (B15) UART0_RTSn.I2C3_SDA */
    >;
};
```
sonrasında bu pinctrl bloğu kendi dts dosyamız içerisinde referansını vererek i2c modülü içerisinde tanımlamamız gerekir.
nunchuk cihaza ait bazı detayları datasheet'inden bulabiliriz. örneğin slave address veya bus speed (i2c için genellikle 100khz veya fast mod 400khz olarak görürüz).

```
.
.

&i2c1 {
    status = "okay";
    clock-frequency = <100000>;
    pinctrl-0 = <&mikrobus-i2c-default-pins>;
    pinctrl-names = "default";

    joystick@52 {
        compatible = "nintendo, nunchuk";// genellikle vendor ve device name i burada belirtiriz
        reg  = <0x52>; // it is the slave address of nunchuk device
    };

};

.
.
```

"binding" adı verilen bir çeşit spec ile sürücü ve donanım arasında bir çeşit handshake sağlanabilir ve doğru donanım gereklilikleri ile sürücü arasında bir anlaşma sağlanır. istenirse kendi donanımınız için bu dosyayı oluşturabilir ve device tree içinde nasıl bir tanımlama yapacağınızı belirlemiş olursunuz. genellikle .yaml dosyası olarak görürüz. ve bu dosya içerisinde de propert'yleri belirleyebiliriz. hangilerinin required olup olmadığını belirleyebiliriz. hatta bir example bile ekleyebiliriz.

doğru şekilde dtb ekleyebildiğimizi test etmek için `find /sys/firmware/devicetree -name "*joystick*"` komutu ile linux kernel'de device'ı görmeyi bekleriz.
