ifneq ($(KERNELRELEASE),)
# call from kernel build system
BASEDIR=$(PWD)
EXTRA_CFLAGS += -I$(BASEDIR)/include
EXTRA_CFLAGS += -I$(BASEDIR)/../include

obj-m	:= osal.o
osal-objs := src/osal_event.o \
	     src/osal_irqlock.o \
	     src/memmap.o \
	     src/osal_irq.o \
	     src/osal_lock.o \
             src/osal_pci.o \
             src/osal_sema.o \
	     src/osal_thread.o \
             src/osal_trace.o \
             src/osal_linux_driver.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINCDIR=$(PWD)/../include modules

endif



clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers *.order
	rm -rf src/*.o src/*~

