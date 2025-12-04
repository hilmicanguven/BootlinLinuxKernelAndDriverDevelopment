
Device Tree Properties

- **Compatible Property:** oluşturduğumuz node'ların hangi modüller ile uyumlu olduğunu (binding) belirtir. OS tarafından hangi sürücünün kullanılacağını karar vermek için kullanılır. real-hardware için oluştururken vendor ve model ismiyle oluşturulur. `compatible = "st,stm32mp1-dwmac", "snps,dwmac-4.20a";` örnek olarak verilebilir. Özel bir değeri vardır bu property'nin. `simple-bus` olarak tanımlanırsa memory mapped tüm node'lar iyi uyumludur.
    - Tüm Linux sürücüleri `struct of_device_id[]` şeklinde tanımlanan string formatında desteklediği cihazları belirtir.

@warning tüm sürücüler "platform_driver" olarak tanımlandır ve en basic struct `struct platform_driver` ile oluşturulur.Bu struct içinde `of_match_table` field'ına compatible olduğu sürücülerin `struct of_device_id xx` şeklinde oluşturulan struct pointer'ı verilir.


```
const struct of_device_id cs42l51_of_match[] = {
    { .compatible = "cirrus,cs42l51", },
    { }
};
MODULE_DEVICE_TABLE(of, cs42l51_of_match);
```

```
static struct i2c_driver cs42l51_i2c_driver = {
    .driver = {
        .name = "cs42l51",
        .of_match_table = cs42l51_of_match,
        .pm = &cs42l51_pm_ops,
    },
    .probe = cs42l51_i2c_probe,
    .remove = cs42l51_i2c_remove,
    .id_table = cs42l51_i2c_id,
};
```


- **Reg Property:** memory-mapped cihazlar için "base physical address" ve memory mapped register'ların "size" bilgisini veririz. node isminde yer alan `50027000` değeri ilk register'a ait adrestir. iki farklı base address varsa en düşük adres isimde yer alır. birden fazla entry'e sahip olabilir. i2c cihazlar için slave address değeri verilebilir. spi cihazlar için chip select değeri verilebilir.

```
sai4: sai@50027000 {
    reg = <0x50027000 0x4>, <0x500273f0 0x10>;
};

&i2c1 {
    hdmi-transmitter@39 {
    reg = <0x39>;
};
cs42l51: cs42l51@4a {
    reg = <0x4a>;
};
};
```

- **cells property:** Property numbers shall fit into 32-bit containers called `cells`. bu örnekte address ve size cell değeri 1 olduğu için `reg` ile tanımlanan addres ve size değeri 32bit'e sığmalıdır yani 1 cell'e. örneğin spi cihazı için bu değerler şöyle olmalıdır. spi cihazı için address değeri chip select'tir. bu da en fazla 8 veya 16 değerinde olabilir. 32-bit yeterli olduğundan `address-cells` 1 olur. `size-cells` ise 0 olur çünkü spi cihazı için size bilgisi vermeyiz.
```
module@a0000 {
    #address-cells = <1>;
    #size-cells = <1>;
    
    serial@1000 {
       reg = <0x1000 0x10>, <0x2000 0x10>;
    };
};
```

- **status property:** cihaz kullanılıyor mu (`okay` veya `ok`) yoksa kullanılmıyor mu (`disabled`)

resources: interrupts, clocks, DMA, reset lines

@todo add example code from the slides

bu kaynakları kullanacak modüller örnekte olduğu gibi kendi içerisinde property olarak tanımlar. interrupt için ne vermesi gerekli olduğu "intc" de belirtiler. interrupt-cells = <3> olduğundan spi bunu kullanmak için 3 tane cell'i tanımlamak zorundadır.

Generic Suffixes

- bir driver gpio'ya kullanmak istediğinde `xxx-gpios` şeklinde bir isimlendirme kullanır.

----------------------

Binding Syntax

xxx


-----------------------

- Introduction to Pin Muxing

genellikle soc üzerinde limitli sayıda pin bulunduğu için ne için kullanacağımızı iyi seçmek gerekir. ancak tüm özellikler aynı anda kullanılmayabilir. bu nedenle pin'lerin nereye bağlı olduğunu farklı zamanlarda farklı seçebilmek işimize gelir. 2 tane pini pinmux ile konfigüre ettiğimizi düşünelim. genellikle şunu yapmak isteriz birisi uart_rx diğeri uart_tx ya da birisi i2c_sda diğeri i2c_scl. bunları bir grup olarak düşünürüz. "pin group" adını veririz. gidip de birini uart_tx diğeri i2c_sda yapmak pek mantıklı olmayacaktır. bu pin group için yine device tree dosyasında property'ler bulunur.

`drivers/pinctrl` subsystem i bunu yapmak için kullanılır. bu sürücüyü genelde bizim güncellememiz gerekmez. soc'yi üreten kim ise kernel'e destek verdiğinde bupinmux işlemini de sağlıyor olmasını bekleriz. 

bu subsystem'in hem kernel hem de consumer (device driver) tarafı bulunur. consumer tarafı için şunu söyelemek isteriz ->  benim şu şu pinleri kullanmaya ihtiyacım var.device tree içinde de hangi pinler olduğunu söyleriz. asıl kullanacağımız kısım burası olur aslında. kernel içerisinde tüm pin tanımları ve pin operasyon fonksiyonları vb belirleriz. This subsystem, located in drivers/pinctrl/ provides a generic subsystem to handle pin muxing. It offers:
- A pin muxing driver interface, to implement the system-on-chip specific drivers that configure the muxing.
- A pin muxing consumer interface, for device drivers
    - bunlar subsystem den istedikleri pin konfiglerini device tree aracılığı ile belirtir.
    - SoC level (*.dtsi) ve Board Level (*.dts) olmak üzere iki şekile kullanılabilir


Lab içerisinde nasıl konfigüre ettiğimize bakacağız.



