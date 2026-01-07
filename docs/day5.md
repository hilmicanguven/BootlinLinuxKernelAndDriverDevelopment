Day 5

- Special File Operation: `long unlocked_ioctl(struct file *f, unsigned int cmd, unsigned long arg)` 
    - `ioctl` syscall ile çağırılan driver file operation'dır. unlocked olarak adlandırılma sebebi "Big Kernel Lock" tutmuyor olması sanıyorum.
    - sürücüyewrite/readötesinde farklı işlevsellik kazandırır. 
    - `cmd` parametresi hangi işlem yapılacağını belirten bir enum olur genellikle.
    - `arg` ile kernel'e data/parametre exchange için kullanılır.
    - örnek olarak bir sürücünün çalışacağı hızı değiştirmek/öğrenmek için bir cmd tanımlayıp ioctl ile çağırılabilir. bu cmd ve arg parametreleri tamamen ilgili sürücüye özel ve anlamlı şeylerdir.

# The Concept of Kernel Frameworks

Kernel de olan tüm driver'lar "character driver" olarak implemente edilmeyebilir. Bazı "framework" lere uygun olacak şekilde oluşturulabilirler. 
Örneğin, framebuffer, V4L(video for linux), TTY/Serial, block core, network, ioo(sensors) vb. örnek verilebilir. bu sayede aynı tür cihazlar için birnevi standardlatmış biryapı olmuş olur.
yeni eklenenler de bu çerçeveye uygun oluşturulabilir. user space açısından character device ile aynı gözükür. bir fark olmaz.
yine klasik şekilde open yapılarak file descriptor alınır ve sonrasında fonksiyonları çağırılabilir.
yeni bir kamera sürücüsü ekleneceği zaman V4L framework'üne uygun şekilde yazılabilir.


framebuffer framework eski bir grafik sub-system i denebilir. pixel bilgilerini bir memory de tutarak (framebuffer) ekranda gösterilmesini sağlar. mevcut yeni sistemlerde DRM bunun yerini almaya başlamıştır. bunun ile uyumlu bir sürücü yazmak için:

- Kernel option CONFIG_FB
    - menuconfig FB
        - tristate "Support for frame buffer devices"
- Implemented in C files in drivers/video/fbdev/core/
- Defines the user/kernel API
    - include/uapi/linux/fb.h (constants and structures)
- Defines the set of operations a framebuffer driver must implement and helper functions for the drivers
    - **struct fb_ops**
    - include/linux/fb.h


# Device Managed Allocations

Normalde sürücüler için gerekli kaynak alokasyonu probe() fonksiyonu içerisinde yapılır ve remove() ile de kaldırılırdı. Ancak bu kod maliyetini çok artırdığı için alternatif olarak *device managed allocations* önerildi. The idea is to associate resource allocation with the struct device, and automatically release those resources
    - When the device disappears
    - When the device is unbound from the driver

Fonksiyonlar **devm_** ön eki ile ayırt edilebilir (**kmalloc(size_t, gfp_t)** vs **devm_kmalloc(struct device*, size_t, gfp_t)** ) ve şu header içerisinde *driver-api/driver-model/devres* detaylı öğrenilebilir.


# Driver Data Structures and Links

Each framework defines a structure that a device driver must register to be
recognized as a device in this framework

- **struct uart_port** for serial ports, **struct net_device** for network devices, **struct fb_info** for framebuffers, etc.

Farklı yaklaşımlar ile bu inheritance işlemi yapılabilir. inheritance'den kasıt ise şudur. i.mx ailesine ait SoC'ler için bir seri port yazılmak istenirse ana structure olan `struct uart_port` ı bu sınıf doğrudan composition şeklinde içinde barındırabilir. Alternatif olarak pointer ile de tutabilirdi.
```
struct imx_port {
    struct uart_port port;
    struct timer_list timer;
    unsigned int old_status;
    int txirq, rxirq, rtsirq;
    unsigned int have_rtscts:1;
    [...]
};
```
Struct'ların birbirleri olan ilişkisi için eğitimde sunulan slaytlara bakılması şiddetle gereklidir.

# The Input Subsystem

