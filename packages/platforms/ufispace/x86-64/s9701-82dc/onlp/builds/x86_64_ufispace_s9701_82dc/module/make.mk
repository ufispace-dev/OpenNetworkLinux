###############################################################################
#
#
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_ufispace_s9701_82dc_INCLUDES := -I $(THIS_DIR)inc
x86_64_ufispace_s9701_82dc_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_ufispace_s9701_82dc_DEPENDMODULE_ENTRIES := init:x86_64_ufispace_s9701_82dc ucli:x86_64_ufispace_s9701_82dc

