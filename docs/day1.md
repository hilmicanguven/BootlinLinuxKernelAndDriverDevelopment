

1. Linux Kernel in the System

Kernel, bir sistem içerisinde "user space (C library, user App, Librarier)" ve "hardware" arasında yer alır. "System Call" aracılığı ile Kernel ile "user space" arası iletişim sağlanır. 

2. Kernel'in ana görevleri

- Donanım kaynaklarını yönetme : CPU, Memory, IO
- Kullanıcıya, donanımdan bağımsız API'lar sağlamak
- Concurrent Erişim ve donanım kaynaklarının farklı uygulamalardan erişimlerinde sorunsuz çalışabilme

3. System Calls

User Space ve Kernel Space arasındaki etkileşimi sağlayan main interface'dir. Örnek system call çağrıları "open", "close", "seek"(file operations), "select" örnek verilebilir. Temel olarak
- File and device operations, networking operations, inter-process communication, process management, memory mapping, timers, threads, synchronization primitives, etc.

C library ile system call oluşturmak için gerekli api'ler sağlanmıştır. Direkt olarak syscall çağırılmaz

4. Pseudo Filesystems

Virtual Filesystem olarak da adlandırılır. Kernel tarafından oluşturulan gerçekte var olmayan bir File System oluşturur ve uygulamaların sanki directory/file varmış gibi erişim yapılmasını sağlar. yani gerçekten bu dosyaları bir diskte saklamaz. kullanıcıya yine open/close/read/write vb fonksiyonları kullanabilmesine olanak sağlar. Çünkü **Linux her şeye dosya gibi davranır.**

İki temel pseudo file system örneği
- proc  : proces, bellek yönetimi ile ilgilidir. Örneğin, CPU ile ilgili bilgilere erişmek için `/proc/cpuinfo` device'ına "read" sistem çağrısı gönderilir.
- sysfs : sistemin bir çeşit device tree olarak gösterilmesini sağlar.

5. Linux Kernel Source Code

C programlama dili ile implemente edilmiştir. Az sayıda Assembly kodu da CPU ile ilgili kısımlar için ya da kritik library routine'leri için kullanılır. Örneğin, `memset` fonksiyonu genellikle Architecture içerisinde assembly ile yazılır. Kodlar gcc ile derlenir. Bazı kısımlar ise LLVM C Compiler (Clang) ile de derlenebilir. Basit Rust ile yazılan driver'lar başlangıç seviyesinde yazılmış halde ancak yalnızca yavaş yavaş eklenebilir. 

Kernel içerisinde C library bulunmuyor. User space kod içermez. Neden?
- architectural reason: user space, kernel'in üstünde çalışacak şekilde geliştirilmelidir.
- technical reason: boot esnasında yalnızca kernel çalışır. henüz root filesystem'a erişim yapılmıyorken. bu nedenle library implementation'ları kendisi implemente etmelidir (string utilities, cryptography, uncompression...)

Bu nedenle Kernel'de C library fonksiyonları bulunmaz Ancak kernel tarafından benzer fonksiyonlar oluşturulmuştur. `(printf(), memset(), malloc(),...)` fonksiyonlar yerine  `printk(), memset(), kmalloc(), ...` gibi fonksiyonlar oluşturulmuştur.

6. Portability

Kodun portable olmasını kim istemez ki? `arch/` dışındaki tüm kodlar hangi platformda olursa olsun derlenebilir.Bunu başarmak adına çeşitli Macro ve Fonksiyonlar mimariye-özgü detayları soyutlamak için Kernel tarafından sağlanır. Bu architecture-specific detaylar:
- Endianness
- I/O memory access
- Memory barriers to provide ordering guarantees if needed
- DMA API to flush and invalidate caches if needed

Floating point sayıları Kernel kodu içerisinde kullanılmamalıdır.

7. Linux Kernel to user API/ABI Stability

Kernel'e ulaşmak için kullanılan API'lerin isimleri, parametreleri mümkün oldukça değişmemelidir. Yeni bir kernel'e geçildiğinde önceki yazılan uygulamalar hala derlenebilir olmalıdır. /proc ve /sys sistem çağrıları kaldırılamaz ve yalnızca yeni entry'ler eklenebilir.

