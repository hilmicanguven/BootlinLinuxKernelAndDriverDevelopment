
Practical lab - I/O memory and ports

- Add UART devices to the board device tree
- Access I/O registers to control the device and send first characters to it.

# The Misc (Miscellaneous) Subsystem

Kernel içerisinde kullanılan başka bir framework'dür. Daha önce "input subsystem = nunchuk driver", "serial framework, serial driver" görmüştük. serial/tty subsystem'i oldukça eski ve biraz daha karışık gelebilir. çok eski olduğundan kimse pek de dokunmak istemiyor ve çalışan sistemi kırmak istemiyor :)

Ancak "misc" en basit subsystem olabilir. net olarak bir framework'e sığmayanları burada değerlendirebiliriz. bu çeşit driver'lar "raw character device" olarak implemente edilebilir. temelde iki fonksiyona sahiptir:
- int misc_register(struct miscdevice * misc);
- void misc_deregister(struct miscdevice * misc);

```
struct miscdevice {
    int minor;
    const char *name;
    // character device'ın sahip olduğu fonksiyon pointer'ları tutar
    const struct file_operations *fops;
    struct list_head list;
    // pointer to underlying physical device, platform device, I2C device
    struct device *parent;
    struct device *this_device;
    const char *nodename;
    umode_t mode;
};
```

Platform bus nedir?  

Linux’ta PCI, USB gibi karmaşık bus’lar için özel altyapılar vardır. Ancak SoC içindeki UART, I²C, SPI gibi çevre birimleri doğrudan CPU adres alanına bağlanır. Bunlar için “platform bus” adı verilen basit bir pseudo-bus tanımlanmıştır. `struct platform_device` rolü Donanımın kernel’e tanıtılması için kullanılır. İçinde cihazın adı, ID’si, kaynakları (I/O adresleri, IRQ numaraları) ve struct device alanı bulunur. Kernel, bu yapı sayesinde cihazı uygun platform_driver ile eşleştirir.

# Processes, Scheduling and Interrupts

## Processes and Scheduling

In UNIX, a process is created using fork() and is composed of

    - An address space, which contains the program code, data, stack, shared libraries, etc.
    - A single thread, which is the only entity known by the scheduler.

Additional threads can be created inside an existing process, using
pthread_create()
    - They run in the same address space as the initial thread of the process
    - They start executing a function passed as argument to pthread_create()

**NOT:** Her thread `struct task_struct` ile tanımlanır.

When speaking about process and thread, these concepts need to be clarified:

- Mode is the level of privilege allowing to perform some operations:
    - Kernel Mode: in this level CPU can perform any operation allowed by its architecture; any instruction, any I/O operation, any area of memory accessed.
    - User Mode: in this level, certain instructions are not permitted (especially those that could alter the global state of the machine), some memory areas cannot be accessed.
- Linux splits its address space in kernel space and user space
    - Kernel space is reserved for code running in Kernel Mode.
    - User space is the place were applications execute (accessible from Kernel Mode).
- Context represents the current state of an execution flow.
    - The process context can be seen as the content of the registers associated to this process: execution register, stack register...
    - The interrupt context replaces the process context when the interrupt handler is executed

### Sleeping

Sleep,bir process bir data'ya ihtiyaç duyduğunda Sleeping state'de beklemesidir. bu esnada scheduler farklı işlemler yapmaya devam eder bu process beklerken. Bir sürücü içerisindewait/sleep mekanizmaları

- Wait Queue: belirli bir event bekleyen thread'ler bu listeye eklenir.
    - `void wait_event(queue, condition)` : Sleeps until the task is woken up and the given C expression is true
    - `void wait_event_killable(queue, condition)` : Can be interrupted, but only by a fatal signal (SIGKILL). Returns -ERESTARTSYS if interrupted.
    - `void wait_event_interruptible(queue, condition)`
    - `void wait_event_timeout(queue, condition, timeout)`: Also stops sleeping when the task is woken up or the timeout expired (a timer is used)
    - `void wait_event_interruptible_timeout(queue, condition, timeout)`

Example Code:
```
sig = wait_event_interruptible(ibmvtpm->wq, !ibmvtpm->tpm_processing_cmd);

if (sig)
    return -EINTR;
```

