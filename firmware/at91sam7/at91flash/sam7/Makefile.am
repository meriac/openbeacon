AM_CFLAGS = -Wall -Werror

AUTOMAKE_OPTIONS = gnu

bin_PROGRAMS = sam7

sam7_SOURCES = main.c io_@IOTYPE@.c samba.c cmd.c
noinst_HEADERS = io.h samba.h cmd.h loader128_data.h loader256_data.h 

EXTRA_DIST = driver/Makefile driver/at91.c \
	io_win32.c io_posix.c io_iokit.c io_libusb.c\
	loader/Makefile loader/at91.h loader/bin2c.c loader/crt0.S \
	loader/loader.c loader/loader.lds