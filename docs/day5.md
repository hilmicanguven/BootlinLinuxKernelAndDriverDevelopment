Day 5

- Special File Operation: `long unlocked_ioctl(struct file *f, unsigned int cmd, unsigned long arg)` 
    - `ioctl` syscall ile çağırılan driver file operation'dır. unlocked olarak adlandırılma sebebi "Big Kernel Lock" tutmuyor olması sanıyorum.
    - sürücüyewrite/readötesinde farklı işlevsellik kazandırır. 
    - `cmd` parametresi hangi işlem yapılacağını belirten bir enum olur genellikle.
    - `arg` ile kernel'e data/parametre exchange için kullanılır.
    - örnek olarak bir sürücünün çalışacağı hızı değiştirmek/öğrenmek için bir cmd tanımlayıp ioctl ile çağırılabilir. bu cmd ve arg parametreleri tamamen ilgili sürücüye özel ve anlamlı şeylerdir.

# The Concept of Kernel Frameworks

Kernel de olan tüm driver'lar "character driver" olarak implemente edilmeyebilir. Bazı "framework" lere uygun olacak şekilde oluşturulabilirler. 
Örneğin, framebuffer, V4L(video for linux), serial, block core, network, ioo vb. örnek verilebilir. bu sayede aynı tür cihazlar için birnevi standardlatmış biryapı olmuş olur.
yeni eklenenler de bu çerçeveye uygun oluşturulabilir. user space açısından character device ile aynı gözükür. bir fark olmaz.
yine klasik şekilde open yapılarak file descriptor alınır ve sonrasında fonksiyonları çağırılabilir.
yeni bir kamera sürücüsü ekleneceği zaman V4L framework'üne uygun şekilde yazılabilir.
framebuffer frameowork ile uyumlu bir sürücü yazmak için.

- Kernel option CONFIG_FB
    - menuconfig FB
        - tristate "Support for frame buffer devices"
- Implemented in C files in drivers/video/fbdev/core/
- Defines the user/kernel API
    - include/uapi/linux/fb.h (constants and structures)
- Defines the set of operations a framebuffer driver must implement and helper functions for the drivers
    - struct fb_ops
    - include/linux/fb.h


