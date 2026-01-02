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
