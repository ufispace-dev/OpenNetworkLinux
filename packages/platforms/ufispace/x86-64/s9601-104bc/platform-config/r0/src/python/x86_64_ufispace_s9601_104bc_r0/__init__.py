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

class OnlPlatform_x86_64_ufispace_s9601_104bc_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9601-104bc-r0'
    MODEL="S9601-104BC"
    SYS_OBJECT_ID=".9601.104"
    PORT_COUNT=104
    PORT_CONFIG="96x25 + 4x200 + 4x100"
    LEVEL_INFO=1
    LEVEL_ERR=2
    BSP_VERSION='1.0.5'
    PATH_SYS_I2C_DEV_ATTR="/sys/bus/i2c/devices/{}-{:0>4x}/{}"
    PATH_SYS_GPIO = "/sys/class/gpio"
    PATH_SYSTEM_LED="/sys/bus/i2c/devices/5-0030/cpld_system_led_sys"
    SYSTEM_LED_GREEN=0b10010000
    PATH_LPC="/sys/devices/platform/x86_64_ufispace_s9601_104bc_lpc"
    PATH_LPC_GRP_BSP=PATH_LPC+"/bsp"
    PATH_LPC_GRP_MB_CPLD=PATH_LPC+"/mb_cpld"
    PATH_LPC_GRP_EC=PATH_LPC+"/ec"
    PATH_PORT_CONFIG="/lib/platform-config/"+PLATFORM+"/onl/port_config.yml"
    PATH_EPDM_CLI="/lib/platform-config/"+PLATFORM+"/onl/epdm_cli"

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

    def config_bsp_ver(self, bsp_ver):
        bsp_version_path=self.PATH_LPC_GRP_BSP+"/bsp_version"
        if os.path.exists(bsp_version_path):
            with open(bsp_version_path, "w") as f:
                f.write(bsp_ver)

    def get_board_version(self):
        board = {}
        board_attrs = {
            "hw_rev"  : {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/board_hw_id"},
            "deph_id" : {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/board_deph_id"},
            "hw_build": {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/board_build_id"},
        }

        for key, val in board_attrs.items():
            cmd = "cat {}".format(val["sysfs"])
            status, output = commands.getstatusoutput(cmd)
            if status != 0:
                self.bsp_pr("Get hwr rev id from LPC failed, status={}, output={}, cmd={}\n".format(status, output, cmd), self.LEVEL_ERR)
                output="1"
            board[key] = int(output, 10)

        return board

    def get_gpio_max(self):
        cmd = "cat {}/bsp_gpio_max".format(self.PATH_LPC_GRP_BSP)
        status, output = commands.getstatusoutput(cmd)
        if status != 0:
            self.bsp_pr("Get gpio max failed, status={}, output={}, cmd={}\n".format(status, output, cmd), self.LEVEL_ERR)
            output="511"

        gpio_max = int(output, 10)

        return gpio_max

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
            ('pca9548', 0x73, bus_ismt),  #9548_ROOT_CPLD
            ('pca9548', 0x72, bus_i801),  #9548_ROOT_PORT_1
            ('pca9548', 0x75, bus_i801),  #9548_ROOT_PORT_2
            ('pca9548', 0x76, 10), #9548_CHILD_SFP_0_7
            ('pca9548', 0x76, 11), #9548_CHILD_SFP_8_15
            ('pca9548', 0x76, 12), #9548_CHILD_SFP_16_23
            ('pca9548', 0x76, 13), #9548_CHILD_SFP_24_31
            ('pca9548', 0x76, 14), #9548_CHILD_SFP_32_39
            ('pca9548', 0x76, 15), #9548_CHILD_SFP_40_47
            ('pca9548', 0x76, 16), #9548_CHILD_SFP_48_55
            ('pca9548', 0x76, 17), #9548_CHILD_SFP_56_63
            ('pca9548', 0x76, 22), #9548_CHILD_SFP_64_71
            ('pca9548', 0x76, 23), #9548_CHILD_SFP_72_79
            ('pca9548', 0x76, 24), #9548_CHILD_SFP_80_87
            ('pca9548', 0x76, 25), #9548_CHILD_SFP_88_95
            ('pca9548', 0x76, 21), #9548_CHILD_QSFP_0_7

        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

    def init_sys_eeprom(self):
        sys_eeprom = [
            ('sys_eeprom', 0x57, 4),
        ]

        self.new_i2c_devices(sys_eeprom)

    def init_cpld(self):
        cpld = [
            ('s9601_104bc_cpld1', 0x30, 5),
            ('s9601_104bc_cpld2', 0x31, 5),
            ('s9601_104bc_cpld3', 0x32, 5),
            ('s9601_104bc_cpld4', 0x33, 5),
            ('s9601_104bc_cpld5', 0x34, 5),
        ]

        self.new_i2c_devices(cpld)

    def init_eeprom(self):
        data = None
        port_eeprom = {
            0:   {"type": "SFP"   , "bus": 26 , "driver": "optoe2"},
            1:   {"type": "SFP"   , "bus": 27 , "driver": "optoe2"},
            2:   {"type": "SFP"   , "bus": 28 , "driver": "optoe2"},
            3:   {"type": "SFP"   , "bus": 29 , "driver": "optoe2"},
            4:   {"type": "SFP"   , "bus": 30 , "driver": "optoe2"},
            5:   {"type": "SFP"   , "bus": 31 , "driver": "optoe2"},
            6:   {"type": "SFP"   , "bus": 32 , "driver": "optoe2"},
            7:   {"type": "SFP"   , "bus": 33 , "driver": "optoe2"},
            8:   {"type": "SFP"   , "bus": 34 , "driver": "optoe2"},
            9:   {"type": "SFP"   , "bus": 35 , "driver": "optoe2"},
            10:  {"type": "SFP"   , "bus": 36 , "driver": "optoe2"},
            11:  {"type": "SFP"   , "bus": 37 , "driver": "optoe2"},
            12:  {"type": "SFP"   , "bus": 38 , "driver": "optoe2"},
            13:  {"type": "SFP"   , "bus": 39 , "driver": "optoe2"},
            14:  {"type": "SFP"   , "bus": 40 , "driver": "optoe2"},
            15:  {"type": "SFP"   , "bus": 41 , "driver": "optoe2"},
            16:  {"type": "SFP"   , "bus": 42 , "driver": "optoe2"},
            17:  {"type": "SFP"   , "bus": 43 , "driver": "optoe2"},
            18:  {"type": "SFP"   , "bus": 44 , "driver": "optoe2"},
            19:  {"type": "SFP"   , "bus": 45 , "driver": "optoe2"},
            20:  {"type": "SFP"   , "bus": 46 , "driver": "optoe2"},
            21:  {"type": "SFP"   , "bus": 47 , "driver": "optoe2"},
            22:  {"type": "SFP"   , "bus": 48 , "driver": "optoe2"},
            23:  {"type": "SFP"   , "bus": 49 , "driver": "optoe2"},
            24:  {"type": "SFP"   , "bus": 50 , "driver": "optoe2"},
            25:  {"type": "SFP"   , "bus": 51 , "driver": "optoe2"},
            26:  {"type": "SFP"   , "bus": 52 , "driver": "optoe2"},
            27:  {"type": "SFP"   , "bus": 53 , "driver": "optoe2"},
            28:  {"type": "SFP"   , "bus": 54 , "driver": "optoe2"},
            29:  {"type": "SFP"   , "bus": 55 , "driver": "optoe2"},
            30:  {"type": "SFP"   , "bus": 56 , "driver": "optoe2"},
            31:  {"type": "SFP"   , "bus": 57 , "driver": "optoe2"},
            32:  {"type": "SFP"   , "bus": 58 , "driver": "optoe2"},
            33:  {"type": "SFP"   , "bus": 59 , "driver": "optoe2"},
            34:  {"type": "SFP"   , "bus": 60 , "driver": "optoe2"},
            35:  {"type": "SFP"   , "bus": 61 , "driver": "optoe2"},
            36:  {"type": "SFP"   , "bus": 62 , "driver": "optoe2"},
            37:  {"type": "SFP"   , "bus": 63 , "driver": "optoe2"},
            38:  {"type": "SFP"   , "bus": 64 , "driver": "optoe2"},
            39:  {"type": "SFP"   , "bus": 65 , "driver": "optoe2"},
            40:  {"type": "SFP"   , "bus": 66 , "driver": "optoe2"},
            41:  {"type": "SFP"   , "bus": 67 , "driver": "optoe2"},
            42:  {"type": "SFP"   , "bus": 68 , "driver": "optoe2"},
            43:  {"type": "SFP"   , "bus": 69 , "driver": "optoe2"},
            44:  {"type": "SFP"   , "bus": 70 , "driver": "optoe2"},
            45:  {"type": "SFP"   , "bus": 71 , "driver": "optoe2"},
            46:  {"type": "SFP"   , "bus": 72 , "driver": "optoe2"},
            47:  {"type": "SFP"   , "bus": 73 , "driver": "optoe2"},
            48:  {"type": "SFP"   , "bus": 74 , "driver": "optoe2"},
            49:  {"type": "SFP"   , "bus": 75 , "driver": "optoe2"},
            50:  {"type": "SFP"   , "bus": 76 , "driver": "optoe2"},
            51:  {"type": "SFP"   , "bus": 77 , "driver": "optoe2"},
            52:  {"type": "SFP"   , "bus": 78 , "driver": "optoe2"},
            53:  {"type": "SFP"   , "bus": 79 , "driver": "optoe2"},
            54:  {"type": "SFP"   , "bus": 80 , "driver": "optoe2"},
            55:  {"type": "SFP"   , "bus": 81 , "driver": "optoe2"},
            56:  {"type": "SFP"   , "bus": 82 , "driver": "optoe2"},
            57:  {"type": "SFP"   , "bus": 83 , "driver": "optoe2"},
            58:  {"type": "SFP"   , "bus": 84 , "driver": "optoe2"},
            59:  {"type": "SFP"   , "bus": 85 , "driver": "optoe2"},
            60:  {"type": "SFP"   , "bus": 86 , "driver": "optoe2"},
            61:  {"type": "SFP"   , "bus": 87 , "driver": "optoe2"},
            62:  {"type": "SFP"   , "bus": 88 , "driver": "optoe2"},
            63:  {"type": "SFP"   , "bus": 89 , "driver": "optoe2"},
            64:  {"type": "SFP"   , "bus": 90 , "driver": "optoe2"},
            65:  {"type": "SFP"   , "bus": 91 , "driver": "optoe2"},
            66:  {"type": "SFP"   , "bus": 92 , "driver": "optoe2"},
            67:  {"type": "SFP"   , "bus": 93 , "driver": "optoe2"},
            68:  {"type": "SFP"   , "bus": 94 , "driver": "optoe2"},
            69:  {"type": "SFP"   , "bus": 95 , "driver": "optoe2"},
            70:  {"type": "SFP"   , "bus": 96 , "driver": "optoe2"},
            71:  {"type": "SFP"   , "bus": 97 , "driver": "optoe2"},
            72:  {"type": "SFP"   , "bus": 98 , "driver": "optoe2"},
            73:  {"type": "SFP"   , "bus": 99 , "driver": "optoe2"},
            74:  {"type": "SFP"   , "bus": 100, "driver": "optoe2"},
            75:  {"type": "SFP"   , "bus": 101, "driver": "optoe2"},
            76:  {"type": "SFP"   , "bus": 102, "driver": "optoe2"},
            77:  {"type": "SFP"   , "bus": 103, "driver": "optoe2"},
            78:  {"type": "SFP"   , "bus": 104, "driver": "optoe2"},
            79:  {"type": "SFP"   , "bus": 105, "driver": "optoe2"},
            80:  {"type": "SFP"   , "bus": 106, "driver": "optoe2"},
            81:  {"type": "SFP"   , "bus": 107, "driver": "optoe2"},
            82:  {"type": "SFP"   , "bus": 108, "driver": "optoe2"},
            83:  {"type": "SFP"   , "bus": 109, "driver": "optoe2"},
            84:  {"type": "SFP"   , "bus": 110, "driver": "optoe2"},
            85:  {"type": "SFP"   , "bus": 111, "driver": "optoe2"},
            86:  {"type": "SFP"   , "bus": 112, "driver": "optoe2"},
            87:  {"type": "SFP"   , "bus": 113, "driver": "optoe2"},
            88:  {"type": "SFP"   , "bus": 114, "driver": "optoe2"},
            89:  {"type": "SFP"   , "bus": 115, "driver": "optoe2"},
            90:  {"type": "SFP"   , "bus": 116, "driver": "optoe2"},
            91:  {"type": "SFP"   , "bus": 117, "driver": "optoe2"},
            92:  {"type": "SFP"   , "bus": 118, "driver": "optoe2"},
            93:  {"type": "SFP"   , "bus": 119, "driver": "optoe2"},
            94:  {"type": "SFP"   , "bus": 120, "driver": "optoe2"},
            95:  {"type": "SFP"   , "bus": 121, "driver": "optoe2"},
            96:  {"type": "QSFP"  , "bus": 122 , "driver": "optoe1"},
            97:  {"type": "QSFP"  , "bus": 123 , "driver": "optoe1"},
            98:  {"type": "QSFP"  , "bus": 124 , "driver": "optoe1"},
            99:  {"type": "QSFP"  , "bus": 125 , "driver": "optoe1"},
            100: {"type": "QSFP"  , "bus": 126 , "driver": "optoe1"},
            101: {"type": "QSFP"  , "bus": 127 , "driver": "optoe1"},
            102: {"type": "QSFP"  , "bus": 128 , "driver": "optoe1"},
            103: {"type": "QSFP"  , "bus": 129 , "driver": "optoe1"},
            104: {"type": "MGMT"  , "bus": 18 , "driver": "optoe2"},
            105: {"type": "MGMT"  , "bus": 19 , "driver": "optoe2"},
        }

        with open(self.PATH_PORT_CONFIG, 'r') as yaml_file:
            data = yaml.safe_load(yaml_file)

        # config eeprom
        for port, config in port_eeprom.items():
            addr=0x50
            self.new_i2c_device(config["driver"], addr, config["bus"])
            port_name = data[config["type"]][port]["port_name"]
            sysfs=self.PATH_SYS_I2C_DEV_ATTR.format( config["bus"], addr, "port_name")
            subprocess.call("echo {} > {}".format(port_name, sysfs), shell=True)

    def init_gpio(self, gpio_max):
        # init GPIO sysfs
        self.new_i2c_devices(
            [
                # ('pca9535', 0x20, 4), #9535_IO_EXP_01 (9535_BOARD_ID)
            ]
        )

        gpio_min = gpio_max - 0
        gpio_tnumber = (gpio_max + 1)
        # init all GPIO direction to "in"
        gpio_dir = ["in"] * gpio_tnumber

        # init GPIO direction to output low
        for off in range(-0, -1, -1):
            gpio_dir[gpio_max + off] = "low"

        # init GPIO direction to output high
        for off in range(-0, -1, -1):
            gpio_dir[gpio_max + off] = "high"

        # export GPIO and configure direction
        for i in range(gpio_min, gpio_tnumber):
            os.system("echo {} > {}/export".format(i, self.PATH_SYS_GPIO))
            os.system("echo {} > {}/gpio{}/direction".format(gpio_dir[i], self.PATH_SYS_GPIO, i))

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

    def baseconfig(self):

        # load default kernel driver
        os.system("rmmod i2c_ismt")
        os.system("rmmod i2c_i801")
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
        self.insmod("x86-64-ufispace-s9601-104bc-lpc")

        # version setting
        self.bsp_pr("BSP version {}".format(self.BSP_VERSION))
        self.config_bsp_ver(self.BSP_VERSION)

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
        self.insmod("x86-64-ufispace-s9601-104bc-cpld")
        self.init_cpld()

        # init EEPROM
        self.bsp_pr("Init port eeprom")
        self.insmod("optoe")
        self.init_eeprom()

        #enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        self.bsp_pr("Init bcm82752")

        # init bcm82752
        os.system("timeout 120s {} init -s auto_10G -d mdio".format(self.PATH_EPDM_CLI))

        # set system led to green
        self.set_system_led_green()

        self.bsp_pr("Init done")
        return True