- API, (Application Programming Interface), kernel ile kullanıcı uygulamalarının arasındaki arayüzdür. Bu arayüzler genellikle şunları kapsar: system calls(open, close vb), /proc ve /sys, socket interfaces vb. Bunların değişmemesini bekleriz.
    - Kernel Internal API ise stabil olmayabilir. geliştirme yapıldıkça sürekli değişikliğe uğrar. (kmalloc gibi)
    - out-of-tree sürücüler Kernel değiştikçe çalışamıyor olabilir. Tekrar derlenmesi gerekir.

- ABI, (Application Binary Interface), memory'nin nasıl organize edildiği veinput/return değerleri binary dosyada nereye koyduğumuz ile ilgilidir. register/stack kullanımı, system call sayısı vb yazılan kodların binary seviyesinde nasıl oluşacağı ile ilgilir. Asıl CPU'da çalışan şeydir yani.


The Linux kernel maintains a very strong promise:
    **“Don’t break user-space.”**

8. Linux Kernel Memory Constraints

Kernel'in kendi içerisinde bazı limitleri bulunur. "Memory Protection"bulunmaz, yani Kernel istediği tüm adreslere erişebilir. Bu nedenle bazı problemlere yol açabilir.
Ulaşmaması gereken noktaları eriştiğinde herhangibir recover bulunmaz ve "oops" mesajını görürüz.

Genellikle 4 veya 8 Kb Fixed stack size kullanılır.

9. Proprietary Code

Sahibinin individual veya company olduğu ve kontrolü de bu kişiler tarafından yapıldığı kodlardır. Closed-Source olarak nitelendirilebilir ve izinsiz dağıtımı yasaktır.
Bunları içeren bir Linux Binary'sinin dağıtımı da yasal değildir.
Linux'da NVIDIA's GPU sürücüleri örnek verilebilir.

Bazı kernel modüllerinin durumu net değildir. tartışmalara açıktır.
Örneğin, Nvidia kernel ve driver'ları arasında wrapper kullanır ve farklı bir wrapper ile farklı bir OS tarafından kullanılabilir iddiasındadırlar.

Şu anda bunu engellemek adına, Logic'in firmware ya da user-space içerisinde saklanmasıdır.
Bunu çalıştıran GPL driver'ı boştur. Asıl işlemleri yapılmasını OpenGL benzeri kütüpahneler binary olarak bulunur user space'de.

10. User Space Device Drivers

Driver'ların Kernel'de yazılması çoğu zamandan mantıklı olan seçimdir. Ancak, bazı durumlarda user-space'de oluşturulması daha mantıklı olabilir.
Kernel tarafından donanımlara hardware erişebilmek için User-Space'in kullanımı için geliştirilen mekanizmalar bulunur.
- USB devices with libusb, https://libusb.info/
- SPI devices with spidev, spi/spidev
- I2C devices with i2cdev, i2c/dev-interface
- Memory-mapped devices with UIO, including interrupt handling, driver-api/uio-howto

Printers ve Scanner'ların kernel desteği bulunmaz. Bazı tarihi (?) sebeplerden kaynaklı bu şekildedir.

User Space'de driver yazmanın avantajları
- kernel kodlarını bilmeye gerek kalmaz ve herhangibir dil ile yazılabilir
- driver'lar Proprietary olarak kalabilir
- potansiyel olarak daha performanslı olabilir. Memory Mapped cihazlar sayesinde syscall lardan kaçınılabilir

dezavantajları

- kernel den erişim yoktur. kernel'in helper fonksiyonları kullanılamaz.
- donanım değiştirkçe artık uygulama kodlarının da değişmesi gerekir.


11. Kernel Configuration

Kernel'i konfigüre ederken başlıca "ARCH" mimariyi ve Capabilities ile sahip olacağı ek özellikleri (network, filesystem vb) belirtiriz.
`make <target>` şeklinde Makefile ile derleriz.

