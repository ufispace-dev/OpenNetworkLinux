
from onl.platform.ufispace import OnlPlatformUfiSpace
from .base.config_base import UFispacePlatformBase

class OnlPlatform_x86_64_ufispace_s9520_28xc_r0(UFispacePlatformBase, OnlPlatformUfiSpace):

    # Provide the functions/variables below for which implementation is to be overwritten
    PATH_S3IP_CFG_DENVERTON = "{}/current/onl/s3ip_config_denverton.yml".format(OnlPlatformUfiSpace.PATH_CONFIG_ROOT_DIR)
    PATH_S3IP_CFG_LCC       = "{}/current/onl/s3ip_config_lcc.yml".format(OnlPlatformUfiSpace.PATH_CONFIG_ROOT_DIR)
