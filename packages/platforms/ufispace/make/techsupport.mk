TECH_SUPPORT_COMMON_DIR := $(ONL)/packages/platforms/ufispace/vendor-config/src/tech-support
TECH_SUPPORT_MAIN := $(TECH_SUPPORT_COMMON_DIR)/show_platform_log_main
TECH_SUPPORT_UTIL := $(TECH_SUPPORT_COMMON_DIR)/show_platform_log_util
TECH_SUPPORT_PLATFORM := $(CURDIR)/show_platform_log_platform
PLATFORM_TECH_SUPPORT_DIR := $(abspath $(CURDIR)/../../src/lib/tech_support)
PLATFORM_TECH_SUPPORT_FILE := $(PLATFORM_TECH_SUPPORT_DIR)/show_platform_log.sh

$(PLATFORM_TECH_SUPPORT_FILE): | $(TECH_SUPPORT_UTIL) $(TECH_SUPPORT_MAIN) $(TECH_SUPPORT_PLATFORM) $(PLATFORM_TECH_SUPPORT_DIR)
	(cat $(TECH_SUPPORT_UTIL); echo ""; cat $(TECH_SUPPORT_PLATFORM); echo ""; cat $(TECH_SUPPORT_MAIN)) > $@
	chmod +x $@

$(PLATFORM_TECH_SUPPORT_DIR):
	mkdir -p $@