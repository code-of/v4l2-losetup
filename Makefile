GTK3		 := $$(pkg-config --cflags --libs gtk+-3.0)
LIBS     := /usr/lib/libswscale.a /usr/lib/libavutil.a /usr/lib/libturbojpeg.a -lm
CC       := gcc -Wall -Ofast
SRC      := src/connection.c src/decoder.c

all:
	@echo 'compile & install kernel-module...'
	@make -C driver_src
	@install -m0755 driver_src/v4l2_loopback.ko.xz /lib/modules/`uname -r`/kernel/drivers/media/v4l2-core/
	@make -C driver_src clean
	@echo 'compile & install user binary...'
	@$(CC) $(SRC) src/v4l2-losetup.c $(LIBS) $(GTK3) -o v4l2-losetup
	@install -m0755 v4l2-losetup /usr/bin
	@install -m0644 src/v4l2-losetup.desktop /usr/share/applications
	@rm -f v4l2-losetup
	@echo 'you may now use "v4l2-losetup" to start the client !'