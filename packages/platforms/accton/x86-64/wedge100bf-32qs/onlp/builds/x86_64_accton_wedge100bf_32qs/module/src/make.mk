###############################################################################
#
#
#
###############################################################################

LIBRARY := x86_64_accton_wedge100bf_32qs
$(LIBRARY)_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/lib.mk
