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

class DriverType():
    I2C_ISMT   = 0
    LPC        = 1
    SYS_EEPROM = 2
    OPTOE      = 3
    CPLD       = 4

class OnlPlatform_x86_64_ufispace_s7801_54xs_r0(OnlPlatformUfiSpace):
    VENDOR_PREFIX="x86-64-ufispace-"
    PLTM_PREFIX=VENDOR_PREFIX + "s7801-54xs-"
    PLATFORM=PLTM_PREFIX + "r0"
    MODEL="S7801-54XS"
    SYS_OBJECT_ID=".7801.54"
    PORT_COUNT=54
    PORT_CONFIG="48x10 + 6x100"

    LEVEL_INFO=1
    LEVEL_ERR=2
    SYSFS_LPC="/sys/devices/platform/x86_64_ufispace_s7801_54xs_lpc"
    SYSFS_SYSTEM_LED="/sys/bus/i2c/devices/2-0030/cpld_system_led_sys"
    SYSTEM_LED_GREEN=0b00001001
    FS_PLTM_CFG="/lib/platform-config/current/onl"
    PORT_CFG=FS_PLTM_CFG + "/port_config.yml"

    DRIVER={DriverType.I2C_ISMT: "i2c-ismt",
            DriverType.LPC: PLTM_PREFIX + "lpc",
            DriverType.SYS_EEPROM: VENDOR_PREFIX + "sys-eeprom",
            DriverType.OPTOE: "optoe",
            DriverType.CPLD: PLTM_PREFIX + "cpld"}


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


    sysfs_port_present = {
        0:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_0_7",     "bit": 0},
        1:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_0_7",     "bit": 1},
        2:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_0_7",     "bit": 2},
        3:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_0_7",     "bit": 3},
        4:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_0_7",     "bit": 4},
        5:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_0_7",     "bit": 5},
        6:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_0_7",     "bit": 6},
        7:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_0_7",     "bit": 7},
        8:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_8_15",    "bit": 0},
        9:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_8_15",    "bit": 1},
        10: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_8_15",    "bit": 2},
        11: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_8_15",    "bit": 3},
        12: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_8_15",    "bit": 4},
        13: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_8_15",    "bit": 5},
        14: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_8_15",    "bit": 6},
        15: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_8_15",    "bit": 7},
        16: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_16_23",   "bit": 0},
        17: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_16_23",   "bit": 1},
        18: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_16_23",   "bit": 2},
        19: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_16_23",   "bit": 3},
        20: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_16_23",   "bit": 4},
        21: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_16_23",   "bit": 5},
        22: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_16_23",   "bit": 6},
        23: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_16_23",   "bit": 7},
        24: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_24_31",   "bit": 0},
        25: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_24_31",   "bit": 1},
        26: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_24_31",   "bit": 2},
        27: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_24_31",   "bit": 3},
        28: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_24_31",   "bit": 4},
        29: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_24_31",   "bit": 5},
        30: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_24_31",   "bit": 6},
        31: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_24_31",   "bit": 7},
        32: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_32_39",   "bit": 0},
        33: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_32_39",   "bit": 1},
        34: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_32_39",   "bit": 2},
        35: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_32_39",   "bit": 3},
        36: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_32_39",   "bit": 4},
        37: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_32_39",   "bit": 5},
        38: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_32_39",   "bit": 6},
        39: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_32_39",   "bit": 7},
        40: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_40_47",   "bit": 0},
        41: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_40_47",   "bit": 1},
        42: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_40_47",   "bit": 2},
        43: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_40_47",   "bit": 3},
        44: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_40_47",   "bit": 4},
        45: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_40_47",   "bit": 5},
        46: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_40_47",   "bit": 6},
        47: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_intr_present_40_47",   "bit": 7},
        48: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfp_intr_present_48_53",  "bit": 0},
        49: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfp_intr_present_48_53",  "bit": 1},
        50: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfp_intr_present_48_53",  "bit": 2},
        51: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfp_intr_present_48_53",  "bit": 3},
        52: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfp_intr_present_48_53",  "bit": 4},
        53: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfp_intr_present_48_53",  "bit": 5},
    }

    def check_i2c_status(self):
        sysfs_mux_reset = self.SYSFS_LPC + "/mb_cpld/mux_reset"

        # Check I2C status
        retcode = os.system("i2cget -f -y 0 0x72 > /dev/null 2>&1")
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
            bsp_pr = self.SYSFS_LPC + "/bsp/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            bsp_pr = self.SYSFS_LPC + "/bsp/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            bsp_pr = self.SYSFS_LPC + "/bsp/bsp_pr_info"

        if os.path.exists(bsp_pr):
            with open(bsp_pr, "w") as f:
                f.write(pr_msg)
        else:
            msg("Warning: bsp_pr sysfs does not exist\n")

    def init_sys_eeprom(self):
        addr_sys_eeprom = 0x53
        bus_sys_eeprom = 5

        self.bsp_pr("Init System EEPROM")

        # load driver
        self.insmod(self.DRIVER[DriverType.SYS_EEPROM])

        # init SYS EEPROM devices
        self.new_i2c_devices(
            [
                ('sys_eeprom', addr_sys_eeprom, bus_sys_eeprom),
            ]
        )

    def init_port_eeprom(self):
        port = 0
        data = None
        ports_info = {
            "SFP": {"bus_start": 26,
                    "bus_end":   74,
                    "dev_name": "optoe2"},
            "QSFP": {"bus_start": 74,
                     "bus_end":   80,
                     "dev_name": "optoe1"},
        }
        port_base = ports_info["SFP"]["bus_start"]
        addr_eeprom = 0x50

        self.bsp_pr("Init Port EEPROM")

        # load driver
        self.insmod(self.DRIVER[DriverType.OPTOE])

        # open port name config file
        with open(self.PORT_CFG, 'r') as yaml_file:
            data = yaml.safe_load(yaml_file)

        # init Port EEPROM
        for port_type, port_info in ports_info.items():
            for bus in range(port_info["bus_start"], port_info["bus_end"]):
                # create i2c  device
                self.new_i2c_device(port_info["dev_name"], addr_eeprom, bus)
                # update port_name
                if data is not None:
                    # get front panel port from bus
                    port = bus - port_base
                    port_name = data[port_type][port]["port_name"]
                    self._write("/sys/bus/i2c/devices/{}-{:0>4x}/port_name".format(bus, addr_eeprom), port_name)

    def init_gpio(self):
        self.bsp_pr("Init GPIO")

        # init GPIO sysfs
        self.new_i2c_devices(
            [
                ('tca6424', 0x22, 7), #6424_RXRS_2
                ('tca6424', 0x22, 6), #6424_RXRS_1
                ('tca6424', 0x23, 7), #6424_TXRS_2
                ('tca6424', 0x23, 6), #6424_TXRS_1
            ]
        )

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

        if is_gpio_base:
            # export GPIO and configure direction
            for i in range(base, base+96):
                os.system("echo {} > /sys/class/gpio/export".format(i))
                os.system("echo high > /sys/class/gpio/gpio{}/direction".format(i))
        else:
            # init all GPIO direction to "in"
            gpio_dir = ["in"] * (gpio_max+1)

            # init GPIO direction to output high
            for i in range(gpio_max-95, gpio_max+1):
                gpio_dir[i] = "high"
            # export GPIO and configure direction
            for i in range(base-95, base+1):
                os.system("echo {} > /sys/class/gpio/export".format(i))
                os.system("echo {} > /sys/class/gpio/gpio{}/direction".format(gpio_dir[i],i))

    def get_gpio_base(self):
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s8901_54xc_lpc/bsp/bsp_gpio_base"
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

    def init_cpld(self):
        bus = 2
        addrs = (0x30, 0x31)
        dev_name_prefix = "s7801_54xs_cpld"

        self.bsp_pr("Init CPLD")

        # load driver
        self.insmod(self.DRIVER[DriverType.CPLD])

        # create i2c device
        for i, addr in enumerate(addrs):
            self.new_i2c_device(dev_name_prefix + str(i+1), addr, bus)

        # enable event ctrl
        for _, addr in enumerate(addrs):
            self._write("/sys/bus/i2c/devices/{}-{:0>4x}/cpld_evt_ctrl".format(bus, addr), 1)

    def ufi_port_present_get(self, port):
        present_raw = ""

        if port not in self.sysfs_port_present:
            self.bsp_pr("Port {} is not valid.".format(port), self.LEVEL_ERR)
            return False

        sysfs = self.sysfs_port_present[port]["sysfs"]
        bit = self.sysfs_port_present[port]["bit"]

        with open(sysfs, "r") as f:
            present_raw = f.read().strip()

        reg_val = int(present_raw, 16)
        pres_status = True if ((reg_val >> bit) & 0x1) == 0 else False

        return pres_status

    def update_dev_class(self):
        for bus in range(74, 80):  # QSFPDD ports
            port = bus - 26

            # check module presence
            if not self.ufi_port_present_get(port):
                continue

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
        msg("Current IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("After IPMI_IOCTL IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

    def disable_bmc_watchdog(self):
        os.system("ipmitool mc watchdog off")

    def set_system_led_green(self):
        if os.path.exists(self.SYSFS_SYSTEM_LED):
            with open(self.SYSFS_SYSTEM_LED, "r+") as f:
                led_reg = f.read().strip()

                #write green to system led
                f.write("{}".format(self.SYSTEM_LED_GREEN))

                self.bsp_pr("Current System LED: {} -> 0x{:02x}".format(led_reg, self.SYSTEM_LED_GREEN))
        else:
            self.bsp_pr("System LED sysfs not exist: {}", self.SYSFS_SYSTEM_LED)

    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = "/sys/bus/i2c/devices/{}-{:0>4x}/idle_state".format(i2c_bus, i2c_addr)
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def init_mux(self, bus_i801, bus_ismt):
        self.bsp_pr("Init I2C Mux")

        i2c_muxs = [
            ('pca9548', 0x70, bus_ismt),  # 9548_CPLD
            ('pca9548', 0x71, bus_ismt),  # 9548_FRU
            ('pca9548', 0x72, bus_i801),  # 9548_ROOT_SFP
            ('pca9548', 0x73, 18),        # 9548_CHILD_SFP_0_7
            ('pca9548', 0x73, 19),        # 9548_CHILD_SFP_8_15
            ('pca9548', 0x73, 20),        # 9548_CHILD_SFP_16_23
            ('pca9548', 0x73, 21),        # 9548_CHILD_SFP_24_31
            ('pca9548', 0x73, 22),        # 9548_CHILD_SFP_32_39
            ('pca9548', 0x73, 23),        # 9548_CHILD_SFP_40_47
            ('pca9548', 0x73, 24),        # 9548_CHILD_QSFP_48_53
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

    def get_gpio_max(self):
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s7801_54xs_lpc/bsp/bsp_gpio_max"
        output = ""
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("get_gpio_max() failed, exception={}\n".format(e), self.LEVEL_ERR)
            self.bsp_pr("Use default GPIO MAX value -1\n", self.LEVEL_ERR)
            output="-1"

        gpio_max = int(output, 10)
        self.bsp_pr("GPIO MAX: {}".format(gpio_max))

        return gpio_max

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

    def _write(self, path, val, perm="w"):
        if os.path.exists(path):
            try:
                with open(path, perm) as f:
                    f.write(str(val))
            except Exception as e:
                self.bsp_pr("Open file failed, exception={}".format(e))
        else:
            self.bsp_pr("File not found: {}".format(path))

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

        # lpc driver
        self.insmod(self.DRIVER[DriverType.LPC])

        # initialize I2C bus 0
        # i2c_i801 is built-in
        # add i2c_ismt

        bus_i801 = 0
        bus_ismt = 1

        # check i2c bus status
        self.check_i2c_status()

        # Golden Finger to show CPLD
        os.system("i2cset -y {} 0x70 0x1 > /dev/null 2>&1".format(bus_ismt))
        os.system("i2cget -y {} 0x30 0x2 > /dev/null 2>&1".format(bus_ismt))
        os.system("i2cget -y {} 0x31 0x2 > /dev/null 2>&1".format(bus_ismt))
        os.system("i2cset -y {} 0x70 0x0 > /dev/null 2>&1".format(bus_ismt))

        # init I2C Mux
        self.init_mux(bus_i801, bus_ismt)

        # init SYS EEPROM devices
        self.init_sys_eeprom()

        # init port EEPROM
        self.init_port_eeprom()

        # init GPIO sysfs
        self.init_gpio()

        # init CPLD
        self.init_cpld()

        # enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        # set system led to green
        self.set_system_led_green()

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        # init dev_class for CMIS/non-CMIS modules
        self.update_dev_class()

        self.bsp_pr("Init done")

        return True
