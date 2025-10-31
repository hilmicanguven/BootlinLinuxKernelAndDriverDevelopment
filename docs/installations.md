Terminal uygulaması için "picocom" uygulamasını deneyeceğiz.
- Kurulum : `sudo apt-get install picocom`
- Kullanım: picocom [options] <tty port device>, -->  `picocom -b 115200 /dev/ttyUSB0`
- USB cihaz hareketlilikleri -> `sudo dmesg -w | grep -i usb`