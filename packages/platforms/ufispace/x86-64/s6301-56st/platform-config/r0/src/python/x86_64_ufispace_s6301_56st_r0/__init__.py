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

class OnlPlatform_x86_64_ufispace_s6301_56st_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s6301-56st-r0'
    MODEL="S6301-56ST"
    SYS_OBJECT_ID=".6301.56"
    PORT_COUNT=56
    PORT_CONFIG="48x1 + 8x10"
    LEVEL_INFO=1
    LEVEL_ERR=2
    HW_REV_ALPHA=1

    bus_i801=0
    bus_ismt=1
    lpc_sysfs_path="/sys/devices/platform/x86_64_ufispace_s6301_56st_lpc"

    def check_bmc_enable(self):
        return 0

    def get_hw_rev_id(self):
        # get hardware revision
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s6301_56st_lpc/mb_cpld/board_hw_id"
        status, output = commands.getstatusoutput(cmd)
        if status != 0:
            self.bsp_pr("Get hwr rev id from LPC failed, status={}, output={}, cmd={}\n".format(status, output, cmd), self.LEVEL_ERR);
            output="0"
        hw_rev_id = int(output, 10)
        return hw_rev_id

    def check_i2c_status(self):
        sysfs_mux_reset = "{}/mb_cpld/mux_reset".format(self.lpc_sysfs_path)

        # Check I2C status,assume i2c-ismt in bus 1
        # MUX#1 0x71 for SFP eeporm
        retcode = os.system("i2cget -f -y 0 0x71 > /dev/null 2>&1")
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
            sysfs_bsp_logging = "{}/bsp/bsp_pr_info".format(self.lpc_sysfs_path)
        elif level == self.LEVEL_ERR:
            sysfs_bsp_logging = "{}/bsp/bsp_pr_err".format(self.lpc_sysfs_path)
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            sysfs_bsp_logging = "{}/bsp/bsp_pr_info".format(self.lpc_sysfs_path)

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
        os.system("rmmod i2c_ismt")
        os.system("rmmod i2c_i801")
        os.system("modprobe i2c_i801")
        os.system("modprobe i2c_ismt")
        os.system("modprobe i2c_dev")
        os.system("modprobe gpio_pca953x")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe eeprom")

        #CPLD
        self.insmod("x86-64-ufispace-s6301-56st-lpc")

        # check i2c bus status
        self.check_i2c_status()

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # check i2c bus status
        hw_rev_id = self.get_hw_rev_id()

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        # init i2c muxs
        self.bsp_pr("Init i2c MUXs");

        i2c_muxs = [
            ('pca9546', 0x72, self.bus_ismt),    #9546_2
            ('pca9546', 0x70, self.bus_ismt),    #9546_0
            ('pca9548', 0x71, self.bus_i801),    #9548_1
        ]
        self.new_i2c_devices(i2c_muxs)
        # init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

        self.insmod("x86-64-ufispace-sys-eeprom")
        self.insmod("optoe")

        # init SYS EEPROM devices
        self.bsp_pr("Init sys eeprom");
        self.new_i2c_devices(
            [
                # syseeprom
                ('sys_eeprom', 0x56, self.bus_ismt),
            ]
        )

        # init SFP/SFP+ EEPROM
        self.bsp_pr("Init port eeprom");
        self.init_eeprom()

        # init GPIO sysfs
        self.bsp_pr("Init gpio");
        self.init_gpio(hw_rev_id)

        # init HWM/Temp
        self.bsp_pr("Init HWMON");
        self.init_hwmon(hw_rev_id)

        # reset port led
        os.system("echo 0 > {}/mb_cpld/port_led_clear".format(self.lpc_sysfs_path))
        time.sleep(0.5)
        os.system("echo 1 > {}/mb_cpld/port_led_clear".format(self.lpc_sysfs_path))

        self.bsp_pr("Init done");
        return True

    def init_gpio(self, hw_rev_id):

        # init GPIO sysfs
        if hw_rev_id > self.HW_REV_ALPHA:
            gpio_exp = [
                ('pca9535', 0x20, 6), #9535_MOD_ABS_0_7
                ('pca9535', 0x21, 7), #9535_RS_TXDIS_0_7
                ('pca9535', 0x22, 8), #9535_TXFLT_RXLOS_0_7
                ('pca9554', 0x21, 3), #9554_FAN_DIR
            ]
        else:
            gpio_exp = [
                ('pca9535', 0x20, 6), #9535_MOD_ABS_0_7
                ('pca9535', 0x21, 7), #9535_RS_TXDIS_0_7
                ('pca9535', 0x22, 8), #9535_TXFLT_RXLOS_0_7
            ]

        self.new_i2c_devices(gpio_exp)

        # get gpio max
        cmd = "cat {}/bsp/bsp_gpio_max".format(self.lpc_sysfs_path)
        status, output = commands.getstatusoutput(cmd)
        if status != 0:
            self.bsp_pr("Get gpio max failed, status={}, output={}, cmd={}\n".format(status, output, cmd), self.LEVEL_ERR);
            output="511"

        gpio_max = int(output, 10)
        gpios_num = 16
        self.bsp_pr("GPIO MAX: {}".format(gpio_max));

        # export gpio
        # MODE ABS Port 0-7 0x20
        index_end = gpio_max + 1
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)

        # RS TX_DIS Port 0-7 0x21
        index_end = index_start
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)
            os.system("echo out > /sys/class/gpio/gpio%d/direction" % i)
            os.system("echo 0 > /sys/class/gpio/gpio%d/value" % i)

        # TX_FLT RX_LOS Port 0-7 0x22
        index_end = index_start
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)

        if hw_rev_id > self.HW_REV_ALPHA:
            # FAN_DIR
            gpios_num = 8
            index_end = index_start
            index_start = index_end - gpios_num
            for i in range(index_start, index_end):
                os.system("echo %d > /sys/class/gpio/export" % i)

    def init_eeprom(self):
        port = 48

        # init SFP EEPROM
        bus_start = 10
        bus_end = bus_start + 8
        for bus in range(bus_start, bus_end):
            self.new_i2c_device('optoe2', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1

        # init PSU(0/1) EEPROM devices
        self.new_i2c_device('eeprom', 0x50, self.bus_ismt+1)
        self.new_i2c_device('eeprom', 0x51, self.bus_ismt+1)

    def init_hwmon(self, hw_rev_id):

        os.system("modprobe ucd9000")
        os.system("modprobe lm75")

        # init hwmon/tmp
        if hw_rev_id > self.HW_REV_ALPHA:
            hwms = [
                ('tmp75', 0x49, 3),    # TMP75 Thermal Sensor
                ('tmp75', 0x4A, 3),    # TMP75 Thermal Sensor
                ('ucd90124', 0x34, 5), # UCD90124 HWMON
            ]
        else:
            hwms = [
                ('tmp75', 0x49, 3),    # TMP75 Thermal Sensor
                ('tmp75', 0x4A, 3),    # TMP75 Thermal Sensor
                ('ucd90124', 0x34, 1), # UCD90124 HWMON
            ]


        self.new_i2c_devices(hwms)

