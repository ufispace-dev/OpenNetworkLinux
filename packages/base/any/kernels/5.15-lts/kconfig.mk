THIS_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
K_MAJOR_VERSION := 5
K_PATCH_LEVEL := 15
K_SUB_LEVEL := 100
K_SUFFIX :=
K_PATCH_DIR := $(THIS_DIR)/patches
K_MODSYNCLIST := tools/objtool
