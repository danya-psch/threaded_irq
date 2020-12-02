KDIR ?= ~/repos/linux-stable
INSTALL_MOD_PATH ?= ~/repos/busybox/_install

default:
	$(MAKE) -C $(KDIR) M=$$PWD
clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean