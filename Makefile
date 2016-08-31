
SUBDIRS =  misc-progs misc-modules \
           scull scullc scullp scullv short snull skull sculld sbull \
	   shortprint pci simple usb  lddbus tty input procfs sysfs \
	   module chrdev netlink readfile

all: subdirs

subdirs:
	for n in $(SUBDIRS); do $(MAKE) -C $$n || exit 1; done

clean:
	for n in $(SUBDIRS); do $(MAKE) -C $$n clean; done
