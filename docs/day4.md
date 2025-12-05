1. Linux Device Driver Model

platformlar arası kod kullanımının maksimum olması hedeflenir. Driver'lar
- framework: kernel tarafından oluşturulan framework'ler ile donanıma generic şekilde erişebilir. kernel ile syscall'lar arasında yer alır.
- bus infrastructure: donanımla konuşmak, onları bulmak ve konfigüre etmek için kullanılır. sürücüler ile donanım arasında yer alır.

ile işlevleri yerine getirir. genel anlamda hiyerarşi yukarıdan aşağıda

Application
    |
System Call Interface
    |
Framework
    |
----------k e r n e l -------------------
Driver
   |
Bus infrastructure
----------k e r n e l -------------------
    |
Hardware

2. Device Model Data Structures

3 ana data structure olacak şekilde organize edilmiştir.
- The `struct bus_type structure`, which represents one type of bus (USB, PCI, I2C, etc.)
- The  `struct device_driver structure`, which represents one driver capable of handling certain devices on a certain bus.
- The `struct device` structure, which represents one device connected to a bus. buna instance da diyebiliriz. birden fazla usb cihazı olduğunda birden fazşa bu struct'tan olur.

- Bus Drivers
    - genellikle protokolleri olan (USB, PCI, I2C, SPI, MMC etc) tanımlanan bus'lar için oluşturulan bus yapısıdır.
    - bus'larüzerinde bulunan cihazların init'inden, tespit edilmesinden, enumaration'dan vb sorumludur.
    - eğer bulunan donanımlarauygun driver varsa bunlar eşleştirmekten sorumludur.
    - `/sys/bus` içerisinde bulunan tüm bus yapıları görülebilir. 
    - komut ile cihazlara ait bilgilere erişebiliriz virtual file system ile 
    ```
        hilmi@lnb-hguven:/sys/bus/i2c/devices/i2c-0$ cat name
        SMBus I801 adapter at efa0
    ```
    - `/sys/devices/` contains the list of devices
    - adapter: i2c controller ti chip için bir adapter ya da stm32 için adapter oluşturulabilir. bus driver, adapter'ler için bir çeşit wrapper'dır. hardware controller olarak da adlandırıldığını gördüm.

- Core infrastructure (bus driver)
    - drivers/usb/core/
    - struct bus_type is defined in drivers/usb/core/driver.c and registered in drivers/usb/core/usb.c
- Adapter drivers
    - drivers/usb/host/
    - For EHCI, UHCI, OHCI, XHCI, and their implementations on various systems (Microchip, IXP, Xilinx, OMAP, Samsung, PXA, etc.)
- Device drivers
    - Everywhere in the kernel tree, classified by their type (Example: drivers/net/usb/)

her bir cihaz için USB Core yapısına kaydedilmesi gereken usb sürücüsü olmalıdır. bu sürücü `struct device_driver` struct'ını inherit ederek kendi fonksiyonlarını bağlar. C dilinde inherit etme bulunmıyor tabi burada `struct usb_driver` içerisinde`struct device_driver` struct'ından bir field bulunur.
    - probe: cihazı ilklendirme ve instance yaratmak için gerekir. eğer bir kernel framework'e kayıt edilecekse (örneğin network interface) bu işlem yapılır.
    - disconnect: cihaz fiziksel olarak çıkarıldığında çalışır. memory free gibi clean-up işlemler yapılabilir
    - id_table: desteklen cihazların vendor/device id'lerini içeren tablodur.
```
static struct usb_driver rtl8150_driver = {
    .name = "rtl8150",
    .probe = rtl8150_probe,
    .disconnect = rtl8150_disconnect,
    .id_table = rtl8150_table,
    .suspend = rtl8150_suspend,
    .resume = rtl8150_resume
};
```


- Platform Drivers

usb, pci gibi bus'ları discoverable bus olarak isimlendiriyorduk. bir de non-discoverable olanlar bulunur ki bu cihazlar System-on-Chip in bir parçası olarak gelir. UART controllers, Ethernet controllers, graphic or audio devices etc. bu cihazların bulunduğu bus'ı Linux Kernel'i içerisinde "platform bus" olarak adlandırıyoruz. Bu bus'da diğerlerine benzer ancak bir "enumeration" bulunmaz. dinamik olarak bulunmaz, aksine static olarak enumerate edilirler.

nxp imx serisi soc yi ait bir örnek verilmiştir. `module_platform_driver()` makrosu ile structure adresi verilerek init/exit fonksiyonlarında bir şey yapılmadan declare edilebilir. 

```
static struct platform_driver serial_imx_driver = {
    .probe = serial_imx_probe,
    .remove = serial_imx_remove,
    .id_table = imx_uart_devtype,
    .driver = {
        .name = "imx-uart",
        .of_match_table = imx_uart_dt_ids, // of:open firmware(device tree)
        .pm = &imx_serial_port_pm_ops,
    },
};
```

DT Device Tree içerisinde bazı hardware resource'larını (I/O registers, IRQ lines, çeşitli subsystem'ler (clocks, GPIOs, DMA channels etc) e dair bilgiler ) tanımlayabiliriz çünkü discover edemeyeceğiz. Array benzeri data structure ile tanımlanırlar.sonrasında çeşitli makrolar yardımıyla bu bilgileri alabiliriz.
```
res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
base = ioremap(res->start, PAGE_SIZE);
sport->rxirq = platform_get_irq(pdev, 0);

clk_get()
gpio_request()
dma_request_channel()

```

