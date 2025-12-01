resources: interrupts, clocks, DMA, reset lines

@todo add example code from the slides

bu kaynakları kullanacak modüller örnekte olduğu gibi kendi içerisinde property olarak tanımlar. interrupt için ne vermesi gerekli olduğu "intc" de belirtiler. interrupt-cells = <3> olduğundan spi bunu kullanmak için 3 tane cell'i tanımlamak zorundadır.

Generic Suffixes

- bir driver gpio'ya kullanmak istediğinde `xxx-gpios` şeklinde bir isimlendirme kullanır.

----------------------

Binding Syntax

xxx


-----------------------