- Target mimari belirleme: Set ARCH to the name of a directory under arch/: ARCH=arm or ARCH=arm64 or ARCH=riscv, etc
- Compiler: native veya cross compiler şeklinde derleyebiliriz.
 - Komut satırından argüman olarak verebiliriz      > `make ARCH=arm CROSS_COMPILE=arm-linux-` ...
 - Environement değişken olarak tanımlayabiliriz    > `export ARCH=arm` ve `export CROSS_COMPILE=arm-linux-` (her terminal kapatıldığında kaybolur. bunu çözmeliyiz.)

12. Initial Configuration

Ayarları .config dosyalarında saklarız. Default bulunduğu gibi (`arch/<arch>/configs/` altında) kendimiz de oluşturabiliriz.
Bu ayarları yaparken bool (true, false) tristate (true, false ve module), int, hex (`CONFIG_PAGE_OFFSET=0xC0000000`), string (`CONFIG_LOCALVERSION=-no-network`) options bulunur.

Bazı modüller driver'ları depen edebilir. network driver, network stack'in enable olmasına ihtiyaç duyar. Bunları belirtmek için `depends on` veya `select` anahtar kelimeleri ile belirtebiliriz. 

`make xconfig` veya `make menuconfig` ile konfigüre edebiliriz.

13. Built-in or Module

Kernel imajı tek bir dosyadan oluşur. Bootloader tarafından belleğe bu yüklenir.
Bazı modülleri (_Kernel Modules_) dinamik olarak run-time da kernel içerisine ekleyebilir ya da çıkarabiliriz.


14. Kernel Compilation Results

- arch/<arch>/boot/Image, uncompressed kernel image that can be booted
- arch/<arch>/boot/*Image*, compressed kernel images that can also be booted
    - bzImage for x86, zImage for ARM, Image.gz for RISC-V, vmlinux.bin.gz for ARC, etc.
- arch/<arch>/boot/dts/<vendor>/*.dtb, compiled Device Tree Blobs
- All kernel modules, spread over the kernel source tree, as .ko (Kernel Object) files.
- vmlinux, a raw uncompressed kernel image in the ELF format, useful for
debugging purposes but generally not used for booting purposes

15. Booting Kernel

16. Hardware Description

Embedded cihazların bir çoğu sorgu atarak öğrenemeyeceğimiz özellikleri bulunur ve bunları Bootloader/Firmware'e sağlamak gereklidir.
- On x86 : using ACPI tables
- On most others: using OpenFirmware Device Tree (DT)

17. Booting with U-Boot

ARM64 veya RISC-V için `Image` dosyası `booti` komutu ile yüklenebilir. ARM32 için `zImage` ve `bootz` kullanılır. Kernel imajına ek olarak U-Boot DTB'yi kernel'i verir. Ayrıca boot esnasında kullanılacak adresleri de belirtmemiz gerekir.

The typical boot process is therefore:
1. Load zImage at address X in memory
2. Load <board>.dtb at address Y in memory
3. Start the kernel with boot[z|i] X - Y
    The - in the middle indicates no initramfs

X: Kernel location in RAM, Y: Offset of device tree in RAM

Kernel command line ile çeşitli argümanlar tekrar derlemeye gerek kalmadan kernel'e verilebilir. Example: `console=ttyS0 root=/dev/mmcblk0p2 rootwait`

U-Boot da bunu sağlayabilir. `bootargs` ile kernel için environment variable oluşturur.

18. Kernel Log

Konsol parametresinidoğru bir şekilde sağladığımızda linux kernel mesajlarını görüyor olmalıyız. Genellikle serial ve yavaş olabiliyor.
- `dmesg` komutu ile o an available kernel log'larını görebiliriz. shell aracılığı ile diagnostic mesajları içerir (takılan bir usb cihazını görebiliriz örneğin)
- log seviyelerine ayrılmıştır ve hangilerinin konsolda gözükeceğini ayarlayabiliriz. Example: `console=ttyS0 root=/dev/mmcblk0p2 loglevel=5`
- düşük log seviyesine sahip seviye daha yüksek önceliğe sahiptir.
