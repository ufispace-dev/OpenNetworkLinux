#!/usr/bin/python

from onl.platform.base import *
from onl.platform.current import OnlPlatformName
from .base.config_base import UFispaceBase, load_platform_config

@load_platform_config
class OnlPlatformUfiSpace(UFispaceBase, OnlPlatformBase):
    MANUFACTURER='UfiSpace'
    PRIVATE_ENTERPRISE_NUMBER=51242
    PLATFORM = OnlPlatformName.replace("_", "-")
    PATH_CONFIG_ROOT_DIR="/lib/platform-config"
    PATH_PLT_CFG = "{}/{}/onl/platform.yml".format(PATH_CONFIG_ROOT_DIR, PLATFORM)
    PATH_S3IP_CFG = "{}/current/onl/s3ip_config.yml".format(PATH_CONFIG_ROOT_DIR)
    LEVEL_DEFAULT = UFispaceBase.LEVEL_INFO