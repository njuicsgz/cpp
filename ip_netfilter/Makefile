PWD := $(shell pwd)
KERNELDIR = /lib/modules/$(shell uname -r)/build

obj-m := ip_nf.o
ip_nf-objs := ip_nf_core.o ip_nf_conn.o

EXTRA_CFLAGS += -DIP_NF_DEBUG  

default:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules

clean:
	@touch *
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions Module.symvers

install:
	/sbin/insmod ip_nf.ko
remove:
	/sbin/rmmod ip_nf