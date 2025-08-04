from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
from ctypes import c_int, sizeof
import os
import sys
import subprocess
import commands
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

class OnlPlatform_x86_64_ufispace_s9501_18smt_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9501-18smt-r0'
    MODEL="S9501-18SMT"
    SYS_OBJECT_ID=".9501.18"
    PORT_COUNT=18
    PORT_CONFIG="12x1 + 6x10"
    LEVEL_INFO=1
    LEVEL_ERR=2

    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self):
        sysfs_mux_reset = "/sys/devices/platform/x86_64_ufispace_s9501_18smt_lpc/mb_cpld/mux_reset"
        controller_remove = "/sys/bus/pci/devices/0000:00:12.0/remove"
        controller_rescan = "/sys/bus/pci/rescan"

        # Check I2C status,assume i2c-ismt in bus 1
        retcode = os.system("i2cget -f -y 1 0x75 > /dev/null 2>&1")
        if retcode != 0:

            #read mux failed, i2c bus may be stuck
            msg("Warning: Read I2C Mux Failed!! (ret=%d)\n" % (retcode) )

            #Recovery I2C
            if os.path.exists(sysfs_mux_reset) and os.path.exists(controller_remove)\
                    and os.path.exists(controller_rescan):
                with open(sysfs_mux_reset, "w") as f:
                    #write 0 to sysfs
                    f.write("{}".format(0))

                with open(controller_remove, "w") as f:
                    #write 1 to sysfs
                    f.write("{}".format(1))

                with open(controller_rescan, "w") as f:
                    #write 1 to sysfs
                    f.write("{}".format(1))

                os.system("rmmod i2c-ismt")
                os.system("modprobe i2c-ismt")
                msg("I2C bus recovery done.\n")
            else:
                msg("Warning: I2C recovery sysfs does not exist!! (path=%s)\n" % (sysfs_mux_reset) )

    def bsp_pr(self, pr_msg, level = LEVEL_INFO):
        if level == self.LEVEL_INFO:
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9501_18smt_lpc/bsp/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9501_18smt_lpc/bsp/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9501_18smt_lpc/bsp/bsp_pr_info"

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

        #CPLD
        self.insmod("x86-64-ufispace-s9501-18smt-lpc")

        # check i2c bus status
        self.check_i2c_status()

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        ########### initialize I2C bus 0 ###########
        # i2c_i801 is built-in

        # add i2c_ismt
        #self.insmod("i2c-ismt") #module not found
        os.system("modprobe i2c-ismt")


        # init PCA9548
        self.bsp_pr("Init i2c MUXs");
        bus_i801=0
        bus_ismt=1
        i2c_muxs = [
            ('pca9546', 0x75, bus_ismt),   #9546_ROOT_TIMING
            ('pca9546', 0x76, bus_ismt),   #9546_ROOT_SFP
            ('pca9548', 0x72, bus_ismt+8), #9548_CHILD_SFP_12_19
            ('pca9548', 0x73, bus_ismt+8), #9548_CHILD_SFP_20_27
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
                ('mb_eeprom', 0x57, bus_ismt),
            ]
        )

        # init SFP/SFP+ EEPROM
        self.bsp_pr("Init port eeprom");
        self.init_eeprom(bus_ismt)

        # init Temperature
        #self.insmod("jc42") #module not found
        os.system("modprobe jc42")

        # init GPIO sysfs
        self.bsp_pr("Init gpio");
        self.init_gpio()

        #enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

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

    def init_gpio(self):

        # init GPIO sysfs
        self.new_i2c_devices(
            [
                ('pca9535', 0x20, 4), #9535_BOARD_ID
                ('pca9535', 0x22, 6), #9535_TX_DIS_0_15
                ('pca9535', 0x24, 6), #9535_TX_DIS_16_31
                ('pca9535', 0x26, 7), #9535_TX_FLT_0_15
                ('pca9535', 0x27, 7), #9535_TX_FLT_16_31
                ('pca9535', 0x25, 7), #9535_RATE_SELECT_0_15
                ('pca9535', 0x23, 7), #9535_RATE_SELECT_16_31
                ('pca9535', 0x20, 8), #9535_MOD_ABS_0_15
                ('pca9535', 0x22, 8), #9535_MOD_ABS_16_31
                ('pca9535', 0x21, 8), #9535_RX_LOS_0_15
                ('pca9535', 0x24, 8)  #9535_RX_LOS_0_15
            ]
        )

        # init all GPIO direction to "in"
        gpio_dir = ["in"] * 512

        # init GPIO direction to output low
        for i in range(491, 487, -1) + \
                 range(479, 477, -1) + \
                 range(471, 463, -1) + \
                 range(427, 423, -1) + \
                 range(407, 403, -1):
            gpio_dir[i] = "low"

        # init GPIO direction to output high
        for i in range(415, 413, -1) + \
                 range(403, 399, -1):
            gpio_dir[i] = "high"
        msg("Beta 1 and later GPIO init\n")

        # export GPIO and configure direction
        for i in range(336, 512):
            os.system("echo {} > /sys/class/gpio/export".format(i))
            os.system("echo {} > /sys/class/gpio/gpio{}/direction".format(gpio_dir[i], i))

    def init_eeprom(self, bus_ismt):
        port = 4

        # init SFP EEPROM
        for bus in range(bus_ismt + 9, bus_ismt + 23):
            self.new_i2c_device('optoe2', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1