input device: kullanıcıdan gelen tüm event'leri alan ve işleyen sistemdir. keyboard, mouse, joysticks, toubhscreens vb. input subsystem'i iki part'a ayrılmış
- Device Drivers : hardware ile konuşan kısımdır. event'leri input core'a sağlar.
- Event Handlers : gelen event'leri alarak gerekli yerlere paslar (genellikle `/evdev` aracılığı ile).

user space'de genellikle graphic stack tarafından kullanılırlar. bu stack'ler -> *X.Org*, *Wayland*, *Android's InputMnanager*.

Defines the set of operations an input driver must implement and helper functions for the drivers:
- `struct input_dev` for the device driver part (Before being used, this structure must be allocated and initialized, typically with: `struct input_dev *devm_input_allocate_device(struct device *dev);`)
- `struct input_handler` for the event handler part
- include/linux/input.h

Event tipine göre event'ler ve kodlarını `EV_KEY` ve `BTN_0` içerisinde bitwise set edilmesi gerekir (`set_bit()` ile atomik şekilde).

The events are sent by the driver to the event handler using `input_event(struct input_dev *dev, unsigned int type, unsigned int code, int value);`

- event tipi `input/event-codes` içerisinde açıklanmıştır.
```
#define EV_SYN			0x00 
#define EV_KEY			0x01
.
.
#define KEY_LEFTBRACE		26
#define KEY_RIGHTBRACE		27
.
.
``` 
şeklinde tanımlar mevcut.After submitting potentially multiple events, the input core must be notified by calling: `void input_sync(struct input_dev *dev)` ya da polling ile düzenli interval'larda kontrol edilir.

user space'den interaksiyon için "event interface" kullanılır. her input cihazı `/dev/input/event<X>` karakter device olarak kayıt edlir. Her okuma istedi `struct input_event` şeklinde bir structure döndürür.

Kolay test işlemi için **evtest** event test komutunu terminalden kullanarak cihaz adı ile sorgulayabiliriz.


# Memory Management

## Physical and Virtual Memory

Process'lerin kullandığı adreslere virtual adres deriz ve bunların karşılık geldiği fiziksel adreslere map'leriz. fiziksel adresler I/O cihazlara ya da peripheral'lari ait olabilir. Ancak user process'ler bu adreslere doğrudan erişemezler.

Kernel dışında kalan adresleri virtual olarak işaretleriz. Her process kendi adres space'ini (program, stack vb) görür. Başka process'lerin adreslerini göremez. Eğitim içeriğinde örneği açıklayarak ilerleyelim. 4gb memory'miz old durumda 1gb kernel kalan 3gb process'lere bölünmüş olduğunu varsayalım.

Kernel'in bir kısmı (low memory) 1:1 ono-to-one contiguously map edilir. bunun dışında bazı bölgeler I/O memory veya farklı mapping işlemi yapılabilir. 32bit sistemlerde bazı limitler mevcut. daha fazla RAM'e sahip olduğumuzda Kernel doğrudan erişemezken user-space erişebiliyor olur gibi. 64-bit sistemlerde hayat çok daha güzeldir.

User space'de map yapılırken gerçekte var olandan daha büyük bir fiziksel alana izin verilebilir. bu elbette out-of-memory durumlarına sebep olabilir. bir çeşit ilüzyon sağlar diyebiliriz. 1gb'lık bir adresi map'leyebiliriz user space'de. Ancak bunun yalnızca 10 byte'ını kullanabiliriz. bu gibi durumlarda çok büyük bir memory allocate edilse de kullanılmadığı için sorun oluşturmaz. yani tüm process'ler aynı anda fiziksel RAM'i kullanmayabilir. ayrıca, Bellek yönetim birimi (MMU) sayesinde sayfalar RAM’e yüklenir. Kullanılmayan sayfalar disk üzerinde (swap alanında) tutulur. Böylece fiziksel RAM’den daha büyük bir sanal adres alanı kullanılabilir. `/proc/sys/vm/overcommit*` kullanımı ile bu engellenebilir.

- Page Allocators: Memory Allocation için genellikle memory page'lere bölünmüştür. genellikle 4Kb olur. Maksimum 8192 Kb olur ancak kernel konfigürasyonuna göre değişir. çeşitli page allocator api'leri mevcuttur.
    - get free pages: `get_zeroed_page`, `__get_free_page`
    - page'leri free etmek/bırakmak için: `free_page`, `free_pages`
    - kullanılan flag'ler (common ones):
        - `GFP_KERNEL`: standard kernel memory allocation. blok'lanabilir. interrupt handler context'i dışında ihtiyacımızı görür. uygun memory bulana kadar bekleyebilir.
        - `GFP_ATOMIC`: interrupt context içerisinde kullanılmak içindir. Blok'lanmaz. fail olabilir ancak direkt return eder.


- SLAB Allocators
    The SLAB allocator allows to create caches, which contain a set of objects of the same size. In English, slab means tile (döşemek, yan yana koymak, fayans :) ). Yani **eş boyutlarda allocate edilmiş bir buffer pool'u diyebiliriz.** genelde bir sürücü içinde pek kullanılmaz. onun yerine, birden fazla instance olan data structure içeren durumlarda kullanılır.

- kmalloc allocator
    
    linux kernel'i içerisinde genel amaçlı kullanılan allocator'dır. user space'deki malloc'un equivalent'i diyebiliriz. daha küçük boyutlar için SLAB allocator kullanır. 
    ```
    #include <linux/slab.h>
    
    // Allocate size bytes, and return a pointer to the area (virtual address)
    // size: number of bytes to allocate
    // flags: same flags as the page allocator

    void *kmalloc(size_t size, gfp_t flags);
    void kfree(const void *objp);
    void *kzalloc(size_t size, gfp_t flags);
    void *kcalloc(size_t n, size_t size, gfp_t flags);
    void *krealloc(const void *p, size_t new_size, gfp_t flags);
    ```

    dev_* ile başlayan aynı kernel alloc fonksiyonlarıbulunur. bunların kullandığı memory otomatik olarak free edilir unprobed olduğu zaman.


- vmalloc alloacator

    virtual address space içerisinde contiguous bellek allocate eder ancak physical olarak böyle olmak zorunda değildir. **DMA kullanımı için uygun değildir** genellikle çok büyük alan allocate edildiğinde gözükebilir.
    `void *vmalloc(unsigned long size);` ve `void vfree(void *addr)` kullanılabilir.

## I/O Memory

**Memory-Mapped I/O:** Register'lara erişim yapma şeklimiz normal bir adrese erişmek ile aynıdır. MMIO’da bellek ve I/O cihazları aynı adres alanını paylaşır. Yani belirli adres aralıkları RAM yerine cihaz register’larına karşılık gelir. Aynı instruction'lar ile (store/load) erişebiliriz. özel farklı instruction'lara gerek kalmaz. Kernel'e hangi memory bölgesinde I/O register'ları olduğunu söylemek gereklidir. `struct resource *request_mem_region(unsigned long start, unsigned long len, char *name);` ile bunu yapabiliriz. **Driver tarafından bu Memory Mapped I/O adresini erişmek için virtual adres için ek bir map işlemi gereklidir.** `ioremap` bunun için kullanılır. `void __iomem *ioremap(phys_addr_t phys_addr, unsigned long size);`

- Managed API: yukarıda bahsedilen ap'ler deprecated olacaklardır.bunun yerie device managed api'ler öncekilere benzer şekilde bulunur.
    - `devm_ioremap()`
    - `devm_ioremap_resource()` : hem request hem de remap işlemini yapar
    - `devm_platform_ioremap_resource()` : gereken tüm işlemleri tek api ile yapar.


`ioremap()`in döndürdüğü adresleri doğrudan read/write yapmak bazı mimarilerde çalışmayabilir. bunlar için bazı accessor fonksiyonlar oluşturulmuştur.
    - read[b/w/l/q] and write[b/w/l/q] for access to **little-endian** devices, includes memory barriers
    - ioread[8/16/32/64]be and iowrite[8/16/32/64]be for access to **big-endian** devices, includes memory barriers
    - __raw_read[b/w/l/q] and __raw_write[b/w/l/q] for raw access: no endianness conversion, no memory barriers

