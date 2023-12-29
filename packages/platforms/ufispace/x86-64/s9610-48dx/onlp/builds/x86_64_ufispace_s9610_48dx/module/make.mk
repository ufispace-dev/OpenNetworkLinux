###############################################################################
#
#
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_ufispace_s9610_48dx_INCLUDES := -I $(THIS_DIR)inc
x86_64_ufispace_s9610_48dx_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_ufispace_s9610_48dx_DEPENDMODULE_ENTRIES := init:x86_64_ufispace_s9610_48dx ucli:x86_64_ufispace_s9610_48dx