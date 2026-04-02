from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
from ctypes import c_int, sizeof
import os
import sys
import subprocess
import time
import fcntl


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
    IPMI_MAINTENANCE_MODE_OFF = 1
    IPMI_MAINTENANCE_MODE_ON = 2

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
        input_buffer = pack('i', 0)
        out_buffer = fcntl.ioctl(self.ipmidev, self.IPMICTL_GET_MAINTENANCE_MODE_CMD, input_buffer)
        maintanence_mode = unpack('i', out_buffer)[0]

        return maintanence_mode

    def set_ipmi_maintenance_mode(self, mode):
        fcntl.ioctl(self.ipmidev, self.IPMICTL_SET_MAINTENANCE_MODE_CMD, c_int(mode))


class OnlPlatform_x86_64_ufispace_s9301_32d_r0(OnlPlatformUfiSpace):
    PLATFORM = 'x86-64-ufispace-s9301-32d-r0'
    MODEL = "s9301-32d"
    SYS_OBJECT_ID = ".9301.32"
    PORT_COUNT = 32
    PORT_CONFIG = "32x400"
    LEVEL_INFO=1
    LEVEL_ERR=2
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
        0:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g0", "bit": 0},
        1:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g0", "bit": 1},
        2:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g0", "bit": 2},
        3:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g0", "bit": 3},
        4:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g0", "bit": 4},
        5:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g0", "bit": 5},
        6:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g0", "bit": 6},
        7:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g0", "bit": 7},
        8:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g1", "bit": 0},
        9:  {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g1", "bit": 1},
        10: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g1", "bit": 2},
        11: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g1", "bit": 3},
        12: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g1", "bit": 4},
        13: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g1", "bit": 5},
        14: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g1", "bit": 6},
        15: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g1", "bit": 7},
        16: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g2", "bit": 0},
        17: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g2", "bit": 1},
        18: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g2", "bit": 2},
        19: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g2", "bit": 3},
        20: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g2", "bit": 4},
        21: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g2", "bit": 5},
        22: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g2", "bit": 6},
        23: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g2", "bit": 7},
        24: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g3", "bit": 0},
        25: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g3", "bit": 1},
        26: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g3", "bit": 2},
        27: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g3", "bit": 3},
        28: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g3", "bit": 4},
        29: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g3", "bit": 5},
        30: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g3", "bit": 6},
        31: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_qsfpdd_pres_g3", "bit": 7},
        32: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_abs"       , "bit": 0},
        33: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_abs"       , "bit": 1},
        34: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_abs"       , "bit": 2},
        35: {"sysfs": "/sys/bus/i2c/devices/2-0031/cpld_sfp_abs"       , "bit": 3},
    }
    
    def check_bmc_enable(self):
        return 1

    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = "/sys/bus/i2c/devices/%d-%s/idle_state" % (i2c_bus, hex(i2c_addr)[2:].zfill(4))
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def check_i2c_status(self):
        sysfs_mux_reset = "/sys/devices/platform/x86_64_ufispace_s9301_32d_lpc/mb_cpld/mux_reset"

        # Check I2C status
        retcode = os.system("i2cget -f -y 0 0x73 > /dev/null 2>&1")
        if retcode != 0:

            # read mux failed, i2c bus may be stuck
            msg("Warning: Read I2C Mux Failed!! (ret=%d)\n" % (retcode) )

            # Recovery I2C
            if os.path.exists(sysfs_mux_reset):
                with open(sysfs_mux_reset, "w") as f:
                    # write 0 to sysfs
                    f.write("{}".format(0))
                    msg("I2C bus recovery done.\n")
            else:
                msg("Warning: I2C recovery sysfs does not exist!! (path=%s)\n" % (sysfs_mux_reset) )

    def init_eeprom(self):
        port = 0

        # init SYS EEPROM devices
        self.new_i2c_devices(
            [
                #  on cpu board
                ('mb_eeprom', 0x57, 0),
            ]
        )

        # init QSFPDD EEPROM
        for bus in range(17, 49):
            self.new_i2c_device('optoe3', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1

        # init SFP EEPROM
        for bus in range(13, 17):
            self.new_i2c_device('sff8436', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1

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
        for bus in range(17, 49):  # QSFPDD ports
            port = bus - 17

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

    def bsp_pr(self, pr_msg, level = LEVEL_INFO):
        if level == self.LEVEL_INFO:
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9301_32d_lpc/bsp/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9301_32d_lpc/bsp/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9301_32d_lpc/bsp/bsp_pr_info"

        if os.path.exists(sysfs_bsp_logging):
            with open(sysfs_bsp_logging, "w") as f:
                f.write(pr_msg)
        else:
            msg("Warning: bsp logging sys is not exist\n")

    def baseconfig(self):

        # init interrupt handler for IRQ 16
        self.insmod("x86-64-ufispace-irq-handler", params={"irq_num": 16})

        os.system("modprobe -rq i2c_ismt")
        os.system("modprobe -rq i2c_i801")
        self.insmod("i2c-smbus", False)
        # load default kernel driver
        os.system("modprobe i2c_i801")
        os.system("modprobe i2c_dev")
        os.system("modprobe gpio_pca953x")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe lm75")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")

        # lpc driver
        self.insmod("x86-64-ufispace-s9301-32d-lpc")

        # check i2c bus status
        self.check_i2c_status()

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        self.bsp_pr("Init i2c");
        # initialize I210 I2C bus 0 #
        # init PCA9548
        i2c_muxs = [
            ('pca9548', 0x73, 0),
            ('pca9548', 0x72, 0),
            ('pca9548', 0x76, 9),
            ('pca9548', 0x76, 10),
            ('pca9548', 0x76, 11),
            ('pca9548', 0x76, 12),
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

        self.bsp_pr("Init eeprom");
        self.insmod("x86-64-ufispace-eeprom-mb")
        self.insmod("optoe")

        # init eeprom
        self.init_eeprom()

        # init CPLD
        self.bsp_pr("Init mb cpld");
        self.insmod("x86-64-ufispace-s9301-32d-cpld")
        for i, addr in enumerate((0x30, 0x31, 0x32)):
            self.new_i2c_device("s9301_32d_cpld" + str(i+1), addr, 2)

        # config mac rov
        # done bye CPLD at power on

        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        # init i40e
        self.bsp_pr("Init i40e");
        self.insmod("intel_auxiliary", False)
        self.insmod("i40e", False)
        os.system("modprobe i40e")

        # enable port led
        os.system("echo 1 > /sys/bus/i2c/devices/2-0030/cpld_port_led_clr_ctrl")

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        # init dev_class for CMIS/non-CMIS modules
        self.update_dev_class()

        self.bsp_pr("Init done");
        return True
