
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