Uykuda olan process'leri uyandırmak için `wake_up(&queue)` veya `wake_up_interruptible(&queue)` çağrıları yapılabilir. Ancak bekledikleri condition gerçekleşmemişse tekrar uykuya geçer. Condition'ı tekrar tekrar evaluate ederler.

Interrupt olmadığı durumda uyumak istersek busy-wait loop ile bunu sağlayabiliyoruz ancak bunun ne kadar verimsiz olduğunu açıklamaya gerek yok.
- uzun beklemeler için wait_event ile bekler `schedule()` ile CPU'yu salabiliriz
- daha kısa beklemeler için software loops kullanabiliriz
    -`msleep()`, `msleep_interruptible()` : put process in a sleep
    -`udelay()`, `udelay_range()` : CPU cycles atığı olabilir ancak kısa sürelerde tekrar context-switch yapılmaması adına göze alınabilir.
    -`cpu_relax()` : does nothing

**NOT:** `fsleep()` fonksiyonu sleep süresine göre auto-select best mechanism.

## Interrupt Management

The managed API is recommended:

`int devm_request_irq(struct device *dev, unsigned int irq, irq_handler_t handler, unsigned long irq_flags, const char *devname, void *dev_id);`

- `device` for automatic freeing at device or module release time.
- `irq` is the requested IRQ channel. For platform devices, use platform_get_irq() to retrieve the interrupt number.
- `handler` is a pointer to the IRQ handler function
- `irq_flags` are option masks (see next slide)
- `devname` is the registered name (for /proc/interrupts). For platform drivers, good idea to use pdev->name which allows to distinguish devices managed by the same driver (example: 44e0b000.i2c).
- `dev_id` is an opaque pointer. It can typically be used to pass a pointer to a per-device data structure. It cannot be NULL as it is used as an identifier for freeing interrupts on a shared line.

Interrupt handler içerisinde bazı limitler bulunur
- user space deki değerlere erişemeyiz. her process için farklı virtual adres olacak ve hangi process çalışırken oldu bilemeyebiliriz.
- interrupt handler CPU tarafından çalıştırılır, scheduler değil. bu nednele sleep vb işlemler yapılamaz. memory allocate işlemi de `GFP_ATOMIC` ile yapılmalıdır.
- bir interrupt çalışırken diğerleri disable edildiğinden (local CPU içerisinde) re-enterant veya nested değildir Linux içerisinde. hızlıca işlemleri bitirmelidir. sonrasında disable edilen interrupt'lar enable edilir.

**NOT:** `cat /proc/interrupts` ile sistemdeki interruptlara ait bilgileri görebiliriz. `watch -n 0.1 cat /proc/interrupts` ile 0.1 saniye aralıklarla aynı komutu çalıştırarak sistemdeki interrupt'ları anlık olarak görebiliriz.

Interrupt handler içerisinde yapılacak işlemlerin kısa olması gerektiğini söylemiştik. Eğer uzun işlem gerektiren işlemler yapılacaksa **Threaded Interrupts** kullanılabilir. int. handler bir thread içerisinde execute edilir. Threaded interrupt içerisindeyken diğer interrupt'lar enablr edilir. Ancak `flags` parametresi ile maskelenmek istenen irq'lar işaretlenebilir.
    - `int devm_request_threaded_irq(struct device *dev, unsigned int irq, irq_handler_t handler, irq_handler_t thread_fn, unsigned long flags, const char *name, void *dev);`

Splitting the execution of interrupt handlers in 2 parts
- Top half
    - This is the real interrupt handler, which should complete as quickly as possible since all interrupts are disabled. It takes the data out of the device and if substantial post-processing is needed, schedule a bottom half to handle it.
- Bottom half
    - Is the general Linux name for various mechanisms which allow to postpone the handling of interrupt-related work. Implemented in Linux as **softirqs, tasklets (deprecated) or workqueues**.
        - Workqueues: Workqueues are a general mechanism for deferring work. It is not limited in usage to handling interrupts. It can typically be used for background work which can be scheduled. Workqueue içerisinde sleep'e izin verilir.

