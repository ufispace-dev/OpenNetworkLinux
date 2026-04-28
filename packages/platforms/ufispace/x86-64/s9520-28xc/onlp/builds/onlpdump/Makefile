PLATFORM := x86-64-ufispace-s9520-28xc
EXTRA_MODULES := ufispace_common
include $(ONL)/packages/base/any/onlp/builds/platform/onlps.mk

comma := ,
$(BINARY_DIR)/$(BINARY): $(BINARY)_LINK_LIBS = \
    $(foreach lib, $($(BINARY)_LIBRARIES), \
        $(if $(findstring ufispace, $(lib)), \
            -Wl$(comma)--whole-archive $(LIBRARY_DIR)/$(lib) -Wl$(comma)--no-whole-archive, \
            $(LIBRARY_DIR)/$(lib) \
        ) \
    )