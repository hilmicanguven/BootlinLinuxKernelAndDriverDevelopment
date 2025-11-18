

1. TFTP Setup and Boot


----
2-1

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

----------------------------------------

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

-------------------------------------------

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
    - statik olarak değil de yalnızca modül olarak eklenebilir
- inside the kernel tree
    - kernel'in konfigüre/derleme süreçlerine dahil edilebilir
    - static olarak (kernel'in bir parçası olur diyebiliriz) veya module olarak eklenebilir.

