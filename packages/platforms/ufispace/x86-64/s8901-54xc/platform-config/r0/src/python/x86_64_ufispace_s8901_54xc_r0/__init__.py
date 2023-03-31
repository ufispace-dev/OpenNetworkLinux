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

class DriverType():
    I2C_ISMT   = 0
    LPC        = 1
    SYS_EEPROM = 2
    OPTOE      = 3
    CPLD       = 4

class OnlPlatform_x86_64_ufispace_s8901_54xc_r0(OnlPlatformUfiSpace):
    VENDOR_PREFIX="x86-64-ufispace-"
    PLTM_PREFIX=VENDOR_PREFIX + "s8901-54xc-"
    PLATFORM=PLTM_PREFIX + "r0"
    MODEL="S8901-54XC"
    SYS_OBJECT_ID=".8901.54"
    PORT_COUNT=54
    PORT_CONFIG="48x25 + 6x100"

    LEVEL_INFO=1
    LEVEL_ERR=2
    SYSFS_LPC="/sys/devices/platform/x86_64_ufispace_s8901_54xc_lpc"
    FS_PLTM_CFG="/lib/platform-config/current/onl"
    PORT_CFG=FS_PLTM_CFG + "/port_config.yml"

    DRIVER={DriverType.I2C_ISMT: "i2c-ismt",
            DriverType.LPC: PLTM_PREFIX + "lpc",
            DriverType.SYS_EEPROM: VENDOR_PREFIX + "sys-eeprom",
            DriverType.OPTOE: "optoe",
            DriverType.CPLD: PLTM_PREFIX + "cpld"}

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
                    subprocess.call("echo {} > /sys/bus/i2c/devices/{}-{:0>4x}/port_name".format(port_name, bus, addr_eeprom), shell=True)

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

        # init all GPIO direction to "in"
        gpio_dir = ["in"] * (gpio_max+1)

        # init GPIO direction to output high
        for i in range(gpio_max-95, gpio_max+1):
            gpio_dir[i] = "high"

        # export GPIO and configure direction
        for i in range(gpio_max-95, gpio_max+1):
            os.system("echo {} > /sys/class/gpio/export".format(i))
            os.system("echo {} > /sys/class/gpio/gpio{}/direction".format(gpio_dir[i], i))

    def init_cpld(self):
        bus = 2
        addrs = (0x30, 0x31)
        dev_name_prefix = "s8901_54xc_cpld"

        self.bsp_pr("Init CPLD")

        # load driver
        self.insmod(self.DRIVER[DriverType.CPLD])

        # create i2c device
        for i, addr in enumerate(addrs):
            self.new_i2c_device(dev_name_prefix + str(i+1), addr, bus)

        # enable event ctrl
        for _, addr in enumerate(addrs):
            subprocess.call("echo 1 > /sys/bus/i2c/devices/{}-{:0>4x}/cpld_evt_ctrl".format(bus, addr), shell=True)

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
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s8901_54xc_lpc/bsp/bsp_gpio_max"
        status, output = commands.getstatusoutput(cmd)
        if status != 0:
            self.bsp_pr("Get gpio max failed, status={}, output={}, cmd={}\n".format(status, output, cmd), self.LEVEL_ERR);
            self.bsp_pr("Use default GPIO MAX value 511\n".format(status, output, cmd), self.LEVEL_ERR);
            output="511"

        gpio_max = int(output, 10)
        self.bsp_pr("GPIO MAX: {}".format(gpio_max));

        return gpio_max

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

        # lpc driver
        self.insmod(self.DRIVER[DriverType.LPC])

        # initialize I2C bus 0
        # i2c_i801 is built-in
        # add i2c_ismt

        bus_i801 = 0
        bus_ismt = 1

        os.system("modprobe {}".format(self.DRIVER[DriverType.I2C_ISMT]))

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

        self.bsp_pr("Init done")

        return True

