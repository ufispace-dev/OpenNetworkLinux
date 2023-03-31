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

class OnlPlatform_x86_64_ufispace_s9600_32x_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9600-32x-r0'
    MODEL="S9600-32X"
    SYS_OBJECT_ID=".9600.32"
    PORT_COUNT=32
    PORT_CONFIG="32x100"
    LEVEL_INFO=1
    LEVEL_ERR=2

    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self):
        sysfs_mux_reset = "/sys/devices/platform/x86_64_ufispace_s9600_32x_lpc/mb_cpld/mux_reset"

        # Check I2C status
        retcode = os.system("i2cget -f -y 0 0x71 > /dev/null 2>&1")
        if retcode != 0:

            # read mux failed, i2c bus may be stuck
            msg("Warning: Read I2C Mux Failed!! (ret=%d)\n" % (retcode))

            # Recovery I2C
            if os.path.exists(sysfs_mux_reset):
                with open(sysfs_mux_reset, "w") as f:
                    # write 0 to sysfs
                    f.write("{}".format(0))
                    msg("I2C bus recovery done.\n")
            else:
                msg("Warning: I2C recovery sysfs does not exist!! (path=%s)\n" % (sysfs_mux_reset))

    def bsp_pr(self, pr_msg, level = LEVEL_INFO):
        if level == self.LEVEL_INFO:
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9600_32x_lpc/bsp/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9600_32x_lpc/bsp/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9600_32x_lpc/bsp/bsp_pr_info"

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

    def baseconfig(self):

        # load default kernel driver
        os.system("modprobe i2c_i801")
        os.system("modprobe i2c_dev")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe lm75")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")

        #CPLD
        self.insmod("x86-64-ufispace-s9600-32x-lpc")

        # check i2c bus status
        self.check_i2c_status()

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        # Golden Finger to show CPLD
        os.system("i2cset -y 0 0x75 0x1")
        os.system("i2cget -y 0 0x30 0x2")
        os.system("i2cget -y 0 0x31 0x2")
        os.system("i2cset -y 0 0x75 0x0")

        ########### initialize I2C bus 0 ###########
        # init PCA9548
        self.bsp_pr("Init i2c MUXs");
        i2c_muxs = [
            ('pca9548', 0x75, 0),
            ('pca9548', 0x73, 0),
            ('pca9548', 0x72, 0),
            ('pca9546', 0x71, 17),
            ('pca9548', 0x76, 17),
            ('pca9548', 0x76, 18),
            ('pca9548', 0x76, 21),
            ('pca9548', 0x76, 22),
        ]

        self.new_i2c_devices(i2c_muxs)
        # init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

        self.insmod("x86-64-ufispace-eeprom-mb")
        self.insmod("optoe")

        # init SYS EEPROM devices
        self.bsp_pr("Init mb eeprom");
        self.new_i2c_devices(
            [
                #  on cpu board
                ('mb_eeprom', 0x57, 0),
            ]
        )

        # init SFP/QSFP EEPROM
        self.bsp_pr("Init port eeprom");
        self.init_eeprom()

        #CPLD
        self.insmod("x86-64-ufispace-s9600-32x-cpld")
        for i, addr in enumerate((0x30, 0x31, 0x32)):
            self.new_i2c_device("s9600_32x_cpld" + str(i+1), addr, 1)

        # Get board ID
        hw_build_rev = self.get_board_id()

        # init Temperature
        self.bsp_pr("Init Thermal");
        self.init_temperature(hw_build_rev)

        #config mac rov

        cpld_addr=30
        cpld_bus=1
        rov_addr=0x76
        rov_reg=0x21
        rov_bus=[10]
        mask=[0b00000111, 0b00111000]
        shift=[0, 3]

        # vid to mac vdd value mapping
        vdd_val_array=( 0.82,  0.82,  0.76,  0.78,  0.80,  0.84,  0.86,  0.88 )
        # vid to rov reg value mapping
        rov_reg_array=( 0x73, 0x73, 0x67, 0x6b, 0x6f, 0x77, 0x7b, 0x7f )

        for index, bus in enumerate(rov_bus):
            #get rov from cpld
            reg_val_str = subprocess.check_output("cat /sys/bus/i2c/devices/{}-00{}/cpld_mac_rov".format(cpld_bus, cpld_addr), shell=True)
            reg_val = int(reg_val_str, 16)
            vid = (reg_val & mask[index]) >> shift[index]
            mac_vdd_val = vdd_val_array[vid]
            rov_reg_val = rov_reg_array[vid]
            #set rov to mac
            msg("Setting mac vdd %1.2f with rov register value 0x%x\n" % (mac_vdd_val, rov_reg_val) )
            os.system("i2cset -y {} {} {} {}".format(rov_bus[index], rov_addr, rov_reg, rov_reg_val))

        #config LED mask
        cpld_led_addr =[31, 32]
        cpld_bus=1

        for index, addr in enumerate(cpld_led_addr):
            #set cpld led mask
            subprocess.call("echo 0 > /sys/bus/i2c/devices/{}-00{}/cpld_led_mask".format(cpld_bus, addr), shell=True)
            msg("Setting disable LED mask with i2c device {}-00{}\n".format(cpld_bus, addr))

        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        self.bsp_pr("Init done");
        return True

    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("Current IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("After IPMI_IOCTL IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

    def disable_bmc_watchdog(self):
        os.system("ipmitool mc watchdog off")

    def get_board_id(self):
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s9600_32x_lpc/mb_cpld/board_id_1"
        status, output = commands.getstatusoutput(
            "cat /sys/devices/platform/x86_64_ufispace_s9600_32x_lpc/mb_cpld/board_id_1")
        if status != 0:
            msg("Get board id from LPC failed, status=%s, output=%s, cmd=%s\n" % (status, output, cmd))
            return

        board_id = int(output, 10)
        hw_rev = (board_id & 0b00000011)
        deph_id = (board_id & 0b00000100) >> 2
        # build_rev = (board_id & 0b00111000) >> 3
        hw_build_id = (deph_id << 2) + hw_rev

        return hw_build_id

    def init_temperature(self, hw_build_rev):
        # init Temperature
        # Alpha
        if hw_build_rev == 1:
            self.new_i2c_devices(
                [
                    # CPU Board Temp
                    ('tmp75', 0x4F, 0),
                ]
            )
        # Beta and later
        elif hw_build_rev >= 2:
            pass

    def init_eeprom(self):
        port = 0

        # init QSFP EEPROM
        for bus in range(29, 61):
            self.new_i2c_device('sff8436', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1
        # init SFP EEPROM
        for bus in range(25, 29):
            self.new_i2c_device('optoe2', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1