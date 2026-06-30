from onl.platform.ufispace import OnlPlatformUfiSpace
from .base.config_base import UFispacePlatformBase

class OnlPlatform_x86_64_ufispace_s9620_40dg_r0(UFispacePlatformBase, OnlPlatformUfiSpace):

    # Provide the functions/variables below for which implementation is to be overwritten
    PATH_S3IP_CFG_ALPHA = "{}/current/onl/s3ip_config_alpha.yml".format(OnlPlatformUfiSpace.PATH_CONFIG_ROOT_DIR)
