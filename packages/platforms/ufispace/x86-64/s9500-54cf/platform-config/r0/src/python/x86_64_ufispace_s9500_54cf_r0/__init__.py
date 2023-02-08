from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
from ctypes import c_int, sizeof
import os
import sys
import commands
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
                self.ipmidev = open(dev, 'rw')
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

class OnlPlatform_x86_64_ufispace_s9500_54cf_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9500-54cf-r0'
    MODEL="S9500-54CF"
    SYS_OBJECT_ID=".9500.54"
    PORT_COUNT=54
    PORT_CONFIG="16x1 + 24x10 + 14x25"
    LEVEL_INFO=1
    LEVEL_ERR=2

    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self, bus_i801):
        sysfs_mux_reset = "/sys/devices/platform/x86_64_ufispace_s9500_54cf_lpc/mb_cpld/mux_reset_all"

        bus=bus_i801

        # Check I2C status,assume
        retcode = os.system("i2cget -f -y {} 0x75 > /dev/null 2>&1".format(bus))
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
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9500_54cf_lpc/bsp/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9500_54cf_lpc/bsp/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9500_54cf_lpc/bsp/bsp_pr_info"

        if os.path.exists(sysfs_bsp_logging):
            with open(sysfs_bsp_logging, "w") as f:
                f.write(pr_msg)
        else:
            msg("Warning: bsp logging sys is not exist\n")

    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = "/sys/bus/i2c/devices/%d-%s/idle_state" % (i2c_bus, hex(i2c_addr)[2:].zfill(4))
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def init_mux(self, bus_i801, bus_ismt, hw_rev_id):
        # Alpha and later
        if hw_rev_id >=1:
            i2c_muxs = [
                ('pca9548', 0x72, bus_ismt), #9546_ROOT_TIMING
                ('pca9548', 0x75, bus_i801), #9546_ROOT_SFP
                ('pca9548', 0x76, 10),       #9548_CHILD_SFP_0_7
                ('pca9548', 0x76, 11),       #9548_CHILD_SFP_8_15
                ('pca9548', 0x76, 12),       #9548_CHILD_SFP_16_23
                ('pca9548', 0x76, 13),       #9548_CHILD_SFP_24_31
                ('pca9548', 0x76, 14),       #9548_CHILD_SFP_32_39
                ('pca9548', 0x76, 15),       #9548_CHILD_SFP_40_47
                ('pca9548', 0x76, 16),       #9548_CHILD_SFP_48_53
            ]

            self.new_i2c_devices(i2c_muxs)

            #init idle state on mux
            self.init_i2c_mux_idle_state(i2c_muxs)

    def init_eeprom(self):

        data = None
        port_eeprom = {
            0: {"type": "SFP"   , "bus": 18, "driver": "optoe2"},
            1: {"type": "SFP"   , "bus": 19, "driver": "optoe2"},
            2: {"type": "SFP"   , "bus": 20, "driver": "optoe2"},
            3: {"type": "SFP"   , "bus": 21, "driver": "optoe2"},
            4: {"type": "SFP"   , "bus": 22, "driver": "optoe2"},
            5: {"type": "SFP"   , "bus": 23, "driver": "optoe2"},
            6: {"type": "SFP"   , "bus": 24, "driver": "optoe2"},
            7: {"type": "SFP"   , "bus": 25, "driver": "optoe2"},
            8: {"type": "SFP"   , "bus": 26, "driver": "optoe2"},
            9: {"type": "SFP"   , "bus": 27, "driver": "optoe2"},
            10:{"type": "SFP"   , "bus": 28, "driver": "optoe2"},
            11:{"type": "SFP"   , "bus": 29, "driver": "optoe2"},
            12:{"type": "SFP"   , "bus": 30, "driver": "optoe2"},
            13:{"type": "SFP"   , "bus": 31, "driver": "optoe2"},
            14:{"type": "SFP"   , "bus": 32, "driver": "optoe2"},
            15:{"type": "SFP"   , "bus": 33, "driver": "optoe2"},
            16:{"type": "SFP+"   , "bus": 34, "driver": "optoe2"},
            17:{"type": "SFP+"   , "bus": 35, "driver": "optoe2"},
            18:{"type": "SFP+"   , "bus": 36, "driver": "optoe2"},
            19:{"type": "SFP+"   , "bus": 37, "driver": "optoe2"},
            20:{"type": "SFP+"   , "bus": 38, "driver": "optoe2"},
            21:{"type": "SFP+"   , "bus": 39, "driver": "optoe2"},
            22:{"type": "SFP+"   , "bus": 40, "driver": "optoe2"},
            23:{"type": "SFP+"   , "bus": 41, "driver": "optoe2"},
            24:{"type": "SFP+"   , "bus": 42, "driver": "optoe2"},
            25:{"type": "SFP+"   , "bus": 43, "driver": "optoe2"},
            26:{"type": "SFP+"   , "bus": 44, "driver": "optoe2"},
            27:{"type": "SFP+"   , "bus": 45, "driver": "optoe2"},
            28:{"type": "SFP+"   , "bus": 46, "driver": "optoe2"},
            29:{"type": "SFP+"   , "bus": 47, "driver": "optoe2"},
            30:{"type": "SFP+"   , "bus": 48, "driver": "optoe2"},
            31:{"type": "SFP+"   , "bus": 49, "driver": "optoe2"},
            32:{"type": "SFP+"   , "bus": 50, "driver": "optoe2"},
            33:{"type": "SFP+"   , "bus": 51, "driver": "optoe2"},
            34:{"type": "SFP+"   , "bus": 52, "driver": "optoe2"},
            35:{"type": "SFP+"   , "bus": 53, "driver": "optoe2"},
            36:{"type": "SFP+"   , "bus": 54, "driver": "optoe2"},
            37:{"type": "SFP+"   , "bus": 55, "driver": "optoe2"},
            38:{"type": "SFP+"   , "bus": 56, "driver": "optoe2"},
            39:{"type": "SFP+"   , "bus": 57, "driver": "optoe2"},
            40:{"type": "SFP28"   , "bus": 58, "driver": "optoe2"},
            41:{"type": "SFP28"   , "bus": 59, "driver": "optoe2"},
            42:{"type": "SFP28"   , "bus": 60, "driver": "optoe2"},
            43:{"type": "SFP28"   , "bus": 61, "driver": "optoe2"},
            44:{"type": "SFP28"   , "bus": 62, "driver": "optoe2"},
            45:{"type": "SFP28"   , "bus": 63, "driver": "optoe2"},
            46:{"type": "SFP28"   , "bus": 64, "driver": "optoe2"},
            47:{"type": "SFP28"   , "bus": 65, "driver": "optoe2"},
            48:{"type": "SFP28"   , "bus": 66, "driver": "optoe2"},
            49:{"type": "SFP28"   , "bus": 67, "driver": "optoe2"},
            50:{"type": "SFP28"   , "bus": 68, "driver": "optoe2"},
            51:{"type": "SFP28"   , "bus": 69, "driver": "optoe2"},
            52:{"type": "SFP28"   , "bus": 70, "driver": "optoe2"},
            53:{"type": "SFP28"   , "bus": 71, "driver": "optoe2"},
        }

        with open("/lib/platform-config/x86-64-ufispace-s9500-54cf-r0/onl/port_config.yml", 'r') as yaml_file:
            data = yaml.safe_load(yaml_file)

        # config eeprom
        for port, config in port_eeprom.items():
            self.new_i2c_device(config["driver"], 0x50, config["bus"])
            port_name = data[config["type"]][port]["port_name"]
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port_name, config["bus"]), shell=True)

    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("Current IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("After IPMI_IOCTL IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

    def disable_bmc_watchdog(self):
        os.system("ipmitool mc watchdog off")

    def baseconfig(self):

        # load default kernel driver
        os.system("modprobe i2c_i801")
        os.system("modprobe i2c_dev")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")

        #lpc driver
        self.insmod("x86-64-ufispace-s9500-54cf-lpc")

        # get hardware revision
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s9500_54cf_lpc/mb_cpld/board_hw_id"
        status, output = commands.getstatusoutput(cmd)
        if status != 0:
            msg("Get hwr rev id from LPC failed, status={}, output={}, cmd={}\n", status, output, cmd)
            output="1"

        hw_rev_id = int(output, 16)

        ########### initialize I2C bus 0 ###########
        # i2c_i801 is built-in
        # add i2c_ismt
        #self.insmod("i2c-ismt") #module not found
        bus_i801=0
        bus_ismt=1
        os.system("modprobe i2c-ismt")

        # check i2c bus status
        self.check_i2c_status(bus_i801)

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        # init MUX sysfs
        self.bsp_pr("Init i2c");
        self.init_mux(bus_i801, bus_ismt, hw_rev_id)

        self.insmod("x86-64-ufispace-eeprom-mb")
        self.insmod("optoe")

        # init SYS EEPROM devices
        self.bsp_pr("Init mb eeprom");
        self.new_i2c_devices(
            [
                #  on cpu board
                ('mb_eeprom', 0x57, bus_ismt),
            ]
        )

        # init EEPROM
        self.bsp_pr("Init port eeprom");
        self.init_eeprom()

        # init CPLD
        self.bsp_pr("Init CPLD")
        self.insmod("x86-64-ufispace-s9500-54cf-cpld")
        for i, addr in enumerate((0x30, 0x31, 0x32)):
            self.new_i2c_device("s9500_54cf_cpld" + str(i + 1), addr, 2)

        #enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        self.bsp_pr("Init done");
        return True
