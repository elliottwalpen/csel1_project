# Part executed when called from kernel build system
ifneq ($(KERNELRELEASE),)
obj-m += fan_driver.o
## name of the generated module
fan_driver-objs := fan_driver_module.o
## list of objets needed by that module
CFLAGS_fan_driver_module := -DDEBUG

# Part executed when called from standard make in the module source directory
else
CPU := arm64
KDIR := /home/lmi/workspace/nano/buildroot/output/build/linux-5.8.6/
TOOLS := /home/lmi/workspace/nano/buildroot/output/host/usr/bin/aarch64-none-linux-gnu-
PWD := $(shell pwd)
MODPATH := /home/lmi/workspace/nano/rootfs # dev mode

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(CPU) CROSS_COMPILE=$(TOOLS) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
install:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install
endif