bazı sürücüler birden fazla device'ı sürebilir çünkü çok küçük farklılıklar bulunan aynı üreticiye ait device'lar bulunabilir. aynı flash cihazın' farklı hafızaya sahip olması durumu sanırım örnek verilebilir.



- Intro to I2C Subsystem

I2C protokolüne ait temel bilgiler:

    - bir master cihaza (microprocessor) birden fazla slave cihazın (sensor vb) low-speed bus'tır. Yalnızca iki hat'a ihtiyaç duyarız. SDA - Data Line, SCL - Clock Line.
    - yalnızca master'dan veri akışı başlatılabilir, slave cihaz bu transaction'a cevap verir. slave'lar unique adrese sahiptir ve kendisine bir istek geldiğini bu adrese göre anlar.
    - bu adresler hard-coded olarak belirlidir. ancak bazı durumlarda birkaç direnç ile bu adresin bazı bitlerini değiştirebiliyor oluruz.

I2C bus sürücüsü:

- Like all bus subsystems, the I2C bus driver is responsible for:
    • Providing an API to implement I2C controller drivers
    • Providing an API to implement I2C device drivers, in kernel space
    • Providing an API to implement I2C device drivers, in user space
    ▶ The core of the I2C bus driver is located in `drivers/i2c/`.
- The I2C controller drivers are located in `drivers/i2c/busses/`.
- The I2C device drivers are located throughout drivers/, depending on the framework used to expose the devices (e.g. drivers/input/ for input devices)

I2C device driver'ımızı register etmek için `struct device_driver` dan inherit olan `struct i2c_driver` kullanırız ve içerisine gerekli fonksiyonları bağlarız. `i2c_add_driver()` ve `i2c_del_driver()` fonksiyonları kullanarak sürücüyü kayıt edebiliriz ya da silebiliriz.

```
static const struct i2c_device_id adxl345_i2c_id[] = {
    { "adxl345", ADXL345 },
    { "adxl375", ADXL375 },
{ }
};

MODULE_DEVICE_TABLE(i2c, adxl345_i2c_id);

static const struct of_device_id adxl345_of_match[] = {
    { .compatible = "adi,adxl345" },
    { .compatible = "adi,adxl375" },
{ },
};

MODULE_DEVICE_TABLE(of, adxl345_of_match);

static struct i2c_driver adxl345_i2c_driver = {
    .driver = {
        .name = "adxl345_i2c",
        .of_match_table = adxl345_of_match,
    },
    .probe = adxl345_i2c_probe,
    .remove = adxl345_i2c_remove,
    .id_table = adxl345_i2c_id,
};

module_i2c_driver(adxl345_i2c_driver)
```

! @note probe fonksiyonu için cihazın kendisini gösteren `struct i2c_client` ı gösteren pointer'ı parametre olarak alır. burada sensor (imu) için bir örnek kod mevcut. bunu probe yapmak istediğimizde `struct iio_dev` framework'ünü kullandığını görürüz. iio = industrial io anlamına gelir ve kernel'in sağladığı bir feature'dur. sensörü tanımlamak için kullanılır. bazı sensörler (imu, temp sensor, gyro, adcs etc)


```
static int da311_probe(struct i2c_client *client,
const struct i2c_device_id *id)
{
    struct iio_dev *indio_dev; // framework structure
    da311_data *data; // per device structure
    ...
    // Allocate framework structure with per device struct inside
    indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
    data = iio_priv(indio_dev);
    data->client = client;
    i2c_set_clientdata(client, indio_dev);
    // Prepare device and initialize indio_dev
    ...
    // Register device to framework
    ret = iio_device_register(indio_dev);
    ...
    return ret;
}
```

- communication with I2C Device: raw API
    - en temelde iki fonksiyona sahip oluruz cihaz ile konuşmak için -> send() ve receive()
        - int i2c_master_send(const struct i2c_client *client, const char *buf, int count); //Sends the contents of buf to the client.
        - int i2c_master_recv(const struct i2c_client *client, char *buf, int count); //Receives count bytes from the client, and store them into buf
    - i2c cihazından bir data okumak için aslında önce hangi adresten okumak istediğimizi göndeririz. sonra okuruz. yani önce send sonra receive yaparız. bu nedenle `transfer()` adında bu transaction'ı yapacak bir fonksiyon bulunur genelde.

- SMBus Calls
    - i2c nin bir subset'idir ve bazı standard set-of-transaction tanımlamaları yapar
    - Linux smbus'ı destekleyen arayüzleri sağlar. `i2c_smbus_read_byte_data()` örnektir. bunu çağırdığımızda bizim yerimizi gerekli transaction'ı başlatır. gerisini düşünmemize gerek kalmaz.


# Kernel Frameworks for Device Drivers

## Types of devices

Genellikle üç çeşit device tipi bulunuyor.

- Network Devices: network interfaces (Wifi, bluetooth, CAN(?) vb) olarak karşımıza çıkar. `ip a` ile bu cihazları listeyip görebiliriz.
- Block Devices: storage cihazlar için kullanılır. `/dev` altında gözükürler. 
- Character Devices: diğer ikisi dışında kalan tipler içindir (graphics, serial,sound vb). /dev altında listelenir. en çok da bu tip device'lar karşımıza çıkar.

Major ve Minor olarak tüm sürücülere identifier numaralar veririz. major sayısı genellikle family'si hakkında bilgi verir. `ls -al /dev` ile device'lar ve major/minor numaraları görülebilir.