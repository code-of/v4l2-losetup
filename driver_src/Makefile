obj-m := v4l2_loopback.o

all:
	@make -C /lib/modules/`uname -r`/build M=`pwd`
	@xz -f v4l2_loopback.ko

test:
	gcc test.c -o test

clean:
	make -C /lib/modules/`uname -r`/build M=`pwd` clean

insmod:
	sudo insmod v4l2_loopback.ko width=640 height=480

rmmod:
	sudo rmmod v4l2_loopback.ko
