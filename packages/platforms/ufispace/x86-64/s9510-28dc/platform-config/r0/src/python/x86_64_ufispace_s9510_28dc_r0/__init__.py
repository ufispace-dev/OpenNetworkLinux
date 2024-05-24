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

class OnlPlatform_x86_64_ufispace_s9510_28dc_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9510-28dc-r0'
    MODEL="S9510-28DC"
    SYS_OBJECT_ID=".9510.28"
    PORT_COUNT=28
    PORT_CONFIG="24x25 + 2x100 + 2x400"
    LEVEL_INFO=1
    LEVEL_ERR=2
    BSP_VERSION='1.0.21'

    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self, bus_i801, bus_ismt, hw_rev_id):
        sysfs_mux_reset = "/sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/mb_cpld/mux_reset_all"

        # Alpha
        if hw_rev_id == 1:
            bus=bus_ismt

        # Beta and later
        else:
            bus=bus_i801

        # Check I2C status,assume
        retcode = os.system("i2cget -f -y {} 0x76 > /dev/null 2>&1".format(bus))
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
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/bsp/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/bsp/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/bsp/bsp_pr_info"

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

    def disable_watchdog(self):        
        sysfs_watchdog = "/dev/watchdog"
            
        if os.path.exists(sysfs_watchdog):
            os.remove(sysfs_watchdog)

    def init_mux(self, bus_i801, bus_ismt, hw_rev_id):
        # Alpha
        if hw_rev_id == 1:
            i2c_muxs = [
                ('pca9546', 0x75, bus_ismt),   #9546_ROOT_TIMING
                ('pca9546', 0x76, bus_ismt),   #9546_ROOT_SFP
                ('pca9546', 0x70, 9), #9546_CHILD_QSFPX_0_3
                ('pca9548', 0x71, 9), #9548_CHILD_SFP_4_11
                ('pca9548', 0x72, 9), #9548_CHILD_SFP_12_19
                ('pca9548', 0x73, 9), #9548_CHILD_SFP_20_27
            ]

        # Beta and later
        else:
            i2c_muxs = [
                ('pca9546', 0x75, bus_ismt),   #9546_ROOT_TIMING
                ('pca9546', 0x76, bus_i801),   #9546_ROOT_SFP
                ('pca9546', 0x70, 9), #9546_CHILD_QSFPX_0_3
                ('pca9548', 0x71, 9), #9548_CHILD_SFP_4_11
                ('pca9548', 0x72, 9), #9548_CHILD_SFP_12_19
                ('pca9548', 0x73, 9), #9548_CHILD_SFP_20_27
            ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

    def init_eeprom(self):
        data = None
        port_eeprom = {
            0: {"type": "QSFPDD", "bus": 13, "driver": "optoe3"},
            1: {"type": "QSFPDD", "bus": 12, "driver": "optoe3"},
            2: {"type": "QSFP"  , "bus": 11, "driver": "optoe1"},
            3: {"type": "QSFP"  , "bus": 10, "driver": "optoe1"},
            4: {"type": "SFP"   , "bus": 14, "driver": "optoe2"},
            5: {"type": "SFP"   , "bus": 15, "driver": "optoe2"},
            6: {"type": "SFP"   , "bus": 16, "driver": "optoe2"},
            7: {"type": "SFP"   , "bus": 17, "driver": "optoe2"},
            8: {"type": "SFP"   , "bus": 18, "driver": "optoe2"},
            9: {"type": "SFP"   , "bus": 19, "driver": "optoe2"},
            10:{"type": "SFP"   , "bus": 20, "driver": "optoe2"},
            11:{"type": "SFP"   , "bus": 21, "driver": "optoe2"},
            12:{"type": "SFP"   , "bus": 22, "driver": "optoe2"},
            13:{"type": "SFP"   , "bus": 23, "driver": "optoe2"},
            14:{"type": "SFP"   , "bus": 24, "driver": "optoe2"},
            15:{"type": "SFP"   , "bus": 25, "driver": "optoe2"},
            16:{"type": "SFP"   , "bus": 26, "driver": "optoe2"},
            17:{"type": "SFP"   , "bus": 27, "driver": "optoe2"},
            18:{"type": "SFP"   , "bus": 28, "driver": "optoe2"},
            19:{"type": "SFP"   , "bus": 29, "driver": "optoe2"},
            20:{"type": "SFP"   , "bus": 30, "driver": "optoe2"},
            21:{"type": "SFP"   , "bus": 31, "driver": "optoe2"},
            22:{"type": "SFP"   , "bus": 32, "driver": "optoe2"},
            23:{"type": "SFP"   , "bus": 33, "driver": "optoe2"},
            24:{"type": "SFP"   , "bus": 34, "driver": "optoe2"},
            25:{"type": "SFP"   , "bus": 35, "driver": "optoe2"},
            26:{"type": "SFP"   , "bus": 36, "driver": "optoe2"},
            27:{"type": "SFP"   , "bus": 37, "driver": "optoe2"},
        }
        with open("/lib/platform-config/x86-64-ufispace-s9510-28dc-r0/onl/port_config.yml", 'r') as yaml_file:
            data = yaml.safe_load(yaml_file)

        # config eeprom
        for port, config in port_eeprom.items():
            self.new_i2c_device(config["driver"], 0x50, config["bus"])
            port_name = data[config["type"]][port]["port_name"]
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port_name, config["bus"]), shell=True)

    def init_gpio(self, gpio_max):
        # init GPIO sysfs
        self.new_i2c_devices(
            [
                ('pca9535', 0x20, 4), #9535_IO_EXP_01 (9535_BOARD_ID)
                ('pca9535', 0x21, 6), #9535_IO_EXP_02 (9535_QSFPX_0_3)
                ('pca9535', 0x22, 6), #9535_IO_EXP_03 (9535_TX_DIS_4_19)
                ('pca9535', 0x24, 6), #9535_IO_EXP_04 (9535_TX_DIS_20_27)
                ('pca9535', 0x26, 7), #9535_IO_EXP_05 (9535_TX_FLT_4_19)
                ('pca9535', 0x27, 7), #9535_IO_EXP_06 (9535_TX_FLT_20_27)
                ('pca9535', 0x25, 7), #9535_IO_EXP_07 (9535_RATE_SELECT_4_19)
                ('pca9535', 0x23, 7), #9535_IO_EXP_08 (9535_RATE_SELECT_20_27)
                ('pca9535', 0x20, 8), #9535_IO_EXP_11 (9535_MOD_ABS_4_19)
                ('pca9535', 0x22, 8), #9535_IO_EXP_09 (9535_MOD_ABS_20_27)
                ('pca9535', 0x21, 8), #9535_IO_EXP_12 (9535_RX_LOS_4_19)
                ('pca9535', 0x24, 8)  #9535_IO_EXP_10 (9535_RX_LOS_20_27)
            ]
        )

        gpio_min = gpio_max - 191
        gpio_tnumber = (gpio_max + 1)
        # init all GPIO direction to "in"
        gpio_dir = ["in"] * gpio_tnumber

        # init GPIO direction to output low
        for off in range(-20, -24, -1) + \
                 range(-32, -48, -1) + \
                 range(-56, -64, -1) + \
                 range(-116, -120, -1):
            gpio_dir[gpio_max + off] = "low"

        # init GPIO direction to output high
        for off in range(-16, -20, -1) + \
                 range(-96, -112, -1) + \
                 range(-120, -128, -1):
            gpio_dir[gpio_max + off] = "high"

        # export GPIO and configure direction
        for i in range(gpio_min, gpio_tnumber):
            os.system("echo {} > /sys/class/gpio/export".format(i))
            os.system("echo {} > /sys/class/gpio/gpio{}/direction".format(gpio_dir[i], i))

    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("Current IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("After IPMI_IOCTL IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

    def baseconfig(self):
        #lpc driver
        self.insmod("x86-64-ufispace-s9510-28dc-lpc")

        # version setting
        self.bsp_pr("BSP version {}".format(self.BSP_VERSION));
        bsp_version_path="/sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/bsp/bsp_version"
        if os.path.exists(bsp_version_path):
            with open(bsp_version_path, "w") as f:
                f.write(self.BSP_VERSION)

        # get hardware revision
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/mb_cpld/board_hw_id"
        status, output = commands.getstatusoutput(cmd)
        if status != 0:
            self.bsp_pr("Get hwr rev id from LPC failed, status={}, output={}, cmd={}\n".format(status, output, cmd), self.LEVEL_ERR);
            output="1"

        hw_rev_id = int(output, 10)

        # get gpio max
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/bsp/bsp_gpio_max"
        status, output = commands.getstatusoutput(cmd)
        if status != 0:
            self.bsp_pr("Get gpio max failed, status={}, output={}, cmd={}\n".format(status, output, cmd), self.LEVEL_ERR);
            output="511"

        gpio_max = int(output, 10)
        self.bsp_pr("GPIO MAX: {}".format(gpio_max));

        ########### initialize I2C bus 0 ###########
        # i2c_i801 is built-in
        # add i2c_ismt
        #self.insmod("i2c-ismt") #module not found
        bus_i801=0
        bus_ismt=1
        os.system("modprobe i2c-ismt")

        # check i2c bus status
        self.check_i2c_status(bus_i801, bus_ismt, hw_rev_id)

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        # init MUX sysfs
        self.bsp_pr("Init i2c");
        self.init_mux(bus_i801, bus_ismt, hw_rev_id)

        self.insmod("x86-64-ufispace-eeprom-mb")
        self.insmod("x86-64-ufispace-optoe")

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

        # init GPIO sysfs
        self.bsp_pr("Init gpio");
        self.init_gpio(gpio_max)

        #enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        #disable watchdog
        self.disable_watchdog()

        self.bsp_pr("Init done");
        return True

