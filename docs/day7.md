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
- when relevant, use atomic operations

