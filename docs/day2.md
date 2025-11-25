

1. Booting Kernel via TFTP over U-Boot

Installations ve labs altında açıklanan tftp kurulumu ve network ayarlarının yapılmış olduğunu varsayarak devamına ilerliyorum.

setenv bootargs root=/dev/nfs rw ip=192.168.1.100:::::eth0 console=ttyS2,115200n8 nfsroot=192.168.1.1:/home/<user>/linux-kernel-beagleplay-labs/modules/nfsroot,nfsvers=3,tcp

- kernel image'ı (zImage) de tftp boot komutu ile 0x8100_0000 adresine yükleriz.
- device tree blob dosyasını (xxx.dtb, örneğin k3-am625-beagleplay.dtb) 0x8200_0000 adresine yükleriz
- sonrasında kernel'i ilklendirmek için "bootz 0x81000000 0x83000000" ile boot edebiliriz.

bu noktada kernel ilklenmeyebilir çünkü bazı temel konfig bilgilerine ihtiyaç duyabiliriz.

=> setenv kernel_comp_addr_r 0x85000000
=> setenv kernel_comp_size 0x2000000
=> saveenv


device tree dosyalarının içerisinde farklı dtb dosyalarının include edildiğini görebiliriz. bazı dosyalar common olarak SoC family'si altında ortak kullanılabilir.
Device tree dosyasının başlangıcı "/ {" ile başlayan dosyadır.

