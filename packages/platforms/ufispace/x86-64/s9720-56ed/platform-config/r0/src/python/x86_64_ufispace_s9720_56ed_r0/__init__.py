from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
from ctypes import c_int, sizeof
import os
import sys
import subprocess
import yaml
import fcntl
import time

def msg(s, fatal=False):
    sys.stderr.write(s)
    sys.stderr.flush()
    if fatal:
        sys.exit(1)

class IPMI_Ioctl(object):
    _IONONE = 0
    _IOWRITE = 1
    _IOREAD = 2

    IPMI_MAINTENANCE_MODE_AUTO = 0
    IPMI_MAINTENANCE_MODE_OFF  = 1
    IPMI_MAINTENANCE_MODE_ON   = 2

    IPMICTL_GET_MAINTENANCE_MODE_CMD = _IOREAD << 30 | sizeof(c_int) << 16 | \
        ord('i') << 8 | 30  # from ipmi.h
    IPMICTL_SET_MAINTENANCE_MODE_CMD = _IOWRITE << 30 | sizeof(c_int) << 16| \
        ord('i') << 8 | 31  # from ipmi.h

    def __init__(self):
        self.ipmidev = None
        devnodes=["/dev/ipmi0", "/dev/ipmi/0", "/dev/ipmidev/0"]
        for dev in devnodes:
            try:
                self.ipmidev = open(dev, 'r+')
                break
            except Exception as e:
                print("open file {} failed, error: {}".format(dev, e))

    def __del__(self):
        if self.ipmidev is not None:
            self.ipmidev.close()

    def get_ipmi_maintenance_mode(self):
        input_buffer=pack('i',0)
        out_buffer=fcntl.ioctl(self.ipmidev, self.IPMICTL_GET_MAINTENANCE_MODE_CMD, input_buffer)
        maintanence_mode=unpack('i',out_buffer)[0]

        return maintanence_mode

    def set_ipmi_maintenance_mode(self, mode):
        fcntl.ioctl(self.ipmidev, self.IPMICTL_SET_MAINTENANCE_MODE_CMD, c_int(mode))

