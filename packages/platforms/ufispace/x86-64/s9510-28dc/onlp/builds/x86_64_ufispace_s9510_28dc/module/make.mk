###############################################################################
#
#
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_ufispace_s9510_28dc_INCLUDES := -I $(THIS_DIR)inc
x86_64_ufispace_s9510_28dc_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_ufispace_s9510_28dc_DEPENDMODULE_ENTRIES := init:x86_64_ufispace_s9510_28dc ucli:x86_64_ufispace_s9510_28dc

