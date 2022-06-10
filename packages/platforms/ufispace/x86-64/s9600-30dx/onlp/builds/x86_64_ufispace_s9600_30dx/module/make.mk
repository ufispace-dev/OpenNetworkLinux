###############################################################################
#
#
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_ufispace_s9600_30dx_INCLUDES := -I $(THIS_DIR)inc
x86_64_ufispace_s9600_30dx_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_ufispace_s9600_30dx_DEPENDMODULE_ENTRIES := init:x86_64_ufispace_s9600_30dx ucli:x86_64_ufispace_s9600_30dx

