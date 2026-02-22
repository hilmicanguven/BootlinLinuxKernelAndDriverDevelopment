# Concurrent Access to Resources: Locking

Concurrency, türkçeye eşzamanlılık olarak çeviriyoruz. Aynı anda olmasa da birbiriyle uyumluluk halinde aynı anda çalışıyormuşçasına birbirine paralel işlemlerin sorunsuz şekilde çalıştırılması diyebiliriz. Concurrency, multi-threaded da diyebiliriz, şu durumlarla oluşur

- Interrupts
- Kernel Preemption : Yüksek öncelikle bir thread'in mevcut thread'in çalışmasını yarıda kesebilmesidir
- Multiprocessing : Birden fazla CPU bulunan sistemlerde gerçekten paralel olarak birden fazla işin birden fazla Core'da çalışabilmesidir.

Bu conccurreny ile ilgili problemleri gidermek için mümkün oldukça lokal olmalı (lokal değişken kullanımı, lokal veri yapıları, shared resource kullanımından kaçınma) diyebiliriz. Olamadığı durumda, **locking** mekanizmaları kullanılır. Mutex, Semaphore vb mekanizmalar en yaygın olanlarıdır. Ortak paylaşılan bir kaynağa erişim gibi critical section içeren kod parçalarını koruma altına alırız. İşlemin yarıda kalmayacağını garanti etmiş oluruz.
 
## Linux Mutexes

Kernel'deki ana locking primitive'idir. *Binary Lock* olarak adlandırılır. Ancak mutex'ler yalnızca sleeping izinli olunan context'de kullanılabilir çünkü "Lock" etmek istediğimiz mutex nesnesi zaten başka birisi tarafından lock'lanmışsa (halihazırda kaynağa erişen başkası var ise) kendisi block'lanır ve bekler. `#include <linux/mutex.h` içerisinde bulunur. Mutex ile ilgili fonksiyonlar
- `void mutex_init(struct mutex *lock);`
- `void mutex_lock(struct mutex *lock)`
- `void mutex_unlock(struct mutex *lock);`

## Spinlocks

Mutex ile temel farkı, bir spinlock'u almak istediğinizde ancak zaten alınmışsa dahi blok'lanmaz/sleep yapmaz. return eder ve sürekli bir döngü içerisinde müsait olup olmadığı kontrol edilir (busy-wait). Spinlock, mutex'in aksine interrupt ve softirq/tasklet context'inden de kullanılabilir. Spinlock kullanımında preempiton'da disable edilir çünkü bu deadlock'a sebep olabilir. sebebi yüksek öncelikli bir task'ın sürekli bloklayabilir. preemption'a benzer şekilde, bir spinlock'u aldığımızı düşünelim. bu esnada interrupt oluşursa, o spinlock'u tekrar almak istersek sonsuza kadar beklememiz anlamına gelir ki bu deadlock olur. O nedenle IRQ'ların spinlock alındıktan sonra disable edilip spinlock bırakılacağı zaman tekrar enable edilmesi Developer tarafından tercih edilebilir. Bunların için özel fonksiyonlar mevcuttur. tabii bunu spinlock alınmadan önce IRQ durumu ne ise onu öğrenip aynı bırakmak gerekir. `flags` bunun için kullanılır.
- irqsave
- _irqrestore

**NOT:** Deadlock ile ilgili şu durumlara dikkat edilmelidir
- Aynı Lock'u iki kere almaya çalışılmamalıdır
- Multiple Lock'a ihtiyaç duyulduğunda, farklı yerlerde aynı sıra ile grab edilmelidir.

## Debugging Locking

Kernel içerisinde bir Tool mevcut. Hangi lock hangi context'de hangi sırayla alındı şeklinde bir log tutar. Oldukça faydalı gözüküyor. Ufak bir overhead ekliyor elbette logladığı için. enable etmek için `CONFIG_PROVE_LOCKING` menuconfig'den seçilebilir.

`CONFIG_DEBUG_ATOMIC_SLEEP` diğer bir faydalı özellik. daha lightweight'dir. İzin olmayan bir noktada sleep yapılmaya çalıştığında `dmesg` de görülen bir warning verir.

Lock yerine kullanılabilecek alternatifler:
- lock-free algorithms like *Read Copy Update (RCU)*
    - ensuring consisten access
- when relevant, use atomic operations
    - shared resource bir integer olduğunda ve Read-Modify-Write operasyonları için kullanışlıdır.
    - bir değişkene olduğu gibi bitwise test-and-set/clear işlemleri yapılabilir.


# Direct Memory Access DMA

DMA, CPU müdahalesi olmaksızın device/peripheral ile RAM arasında veri aktarımı yapılmasıdır.

Bazı peripheral'lar kendi içerisinde DMA Controller donanımını bulundurabilir. Network controller cihazlarında yaygındır. 
Onun dışında "External DMA Controller" dediğimiz SoC'nin sahip olduğu DMA Engine donanımı bulunur. 
Diğer peripheral'lar DMA Controller'dan bu bus'ı (channel olarak da adlandırırız) kullanarak veri aktarımı yapar.
DMA için Kernel de yazılan driver'şar bulunur.
Veri aktarımını "DMA Descriptor" ları aracılığı yapar. Bu descriptorlar basit data structure'lardır.
Bu descriptor'lar içerisinde genellik transferi açıklayan soruce/destination addresses, data, data size, bir sonraki descriptor'ı gösteren pointer vb bulunur.

