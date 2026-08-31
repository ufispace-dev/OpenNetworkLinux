import math
from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
from ctypes import c_int, sizeof
import os
import sys
import subprocess
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

class OnlPlatform_x86_64_ufispace_s9620_32e_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9620-32e-r0'
    MODEL="S9620-32E"
    SYS_OBJECT_ID=".9620.32"
    PORT_COUNT=32
    PORT_CONFIG="32x800"
    LEVEL_INFO=1
    LEVEL_ERR=2
    BSP_VERSION='1.0.0'
    PATH_SYS_I2C_DEV_ATTR="/sys/bus/i2c/devices/{}-{:0>4x}/{}"
    PATH_SYS_GPIO = "/sys/class/gpio"
    PATH_I2C_CPLD1="/sys/bus/i2c/devices/2-0030"
    PATH_I2C_CPLD2="/sys/bus/i2c/devices/2-0031"
    PATH_I2C_CPLD3="/sys/bus/i2c/devices/2-0032"
    PATH_I2C_CPLD4="/sys/bus/i2c/devices/16-0033"
    PATH_I2C_FPGA="/sys/bus/i2c/devices/2-0037"
    PATH_SYSTEM_LED=PATH_I2C_CPLD1+"/cpld_system_led_sys"
    PATH_MAC_ROV=[PATH_I2C_FPGA+"/fpga_mac_rov_1", PATH_I2C_FPGA+"/fpga_mac_rov_2"]
    PATH_CPLD1_EVT_CTRL=PATH_I2C_CPLD1+"/event_detect_ctrl"
    PATH_CPLD2_EVT_CTRL=PATH_I2C_CPLD2+"/event_detect_ctrl"
    PATH_CPLD3_EVT_CTRL=PATH_I2C_CPLD3+"/event_detect_ctrl"
    PATH_CPLD4_EVT_CTRL=PATH_I2C_CPLD4+"/event_detect_ctrl"
    PATH_FPGA_EVT_CTRL=PATH_I2C_FPGA+"/event_detect_ctrl"
    #PATH_PORT_LED_CTRL=PATH_I2C_CPLD1+"/cpld_port_led_clr"
    SYSTEM_LED_GREEN=0b00001001
    PATH_LPC="/sys/devices/platform/x86_64_ufispace_s9620_32e_lpc"
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

    port_conf = {
        0:  {"type": "QSFPDD_NIF" , "bus": 18, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p0_abs" }},
        1:  {"type": "QSFPDD_NIF" , "bus": 19, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p1_abs" }},
        2:  {"type": "QSFPDD_NIF" , "bus": 20, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p2_abs" }},
        3:  {"type": "QSFPDD_NIF" , "bus": 21, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p3_abs" }},
        4:  {"type": "QSFPDD_NIF" , "bus": 22, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p4_abs" }},
        5:  {"type": "QSFPDD_NIF" , "bus": 23, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p5_abs" }},
        6:  {"type": "QSFPDD_NIF" , "bus": 24, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p6_abs" }},
        7:  {"type": "QSFPDD_NIF" , "bus": 25, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p7_abs" }},
        8:  {"type": "QSFPDD_NIF" , "bus": 26, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p8_abs" }},
        9:  {"type": "QSFPDD_NIF" , "bus": 27, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p9_abs" }},
        10: {"type": "QSFPDD_NIF" , "bus": 28, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p10_abs"}},
        11: {"type": "QSFPDD_NIF" , "bus": 29, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p11_abs"}},
        12: {"type": "QSFPDD_NIF" , "bus": 30, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p12_abs"}},
        13: {"type": "QSFPDD_NIF" , "bus": 31, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p13_abs"}},
        14: {"type": "QSFPDD_NIF" , "bus": 32, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p14_abs"}},
        15: {"type": "QSFPDD_NIF" , "bus": 33, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/qsfpdd_p15_abs"}},
        16: {"type": "QSFPDD_NIF" , "bus": 34, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p16_abs"}},
        17: {"type": "QSFPDD_NIF" , "bus": 35, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p17_abs"}},
        18: {"type": "QSFPDD_NIF" , "bus": 36, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p18_abs"}},
        19: {"type": "QSFPDD_NIF" , "bus": 37, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p19_abs"}},
        20: {"type": "QSFPDD_NIF" , "bus": 38, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p20_abs"}},
        21: {"type": "QSFPDD_NIF" , "bus": 39, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p21_abs"}},
        22: {"type": "QSFPDD_NIF" , "bus": 40, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p22_abs"}},
        23: {"type": "QSFPDD_NIF" , "bus": 41, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p23_abs"}},
        24: {"type": "QSFPDD_NIF" , "bus": 42, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p24_abs"}},
        25: {"type": "QSFPDD_NIF" , "bus": 43, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p25_abs"}},
        26: {"type": "QSFPDD_NIF" , "bus": 44, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p26_abs"}},
        27: {"type": "QSFPDD_NIF" , "bus": 45, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p27_abs"}},
        28: {"type": "QSFPDD_NIF" , "bus": 46, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p28_abs"}},
        29: {"type": "QSFPDD_NIF" , "bus": 47, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p29_abs"}},
        30: {"type": "QSFPDD_NIF" , "bus": 48, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p30_abs"}},
        31: {"type": "QSFPDD_NIF" , "bus": 49, "driver": "optoe3", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD3+"/qsfpdd_p31_abs"}},
        32: {"type": "SFP"        , "bus": 52, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/sfp_p32_abs"   }},
        33: {"type": "SFP"        , "bus": 53, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/sfp_p33_abs"   }},
        34: {"type": "SFP"        , "bus": 54, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/sfp_p34_abs"   }},
        35: {"type": "SFP"        , "bus": 55, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/sfp_p35_abs"   }},
        36: {"type": "MGMT"       , "bus": 50, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/mgmt_p0_abs"   }},
        37: {"type": "MGMT"       , "bus": 51, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD4+"/mgmt_p1_abs"   }},
    }

    gpio_map = {
        # 511:{'offset':  0  , 'dir': 'low'   , 'desc': "ID LED"},
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

    def check_i2c_status(self):
        sysfs_mux_reset = self.PATH_MUX_RESET_ALL

        # Check I2C status
        retcode = os.system("i2cget -f -y 0 0x71 > /dev/null 2>&1")
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
            msg("Warning: bsp_pr sysfs does not exist\n")

    def config_bsp_ver(self, bsp_ver):
        bsp_version_path=self.PATH_LPC_GRP_BSP+"/bsp_version"
        if os.path.exists(bsp_version_path):
            with open(bsp_version_path, "w") as f:
                f.write(bsp_ver)

    def get_board_version(self):
        board = {}
        board_attrs = {
            "hw_rev"  : {"sysfs": self.PATH_BOARD_HW_ID},
            "deph_id" : {"sysfs": self.PATH_BOARD_DEPH_ID},
            "hw_build": {"sysfs": self.PATH_BOARD_BUILD_ID},
        }

        for key, val in board_attrs.items():
            cmd = "cat {}".format(val["sysfs"])
            output = ""
            try:
                output = subprocess.check_output(cmd.split())
            except Exception as e:
                self.bsp_pr("get_board_version() failed, exception={}\n".format(e), self.LEVEL_ERR)
                self.bsp_pr("Use default output value 1\n", self.LEVEL_ERR)
                output="1"

            board[key] = int(output, 10)

        return board

    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = self.PATH_SYS_I2C_DEV_ATTR.format(i2c_bus, i2c_addr, "idle_state")
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def get_gpio_max(self):
        cmd = " ".join(["cat", self.PATH_BSP_GPIO_MAX])
        output = ""
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("get_gpio_max() failed, exception={}".format(e), self.LEVEL_ERR)
            self.bsp_pr("Use default GPIO MAX value -1", self.LEVEL_ERR)
            output="-1"

        gpio_max = int(output, 10)
        self.bsp_pr("GPIO MAX: {}".format(gpio_max))

        return gpio_max

    def get_gpio_base(self):
        cmd = " ".join(["cat", self.PATH_BSP_GPIO_BASE])
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

    def init_gpio(self):
        #get gpio_max/gpio_base
        gpio_max = self.get_gpio_max()
        gpio_base = self.get_gpio_base()

        return

    def init_mux(self, i2c_mux_init_order):
        i2c_mux_group = {
            0: [
                ('pca9548', 0x71, 0),  # 9548_ROOT_FPGA_CPLD
                ('pca9548', 0x76, 9),  # 9548_CHILD_CPLD_4
            ],
            #   ('pca9548', 0x31, 9),  # CPLD_MUX_2
            #   ('pca9548', 0x32, 9),  # CPLD_MUX_3
            #   ('pca9548', 0x33, 9),  # CPLD_MUX_4
            1: [
                ('pca9548', 0x73, 0),  # 9548_ROOT_PWR
            ],
        }

        i2c_muxs = i2c_mux_group[i2c_mux_init_order]
        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

    def init_sys_eeprom(self, board):

        hw_rev = board["hw_rev"]
        i2c_bus = -1

        if hw_rev <= 1:
            i2c_bus = 0 # Alpha build or before
        else:
            i2c_bus = 1 # Beta build or later

        sys_eeprom = [
            ('sys_eeprom', 0x57, i2c_bus),
        ]

        self.new_i2c_devices(sys_eeprom)

    def init_cpld(self):
        cpld = [
            ('s9620_32e_cpld1', 0x30, 2),
            ('s9620_32e_cpld2', 0x31, 2),
            ('s9620_32e_cpld3', 0x32, 2),
            ('s9620_32e_cpld4', 0x33, 16),
            ('s9620_32e_fpga' , 0x37, 2),
        ]

        self.new_i2c_devices(cpld)

    def init_eeprom(self):
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

    def get_port_presence(self, port, gpio_max=511, gpio_base=0, board=None):
        try:
            port_conf, gpio_map = self.get_conf(board)
            if port not in port_conf:
                return False

            abs_type = port_conf[port]['abs'].get('type')
            abs_data = port_conf[port]['abs'].get('data')
            abs_bit = port_conf[port]['abs'].get('bit', 0)
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

            reg_val = (int(present_raw, 0) & (1 << abs_bit))
            pres_status = True if reg_val == 0 else False

            return pres_status

        except:
            return False

    def update_dev_class(self, gpio_max=511, gpio_base=0, board=None):
        port_conf, _ = self.get_conf(board)

        for port, config in port_conf.items():  # QSFPX ports

            if config.get('type') not in ['QSFPDD_NIF']:
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

    def set_system_led_green(self):
        if os.path.exists(self.PATH_SYSTEM_LED):
            with open(self.PATH_SYSTEM_LED, "r+") as f:
                led_reg = f.read()

                #write green to system led
                f.write("{}".format(self.SYSTEM_LED_GREEN))

                self.bsp_pr("Current System LED: {} -> 0x{:02x}".format(led_reg, self.SYSTEM_LED_GREEN))
        else:
            self.bsp_pr("System LED sysfs not exist")

    def _twos_complement(self, n, w):
        if n & (1 << (w - 1)):
            n = n - (1 << w)
        return n

    def init_rov(self):
        rov_addrs=[0x64, 0x68]
        rov_bus=56

        PMBUS_PAGE_REG         = 0x00
        PMBUS_OPERATION_REG    = 0x01
        PMBUS_VOUT_MODE_REG    = 0x20
        PMBUS_VOUT_COMMAND_REG = 0x21
        PMBUS_READ_VOUT_REG    = 0x8B

        rov_avs_array={
            0x7A: {'vdd_val': 0.85  },
            0x7C: {'vdd_val': 0.8375},
            0x7E: {'vdd_val': 0.825 },
            0x80: {'vdd_val': 0.8125},
            0x82: {'vdd_val': 0.8   },
            0x84: {'vdd_val': 0.7875},
            0x86: {'vdd_val': 0.775 },
            0x88: {'vdd_val': 0.7625},
            0x8A: {'vdd_val': 0.75  },
            0x8C: {'vdd_val': 0.7375},
            0x8E: {'vdd_val': 0.725 },
            0x90: {'vdd_val': 0.7125},
            0x92: {'vdd_val': 0.7   }
        }

        for i, rov_addr in enumerate(rov_addrs):

            if self._is_tr45_pass():
                # VDDC 715mV
                reg_val = 0x90
                self.bsp_pr("TR45 Test Pass, Set MAC to fixed VDDC 715mV, reg_val=0x{:02X}".format(reg_val))
            else:
                # get rov from cpld
                reg_val_str = subprocess.check_output("cat {}".format(self.PATH_MAC_ROV[i]).split())
                reg_val = int(reg_val_str, 0)
                self.bsp_pr("TR45 Test Fail or not configured, Set MAC to runtime ROV")
                self.bsp_pr("{}=0x{:02X}".format(self.PATH_MAC_ROV[i], reg_val))

            if reg_val in rov_avs_array:
                # set page to 0x0
                os.system("i2cset -f -y {} {} {} {} ".format(rov_bus, rov_addr, PMBUS_PAGE_REG, 0x0))

                # set operation to vout command (0x80 go to set vout_command)
                os.system("i2cset -f -y {} {} {} {} ".format(rov_bus, rov_addr, PMBUS_OPERATION_REG, 0x80))

                # get exp in vout mode
                vout_mode_str = subprocess.check_output("i2cget -f -y {} {} {} ".format(rov_bus, rov_addr, PMBUS_VOUT_MODE_REG).split())
                vout_mode_val = int(vout_mode_str, 0)
                vout_mode_exp = vout_mode_val & 0x1F
                vout_mode_exp_twos = self._twos_complement(vout_mode_exp, 5)

                self.bsp_pr("vout_mode_str={}".format(vout_mode_str))
                self.bsp_pr("vout_mode_exp={}".format(vout_mode_exp))
                self.bsp_pr("vout_mode_exp_twos={}".format(vout_mode_exp_twos))

                # set vout_command to PMBUS_VOUT_COMMAND_REG
                mac_vdd_val=rov_avs_array[reg_val]['vdd_val']
                vout_command_val = int(round(mac_vdd_val / math.pow(2, vout_mode_exp_twos)))
                os.system("i2cset -f -y {} {} {} {} w".format(rov_bus, rov_addr, PMBUS_VOUT_COMMAND_REG, vout_command_val))
                self.bsp_pr("Setting mac[{}] voltage {} with vout_command register value 0x{:04X}".format(i, mac_vdd_val, vout_command_val) )

                # get read_vout from PMBUS_READ_VOUT_REG
                read_vout_str = subprocess.check_output("i2cget -f -y {} {} {} w".format(rov_bus, rov_addr, PMBUS_READ_VOUT_REG).split())
                read_vout_val = int(read_vout_str, 0)
                read_vout_volt = read_vout_val * math.pow(2, vout_mode_exp_twos)
                self.bsp_pr("Read mac[{}] voltage {:.4f} from read_vout register value 0x{:04X}".format(i, read_vout_volt, read_vout_val))


    def enable_event_ctrl(self):
        # enable event ctrl
        self._write(self.PATH_CPLD1_EVT_CTRL, 1)
        self._write(self.PATH_CPLD2_EVT_CTRL, 1)
        self._write(self.PATH_CPLD3_EVT_CTRL, 1)
        self._write(self.PATH_CPLD4_EVT_CTRL, 1)
        self._write(self.PATH_FPGA_EVT_CTRL, 1)

    #def enable_port_led_ctrl(self, board):
    #    # port led enable
    #    os.system("echo 1 > "+ self.PATH_PORT_LED_CTRL)

    def config_fpga_pci_enable(self, en):
        path=self.PATH_FPGA_PCI_EN
        if os.path.exists(path):
            with open(path, "w") as f:
                f.write("{}".format(en))

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
            ("i801_smbus", "0000:00:1f.4", "bind"),
            ("ismt_smbus", "0000:00:0f.0", "bind"),
        ]

        # Iterate over the list and call modify_device for each tuple
        for driver_name, bus_address, action in device_actions:
            self.update_pci_device(driver_name, bus_address, action)

    def _write(self, path, val, perm="w"):
        if os.path.exists(path):
            try:
                with open(path, perm) as f:
                    f.write(str(val))
            except Exception as e:
                self.bsp_pr("Open file failed, exception={}".format(e))
        else:
            self.bsp_pr("File not found: {}".format(path))

    def _is_tr45_pass(self):
        val = self._get_tr45_result()

        if val is None:
            self.bsp_pr("TR45 Test Value: None (fail)")
            return False

        try:
            val = int(val, 16)
        except ValueError:
            self.bsp_pr("Failed to convert EEPROM data '{}' to integer.".format(val))
            return False

        if val == 1:
            self.bsp_pr("TR45 Test Value: {} (pass)".format(val))
            return True
        else:
            self.bsp_pr("TR45 Test Value: {} (fail or not configured)".format(val))
            return False

    def _get_tr45_result(self):
        # Get EEPROM data (offset 0x500).
        cmd = "ipmitool i2c bus=3 0xa0 0x1 0x5 0x0"

        try:
            with open(os.devnull, 'w') as devnull:
                raw_out = subprocess.check_output(cmd.split(), stderr=devnull)
                # Python 2 & 3 Compatible Fix:
                if isinstance(raw_out, bytes):
                    raw_out = raw_out.decode('utf-8')

        except subprocess.CalledProcessError as e:
            # This triggers if ipmitool returns anything other than 0
            self.bsp_pr("Failed to read EEPROM. Exit code: {}".format(e.returncode))
            return None
        except OSError as e:
            # This triggers if the ipmitool binary doesn't exist
            self.bsp_pr("Failed to execute command: {}".format(e))
            return None
        except Exception as e:
            # Catch-all for any other exceptions
            self.bsp_pr("An unexpected error occurred: {}".format(e))
            return None

        if not raw_out.strip():
            self.bsp_pr("Failed to read EEPROM data at offset 0x500 for tr45 result.")
            return None

        hex_val = raw_out.strip().splitlines()[0].replace(" ", "")

        return hex_val

    def baseconfig(self):
        # init interrupt handler for IRQ 17
        self.insmod("x86-64-ufispace-irq-handler", params={"irq_num": 17})

        # load default kernel driver
        os.system("rmmod gpio_ich")
        os.system("rmmod pinctrl_cedarfork")
        os.system("rmmod ee1004")
        os.system("modprobe -rq i2c_i801")
        self.insmod("i2c-smbus", False)
        os.system("modprobe i2c_i801")
        self.insmod("i2c-ismt", False)
        os.system("modprobe i2c_ismt")
        os.system("modprobe i2c_dev")
        os.system("modprobe i2c_mux")
        os.system("modprobe gpio_pca953x")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")
        self.init_i2c_bus_order()

        self.insmod("x86-64-ufispace-s9620-32e-lpc")
        # version setting
        self.bsp_pr("BSP version {}".format(self.BSP_VERSION))
        self.config_bsp_ver(self.BSP_VERSION)

        board = self.get_board_version()

        self.check_i2c_status()

        bmc_enable = self.check_bmc_enable()
        self.bsp_pr("bmc enable : {}".format(True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        self.bsp_pr("Init I2C - Pre init")
        # pre init mux before cpld mux is inited
        self.init_mux(0)

        self.bsp_pr("Init sys eeprom")
        self.insmod("x86-64-ufispace-sys-eeprom")
        self.init_sys_eeprom(board)

        self.bsp_pr("Init CPLD and CPLD Mux")
        self.config_fpga_pci_enable(self.FPGA_PCI_ENABLE)

        if self.FPGA_PCI_ENABLE == 1:
            self.insmod("x86-64-ufispace-s9620-32e-cpld", params={'mux_en': 0})
        else:
            self.insmod("x86-64-ufispace-s9620-32e-cpld", params={'mux_en': 1})

        self.init_cpld()

        self.bsp_pr("Init I2C - Post init")
        # post init mux after cpld mux is inited
        self.init_mux(1)

        # init EEPROM
        if self.FPGA_PCI_ENABLE == 1:
            self.bsp_pr("Init FPGA PCI port eeprom")
            os.system("setpci -s 18:00.0 COMMAND=0x02")
        else:
            self.bsp_pr("Init legacy I2C port eeprom")
            self.insmod("optoe")
            self.init_eeprom()

        self.bsp_pr("Init gpio")
        self.init_gpio()

        #config mac rov
        self.bsp_pr("Init MAC ROV")
        self.init_rov()

        self.enable_ipmi_maintenance_mode()

        self.disable_bmc_watchdog()

        self.bsp_pr("Enable event control")
        self.enable_event_ctrl()

        #self.bsp_pr("Enable port led control")
        #self.enable_port_led_ctrl(board)

        # init ice (need to have ice before bcm82399 init to avoid failure)
        self.bsp_pr("Init ice")
        self.insmod("intel_auxiliary", False)
        self.insmod("ice", False)
        os.system("modprobe ice")

        # init bcm82399
        self.bsp_pr("Init bcm82399")
        os.system("timeout 120s {} init -s 10G".format(self.PATH_EPDM_CLI))

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        # init dev_class for CMIS/non-CMIS modules
        self.update_dev_class()

        self.bsp_pr("Init done")

        return True