bu dosya içerisinde kullanılan bazı değerler için UBoot içerisinde "bootargs" ile belirlenebilir. UBoot tarafından bu dosyalar kernel başlamadan önce Parse edilir. "root file system" in nerede olduğu da bu şekilde belirtilir. network üzerinden remote bir makinede olduğu durumda (örneğin bizde de beagleplay board'ı host ubuntu makine üzerindeki dosyalara erişecektir ve gerekli network ayarları da bootargs ile verilmelidir.)
- `setenv bootargs root=/dev/nfs rw ip=192.168.1.100:::::eth0 console=ttyS2,115200n8 nfsroot=192.168.1.1:/home/hilmi/linux-kernel-beagleplay-labs/modules/nfsroot,nfsvers=3,tcp`

bu argümanları açıklamak gerekirse
- root=/dev/nfs

Kernel’e root file system’in NFS üzerinden geleceğini söyler. Normalde root=/dev/mmcblk0p2 gibi bir block device yazılırdı. Burada /dev/nfs yazarak aslında kernel, rootfs’i NFS’den mount etmeye çalışacağını söyleriz bir blok cihazdan değil.

- nfsroot=192.168.1.1:/home/hilmi/linux-kernel-beagleplay-labs/modules/nfsroot,nfsvers=3,tcp

Kernel'in root filesystem için bağlanacağı NFS sunucusunu belirtir.

Bölerek açıklayalım:

📌 192.168.1.1 NFS server’ın IP adresi (Host Ubuntu makinenin IP’si)

📌 :/home/hilmi/.../nfsroot Ubuntu host üzerinde NFS share edilen dizin. Kernel burayı root filesystem olarak mount eder.

📌 nfsvers=3 NFS protokolünün v3 kullanılacağını belirtir.

📌 tcp

NFS bağlantısının TCP üzerinden yapılacağını belirtir.


boot etmek için image ve dtb dosyasını ayrı ayrı yüklemek yerine bu işlemi otomatize edecek şekilde bootcmd ayarlayabiliriz.

-----------------------------------------------------------------------------------

# advantages of modules

reboot gerektirmeden load/unload yapılabilir ve geliştiriciye kolaylık sağlar.

- `sudo insmod <module_path>.ko`
- `sudo modprobe <top_module_name>` top modülü ve buna bağlı dependency'leri yükler.
- `lsmod` loaded edilmiş modülleri listeler. /proc/modules altında bulunan modüller ile kıyaslar.


modülü yüklemeye çalıştığımızda hat alırsak çok da anlamlı veya yeterli detayı göremeyiz hata mesajlarında. detayı görebilmek için `dmesg` ile log'lara bakmak daha yararlı olacaktır.

- `sudo rmmod <module_name>` insmod'un tersi olarak modülü kaldırmaya yarar.
- ` sudo modprobe-r <top_module_name>` top modülü ve bağımlı olan kullanılmayan modülleri kaldırır.

# passing parameters to modules

`modinfo usb-storage` komutu ile modül için verebileceğimiz parametreleri öğrenebiliriz. sonrasında ya insmod ya da command line aracılığı (static build şeklinde) ile parametreyi verebiliriz. Through modprobe: Set parameters in /etc/modprobe.conf or in any file in /etc/modprobe.d/: options usb-storage delay_use=0

mevcut bir modüle ait parametreye erişmek istersek `/sys/module/<name>/parameters.`

-----------------------------------------------------------------------------------

# developing kernel modules

genellikle driver ve kernel module kelimelerini birbirlerine yerine kullanabiliyoruz. kernel modülü .ko uzantılı şekilde olur driver'dan farklı olarak.

en basit hello world modülünü şu şekilde yazabiliriz gerekli tüm bölümleri ile

cppcode
```
// SPDX-License-Identifier: GPL-2.0
 /* hello.c */
 #include <linux/init.h>
 #include <linux/module.h>
 #include <linux/kernel.h>
 static int __init hello_init(void)
 {
 pr_alert("Good morrow to this fair assembly.\n");
 return 0;
 }
 static void __exit hello_exit(void)
 {
 pr_alert("Alas, poor world, what treasure hast thou lost!\n");
 }
 module_init(hello_init);
 module_exit(hello_exit);
 MODULE_LICENSE("GPL");
 MODULE_DESCRIPTION("hi lmi world module");
 MODULE_AUTHOR("Hilmi Guven");
```

- lisans bilgisi verilir
- birkaç include bulunabilir ihtiyaç halinde
- init, kernel başladığında ve modül yüklendiğinde çalışan main entry point'tir. `module_init(hello_init)` ile starting point'in neresi olduğunu kernel'e söyleriz. benzer şekilde driver remove edildiğinde exit ile belirtilen fonksiyon çalışır.
- modüle ait lisans, tanımı ne olduğu, yazarı kim gibi bilgiler makrolar ile belirtilir.

__init ile işaretlenen fonksiyonlar kernel ilklendikten sonra silinir ve ram de yer açılır.

bir kernel modül içerisindeki fonksiyonlar veya değişkenler farklı bir modül tarafından erişilmek istendiğinde bunların implict şekilde export edilmesi gerekir. bunları 
- `EXPORT_SYMBOL(symbolname)`, which exports a function or variable to all modules
- `EXPORT_SYMBOL_GPL(symbolname)`, which exports a function or variable only to GPL modules

ile yaparız. daha iyi açıklandığı slayt "symbols exported to modules 2/2" içerisinde bulunup incelenebilir.

# compiling a module

modülü derlerken iki seçenek bulunuyor

- out of tree
    - modül kodları kernel içerisinde bulunmaz ve ayrı şekilde derlenmesi gerekir.
    - statik olarak değil de yalnızca modül olarak eklenebilir.
    - kernel'inderlenmesi ya da konfigürasyon içerisine dahil değildir.
    - make dosyaları ile bunu yapabiliriz.

dışarıdan bir dosya derlemek için aşağıdakine benzer basit bir makefile dosyası oluşturabilir. hello.c dosyasıiçin oluşturulmuş. eğer kernel release olarak tanımlanmamışsa bunu "M" parametresi ile kernel e verebiliriz. bu out-of-tree modül ile kernel'in header larına erişmesini ve sürüm farklılıklarını yönetmemiz gerekir.

cppcode 
```
ifneq ($(KERNELRELEASE),)
obj-m := hello.o
else
KDIR := /path/to/kernel/sources
all:
<tab>$(MAKE) -C $(KDIR) M=$$PWD
endif
```
- inside the kernel tree
    - kernel'in konfigüre/derleme süreçlerine dahil edilir
    - static olarak (kernel'in bir parçası olur diyebiliriz) veya module olarak eklenebilir.

# New driver in kernel sources

driver için uygun konuma folder açıp .kconfig dosyasını oluştururuz.
```
config USB_SERIAL_NAVMAN
tristate "USB Navman GPS device"
depends on USB_SERIAL
help
   To compile this driver as a module, choose M
   here: the module will be called navman
```

Add a line in the Makefile file based on the Kconfig setting: `obj-$(CONFIG_USB_SERIAL_NAVMAN) += navman.o`
sonrasında `CONFIG_USB_SERIAL_NAVMAN=y/n/m ` şeklinde eklenip eklenmeyeceğiya da modül olarak kullanılacağını belirtiriz.

# hello module with parameters



```
static char *whom = "world";
module_param(whom, charp, 0644);
MODULE_PARM_DESC(whom, "Recipient of the hello message");

static int howmany = 1;
module_param(howmany, int, 0644);
MODULE_PARM_DESC(howmany, "Number of greetings");
```


```
module_param(
name, /* name of an already defined variable */
type, /* standard types (different from C types) are:
        * byte, short, ushort, int, uint, long, ulong
        * charp: a character pointer
        * bool: a bool, values 0/1, y/n, Y/N.
        * invbool: the above, only sense-reversed (N = true). */
perm    /* for /sys/module/<module_name>/parameters/<param>,
        * 0: no such module parameter value file */
);
```

# hello world module

!!! genelde asm/xxx dosyası bulunamadı şeklinde bir derleme hatası alıyorsak bu genellikle yanlış compiler seçimi ile ilgilidir.

parametre alacak şeklide oluşturduğumuzda C dosyamızı ve Makefile oluşturalım.

```
// SPDX-License-Identifier: GPL-2.0
/* hello_param.c */
#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

static char *whom = "world";
module_param(whom, charp, 0644);
MODULE_PARM_DESC(whom, "Recipient of the hello message");

static int howmany = 1;
module_param(howmany, int, 0644);
MODULE_PARM_DESC(howmany, "Number of greetings");

static int __init hello_init(void)
{
	int i;

	for (i = 0; i < howmany; i++)
		pr_alert("(%d) Hello, %s\n", i, whom);
	return 0;
}

static void __exit hello_exit(void)
{
	pr_alert("Goodbye, cruel %s\n", whom);
}

module_init(hello_init);
module_exit(hello_exit);
```

Makefile:
```
obj-m := hello_param.o

KDIR := /lib/modules/`uname -r`/build 
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules
```

host makinede nfs altında bulunan bu modülü (konumu /sys/module/hello_version/ gibi bir şey olabilir) "make" ile derleyip boot ettiğimiz kernel'de bunu "insmod hello_version.ko" şeklinde load edebiliriz. NFS ayarlarımızı yaptığımız için Linux Kernel, nfs üzerinden ubuntu host makinedeki dosyalara erişebilmektedir. bu şekilde yaptığımızda bu modül, linux kernel dışında out-of-tree örneği oldu.

modül içerisinde linux kernel'in sağladığı makrolar sayesinde şu bilgileri de ekleyebiliriz
- load-unload arasında kaç saniye boyunca çalıştığı (`ktime_get_seconds()` fonksiyonu, <linux/timekeeping.h>)
- çalıştığı linux version bilgisi (UTS_RELEASE makrosu ile, <generate/utsrelease.h>)

Yazdığımız modüldeki kodların standarda ve kodlama kurallarına uyduğundan emin olmak gerekir. Bunun için `~/linux-kernel-beagleplay-labs/src/linux/scripts/checkpatch.pl --file --no-tree hello_version.c` şeklinde komutu çalıştırırsak kodumuzda düzenlememiz gereken kısımları WARNING olarak gösterecektir.

modülü linux source dosyası içinde de derleyebiliriz. bunun için source dosyalarıyla birlikte kconfig ve cmake dosyalarına gerekli eklemeler yapılmalı ve driver enable edilemlidir ("make menuconfig" ile modülün kconfig oluştururken verdiğimiz isim ile arayabiliriz.)
dosyalar ve içeriklerini buraya yazmayacağım..

-----------------------------------------------------------------------------------

Discoverable hardware: USB and PCI

- bazı bus'lar (genellikle USB ve PCI) yapısı gereği cihazları bulup (tespit etmek, discover) onları sürücüler aracılığı ile konfigüre edebilir. genellikle device/vendor id gibi bilgilerle ayırt edilebilir. (PCIe , my good old friend :) )
- `lsusb` veya `lspci` ile bus'da tespit edilen cihazlar listelenebilir.
- kernel'in bu cihazları buluyor olması oonların sürücülerinin olduğu anlamına gelmez.pcie tarafından cihazlar bulunur ve konfigüre edilebilir ancak bunların çalıştırılması için farklı sürücülere ihtiyaç olabilir.

Non-discoverable buses

- bu tarz bus yapılarında "enumeration", "hot-plug", "unique-identifier" gibi özellikler bulunmaz.
- I2C, SPI gibi bus'lar buna örnek verilebilir. bunların statik olarak tanımlanmış olması gerekir. yani statik şekilde özelliklerinin belirtilmiş olmasına ihtiyaç duyarız. bunu nasıl yapabiliriz
    1. Directly OS/Bootlaoder Code : legacy bir yöntem olarak kalmıştır. derlenen kodları vb içerir. maintaible değildir.
    2. ACPI tables : genellikle x86 da olan firmware tarafından oluşturulan tabloları içerir.
    3. *Device Tree* : farklı bir syntax'e sahip olup human/machine readable formatta tanımlamaya yarar. OpenFirmware tarafından geliştirilmiş bu nedenle genellikle _of öne eki yer alır dosyalarda.

Device Tree: from source to blob

"Device Tree Source" dosyası geliştirici falan yazılır. *.dts uzantılıdır. Derlenir ve sonrasında *.dtb uzantılı Device Tree Blob dosyasına dönüştürülür. OS'un anlayacağı formata dönüştürülür.

`dtc -I dts -O dtb -o foo.dtb foo.dts` şeklinde*.dts dosyasından *.dtb üretilebilir.

kernel içerisinde `arch/<ARCH>/boot/dts/<vendor>/` lokasyonunda bulunur. syntax'i incelediğimiz biraz okunabilir seviyededir. Node'lar ve node'ların property'lerinden oluşur. bu property'ler 
- string, array of something vb formatta olabilir. örneğin register adresleri ya da node ismi
- farklı bir node'a reference verilebilir. Ancak ismiyle değilde `phandle` adını verdiliğimiz node'un label'i ile veririz. 
    - aşağıdaki örnekte `nvic` node'una referans verilmiştir. 
    - `interrupt-controller@e000e100` ile bu node'un ismidir. @e000e100 ile bu donanımın registerlarının başlangıç adresidir.
    - `soc` için bir adres belirtilmemiş. bu sanal bir bus'ı temsil ediyor olabilir.
    - `compatible = "arm,armv7m-nvic";` string property. buna göre linux kernel uygun işlem yapar
    - `interrupt-controller;` flag'dir. Değeri verilmez boolean gibi çalışır. Var olması tanımlı olduğu=true anlamına gelir.
    - `#interrupt-cells = <1>;` numeric variable olarak tanılamaya yarar.
    - `reg` Register map property dir. Adres ve registerların size'ını belirtir. Yani NVIC register aralığı: 0xE000E100 – 0xE000ECFF olarak düşünülebilir.


bu syntax dtc compiler tarafından kontrol edilir.

```
// SPDX-License-Identifier: GPL-2.0
/ {
	nvic: interrupt-controller@e000e100  {
		compatible = "arm,armv7m-nvic";
		interrupt-controller;
		#interrupt-cells = <1>;
		reg = <0xe000e100 0xc00>;
	};

	systick: timer@e000e010 {
		compatible = "arm,armv7m-systick";
		reg = <0xe000e010 0x10>;
		status = "disabled";
	};

	soc {
		#address-cells = <1>;
		#size-cells = <1>;
		compatible = "simple-bus";
		interrupt-parent = <&nvic>;
		ranges;
	};
};
```

Farklı bir örneği incelersek (eğitim sunumunda da paylaşılmış görsel ile birlikte incelenmesi daha faydalı olacaktır)

- BeagleBone Black için biraz sadeleştirilmiş dtc dosyasıdır. cortex a8 cpu core bulunur.
- l4_per:   // bu aslında donanım üzerinde low-power modül kontrolü için oluşturulan peripheral domain'i olarak adlandırılabilir
- memory ile DDR RAM memory tanımlanır
- status ile enable/disable yapılmasına karar verilebilir. kullanılmayacakların disable edilmesi gereklidir.

```
/ {
#address-cells = <1>;
#size-cells = <1>;
model = "TI AM335x BeagleBone Black";
compatible = "ti,am335x-bone-black", "ti,am335x-bone", "ti,am33xx";

cpus { ... };
    memory@0x80000000 {
        device_type = "memory";
        reg = <0x80000000 0x10000000>; /* 256 MB */
    };
chosen { ... };
 ocp {
    intc: interrupt-controller@48200000 { ... };
    usb0: usb@47401300 { ... };
    l4_per: interconnect@44c00000 {
        i2c0: i2c@40012000 {
            compatible = "ti,omap4-i2c";
            #address-cells = <1>;
            #size-cells = <0>;
            reg = <0x0 0x1000>;
            interrupts = <70>;
            status = "okay";
            pinctrl-names = "default";
            pinctrl-0 = <&i2c0_pins>;
            clock-frequency = <400000>;
            baseboard_eeprom: eeprom@50 {
                compatible = "atmel,24c256";
                    reg = <0x50>;
        };
    };
 };
};
```

Device Tree dosyaları monolithic değildir. birden fazla dosyadan oluşabilir. başka dosyaları include edebiliriz. ya da benzer şekilde SoC Family'si için oluşturulan dts ile board specific oluşturulan dts dosyası birleşerek resulting dts dosyası oluşturulabilir.

Device Tree Design Principles

- describe hardware,not configuration 
