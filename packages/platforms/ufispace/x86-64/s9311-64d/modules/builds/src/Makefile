x86-64-ufispace-s9311-64d-cpld-objs := x86-64-ufispace-s9311-64d-cpld-main.o x86-64-ufispace-s9311-64d-cpld-mux.o
obj-m += x86-64-ufispace-s9311-64d-cpld.o
obj-m += x86-64-ufispace-s9311-64d-lpc.o

KVERSION_MAJOR := $(shell echo $(KERNELVERSION) | cut -f1 -d.)
KVERSION_MINOR := $(shell echo $(KERNELVERSION) | cut -f2 -d.)
PATCH_MAJOR := 5
PATCH_MINOR := 0
KV_GE_PATCH := $(shell [ $(KVERSION_MAJOR) -gt $(PATCH_MAJOR) -o \( $(KVERSION_MAJOR) -eq $(PATCH_MAJOR) -a $(KVERSION_MINOR) -ge $(PATCH_MINOR) \) ] && echo true)
# patch i2c-ismt if kernel < 5.0
ifneq ($(KV_GE_PATCH),true)
 	obj-m += i2c-ismt.o
endif