- Cache Constraints: memory'de transfer yaparken CPU'yu devre dışı bıraktığımızda Cache ile ilgili kısımlar da devre dışı kalır.
bu durumda cache ve memory'deki dataların uyumlu (Coherent) olması gerekir. yanlışlıkla eski/stale data aktarımı yapmak istemeyiz.
bu durumu önlemek adına bazıbölgeler cache-disable edilerek memory'den okuamasını zorunlu hale getiririz ve doğru datanın olduğunu garanti etmiş oluruz.
buna benzer diğer durumlar sunumda açıklanmıştır. özel DMA api'leri içerisinde bu memory işlemleri yapıldığından sürücüde kullanırken endişe etmemize gerek kalmaz.
cahceflush veyainvalidate gibi işlemleri explicit olarak sürücüde yapmayız.

- Addressing Constraints
    -Memory ve Device adresleri için genellikle `phys_addr_t` türünde fiziksel adrese sahiptir.
    - CPU'larda memory'ye MMU(memory management unit) ile, Virtual Pointer'lar ile erişir. `void *`
    - DMA Controller, MMU kullanamadığı için sonucu olarak virtual adresleri dekullanmaz. dolayısı ile physical adres ya da IOMMU kullanmak zorundadır.
    - genelde DMA'ler 32-bit adres olacağını varsayar. 64bit sistem olsa bile bazı peripheral'lar 32bit destekliyor olabilir. bu soc'da bulunan her device için farklıdır. Map işlemini bunu consider ederek yapmalıyız. bu durumla ilişkili olabilecek bazı performans sorunları yaşanabilir. bunu çözmek adına optimist buffer boyutunu iyi belirlemek gerekir.

- Kernel APIs
    - `dma-mapping` API:
        - DMA buffer'larını allocate eder ve yönetir.
        - Coherency için generic interface'ler sağlar.
        - buffer map işlemi için iki yol vardır
            - Coherent Mapping: kernel'de buffer allocate eder ve bunu "coherent" memoryden alır. cache coherent adı da verilir. cache'i disable eder. descriptor'lar bu bölgeden olmalıdır. physical adreslerdir.
            - Streaming Mapping: buffer'ımız var. başka bir yerde alınmış olabilir. bunu map'leyerek DMA için kullanabiliriz.
    - `dmaengine` API:
        - DMA controller'ı soyutlar. Generic fonksiyonları sağlar.
    - `dma-buf` API
        - kernel içerisinde cihazlar arası DMA buffer paylaşımını mümkün kılar. Graphic Kartı ve Display Controller arası buffer'ları paylaşır data kopyalamak yerine.



User space'de bu bahsettiğimiz descriptor'lar, kullanılan buffer adresleri ya da sayısı vb kernel'deki oluşan olayları bilmek zorunda değildir. bilmez de.

```
The kernel takes care of both buffer allocation and mapping:
#include <linux/dma-mapping.h>

// allocate a region

void *                          /* Output: buffer address */
    dma_alloc_coherent(
    struct device *dev,         /* device structure, stores dma capabilities */
    size_t size,                /* Needed buffer size in bytes */
    dma_addr_t *handle,         /* Output Parameter: DMA bus address. either physical or IOMMU */
    gfp_t gfp                   /* Standard GFP flags */
);

void dma_free_coherent(struct device *dev, size_t size, void *cpu_addr, dma_addr_t handle);
```
    

NOT: Birden fazla buffer'ı birleştirerek scatter-gather adı verilen yöntemle daha büyük boyutlu kopyalama işlemleri yapabiliriz.

NOT: MMIO register'ları physical adrese sahip olduğunda bunun remapped edilmesi gerekir çünkü aksi halde IO-MMU ile erişemeyiz. IOMMU physical adresler ile çalışır. 

CPU virtual addr  ──MMU──▶ Physical RAM ◀──IOMMU──  DMA addr


Örnek kod parçası:

```
#include <linux/dma-mapping.h>

dma_addr_t dma_map_resource(
    struct device *,            /* device structure */
    phys_addr_t,                /* input: resource to use */
    size_t,                     /* buffer size */
    enum dma_data_direction,    /* Either DMA_BIDIRECTIONAL,
                                 * DMA_TO_DEVICE or
                                 * DMA_FROM_DEVICE */
    unsigned long attrs,        /* optional attributes */
);

void dma_unmap_resource(struct device *dev, dma_addr_t handle, size_t size, enum dma_data_direction dir, unsigned long attrs);
```

tüm DMA memory işlemleri sonrasında `int dma_mapping_error(struct device * dev, dma_addr_t dma_addr)` ile error check yapılması tavsiye edilir.


- Starting DMA Transfers
    - If the device you’re writing a driver for is doing peripheral DMA, no external API is involved.
    
    - If it relies on an external DMA controller, you’ll need to
        1. Ask the hardware to use DMA, so that it will drive its request line
        2. Use Linux **dmaengine framework**, especially its slave API

**dmaengine framework:** slave api'ler sağlar ve soc'deki dma controller kullanımı için bu framework ve sağladığı apileri kullanırız. struct ve api detayları için sunum incelenmesi şiddetle tavsiye edilir! ayrıca serial driver içerisinde de örnek kullanımları görebiliriz.


