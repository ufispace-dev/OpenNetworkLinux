from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
from ctypes import c_int, sizeof
import os
import sys
import subprocess
import time
import fcntl
import yaml

def msg(s, fatal=False):
    sys.stderr.write(s)
    sys.stderr.flush()
    if fatal:
        sys.exit(1)

class IPMI_Ioctl(object):
    _IONONE  = 0
    _IOWRITE = 1
    _IOREAD  = 2

    IPMI_MAINTENANCE_MODE_AUTO = 0
    IPMI_MAINTENANCE_MODE_OFF  = 1
    IPMI_MAINTENANCE_MODE_ON   = 2

    IPMICTL_GET_MAINTENANCE_MODE_CMD = _IOREAD << 30 | sizeof(c_int) << 16 | \
        ord('i') << 8 | 30  # from ipmi.h
    IPMICTL_SET_MAINTENANCE_MODE_CMD = _IOWRITE << 30 | sizeof(c_int) << 16 | \
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

class OnlPlatform_x86_64_ufispace_s9311_64d_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9311-64d-r0'
    MODEL="S9311-64D"
    SYS_OBJECT_ID=".9311.64"
    PORT_COUNT=48
    PORT_CONFIG="64x400 + 2x25"
    LEVEL_INFO=1
    LEVEL_ERR=2
    PATH_SYS_I2C_DEV_ATTR="/sys/bus/i2c/devices/{}-{:0>4x}/{}"
    PATH_SYS_GPIO = "/sys/class/gpio"
    PATH_I2C_CPLD1="/sys/bus/i2c/devices/2-0030"
    PATH_I2C_CPLD2="/sys/bus/i2c/devices/2-0031"
    PATH_I2C_CPLD3="/sys/bus/i2c/devices/2-0032"
    PATH_I2C_FPGA="/sys/bus/i2c/devices/2-0037"
    PATH_SYSTEM_LED=PATH_I2C_CPLD1+"/cpld_system_led_sys"
    PATH_MAC_ROV=PATH_I2C_CPLD1+"/cpld_mac_rov"
    PATH_CPLD1_EVT_CTRL=PATH_I2C_CPLD1+"/cpld_evt_ctrl"
    PATH_CPLD2_EVT_CTRL=PATH_I2C_CPLD2+"/cpld_evt_ctrl"
    PATH_CPLD3_EVT_CTRL=PATH_I2C_CPLD3+"/cpld_evt_ctrl"
    PATH_PORT_LED_CTRL=PATH_I2C_CPLD1+"/cpld_port_led_clr"
    SYSTEM_LED_GREEN=0b00001001
    PATH_LPC="/sys/devices/platform/x86_64_ufispace_s9311_64d_lpc"
    PATH_LPC_GRP_BSP=PATH_LPC+"/bsp"
    PATH_LPC_GRP_MB_CPLD=PATH_LPC+"/mb_cpld"
    PATH_BSP_GPIO_MAX=PATH_LPC_GRP_BSP+"/bsp_gpio_max"
    PATH_BSP_GPIO_BASE=PATH_LPC_GRP_BSP+"/bsp_gpio_base"
    PATH_MUX_RESET_ALL=PATH_LPC_GRP_MB_CPLD + "/mux_reset_all"
    PATH_BOARD_HW_ID=PATH_LPC_GRP_MB_CPLD+"/board_hw_id"
    PATH_BOARD_DEPH_ID=PATH_LPC_GRP_MB_CPLD+"/board_deph_id"
    PATH_BOARD_BUILD_ID=PATH_LPC_GRP_MB_CPLD+"/board_build_id"
    PATH_PORT_CONFIG="/lib/platform-config/"+PLATFORM+"/onl/port_config.yml"
    PATH_EPDM_CLI="/lib/platform-config/current/onl/epdm_cli"
    PATH_FPGA_PCI_EN=PATH_LPC_GRP_BSP+"/bsp_fpga_pci_enable"
    FPGA_PCI_ENABLE=0

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
    qsfpdd_port_config = {
        0:  {"type": "QSFPDD" , "bus": 18, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_0", 'bit': 0}},
        1:  {"type": "QSFPDD" , "bus": 19, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_0", 'bit': 1}},
        2:  {"type": "QSFPDD" , "bus": 20, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_0", 'bit': 2}},
        3:  {"type": "QSFPDD" , "bus": 21, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_0", 'bit': 3}},
        4:  {"type": "QSFPDD" , "bus": 22, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_0", 'bit': 4}},
        5:  {"type": "QSFPDD" , "bus": 23, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_0", 'bit': 5}},
        6:  {"type": "QSFPDD" , "bus": 24, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_0", 'bit': 6}},
        7:  {"type": "QSFPDD" , "bus": 25, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_0", 'bit': 7}},
        8:  {"type": "QSFPDD" , "bus": 26, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_1", 'bit': 0}},
        9:  {"type": "QSFPDD" , "bus": 27, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_1", 'bit': 1}},
        10: {"type": "QSFPDD" , "bus": 28, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_1", 'bit': 2}},
        11: {"type": "QSFPDD" , "bus": 29, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_1", 'bit': 3}},
        12: {"type": "QSFPDD" , "bus": 30, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_1", 'bit': 4}},
        13: {"type": "QSFPDD" , "bus": 31, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_1", 'bit': 5}},
        14: {"type": "QSFPDD" , "bus": 32, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_1", 'bit': 6}},
        15: {"type": "QSFPDD" , "bus": 33, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_1", 'bit': 7}},
        16: {"type": "QSFPDD" , "bus": 52, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_0", 'bit': 0}},
        17: {"type": "QSFPDD" , "bus": 53, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_0", 'bit': 1}},
        18: {"type": "QSFPDD" , "bus": 54, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_0", 'bit': 2}},
        19: {"type": "QSFPDD" , "bus": 55, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_0", 'bit': 3}},
        20: {"type": "QSFPDD" , "bus": 56, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_0", 'bit': 4}},
        21: {"type": "QSFPDD" , "bus": 57, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_0", 'bit': 5}},
        22: {"type": "QSFPDD" , "bus": 58, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_0", 'bit': 6}},
        23: {"type": "QSFPDD" , "bus": 59, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_0", 'bit': 7}},
        24: {"type": "QSFPDD" , "bus": 60, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_1", 'bit': 0}},
        25: {"type": "QSFPDD" , "bus": 61, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_1", 'bit': 1}},
        26: {"type": "QSFPDD" , "bus": 62, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_1", 'bit': 2}},
        27: {"type": "QSFPDD" , "bus": 63, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_1", 'bit': 3}},
        28: {"type": "QSFPDD" , "bus": 64, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_1", 'bit': 4}},
        29: {"type": "QSFPDD" , "bus": 65, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_1", 'bit': 5}},
        30: {"type": "QSFPDD" , "bus": 66, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_1", 'bit': 6}},
        31: {"type": "QSFPDD" , "bus": 67, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_1", 'bit': 7}},
        32: {"type": "QSFPDD" , "bus": 34, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_2", 'bit': 0}},
        33: {"type": "QSFPDD" , "bus": 35, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_2", 'bit': 1}},
        34: {"type": "QSFPDD" , "bus": 36, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_2", 'bit': 2}},
        35: {"type": "QSFPDD" , "bus": 37, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_2", 'bit': 3}},
        36: {"type": "QSFPDD" , "bus": 38, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_2", 'bit': 4}},
        37: {"type": "QSFPDD" , "bus": 39, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_2", 'bit': 5}},
        38: {"type": "QSFPDD" , "bus": 40, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_2", 'bit': 6}},
        39: {"type": "QSFPDD" , "bus": 41, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_2", 'bit': 7}},
        40: {"type": "QSFPDD" , "bus": 42, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_3", 'bit': 0}},
        41: {"type": "QSFPDD" , "bus": 43, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_3", 'bit': 1}},
        42: {"type": "QSFPDD" , "bus": 44, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_3", 'bit': 2}},
        43: {"type": "QSFPDD" , "bus": 45, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_3", 'bit': 3}},
        44: {"type": "QSFPDD" , "bus": 46, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_3", 'bit': 4}},
        45: {"type": "QSFPDD" , "bus": 47, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_3", 'bit': 5}},
        46: {"type": "QSFPDD" , "bus": 48, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_3", 'bit': 6}},
        47: {"type": "QSFPDD" , "bus": 49, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfpdd_intr_present_3", 'bit': 7}},
        48: {"type": "QSFPDD" , "bus": 68, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_2", 'bit': 0}},
        49: {"type": "QSFPDD" , "bus": 69, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_2", 'bit': 1}},
        50: {"type": "QSFPDD" , "bus": 70, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_2", 'bit': 2}},
        51: {"type": "QSFPDD" , "bus": 71, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_2", 'bit': 3}},
        52: {"type": "QSFPDD" , "bus": 72, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_2", 'bit': 4}},
        53: {"type": "QSFPDD" , "bus": 73, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_2", 'bit': 5}},
        54: {"type": "QSFPDD" , "bus": 74, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_2", 'bit': 6}},
        55: {"type": "QSFPDD" , "bus": 75, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_2", 'bit': 7}},
        56: {"type": "QSFPDD" , "bus": 76, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_3", 'bit': 0}},
        57: {"type": "QSFPDD" , "bus": 77, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_3", 'bit': 1}},
        58: {"type": "QSFPDD" , "bus": 78, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_3", 'bit': 2}},
        59: {"type": "QSFPDD" , "bus": 79, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_3", 'bit': 3}},
        60: {"type": "QSFPDD" , "bus": 80, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_3", 'bit': 4}},
        61: {"type": "QSFPDD" , "bus": 81, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_3", 'bit': 5}},
        62: {"type": "QSFPDD" , "bus": 82, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_3", 'bit': 6}},
        63: {"type": "QSFPDD" , "bus": 83, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/cpld_qsfpdd_intr_present_3", 'bit': 7}},
    }

    mgmt_port_config_alpha = {
            64: {"type": "MGMT" , "bus": 15, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD1+"/cpld_mgmt_port_pres_0", 'bit': 0}},
            65: {"type": "MGMT" , "bus": 16, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD1+"/cpld_mgmt_port_pres_1", 'bit': 0}},
        }

    mgmt_port_config_beta = {
            64: {"type": "MGMT" , "bus": 50, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD1+"/cpld_mgmt_port_pres_0", 'bit': 0}},
            65: {"type": "MGMT" , "bus": 51, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD1+"/cpld_mgmt_port_pres_1", 'bit': 0}},
        }

    def get_conf(self, board=None):
        if board['hw_rev'] == 1:
            port_config = self.qsfpdd_port_config.copy()
            port_config.update(self.mgmt_port_config_alpha)
        else:
            port_config = self.qsfpdd_port_config.copy()
            port_config.update(self.mgmt_port_config_beta)
        
        return port_config
        
    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self, board):
        sysfs_mux_reset = self.PATH_MUX_RESET_ALL

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

    def get_board_version(self):
        board = {}
        board_attrs = {
            "hw_rev"  : {"sysfs": self.PATH_BOARD_HW_ID},
            "deph_id" : {"sysfs": self.PATH_BOARD_DEPH_ID},
            "hw_build": {"sysfs": self.PATH_BOARD_BUILD_ID},
        }

        for key, val in board_attrs.items():
            cmd = "cat {}".format(val["sysfs"])

            output = "1"
            try:
                output = subprocess.check_output(cmd.split())
            except Exception as e:
                self.bsp_pr("Get {} from LPC failed, exception={}, output={}, cmd={}\n".format(key, e, output, cmd), self.LEVEL_ERR)

            board[key] = int(output, 10)

        return board

    def get_gpio_max(self):
        cmd = "cat " + self.PATH_BSP_GPIO_MAX

        output = ""
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("Get gpio max failed, exception={}, output={}, cmd={}\n".format(e, output, cmd), self.LEVEL_ERR)

        gpio_max = int(output, 10)
        self.bsp_pr("GPIO MAX: {}".format(gpio_max))

        return gpio_max

    def get_gpio_base(self):
        cmd = "cat " + self.PATH_BSP_GPIO_BASE

        output = ""
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("get_gpio_base() failed, exception={}".format(e), self.LEVEL_ERR)
            self.bsp_pr("Use default GPIO Base value -1", self.LEVEL_ERR)
            output="-1"

        gpio_base = int(output, 10)
        self.bsp_pr("GPIO Base: {}".format(gpio_base))

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

        i2c_muxs = [
            ('pca9548', 0x72, 0),  # 9548_CPLD
            ('pca9548', 0x73, 0),  # 9548_DC/SFP
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

    def init_sys_eeprom(self, board):
        sys_eeprom = [
            ('sys_eeprom', 0x57, 1),
        ]

        self.new_i2c_devices(sys_eeprom)

    def init_cpld(self, board):
        cpld = [
            ('s9311_64d_cpld1', 0x30, 2),
            ('s9311_64d_cpld2', 0x31, 2),
            ('s9311_64d_cpld3', 0x32, 2),
            ('s9311_64d_fpga' , 0x37, 2),
        ]

        self.new_i2c_devices(cpld)

    def init_eeprom(self, board):
        data = None
        with open(self.PATH_PORT_CONFIG, 'r') as yaml_file:
            data = yaml.safe_load(yaml_file)

        port_conf = self.get_conf(board)
        for port, config in port_conf.items():
            addr=0x50
            self.new_i2c_device(config["driver"], addr, config["bus"])
            port_name = data[config["type"]][port]["port_name"]
            sysfs=self.PATH_SYS_I2C_DEV_ATTR.format( config["bus"], addr, "port_name")
            os.system("echo {} > {}".format(port_name, sysfs))
    
    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        # self.bsp_pr("Current IPMI_MAINTENANCE_MODE={}".format(mode) )

        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        # self.bsp_pr("After IPMI_IOCTL IPMI_MAINTENANCE_MODE={}".format(mode) )

    def disable_bmc_watchdog(self):
        os.system("ipmitool mc watchdog off")

    def set_system_led_green(self):
        if os.path.exists(self.PATH_SYSTEM_LED):
            with open(self.PATH_SYSTEM_LED, "r+") as f:
                led_reg = f.read()

                #write green to system led
                f.write("{}".format(self.SYSTEM_LED_GREEN))

                self.bsp_pr("Current System LED: {} -> 0x{:02x}".format(led_reg, self.SYSTEM_LED_GREEN))
        else:
            self.bsp_pr("System LED sysfs not exist")

    def get_port_presence(self, port, board=None):
        try:
            port_conf = self.get_conf(board)
            if port not in port_conf:
                return False

            abs_type = port_conf[port]['abs'].get('type')
            abs_data = port_conf[port]['abs'].get('data')
            abs_bit = port_conf[port]['abs'].get('bit', 0)
            if abs_type == 'sysfs':
                sysfs = abs_data
            else:
                return False

            with open(sysfs, "r") as f:
                present_raw = f.read().strip()

            reg_val = (int(present_raw, 0) & (1 << abs_bit))
            pres_status = True if reg_val == 0 else False

            return pres_status

        except:
            return False

    def update_dev_class(self, board=None):
        port_conf = self.get_conf(board)

        for port, config in port_conf.items():  # QSFPX ports

            if config.get('type') not in ['QSFPDD', 'QSFP']:
                continue

            # check module presence
            if not self.get_port_presence(port, board):
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
                self.check_i2c_status(board)
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

    def init_rov(self):
        rov_addrs=[0x58]
        rov_reg=0x21
        rov_bus=12
        # TBD, need update cmd value
        rov_avs_array={
            0x7E: {'vdd_val': '0.825'  ,'vout_cmd':'0x034D'},
            0x82: {'vdd_val': '0.8'    ,'vout_cmd':'0x0333'},
            0x86: {'vdd_val': '0.775'  ,'vout_cmd':'0x031A'},
            0x8A: {'vdd_val': '0.75'   ,'vout_cmd':'0x0300'},
            0x8E: {'vdd_val': '0.725'  ,'vout_cmd':'0x02E6'},
        }

        #get rov from cpld
        reg_val_str = subprocess.check_output("cat {}".format(self.PATH_MAC_ROV).split())
        reg_val = int(reg_val_str, 0)
        self.bsp_pr("{}={}".format(self.PATH_MAC_ROV, reg_val_str))

        for index, rov_addr in enumerate(rov_addrs):
            if reg_val in rov_avs_array:
                mac_vdd_val=rov_avs_array[reg_val]['vdd_val']
                rov_reg_val=rov_avs_array[reg_val]['vout_cmd']
                self.bsp_pr("Setting mac[{}] vdd {} with rov register value {}".format(index, mac_vdd_val, rov_reg_val) )
                os.system("i2cset -y {} {} {} {} w".format(rov_bus, rov_addr, rov_reg, rov_reg_val))

    def enable_event_ctrl(self, board):
        # enable event ctrl
        os.system("echo 1 > "+ self.PATH_CPLD1_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_CPLD2_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_CPLD3_EVT_CTRL)

    def enable_port_led_ctrl(self, board):
        # port led enable
        os.system("echo 1 > "+ self.PATH_PORT_LED_CTRL)

    def config_fpga_pci_enable(self, en):
        path=self.PATH_FPGA_PCI_EN
        if os.path.exists(path):
            with open(path, "w") as f:
                f.write("{}".format(en))

    def update_pci_device(self, driver, device, action):
        driver_path = os.path.join("/sys/bus/pci/drivers", driver, action)

        if os.path.exists(driver_path):
            with open(driver_path, "w") as file:
                file.write(device)


    def init_i2c_bus_order(self):
        device_actions = [
            #driver_name   bus_address     action
            ("i801_smbus", "0000:00:1f.4", "unbind"),
            ("ismt_smbus", "0000:00:0f.0", "unbind"),
            ("i801_smbus", "0000:00:1f.4", "bind"),
            ("ismt_smbus", "0000:00:0f.0", "bind")
        ]

        # Iterate over the list and call modify_device for each tuple
        for driver_name, bus_address, action in device_actions:
            self.update_pci_device(driver_name, bus_address, action)

    def init_gpio(self):
        self.bsp_pr("Init GPIO")
        
        # init GPIO sysfs

        #get gpio_max
        gpio_max = self.get_gpio_max()
        #get gpio_base
        gpio_base = self.get_gpio_base()
        is_gpio_base = False

        if gpio_base >= 0 :
            base = gpio_base
            is_gpio_base = True
        elif gpio_max >= 0:
            base = gpio_max
            is_gpio_base = False
        else:
            self.bsp_pr("invalid gpio_max {} and gpio_base {}, bsp init stopped".format(gpio_max, gpio_base), self.LEVEL_ERR)
            exit(1)

    def baseconfig(self):
        # init interrupt handler for IRQ 17
        self.insmod("x86-64-ufispace-irq-handler", params={"irq_num": 17})

        self.init_i2c_bus_order()
        os.system("modprobe i2c_i801")
        self.insmod("i2c-ismt", False)
        os.system("modprobe i2c_ismt")
        os.system("modprobe i2c_dev")
        os.system("modprobe i2c_mux")
        os.system("modprobe gpio_pca953x")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe lm75")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")

        self.insmod("x86-64-ufispace-s9311-64d-lpc")

        board = self.get_board_version()


        self.check_i2c_status(board)

        bmc_enable = self.check_bmc_enable()
        self.bsp_pr("bmc enable : {}".format(True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        self.bsp_pr("Init I2C")
        self.init_mux(board)

        self.bsp_pr("Init sys eeprom")
        self.insmod("x86-64-ufispace-sys-eeprom")
        self.init_sys_eeprom(board)

        self.bsp_pr("Init CPLD")
        self.config_fpga_pci_enable(self.FPGA_PCI_ENABLE)

        if self.FPGA_PCI_ENABLE == 1:
            self.insmod("x86-64-ufispace-s9311-64d-cpld")
        else:
            self.insmod("x86-64-ufispace-s9311-64d-cpld", params={'mux_en': 1})
        
        self.init_cpld(board)

        # init GPIO sysfs
        self.init_gpio()

        # init EEPROM
        if self.FPGA_PCI_ENABLE == 1:
            self.bsp_pr("Init FPGA PCI port eeprom")
            os.system("setpci -s 18:00.0 COMMAND=0x02")
        else:
            self.bsp_pr("Init legacy I2C port eeprom")
            self.insmod("optoe")
            self.init_eeprom(board)

        #config mac rov
        self.bsp_pr("Init MAC ROV")
        self.init_rov()

        self.enable_ipmi_maintenance_mode()

        self.disable_bmc_watchdog()

        self.bsp_pr("Enable event control")
        self.enable_event_ctrl(board)

        self.bsp_pr("Enable port led control")
        self.enable_port_led_ctrl(board)

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        # init dev_class for CMIS/non-CMIS modules
        self.update_dev_class(board)

        self.bsp_pr("Init done")

        return True