class OnlPlatform_x86_64_ufispace_s9720_56ed_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9720-56ed-r0'
    MODEL="S9720-56ED"
    SYS_OBJECT_ID=".9720.56"
    PORT_COUNT=56
    PORT_CONFIG="36x400 + 20x800"
    LEVEL_INFO=1
    LEVEL_ERR=2
    BSP_VERSION='1.0.0'
    PATH_I2C_CPLD1="/sys/bus/i2c/devices/1-0030"
    PATH_I2C_CPLD2="/sys/bus/i2c/devices/1-0031"
    PATH_I2C_FPGA="/sys/bus/i2c/devices/1-0037"
    PATH_I2C_CPLD4="/sys/bus/i2c/devices/24-0032"
    PATH_I2C_CPLD5="/sys/bus/i2c/devices/24-0033"
    PATH_SYS_I2C_DEV_ATTR="/sys/bus/i2c/devices/{}-{:0>4x}/{}"
    PATH_SYS_GPIO = "/sys/class/gpio"
    PATH_SYSTEM_LED="/sys/bus/i2c/devices/1-0030/system_led_status"
    SYSTEM_LED_GREEN=0b00001001
    PATH_LPC="/sys/devices/platform/x86_64_ufispace_s9720_56ed_lpc"
    PATH_LPC_GRP_BSP=PATH_LPC+"/bsp"
    PATH_LPC_GRP_MB_CPLD=PATH_LPC+"/mb_cpld"
    PATH_PORT_CONFIG="/lib/platform-config/"+PLATFORM+"/onl/port_config.yml"
    PATH_EPDM_CLI="/lib/platform-config/current/onl/epdm_cli"
    PATH_CPLD1_EVT_CTRL=PATH_I2C_CPLD1+"/event_detect_ctrl"
    PATH_CPLD2_EVT_CTRL=PATH_I2C_CPLD2+"/event_detect_ctrl"
    PATH_FPGA_EVT_CTRL=PATH_I2C_FPGA+"/event_detect_ctrl"
    PATH_CPLD4_EVT_CTRL=PATH_I2C_CPLD4+"/event_detect_ctrl"
    PATH_CPLD5_EVT_CTRL=PATH_I2C_CPLD5+"/event_detect_ctrl"
    PATH_BSP_GPIO_MAX=PATH_LPC_GRP_BSP+"/bsp_gpio_max"
    PATH_BSP_GPIO_BASE=PATH_LPC_GRP_BSP+"/bsp_gpio_base"

    port_type_dict = {
        0x03: [2, 'SFP/SFP+/SFP28'],  # [dev_class, type_str]
        0x0B: [2, 'DWDM-SFP/SFP+'],
        0x0C: [1, 'QSFP'],
        0x0D: [1, 'QSFP+'],
        0x11: [1, 'QSFP28'],
        0x18: [3, 'QSFP-DD Double Density 8x (INF-8628)'],
        0x19: [3, 'OSFP 8x Pluggable Transceiver'],
        0x1E: [3, 'QSFP+ or later with CMIS spec'],
        0x1F: [3, 'SFP-DD Double Density 2X Pluggable Transceiver with CMIS spec'],
    }

    port_conf = {
        0:  {"type": "QSFPDD_NIF" , "bus": 35, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p0_abs" }},
        1:  {"type": "QSFPDD_NIF" , "bus": 36, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p1_abs" }},
        2:  {"type": "QSFPDD_NIF" , "bus": 37, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p2_abs" }},
        3:  {"type": "QSFPDD_NIF" , "bus": 38, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p3_abs" }},
        4:  {"type": "QSFPDD_NIF" , "bus": 39, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p4_abs" }},
        5:  {"type": "QSFPDD_NIF" , "bus": 40, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p5_abs" }},
        6:  {"type": "QSFPDD_NIF" , "bus": 41, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p6_abs" }},
        7:  {"type": "QSFPDD_NIF" , "bus": 42, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p7_abs" }},
        8:  {"type": "QSFPDD_NIF" , "bus": 43, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p8_abs" }},
        9:  {"type": "QSFPDD_NIF" , "bus": 44, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p9_abs" }},
        10: {"type": "QSFPDD_NIF" , "bus": 45, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p10_abs"}},
        11: {"type": "QSFPDD_NIF" , "bus": 46, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p11_abs"}},
        12: {"type": "QSFPDD_NIF" , "bus": 47, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p12_abs"}},
        13: {"type": "QSFPDD_NIF" , "bus": 48, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p13_abs"}},
        14: {"type": "QSFPDD_NIF" , "bus": 49, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p14_abs"}},
        15: {"type": "QSFPDD_NIF" , "bus": 50, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p15_abs"}},
        16: {"type": "QSFPDD_NIF" , "bus": 51, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p16_abs"}},
        17: {"type": "QSFPDD_NIF" , "bus": 52, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p17_abs"}},
        18: {"type": "QSFPDD_NIF" , "bus": 53, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p18_abs"}},
        19: {"type": "QSFPDD_NIF" , "bus": 54, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_nif_p19_abs"}},
        20: {"type": "QSFPDD_NIF" , "bus": 65, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p20_abs"}},
        21: {"type": "QSFPDD_NIF" , "bus": 66, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p21_abs"}},
        22: {"type": "QSFPDD_NIF" , "bus": 67, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p22_abs"}},
        23: {"type": "QSFPDD_NIF" , "bus": 68, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p23_abs"}},
        24: {"type": "QSFPDD_NIF" , "bus": 69, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p24_abs"}},
        25: {"type": "QSFPDD_NIF" , "bus": 70, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p25_abs"}},
        26: {"type": "QSFPDD_NIF" , "bus": 71, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p26_abs"}},
        27: {"type": "QSFPDD_NIF" , "bus": 72, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p27_abs"}},
        28: {"type": "QSFPDD_NIF" , "bus": 73, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p28_abs"}},
        29: {"type": "QSFPDD_NIF" , "bus": 74, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p29_abs"}},
        30: {"type": "QSFPDD_NIF" , "bus": 75, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p30_abs"}},
        31: {"type": "QSFPDD_NIF" , "bus": 76, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p31_abs"}},
        32: {"type": "QSFPDD_NIF" , "bus": 77, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p32_abs"}},
        33: {"type": "QSFPDD_NIF" , "bus": 78, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p33_abs"}},
        34: {"type": "QSFPDD_NIF" , "bus": 79, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p34_abs"}},
        35: {"type": "QSFPDD_NIF" , "bus": 80, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_nif_p35_abs"}},
        36: {"type": "SFP"        , "bus": 83, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_FPGA+"/sfp28_p36_abs"      }},
        37: {"type": "SFP"        , "bus": 84, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_FPGA+"/sfp28_p37_abs"      }},
        38: {"type": "QSFPDD_FAB" , "bus": 25, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_fab_p0_abs" }},
        39: {"type": "QSFPDD_FAB" , "bus": 26, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_fab_p1_abs" }},
        40: {"type": "QSFPDD_FAB" , "bus": 27, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_fab_p2_abs" }},
        41: {"type": "QSFPDD_FAB" , "bus": 28, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_fab_p3_abs" }},
        42: {"type": "QSFPDD_FAB" , "bus": 29, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_fab_p4_abs" }},
        43: {"type": "QSFPDD_FAB" , "bus": 30, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_fab_p5_abs" }},
        44: {"type": "QSFPDD_FAB" , "bus": 31, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_fab_p6_abs" }},
        45: {"type": "QSFPDD_FAB" , "bus": 32, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_fab_p7_abs" }},
        46: {"type": "QSFPDD_FAB" , "bus": 33, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_fab_p8_abs" }},
        47: {"type": "QSFPDD_FAB" , "bus": 34, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/qsfpdd_fab_p9_abs" }},
        48: {"type": "QSFPDD_FAB" , "bus": 55, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_fab_p10_abs"}},
        49: {"type": "QSFPDD_FAB" , "bus": 56, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_fab_p11_abs"}},
        50: {"type": "QSFPDD_FAB" , "bus": 57, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_fab_p12_abs"}},
        51: {"type": "QSFPDD_FAB" , "bus": 58, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_fab_p13_abs"}},
        52: {"type": "QSFPDD_FAB" , "bus": 59, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_fab_p14_abs"}},
        53: {"type": "QSFPDD_FAB" , "bus": 60, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_fab_p15_abs"}},
        54: {"type": "QSFPDD_FAB" , "bus": 61, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_fab_p16_abs"}},
        55: {"type": "QSFPDD_FAB" , "bus": 62, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_fab_p17_abs"}},
        56: {"type": "QSFPDD_FAB" , "bus": 63, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_fab_p18_abs"}},
        57: {"type": "QSFPDD_FAB" , "bus": 64, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_fab_p19_abs"}},
        58: {"type": "MGMT"       , "bus": 81, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_FPGA+"/mgmt_p0_abs"        }},
        59: {"type": "MGMT"       , "bus": 82, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_FPGA+"/mgmt_p1_abs"        }}
    }

    gpio_map = {
        511:{'offset': {"max": 0 , "base": 15}, 'dir': 'in'   , 'desc': "reserve"},
        510:{'offset': {"max": 1 , "base": 14}, 'dir': 'low'  , 'desc': "7SEG_RD"},
        509:{'offset': {"max": 2 , "base": 13}, 'dir': 'low'  , 'desc': "7SEG_RC"},
        508:{'offset': {"max": 3 , "base": 12}, 'dir': 'low'  , 'desc': "7SEG_RE"},
        507:{'offset': {"max": 4 , "base": 11}, 'dir': 'low'  , 'desc': "7SEG_RB"},
        506:{'offset': {"max": 5 , "base": 10}, 'dir': 'high' , 'desc': "7SEG_RG"},
        505:{'offset': {"max": 6 , "base": 9 }, 'dir': 'low'  , 'desc': "7SEG_RF"},
        504:{'offset': {"max": 7 , "base": 8 }, 'dir': 'low'  , 'desc': "7SEG_RA"},
        503:{'offset': {"max": 8 , "base": 7 }, 'dir': 'in'   , 'desc': "reserve"},
        502:{'offset': {"max": 9 , "base": 6 }, 'dir': 'low'  , 'desc': "7SEG_LA"},
        501:{'offset': {"max": 10, "base": 5 }, 'dir': 'low'  , 'desc': "7SEG_LB"},
        500:{'offset': {"max": 11, "base": 4 }, 'dir': 'low'  , 'desc': "7SEG_LF"},
        499:{'offset': {"max": 12, "base": 3 }, 'dir': 'high' , 'desc': "7SEG_LG"},
        498:{'offset': {"max": 13, "base": 2 }, 'dir': 'low'  , 'desc': "7SEG_LD"},
        497:{'offset': {"max": 14, "base": 1 }, 'dir': 'low'  , 'desc': "7SEG_LE"},
        496:{'offset': {"max": 15, "base": 0 }, 'dir': 'low'  , 'desc': "7SEG_LC"}
    }


    def get_conf(self, board=None):
        if board is None:
            gpio_map = self.gpio_map
            port_conf = self.port_conf
        elif board['hw_rev'] == 0:
            gpio_map = self.gpio_map_proto if hasattr(self, 'gpio_map_proto') else self.gpio_map
            port_conf = self.port_conf_proto if hasattr(self, 'port_conf_proto') else self.port_conf
        elif board['hw_rev'] == 1:
            gpio_map = self.gpio_map_alpha if hasattr(self, 'gpio_map_alpha') else self.gpio_map
            port_conf = self.port_conf_alpha if hasattr(self, 'port_conf_alpha') else self.port_conf
        elif board['hw_rev'] == 2:
            gpio_map = self.gpio_map_beta if hasattr(self, 'gpio_map_beta') else self.gpio_map
            port_conf = self.port_conf_beta if hasattr(self, 'port_conf_beta') else self.port_conf
        elif board['hw_rev'] == 3:
            gpio_map = self.gpio_map_pvt if hasattr(self, 'gpio_map_pvt') else self.gpio_map
            port_conf = self.port_conf_pvt if hasattr(self, 'port_conf_pvt') else self.port_conf
        else:
            gpio_map = self.gpio_map
            port_conf = self.port_conf
        return (port_conf, gpio_map)


    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self, board):
        sysfs_mux_reset = self.PATH_LPC_GRP_MB_CPLD + "/mb_i2c_mux_rst"

        # Check I2C status
        retcode = os.system("i2cget -f -y 0 0x72 > /dev/null 2>&1")
        if retcode != 0:

            #read mux failed, i2c bus may be stuck
            self.bsp_pr("Warning: Read I2C Mux Failed!! (ret={})".format(retcode))

            # Not supporting I2C recovery
            if 0:
            #Recovery I2C   
                if os.path.exists(sysfs_mux_reset):
                    with open(sysfs_mux_reset, "w") as f:
                        #write 0 to sysfs
                        f.write("{}".format(0))
                        self.bsp_pr("I2C bus recovery done.")
                else:
                    self.bsp_pr("Warning: I2C recovery sysfs does not exist!! (path={})".format(sysfs_mux_reset))

    def bsp_pr(self, pr_msg, level = LEVEL_INFO):
        if level == self.LEVEL_INFO:
            bsp_pr = self.PATH_LPC_GRP_BSP+"/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            bsp_pr = self.PATH_LPC_GRP_BSP+"/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            bsp_pr = self.PATH_LPC_GRP_BSP+"/bsp_pr_info"

        if os.path.exists(bsp_pr):
            with open(bsp_pr, "w") as f:
                f.write(pr_msg)
        else:
            msg("Warning: bsp logging sys is not exist\n")

    def config_bsp_ver(self, bsp_ver):
        bsp_version_path=self.PATH_LPC_GRP_BSP+"/bsp_version"
        if os.path.exists(bsp_version_path):
            with open(bsp_version_path, "w") as f:
                f.write(bsp_ver)

    def get_board_version(self):
        board = {}
        board_attrs = {
            "hw_rev"  : {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/cpld_hw_rev"},
            "deph_id" : {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/cpld_deph_rev"},
            "hw_build": {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/cpld_build_rev"},
        }

        for key, val in board_attrs.items():
            cmd = "cat {}".format(val["sysfs"])
            try:
                output = subprocess.check_output(cmd.split())
            except Exception as e:
                self.bsp_pr("Get hw rev id from LPC failed, exception={}".format(e), self.LEVEL_ERR)
                output="1"
            board[key] = int(output, 10)

        return board

    def get_gpio_max(self):
        cmd = "cat {}".format(self.PATH_BSP_GPIO_MAX)
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("Get gpio max failed, exception={}".format(e), self.LEVEL_ERR)
            output="511"

        gpio_max = int(output, 10)

        return gpio_max

    def get_gpio_base(self):
        cmd = "cat {}".format(self.PATH_BSP_GPIO_BASE)
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("Get gpio base failed, exception={}".format(e), self.LEVEL_ERR)
            output="512"

        gpio_base = int(output, 10)

        return gpio_base

    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = self.PATH_SYS_I2C_DEV_ATTR.format(i2c_bus, i2c_addr, "idle_state")
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def init_mux(self, board):
        bus_i801=0
        i2c_muxs = [
            ('pca9548',       0x71, bus_i801),  #9548_ROOT_FPGA_CPLD
            ('pca9546',       0x72, bus_i801),  #9546_ROOT_CLK
            ('pca9548',       0x73, bus_i801),  #9548_ROOT_PWR
            ('pca9546',       0x76, 12),        #9546_CHILD_CPLD4_5
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux 
        self.init_i2c_mux_idle_state(i2c_muxs)

    def init_sys_eeprom(self, board):
        sys_eeprom = [
            ('sys_eeprom', 0x57, 0),
        ]

        self.new_i2c_devices(sys_eeprom)

    def init_cpld(self, board):
        cpld = [
            ('s9720_56ed_cpld4', 0x32, 24),
            ('s9720_56ed_cpld5', 0x33, 24),
            ('s9720_56ed_cpld1', 0x30, 1),
            ('s9720_56ed_cpld2', 0x31, 1),
            ('s9720_56ed_fpga' , 0x37, 1),
        ]

        self.new_i2c_devices(cpld)

    def init_eeprom(self, board):
        data = None

        with open(self.PATH_PORT_CONFIG, 'r') as yaml_file:
            data = yaml.safe_load(yaml_file)

        # config eeprom
        port_conf, _ = self.get_conf()
        for port, config in port_conf.items():
            addr=0x50
            self.new_i2c_device(config["driver"], addr, config["bus"])
            port_name = data[config["type"]][port]["port_name"]
            sysfs=self.PATH_SYS_I2C_DEV_ATTR.format( config["bus"], addr, "port_name")
            os.system("echo {} > {}".format(port_name, sysfs))

    def init_gpio(self, gpio_max, gpio_base, board=None):

        self.new_i2c_devices(
            [
                ('pca9555', 0x20, 8), #9555_IO_EXP_TCA9555_1 (9555_LED_BOARD)
            ]
        )

        _, gpio_map = self.get_conf(board)
        for _, conf in gpio_map.items():
            if gpio_max < 0:
                gpio_num = gpio_base + conf['offset'].get("base")
            else:
                gpio_num = gpio_max - conf['offset'].get("max")
            gpio_dir = conf['dir']
            os.system("echo {} > {}/export".format(gpio_num, self.PATH_SYS_GPIO))
            os.system("echo {}   > {}/gpio{}/direction".format(gpio_dir, self.PATH_SYS_GPIO, gpio_num))

    # Certain signals (e.g., qsfp_reset) need time to settle after being set.
        # A delay is added here to prevent failures in subsequent operations and ensure
        # the configuration is applied correctly.
        time.sleep(0.5)

    def get_port_presence(self, port, gpio_max = 511, gpio_base = 0, board=None):
        try:
            port_conf, gpio_map = self.get_conf(board)
            if port not in port_conf:
                return False

            abs_type = port_conf[port]['abs'].get('type')
            abs_data = port_conf[port]['abs'].get('data')

            if abs_type == 'gpio':
                if gpio_max < 0:
                    gpio_num = gpio_base + gpio_map[abs_data]['offset'].get("base")
                else:
                    gpio_num = gpio_max - gpio_map[abs_data]['offset'].get("max")
                    sysfs = "{}/gpio{}/value".format(self.PATH_SYS_GPIO, gpio_num)
            elif abs_type == 'sysfs':
                sysfs = abs_data
            else:
                return False

            with open(sysfs, "r") as f:
                present_raw = f.read().strip()

            pres_status = True if reg_val == 0 else False

            return pres_status

        except:
            return False

    def update_dev_class(self, gpio_max = 511, gpio_base = 0, board=None):
        port_conf, _ = self.get_conf(board)

        for port, config in port_conf.items():  # QSFPX ports

            if config.get('type') not in ['QSFPDD', 'QSFP']:
                continue

            # check module presence
            if not self.get_port_presence(port, gpio_max, gpio_base, board):
                continue

            bus = config.get('bus')
            # get dev_class
            sysfs = "/sys/bus/i2c/devices/{}-0050/dev_class".format(bus)
            cmd = ["cat", sysfs]
            dev_class_str = subprocess.check_output(cmd)
            dev_class = int(dev_class_str, 10)

            # get port type
            cmd = ["dd", "if=/sys/bus/i2c/devices/{}-0050/eeprom".format(bus), "bs=1", "count=1", "skip=0", "status=none"]
            output = subprocess.check_output(cmd)
            hex_str = unpack('B', output)[0]
            type_str = "{:02x}".format(hex_str)
            if type_str == "": #i2c maybe stuck
                self.check_i2c_status()
                continue
            port_type = int(type_str, 16)


            # check if port_type is in port_type_dict
            if port_type not in self.port_type_dict:
                self.bsp_pr("Port[{}] Type: {} is Unknown.".format(port, hex(port_type)))
                continue

            # check if dev_class matches port_type_dev_class
            port_type_dev_class = self.port_type_dict.get(port_type)[0]
            if dev_class != port_type_dev_class:
                with open(sysfs, "w") as f:
                    f.write("{}".format(port_type_dev_class))
                self.bsp_pr("Port[{}] dev_class is changed from {} to {}".format(port, dev_class, port_type_dev_class))

        self.bsp_pr("Please run ONLP API onlp_sfpi_dev_class_update() after inserting QSFP/QSFPDD modules at runtime")



    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        # self.bsp_pr("Current IPMI_MAINTENANCE_MODE={}".format(mode) )

        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        # self.bsp_pr("After IPMI_IOCTL IPMI_MAINTENANCE_MODE={}".format(mode) )

    def disable_bmc_watchdog(self):
        os.system("ipmitool mc watchdog off")

    def set_bmc_sel_time(self):
        os.system("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

    def enable_event_ctrl(self, board):
        # enable event ctrl
        os.system("echo 1 > "+ self.PATH_CPLD1_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_CPLD2_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_CPLD4_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_CPLD5_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_FPGA_EVT_CTRL)


    def set_system_led_green(self):
        raise NotImplementedError("Not Support")
        # if os.path.exists(self.PATH_SYSTEM_LED):
        #     with open(self.PATH_SYSTEM_LED, "r+") as f:
        #         led_reg = f.read()
        #  
        #         #write green to system led
        #         f.write("{}".format(self.SYSTEM_LED_GREEN))
        #  
        #         self.bsp_pr("Current System LED: {} -> 0x{:02x}".format(led_reg, self.SYSTEM_LED_GREEN))
        # else:
        #     self.bsp_pr("System LED sysfs not exist")
    
    def update_pci_device(self, driver, device, action):
        driver_path = os.path.join("/sys/bus/pci/drivers", driver, action)

        if os.path.exists(driver_path):
            try:
                with open(driver_path, "w") as file:
                    file.write(device)
            except Exception as e:
                print("Open file failed, error: {}".format(e))

    def init_i2c_bus_order(self):
        device_actions = [
            #driver_name   bus_address     action
            ("i801_smbus", "0000:00:1f.4", "unbind"),
            ("ismt_smbus", "0000:00:0f.0", "unbind"),
            ("i801_smbus", "0000:00:1f.4", "bind")
        ]

        # Iterate over the list and call modify_device for each tuple
        for driver_name, bus_address, action in device_actions:
            self.update_pci_device(driver_name, bus_address, action)

    def baseconfig(self):

        # load default kernel driver
        os.system("modprobe -rq i2c_i801")
        self.insmod("i2c-smbus", False)
        os.system("modprobe i2c_i801")
        os.system("modprobe i2c_dev")
        os.system("modprobe gpio_pca953x")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")
        self.init_i2c_bus_order()

        #lpc driver
        self.insmod("x86-64-ufispace-s9720-56ed-lpc")

        # init interrupt handler for IRQ 17
        self.insmod("x86-64-ufispace-irq-handler", params={"irq_num": 17})

        # version setting
        self.bsp_pr("BSP version {}".format(self.BSP_VERSION))
        self.config_bsp_ver(self.BSP_VERSION)

        # get board version
        board = self.get_board_version()

        # get gpio max
        gpio_max = self.get_gpio_max()
        gpio_base = self.get_gpio_base()
        self.bsp_pr("GPIO MAX: {}, BASE: {}".format(gpio_max, gpio_base))

        # check i2c bus status
        self.check_i2c_status(board)

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        # init MUX sysfs
        self.bsp_pr("Init i2c")
        self.init_mux(board)

        # init SYS EEPROM devices
        self.bsp_pr("Init sys eeprom")
        self.insmod("x86-64-ufispace-sys-eeprom")
        self.init_sys_eeprom(board)

        # init CPLD
        self.bsp_pr("Init CPLD")
        self.insmod("x86-64-ufispace-s9720-56ed-cpld", params={"mux_en":1})
        self.init_cpld(board)

        # enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        # set bmc sel time now
        self.set_bmc_sel_time()

        # init gpio
        self.bsp_pr("Init gpio")
        self.init_gpio(gpio_max, gpio_base, board)

        # init EEPROM
        self.bsp_pr("Init port eeprom")
        self.insmod("optoe")
        self.init_eeprom(board)

        # init dev_class for CMIS/non-CMIS modules
        self.update_dev_class(gpio_max, 0, board)

        self.bsp_pr("Enable event control")
        self.enable_event_ctrl(board)

        #self.bsp_pr("Init bcm88860")
        self.insmod("intel_auxiliary", False)
        self.insmod("ice", False)
        os.system("modprobe ice")

        # init BCM82399
        os.system("timeout 120s {} init -s 10G".format(self.PATH_EPDM_CLI))

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        self.bsp_pr("Init done")
        return True

