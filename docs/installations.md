Terminal uygulaması için "picocom" uygulamasını deneyeceğiz.
- Kurulum : `sudo apt-get install picocom`
- Kullanım: picocom [options] <tty port device>, -->  `picocom -b 115200 /dev/ttyUSB0`
- USB cihaz hareketlilikleri -> `sudo dmesg -w | grep -i usb`
- `sudo adduser $USER dialout` mesajları alabilmek için kullanıcının eklenmesi gerekir.

TFTP Kurulumu

- `sudo apt update`
- `sudo apt install tftpd-hpa`
- `cat /etc/default/tftpd-hpa` ile config dosyasının içeriği gözlemlenebilir. Kendi ortamımda bu şekilde çıktı elde ettim.  
```
hilmi@lnb-hguven:~$ cat /etc/default/tftpd-hpa
# /etc/default/tftpd-hpa

TFTP_USERNAME="tftp"
TFTP_DIRECTORY="/srv/tftp"
TFTP_ADDRESS=":69"
TFTP_OPTIONS="--secure"
```
- Gerekli olursa izinlerin verilmesi gerekir.
    - `sudo mkdir -p /srv/tftp`
    - `sudo chmod -R 777 /srv/tftp`

- Servisleri restart edip çalışmaya başlayabiliriz.
    - `sudo systemctl restart tftpd-hpa`
    - `sudo systemctl status tftpd-hpa`