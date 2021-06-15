###############################################################################
#
#
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_ufispace_s9710_76d_INCLUDES := -I $(THIS_DIR)inc
x86_64_ufispace_s9710_76d_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_ufispace_s9710_76d_DEPENDMODULE_ENTRIES := init:x86_64_ufispace_s9710_76d ucli:x86_64_ufispace_s9710_76d

