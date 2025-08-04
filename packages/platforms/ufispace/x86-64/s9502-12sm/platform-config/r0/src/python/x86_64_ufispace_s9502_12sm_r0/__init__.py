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

class OnlPlatform_x86_64_ufispace_s9502_12sm_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9502-12sm-r0'
    MODEL="S9502-12SM"
    SYS_OBJECT_ID=".9502.12"
    PORT_COUNT=12
    PORT_CONFIG="8x1 + 4x10"
    LEVEL_INFO=1
    LEVEL_ERR=2

    def check_bmc_enable(self):
        return 0
    def check_i2c_status(self):
        sysfs_mux_reset = "/sys/devices/platform/x86_64_ufispace_s9502_12sm_lpc/mb_cpld/mux_reset"

        # Check I2C status,assume i2c-ismt in bus 1
        retcode = os.system("i2cget -f -y 0 0x76 > /dev/null 2>&1")
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
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9502_12sm_lpc/bsp/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9502_12sm_lpc/bsp/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            sysfs_bsp_logging = "/sys/devices/platform/x86_64_ufispace_s9502_12sm_lpc/bsp/bsp_pr_info"

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
        os.system("modprobe lm90")
        os.system("modprobe ucd9000")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")

        #CPLD
        self.insmod("x86-64-ufispace-s9502-12sm-lpc")

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

        self.init_mux_ctrl()

        # init PCA9548
        self.bsp_pr("Init i2c MUXs");
        bus_i801=0
        bus_ismt=1
        last_bus=bus_ismt
        i2c_muxs = [
            ('pca9546', 0x75, bus_ismt),    #9546_ROOT_TIMING
            ('pca9546', 0x77, bus_i801),    #9546_ROOT_HWMON
            ('pca9546', 0x76, bus_i801),    #9546_ROOT_SFP
            ('pca9548', 0x72, last_bus+12), #9548_CHILD_SFP_12_19
            ('pca9548', 0x73, last_bus+12), #9548_CHILD_SFP_20_27
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

        # init GPIO sysfs
        self.bsp_pr("Init gpio");
        self.init_gpio()

        # init Temperature
        self.bsp_pr("Init HWMON");
        self.init_hwmon()

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        self.bsp_pr("Init done");
        return True

    def init_mux_ctrl(self):
        mux_ctrl_path="/sys/devices/platform/x86_64_ufispace_s9502_12sm_lpc/mb_cpld/mux_ctrl"
        default = 0
        status, output = commands.getstatusoutput(
            "cat {}".format(mux_ctrl_path))
        if status != 0:
            msg("Get mux ctrl from LPC failed, status=%s, output=%s, mux_ctrl_path=%s\n" % (status, output, mux_ctrl_path))
            return
        mux_ctrl = int(output, 10)
        if mux_ctrl != default:
            msg("Invalid Mux ctrl value(%d), set to default %d\n" % (mux_ctrl, default))
            subprocess.call("echo {} > {}".format(default, mux_ctrl_path), shell=True)



    def init_gpio(self):

        # init GPIO sysfs
        self.new_i2c_devices(
            [
                ('pca9535', 0x20, 4), #9535_BOARD_ID
                ('pca9535', 0x22, 10), #9535_TX_DIS_0_15
                ('pca9535', 0x24, 10), #9535_TX_DIS_16_31
                ('pca9535', 0x26, 11), #9535_TX_FLT_0_15
                ('pca9535', 0x27, 11), #9535_TX_FLT_16_31
                ('pca9535', 0x25, 11), #9535_RATE_SELECT_0_15
                ('pca9535', 0x23, 11), #9535_RATE_SELECT_16_31
                ('pca9535', 0x20, 12), #9535_MOD_ABS_0_15
                ('pca9535', 0x22, 12), #9535_MOD_ABS_16_31
                ('pca9535', 0x21, 12), #9535_RX_LOS_0_15
                ('pca9535', 0x24, 12)  #9535_RX_LOS_0_15
            ]
        )

        # init all GPIO direction to "in"
        gpio_dir = ["in"] * 512

        # get board id
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s9502_12sm_lpc/mb_cpld/board_id_0"
        status, output = commands.getstatusoutput("cat /sys/devices/platform/x86_64_ufispace_s9502_12sm_lpc/mb_cpld/board_id_0")
        if status != 0:
            msg("Get board id from LPC failed, status=%s, output=%s, cmd=%s\n" % (status, output, cmd))
            return

        board_id = int(output, 10)
        model_id = board_id & 0b00001111
        hw_rev = (board_id & 0b00110000) >> 4
        build_rev = (board_id & 0b11000000) >> 6
        hw_build_rev = (hw_rev << 2) | (build_rev)

        # init GPIO direction to output low
        for i in range(491, 487, -1) + \
                 range(471, 463, -1) + \
                 range(427, 423, -1) + \
                 range(407, 403, -1):
            gpio_dir[i] = "low"

        # init GPIO direction to output high
        for i in range(403, 399, -1):
            gpio_dir[i] = "high"
        msg("Alpha 1 and later GPIO init\n")

        # export GPIO and configure direction
        for i in range(336, 512):
            os.system("echo {} > /sys/class/gpio/export".format(i))
            os.system("echo {} > /sys/class/gpio/gpio{}/direction".format(gpio_dir[i], i))

    def init_eeprom(self, bus_ismt):
        port = 0

        # init SFP EEPROM
        for bus in range(bus_ismt + 13, bus_ismt + 25):
            self.new_i2c_device('optoe2', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1

    def init_hwmon(self):

        os.system("modprobe ucd9000")
        os.system("modprobe lm90")

        # init hwmon
        self.new_i2c_devices(
            [
                ('ucd90124', 0x41, 9), # UCD90124 HWMON
                ('tmp451', 0x4e, 9)    # TMP451 Thermal Sensor
            ]
        )

        #init dram thermal sensors
        os.system("modprobe jc42")
        self.new_i2c_devices(
            [
                ('jc42', 0x1a, 0),  # DRAM 1
                ('jc42', 0x1b, 0),  # DRAM 2
            ]
        )