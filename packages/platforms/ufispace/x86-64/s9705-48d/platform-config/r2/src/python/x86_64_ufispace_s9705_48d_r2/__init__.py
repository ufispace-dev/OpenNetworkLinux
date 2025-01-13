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

class OnlPlatform_x86_64_ufispace_s9705_48d_r2(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9705-48d-r2'
    MODEL="S9705-48D"
    SYS_OBJECT_ID=".9705.48"
    PORT_COUNT=48
    PORT_CONFIG="48x400"

    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self):
        sysfs_mux_reset = "/sys/devices/platform/x86_64_ufispace_s9705_48d_lpc/cpu_cpld/mux_reset"

        # Check I2C status
        retcode = os.system("i2cget -f -y 0 0x75 > /dev/null 2>&1")
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

    def init_eeprom(self):
        port = 0

        # init QSFPDD EEPROM
        for bus in range(21, 69):
            self.new_i2c_device('optoe3', 0x50, bus)
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

    def get_gpio_max(self):
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s9705_48d_lpc/bsp/bsp_gpio_max"
        output = ""
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            msg("get_gpio_max() failed, exception={}\n".format(e))
            msg("Use default GPIO MAX value -1\n")
            output="-1"

        gpio_max = int(output, 10)
        msg("GPIO MAX: {}\n".format(gpio_max))

        return gpio_max

    def get_gpio_base(self):
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s9705_48d_lpc/bsp/bsp_gpio_base"
        output = ""
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            msg("get_gpio_base() failed, exception={}\n".format(e))
            msg("Use default GPIO Base value -1\n")
            output="-1"

        gpio_base = int(output, 10)
        msg("GPIO Base: {}\n".format(gpio_base))

        return gpio_base

    def init_gpio(self):
        # init GPIO sysfs
        self.new_i2c_device('pca9539', 0x74, 1)
        self.new_i2c_device('pca9555', 0x20, 3)
        self.new_i2c_device('pca9539', 0x77, 0)

        #get gpio_max/gpio_base
        gpio_max = self.get_gpio_max()
        gpio_base = self.get_gpio_base()
        is_gpio_base = False

        if gpio_base >= 0 :
            base = gpio_base
            is_gpio_base = True
        elif gpio_max >= 0:
            base = gpio_max
            is_gpio_base = False
        else:
            msg("invalid gpio_max {} and gpio_base {}, bsp init stopped".format(gpio_max, gpio_base), self.LEVEL_ERR)
            exit(1)

        if is_gpio_base:
            # export GPIO
            for i in range(base, base+48):
                os.system("echo {} > /sys/class/gpio/export".format(i))

            # init GPIO direction
            # pca9539 0x74
            os.system("echo high > /sys/class/gpio/gpio" + str(base)    + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+1)  + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base+2)  + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+3)  + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+4)  + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+5)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+6)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+7)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+8)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+9)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+10) + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+11) + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+12) + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+13) + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base+14) + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base+15) + "/direction")

            # init GPIO direction
            # pca9535 0x20, pca9539 0x77
            for i in range(base+16, base+48):
                os.system("echo in > /sys/class/gpio/gpio{}/direction".format(i))
        else:
            # export GPIO
            for i in range(base-47, base+1):
                os.system("echo {} > /sys/class/gpio/export".format(i))

            # init GPIO direction
            # pca9539 0x74
            os.system("echo high > /sys/class/gpio/gpio" + str(base)    + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base-1)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-2)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-3)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-4)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-5)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-6)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-7)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-8)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-9)  + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-10) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-11) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-12) + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base-13) + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-14) + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base-15) + "/direction")

            # init GPIO direction
            # pca9535 0x20, pca9539 0x77
            for i in range(base-47, base-15):
                os.system("echo in > /sys/class/gpio/gpio{}/direction".format(i))

    def baseconfig(self):

        # load default kernel driver
        os.system("modprobe -r i2c_i801")
        self.insmod("i2c-smbus", False)
        os.system("modprobe i2c_i801")
        os.system("modprobe i2c_dev")
        os.system("modprobe gpio_pca953x")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe lm75")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")

        # lpc driver
        self.insmod("x86-64-ufispace-s9705-48d-lpc")

        # check i2c bus status
        self.check_i2c_status()

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        # Golden Finger to show CPLD
        os.system("i2cset -y 0 0x75 0x2")
        os.system("i2cget -y 0 0x30 0x2")
        os.system("i2cget -y 0 0x31 0x2")
        os.system("i2cget -y 0 0x32 0x2")
        os.system("i2cget -y 0 0x33 0x2")
        os.system("i2cset -y 0 0x75 0x0")

        #clk free run
        self.set_clk_freerun()

        ########### initialize I2C bus 0 ###########
        # init PCA9548

        i2c_muxs = [
            ('pca9548', 0x75, 0),
            ('pca9546', 0x71, 0),
            ('pca9546', 0x72, 6),
            ('pca9546', 0x74, 6),
            ('pca9548', 0x70, 14),
            ('pca9548', 0x70, 15),
            ('pca9548', 0x70, 16),
            ('pca9548', 0x70, 18),
            ('pca9548', 0x70, 19),
            ('pca9548', 0x70, 20),
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

        self.insmod("x86-64-ufispace-eeprom-mb")
        self.insmod("optoe")

        # init SYS EEPROM devices
        self.new_i2c_devices(
            [
                #  on cpu board
                ('mb_eeprom', 0x57, 0),
            ]
        )

        # init EEPROM
        self.init_eeprom()

        # init Temperature
        self.new_i2c_devices(
            [
                # CPU Board Temp
                ('tmp75', 0x4F, 0),
            ]
        )

        self.init_gpio()

        #CPLD
        self.insmod("x86-64-ufispace-s9705-48d-cpld")
        for i in range(4):
            self.new_i2c_device("s9705_48d_cpld" + str(i+1), 0x30 + i, 2)

        #set led clk source
        os.system("echo '0' > /sys/bus/i2c/devices/2-0031/cpld_led_clk_src")
        os.system("echo '0' > /sys/bus/i2c/devices/2-0033/cpld_led_clk_src")

        #config mac rov

        cpld_addr=[30, 32]
        cpld_bus=2
        rov_bus=[5,4]

        # vid to mac vdd value mapping
        vdd_val_array=( 0.82,  0.82,  0.80,  0.82,  0.84,  0.86,  0.82,  0.78 )
        # vid to rov reg value mapping
        rov_reg_array=( 0x73, 0x73, 0x6f, 0x73, 0x77, 0x7b, 0x73, 0x6b )

        for index, cpld in enumerate(cpld_addr):
            #get rov from cpld
            reg_val_str = subprocess.check_output("cat /sys/bus/i2c/devices/{}-00{}/cpld_10gmux_config".format(cpld_bus, cpld), shell=True)
            reg_val = int(reg_val_str, 16)
            vid = reg_val & 0x7
            mac_vdd_val = vdd_val_array[vid]
            rov_reg_val = rov_reg_array[vid]
            #set rov to mac
            msg("Setting mac vdd %1.2f with rov register value 0x%x\n" % (mac_vdd_val, rov_reg_val) )
            os.system("i2cset -y {} {} {} {} w".format(rov_bus[index], 0x70, 0x21, rov_reg_val))

        # enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        #disable watchdog
        self.disable_bmc_watchdog()

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        return True

    def set_clk_freerun(self):
        addr = 0x64
        bus = 0
        CLKGEN_CONFIG = {
            "FREE_RUN": {
                "write_preamble": [
                    {"HiData": 0xb, "LowData": 0x24, "value": 0xc0},
                    {"HiData": 0xb, "LowData": 0x25, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x40, "value": 0x01}
                ],
                "perform_freerun": [
                    {"HiData": 0x0, "LowData": 0x18, "value": 0xff},
                    {"HiData": 0x0, "LowData": 0x19, "value": 0xff},
                    {"HiData": 0x0, "LowData": 0x1a, "value": 0xff},
                    {"HiData": 0x0, "LowData": 0x2c, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x2e, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x36, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x3e, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x3f, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x41, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x46, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x4a, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x4e, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x51, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x55, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x59, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x5a, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x5b, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x5c, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x92, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x93, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x96, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x98, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x9a, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x9b, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x9d, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0x9e, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0xa0, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0xa9, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0xaa, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0xe5, "value": 0x01},
                    {"HiData": 0x0, "LowData": 0xea, "value": 0x00},
                    {"HiData": 0x0, "LowData": 0xeb, "value": 0x00},
                    {"HiData": 0x2, "LowData": 0x08, "value": 0x00},
                    {"HiData": 0x2, "LowData": 0x0e, "value": 0x00},
                    {"HiData": 0x2, "LowData": 0x94, "value": 0x80},
                    {"HiData": 0x2, "LowData": 0x96, "value": 0x00},
                    {"HiData": 0x2, "LowData": 0x97, "value": 0x00},
                    {"HiData": 0x2, "LowData": 0x99, "value": 0x00},
                    {"HiData": 0x2, "LowData": 0x9d, "value": 0x0000},
                    {"HiData": 0x2, "LowData": 0xa9, "value": 0x0000},
                    {"HiData": 0x5, "LowData": 0x08, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x09, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x0a, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x0b, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x0c, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x0d, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x0e, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x0f, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x10, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x11, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x12, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x13, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x19, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x1a, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x1f, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x2c, "value": 0x0f},
                    {"HiData": 0x5, "LowData": 0x2e, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x2f, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x32, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x33, "value": 0x04},
                    {"HiData": 0x5, "LowData": 0x35, "value": 0x01},
                    {"HiData": 0x5, "LowData": 0x3d, "value": 0x0a},
                    {"HiData": 0x5, "LowData": 0x3e, "value": 0x06},
                    {"HiData": 0x5, "LowData": 0x88, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x89, "value": 0x0c},
                    {"HiData": 0x5, "LowData": 0x8b, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x8c, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x9b, "value": 0x18},
                    {"HiData": 0x5, "LowData": 0x9c, "value": 0x0c},
                    {"HiData": 0x5, "LowData": 0x9d, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x9e, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0x9f, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0xa0, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0xa1, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0xa2, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0xa4, "value": 0x20},
                    {"HiData": 0x5, "LowData": 0xa6, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0xac, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0xad, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0xae, "value": 0x00},
                    {"HiData": 0x5, "LowData": 0xb2, "value": 0x00},
                    {"HiData": 0x8, "LowData": 0x04, "value": 0x01},
                    {"HiData": 0x9, "LowData": 0x49, "value": 0x00},
                    {"HiData": 0x9, "LowData": 0x4a, "value": 0x00},
                    {"HiData": 0xb, "LowData": 0x44, "value": 0x0f},
                    {"HiData": 0xb, "LowData": 0x47, "value": 0x0f},
                    {"HiData": 0xb, "LowData": 0x48, "value": 0x0f},
                    {"HiData": 0xc, "LowData": 0x03, "value": 0x00},
                    {"HiData": 0xc, "LowData": 0x07, "value": 0x00},
                    {"HiData": 0xc, "LowData": 0x08, "value": 0x00}
                ],
                "write_soft_rst": [
                    {"HiData": 0x0, "LowData": 0x1c, "value": 0x01}
                ],
                "write_post_amble": [
                    {"HiData": 0x5, "LowData": 0x40, "value": 0x00},
                    {"HiData": 0xb, "LowData": 0x24, "value": 0xc3},
                    {"HiData": 0xb, "LowData": 0x25, "value": 0x02},
                ]
            }
        }

        #open channel for CLKGEN
        os.system("i2cset -y 0 0x71 0x2")

        #Step 1: write in the preamble
        for i in range(len(CLKGEN_CONFIG["FREE_RUN"]["write_preamble"])):
            hidata = CLKGEN_CONFIG["FREE_RUN"]["write_preamble"][i]["HiData"]
            lowdata = CLKGEN_CONFIG["FREE_RUN"]["write_preamble"][i]["LowData"]
            set_value = CLKGEN_CONFIG["FREE_RUN"]["write_preamble"][i]["value"]
            out = subprocess.check_output("i2cset -y {} {} 0x1 {}".format(bus, addr, hidata), shell=True)
            if len(out) != 0:
                msg("Set write_preamble hidata {} for CLKGEN failed.".format(i))
            out = subprocess.check_output("i2cset -y {} {} {} {}".format(bus, addr, lowdata, set_value), shell=True)
            if len(out) != 0:
                msg("Set write_preamble lowdata {} for CLKGEN failed.".format(i))
            out = subprocess.check_output("i2cget -y {} {} {}".format(bus, addr, lowdata), shell=True)
            if int(out, 16) != set_value:
                msg("Get write_preamble {} for CLKGEN failed.{}=/={}".format(i, int(out, 16), set_value))

        # *Step 2: Wait 1 sec
        time.sleep(1)

        # *Step 3: Perform the desired register modifications
        for i in range(len(CLKGEN_CONFIG["FREE_RUN"]["perform_freerun"])):

            hidata = CLKGEN_CONFIG["FREE_RUN"]["perform_freerun"][i]["HiData"]
            lowdata = CLKGEN_CONFIG["FREE_RUN"]["perform_freerun"][i]["LowData"]
            set_value = CLKGEN_CONFIG["FREE_RUN"]["perform_freerun"][i]["value"]
            if (hidata == 0x2 and lowdata == 0x9d) or (hidata == 0x2 and lowdata == 0xa9):
                out = subprocess.check_output("i2cset -y {} {} 0x1 {}".format(bus, addr, hidata), shell=True)
                if len(out) != 0:
                    msg("Set perform_freerun {} for CLKGEN failed.".format(i))
                out = subprocess.check_output("i2cset -y {} {} 0x1 {} w".format(bus, addr, lowdata), shell=True)
                if len(out) != 0:
                    msg("Set word data perform_freerun {} for CLKGEN failed.".format(i))
                out = subprocess.check_output("i2cget -y {} {} {} w".format(bus, addr, lowdata), shell=True)
                if int(out, 16) != set_value:
                    msg("Get word data perform_freerun {} for compare failed.{}=/={}".format(i, int(out, 16), set_value))
            else:
                out = subprocess.check_output("i2cset -y {} {} 0x1 {}".format(bus, addr, hidata), shell=True)
                if len(out) != 0:
                    msg("Set perform_freerun {} for CLKGEN failed.".format(i))
                out = subprocess.check_output("i2cset -y {} {} {} {}".format(bus, addr, lowdata, set_value), shell=True)
                if len(out) != 0:
                    msg("Set perform_freerun {} for CLKGEN failed.".format(i))
                out = subprocess.check_output("i2cget -y {} {} {}".format(bus, addr, lowdata), shell=True)
                if int(out, 16) != set_value:
                    msg("Get perform_freerun {} for CLKGEN failed.{}=/={}".format(i, int(out, 16), set_value))

        #write_soft_rst
        for i in range(len(CLKGEN_CONFIG["FREE_RUN"]["write_soft_rst"])):
            hidata = CLKGEN_CONFIG["FREE_RUN"]["write_soft_rst"][i]["HiData"]
            lowdata = CLKGEN_CONFIG["FREE_RUN"]["write_soft_rst"][i]["LowData"]
            set_value = CLKGEN_CONFIG["FREE_RUN"]["write_soft_rst"][i]["value"]
            out = subprocess.check_output("i2cset -y {} {} 0x1 {}".format(bus, addr, hidata), shell=True)
            if len(out) != 0:
                msg("Set write_soft_rst {} for CLKGEN failed.".format(i))
            out = subprocess.check_output("i2cset -y {} {} {} {}".format(bus, addr, lowdata, set_value), shell=True)
            if len(out) != 0:
                msg("Set write_soft_rst {} for CLKGEN failed.".format(i))
            out = subprocess.check_output("i2cget -y {} {} {}".format(bus, addr, lowdata), shell=True)
            if int(out, 16) != set_value:
                msg("Get write_soft_rst {} for CLKGEN diff.{}=/={}".format(i, int(out, 16), set_value))

        #write_post_amble
        for i in range(len(CLKGEN_CONFIG["FREE_RUN"]["write_post_amble"])):
            hidata = CLKGEN_CONFIG["FREE_RUN"]["write_post_amble"][i]["HiData"]
            lowdata = CLKGEN_CONFIG["FREE_RUN"]["write_post_amble"][i]["LowData"]
            set_value = CLKGEN_CONFIG["FREE_RUN"]["write_post_amble"][i]["value"]
            out = subprocess.check_output("i2cset -y {} {} 0x1 {}".format(bus, addr, hidata), shell=True)
            if len(out) != 0:
                msg("Set write_post_amble {} for CLKGEN failed.".format(i))
            out = subprocess.check_output("i2cset -y {} {} {} {}".format(bus, addr, lowdata, set_value), shell=True)
            if len(out) != 0:
                msg("Set write_post_amble {} for CLKGEN failed.".format(i))
            out = subprocess.check_output("i2cget -y {} {} {}".format(bus, addr, lowdata), shell=True)
            if int(out, 16) != set_value:
                msg("Get write_post_amble {} for CLKGEN failed. {}=/={}.".format(i, int(out, 16), set_value))

        #close channel for CLKGEN
        os.system("i2cset -y 0 0x71 0x0")
