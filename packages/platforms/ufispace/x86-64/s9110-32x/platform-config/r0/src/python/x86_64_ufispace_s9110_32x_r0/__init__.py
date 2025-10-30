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
    _IONONE = 0
    _IOWRITE = 1
    _IOREAD = 2

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

class OnlPlatform_x86_64_ufispace_s9110_32x_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9110-32x-r0'
    MODEL="S9110-32X"
    SYS_OBJECT_ID=".9110.32"
    PORT_COUNT=33
    PORT_CONFIG="32x100 + 1x10"
    LEVEL_INFO=1
    LEVEL_ERR=2
    PATH_SYS_I2C_DEV_ATTR="/sys/bus/i2c/devices/{}-{:0>4x}/{}"
    PATH_SYS_GPIO = "/sys/class/gpio"
    PATH_I2C_CPLD2="/sys/bus/i2c/devices/2-0031"
    PATH_SYSTEM_LED="/sys/bus/i2c/devices/2-0030/cpld_system_led_sys"
    SYSTEM_LED_GREEN=0b10010000
    PATH_LPC="/sys/devices/platform/x86_64_ufispace_s9110_32x_lpc"
    PATH_LPC_GRP_BSP=PATH_LPC+"/bsp"
    PATH_LPC_GRP_MB_CPLD=PATH_LPC+"/mb_cpld"
    PATH_LPC_GRP_EC=PATH_LPC+"/ec"
    PATH_PORT_CONFIG="/lib/platform-config/"+PLATFORM+"/onl/port_config.yml"

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
        0 : {"type": "QSFP"  , "bus": 18, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_0_7", 'bit': 0}},
        1 : {"type": "QSFP"  , "bus": 19, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_0_7", 'bit': 1}},
        2 : {"type": "QSFP"  , "bus": 20, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_0_7", 'bit': 2}},
        3 : {"type": "QSFP"  , "bus": 21, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_0_7", 'bit': 3}},
        4 : {"type": "QSFP"  , "bus": 22, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_0_7", 'bit': 4}},
        5 : {"type": "QSFP"  , "bus": 23, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_0_7", 'bit': 5}},
        6 : {"type": "QSFP"  , "bus": 24, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_0_7", 'bit': 6}},
        7 : {"type": "QSFP"  , "bus": 25, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_0_7", 'bit': 7}},
        8 : {"type": "QSFP"  , "bus": 26, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_8_15", 'bit': 0}},
        9 : {"type": "QSFP"  , "bus": 27, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_8_15", 'bit': 1}},
        10: {"type": "QSFP"  , "bus": 28, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_8_15", 'bit': 2}},
        11: {"type": "QSFP"  , "bus": 29, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_8_15", 'bit': 3}},
        12: {"type": "QSFP"  , "bus": 30, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_8_15", 'bit': 4}},
        13: {"type": "QSFP"  , "bus": 31, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_8_15", 'bit': 5}},
        14: {"type": "QSFP"  , "bus": 32, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_8_15", 'bit': 6}},
        15: {"type": "QSFP"  , "bus": 33, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_8_15", 'bit': 7}},
        16: {"type": "QSFP"  , "bus": 34, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_16_23", 'bit': 0}},
        17: {"type": "QSFP"  , "bus": 35, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_16_23", 'bit': 1}},
        18: {"type": "QSFP"  , "bus": 36, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_16_23", 'bit': 2}},
        19: {"type": "QSFP"  , "bus": 37, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_16_23", 'bit': 3}},
        20: {"type": "QSFP"  , "bus": 38, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_16_23", 'bit': 4}},
        21: {"type": "QSFP"  , "bus": 39, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_16_23", 'bit': 5}},
        22: {"type": "QSFP"  , "bus": 40, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_16_23", 'bit': 6}},
        23: {"type": "QSFP"  , "bus": 41, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_16_23", 'bit': 7}},
        24: {"type": "QSFP"  , "bus": 42, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_24_31", 'bit': 0}},
        25: {"type": "QSFP"  , "bus": 43, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_24_31", 'bit': 1}},
        26: {"type": "QSFP"  , "bus": 44, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_24_31", 'bit': 2}},
        27: {"type": "QSFP"  , "bus": 45, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_24_31", 'bit': 3}},
        28: {"type": "QSFP"  , "bus": 46, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_24_31", 'bit': 4}},
        29: {"type": "QSFP"  , "bus": 47, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_24_31", 'bit': 5}},
        30: {"type": "QSFP"  , "bus": 48, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_24_31", 'bit': 6}},
        31: {"type": "QSFP"  , "bus": 49, "driver": "optoe1", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_qsfp_abs_24_31", 'bit': 7}},
        32: {"type": "SFP"   , "bus": 51, "driver": "optoe2", "abs": {'type': 'sysfs', 'data': PATH_I2C_CPLD2+"/cpld_sfp_abs_0_1", 'bit': 0}},
    }

    gpio_map = {
        # 511:{'offset':  0  , 'dir': 'in'   , 'desc': "reserve"},
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

    def check_i2c_status(self, bus_i801, bus_ismt, board):
        sysfs_mux_reset = self.PATH_LPC_GRP_MB_CPLD + "/mux_reset_all"

        bus=bus_i801

        # Check I2C status,assume
        retcode = os.system("i2cget -f -y {} 0x72 > /dev/null 2>&1".format(bus))
        if retcode != 0:

            #read mux failed, i2c bus may be stuck
            msg("Warning: Read I2C Mux Failed!! (ret=%d)\n" % (retcode) )

            #Recovery I2C
            if os.path.exists(sysfs_mux_reset):
                with open(sysfs_mux_reset, "w") as f:
                    #write 0 to sysfs
                    f.write("{}".format(0))
                    msg("I2C bus recovery done.\n")
            else:
                msg("Warning: I2C recovery sysfs does not exist!! (path=%s)\n" % (sysfs_mux_reset) )

    def bsp_pr(self, pr_msg, level = LEVEL_INFO):
        if level == self.LEVEL_INFO:
            sysfs_bsp_logging = self.PATH_LPC_GRP_BSP+"/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            sysfs_bsp_logging = self.PATH_LPC_GRP_BSP+"/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            sysfs_bsp_logging = self.PATH_LPC_GRP_BSP+"/bsp_pr_info"

        if os.path.exists(sysfs_bsp_logging):
            with open(sysfs_bsp_logging, "w") as f:
                f.write(pr_msg)
        else:
            msg("Warning: bsp logging sys is not exist\n")

    def get_board_version(self):
        board = {}
        board_attrs = {
            "hw_rev"  : {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/board_hw_id"},
            "deph_id" : {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/board_deph_id"},
            "hw_build": {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/board_build_id"},
            "ext_id"  : {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/board_ext_id"},
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
        cmd = "cat {}/bsp_gpio_max".format(self.PATH_LPC_GRP_BSP)
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("Get gpio max failed, exception={}".format(e), self.LEVEL_ERR)
            output="511"

        gpio_max = int(output, 10)

        return gpio_max

    def get_port_presence(self, port, gpio_max = 511, gpio_base = 0, board=None):
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

    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = self.PATH_SYS_I2C_DEV_ATTR.format(i2c_bus, i2c_addr, "idle_state")
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def init_mux(self, bus_i801, bus_ismt, board):

        i2c_muxs = [
            ('pca9548', 0x70, bus_ismt),   #9548_ROOT_CPLD
            ('pca9548', 0x72, bus_i801),   #9546_ROOT_PORT
            ('pca9548', 0x73, 10), #9548_CHILD_QSFP_0_7
            ('pca9548', 0x73, 11), #9548_CHILD_QSFP_8_15
            ('pca9548', 0x73, 12), #9548_CHILD_QSFP_16_23
            ('pca9548', 0x73, 13), #9548_CHILD_QSFP_24_31
            ('pca9548', 0x73, 14), #9548_CHILD_SFP_0_1
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

    def init_sys_eeprom(self):
        sys_eeprom = [
            ('sys_eeprom', 0x57, 1),
        ]

        self.new_i2c_devices(sys_eeprom)

    def init_cpld(self):
        cpld = [
            ('s9110_32x_cpld1', 0x30, 2),
            ('s9110_32x_cpld2', 0x31, 2),
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
            if board.get('ext_id') == 1:
                port_name = data[config["type"]][port]["port_name_1_base"]
            else:
                port_name = data[config["type"]][port]["port_name"]
            sysfs=self.PATH_SYS_I2C_DEV_ATTR.format( config["bus"], addr, "port_name")
            subprocess.call("echo {} > {}".format(port_name, sysfs), shell=True)

    def init_gpio(self, gpio_max, board):
        # init GPIO sysfs
        self.new_i2c_devices(
            [
                # ('pca9535', 0x20, 4), #9535_IO_EXP_01 (9535_BOARD_ID)
            ]
        )

        _, gpio_map = self.get_conf()
        for _, conf in gpio_map.items():
            gpio_num=gpio_max+conf['offset']
            gpio_dir=conf['dir']
            os.system("echo {} > {}/export".format(gpio_num, self.PATH_SYS_GPIO))
            os.system("echo {}   > {}/gpio{}/direction".format(gpio_dir, self.PATH_SYS_GPIO, gpio_num))

        # Certain signals (e.g., qsfp_reset) need time to settle after being set.
        # A delay is added here to prevent failures in subsequent operations and ensure
        # the configuration is applied correctly.
        time.sleep(0.5)

    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("Current IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("After IPMI_IOCTL IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

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
            ("ismt_smbus", "0000:00:12.0", "unbind"),
            ("i801_smbus", "0000:00:1f.4", "bind"),
            ("ismt_smbus", "0000:00:12.0", "bind"),
        ]

        # Iterate over the list and call modify_device for each tuple
        for driver_name, bus_address, action in device_actions:
            self.update_pci_device(driver_name, bus_address, action)

    def baseconfig(self):

        # load default kernel driver
        self.init_i2c_bus_order()
        os.system("modprobe i2c_i801")
        os.system("modprobe i2c_ismt")
        os.system("modprobe i2c_dev")
        os.system("modprobe gpio_pca953x")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")

        bus_i801=0
        bus_ismt=1

        #lpc driver
        self.insmod("x86-64-ufispace-s9110-32x-lpc")

        # get board version
        board = self.get_board_version()

        # get gpio max
        gpio_max = self.get_gpio_max()
        self.bsp_pr("GPIO MAX: {}".format(gpio_max))

        # check i2c bus status
        self.check_i2c_status(bus_i801, bus_ismt, board)

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        # init MUX sysfs
        self.bsp_pr("Init i2c")
        self.init_mux(bus_i801, bus_ismt, board)

        # init SYS EEPROM devices
        self.bsp_pr("Init sys eeprom")
        self.insmod("x86-64-ufispace-sys-eeprom")
        self.init_sys_eeprom()

        # init CPLD
        self.bsp_pr("Init CPLD")
        self.insmod("x86-64-ufispace-s9110-32x-cpld")
        self.init_cpld()

        # init EEPROM
        self.bsp_pr("Init port eeprom")
        self.insmod("optoe")
        self.init_eeprom(board)

        # init dev_class for CMIS/non-CMIS modules
        self.update_dev_class(gpio_max, 0, None)

        #enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        # set system led to green
        self.set_system_led_green()

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        self.bsp_pr("Init done")
        return True

