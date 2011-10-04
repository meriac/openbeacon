TARGET=openbeacon-blinkenlights-wmcu
ARCH=AT91SAM7X
CPU=$(ARCH)256
DEBUG=-g
OPTIM=-O2

APP_CFLAGS= \
    -Iapplication \
    -Inetwork/src/include \
    -Inetwork/src/include/ipv4 \
    -Inetwork/port \
    -Inetwork/port/emac \
    -Wno-unused-parameter

NETWORK_SRC= \
  network/port/emac/SAM7_EMAC.c \
  network/port/emac/SAM7_EMAC_ISR.c \
  network/port/sys_arch.c \
  network/src/core/dhcp.c \
  network/src/core/init.c \
  network/src/core/ipv4/icmp.c \
  network/src/core/ipv4/inet_chksum.c \
  network/src/core/ipv4/ip_addr.c \
  network/src/core/ipv4/ip.c \
  network/src/core/mem.c \
  network/src/core/memp.c \
  network/src/core/netif.c \
  network/src/core/pbuf.c \
  network/src/core/raw.c \
  network/src/core/stats.c \
  network/src/core/sys.c \
  network/src/core/udp.c \
  network/src/netif/etharp.c \
  network/src/netif/ethernetif.c

APP_SRC= \
  application/main.c \
  application/network.c \
  application/proto.c \
  application/xxtea.c \
  application/bprotocol.c \
  application/usbshell.c \
  $(CORE)/openbeacon/crc32.c \
  $(CORE)/openbeacon/rnd.c \
  $(NETWORK_SRC)
  
all: $(TARGET).bin
  
app_clean:
	find application network -name '*.o' -exec rm \{\} \;

include ../core/Makefile.rules
