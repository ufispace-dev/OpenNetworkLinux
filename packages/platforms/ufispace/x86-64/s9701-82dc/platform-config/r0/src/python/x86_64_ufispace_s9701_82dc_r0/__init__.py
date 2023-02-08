from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
from ctypes import c_int, sizeof
import os
import sys
import subprocess
import time
import fcntl
import commands


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
                self.ipmidev = open(dev, 'rw')
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


class OnlPlatform_x86_64_ufispace_s9701_82dc_r0(OnlPlatformUfiSpace):
    PLATFORM = 'x86-64-ufispace-s9701-82dc-r0'
    MODEL = "s9701-82dc"
    SYS_OBJECT_ID = ".9701.82"
    PORT_COUNT = 82
    PORT_CONFIG = "64x25 + 12x100 + 6x400"
    ROV_STAMP_SYSFS = "/sys/bus/i2c/devices/1-0030/cpld_mac_status_3"
    CPLD_MAC_ROV_MASK = 0b11100000
    ROV_CONFIG = [0x73, 0x73, 0x83, 0x6B, 0x6F, 0x77, 0x7B, 0x7F]
    ROV_CONFIG_REG = 0x21
    ROV_I2C_ADDR = 0x70
    ROV_I2C_BUS = 9
    ROV_WRITE_CMD = "i2cset -y {} {} {} {} w"
    LEVEL_INFO=1
    LEVEL_ERR=2

    def vid_to_volt_str(self, vid):
        volt = (vid - 1)*0.005 + 0.25
        # return str
        return "{0:.3f}V".format(volt)

    def get_mac_rov_stamp(self):
        if os.path.exists(self.ROV_STAMP_SYSFS):
            with open(self.ROV_STAMP_SYSFS, "r") as f:
                # read content
                content = f.read()
                reg_val = int(content.strip(), 0)
                stamp = (reg_val & self.CPLD_MAC_ROV_MASK) >> 5
                return stamp
        else:
            raise IOError("sysfs_path does not exist: {0}".format(self.ROV_STAMP_SYSFS))

    def set_mac_rov_config(self, rov_stamp):
        cmd = self.ROV_WRITE_CMD.format(self.ROV_I2C_BUS,
                                        self.ROV_I2C_ADDR,
                                        self.ROV_CONFIG_REG,
                                        self.ROV_CONFIG[rov_stamp])
        retcode, output = commands.getstatusoutput(cmd)
        if retcode != 0:
            mgs("set_macrov_config failed, cmd={}, output={}\n".format(cmd, output))
            return False
        else:
            msg("set_mac_rov_config addr=0x{:02X}, rov_stamp=0x{:02X}, rov_voltage={}\n".
                format(self.ROV_I2C_ADDR, rov_stamp, self.vid_to_volt_str(self.ROV_CONFIG[rov_stamp])))
        return True

    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self):
        sysfs_mux_reset = "/sys/devices/platform/x86_64_ufispace_s9701_82dc_lpc/mb_cpld/mux_reset"

        # Check I2C status
        retcode = os.system("i2cget -f -y 0 0x75 > /dev/null 2>&1")
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

        # init SFP+ EEPROM
        for bus in range(25, 89):
            self.new_i2c_device('optoe2', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1

        # init QSFP EEPROM
        for bus in range(89, 97):
            self.new_i2c_device('optoe1', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1

        # init QSFP EEPROM
        for bus in range(97, 101):
            self.new_i2c_device('optoe1', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1

        # init QSFPDD EEPROM
        for bus in range(105, 111):
            self.new_i2c_device('optoe3', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1

        # init MGMT SFP+ EEPROM
        for bus in range(10, 12):
            self.new_i2c_device('optoe2', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1

    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("Current IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("After IPMI_IOCTL IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

    def disable_bmc_watchdog(self):
        os.system("ipmitool mc watchdog off")

    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = "/sys/bus/i2c/devices/%d-%s/idle_state" % (i2c_bus, hex(i2c_addr)[2:].zfill(4))
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def bsp_pr(self, pr_msg, level = LEVEL_INFO):
        if level == self.LEVEL_INFO:
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9701_82dc_lpc/bsp/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9701_82dc_lpc/bsp/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9701_82dc_lpc/bsp/bsp_pr_info"

        if os.path.exists(sysfs_bsp_logging):
            with open(sysfs_bsp_logging, "w") as f:
                f.write(pr_msg)
        else:
            msg("Warning: bsp logging sys is not exist\n")

    def baseconfig(self):

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
        self.insmod("x86-64-ufispace-s9701-82dc-lpc")

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
            ('pca9548', 0x75, 0),
            ('pca9548', 0x73, 0),
            ('pca9548', 0x72, 0),
            ('pca9548', 0x76, 17),
            ('pca9548', 0x76, 18),
            ('pca9548', 0x76, 19),
            ('pca9548', 0x76, 20),
            ('pca9548', 0x76, 21),
            ('pca9548', 0x76, 22),
            ('pca9548', 0x76, 23),
            ('pca9548', 0x76, 24),
            ('pca9548', 0x76, 6),
            ('pca9548', 0x76, 7),
            ('pca9548', 0x76, 8),
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

        self.insmod("x86-64-ufispace-eeprom-mb")
        self.insmod("optoe")

        self.bsp_pr("Init eeprom");
        # init eeprom
        self.init_eeprom()

        # init Temperature

        # init CPLD
        self.bsp_pr("Init mb cpld");
        self.insmod("x86-64-ufispace-s9701-82dc-cpld")
        for i, addr in enumerate((0x30, 0x31, 0x32, 0x33)):
            self.new_i2c_device("s9701_82dc_cpld" + str(i+1), addr, 1)

        # config mac rov
        self.bsp_pr("Init mac rov");
        stamp = self.get_mac_rov_stamp()
        if not self.set_mac_rov_config(stamp):
            msg("Warning: fail to set mac rov\n")

        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        self.bsp_pr("Init bcm82752");
        # init i40e (need to have i40e before bcm82752 init to avoid failure)
        self.insmod("i40e")
        # init bcm82752
        os.system("timeout 120s /lib/platform-config/x86-64-ufispace-s9701-82dc-r0/onl/epdm_cli init")

        self.bsp_pr("Init done");
        return True
