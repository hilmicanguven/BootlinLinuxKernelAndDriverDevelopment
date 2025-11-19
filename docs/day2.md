

1. Booting Kernel via TFTP over U-Boot

Installations ve labs altında açıklanan tftp kurulumu ve network ayarlarının yapılmış olduğunu varsayarak devamına ilerliyorum.

setenv bootargs root=/dev/nfs rw ip=192.168.1.100:::::eth0 console=ttyS2,115200n8 nfsroot=192.168.1.1:/home/<user>/linux-kernel-beagleplay-labs/modules/nfsroot,nfsvers=3,tcp