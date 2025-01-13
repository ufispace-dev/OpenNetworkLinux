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

class OnlPlatform_x86_64_ufispace_s6301_56stp_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s6301-56stp-r0'
    MODEL="S6301-56STP"
    SYS_OBJECT_ID=".6301.56"
    PORT_COUNT=56
    PORT_CONFIG="48x1 + 8x10"
    LEVEL_INFO=1
    LEVEL_ERR=2
    HW_REV_ALPHA=1
    EXT_1BASE=2

    bus_i801=0
    bus_ismt=1
    lpc_sysfs_path="/sys/devices/platform/x86_64_ufispace_s6301_56stp_lpc"
    gpio_sys_path = "/sys/class/gpio"

    def check_bmc_enable(self):
        return 0

    def get_hw_ext_id(self):
        # get hardware ext id
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s6301_56stp_lpc/mb_cpld/board_ext_id"

        output="0"
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("Get hwr ext id from LPC failed, exception={}, output={}, cmd={}\n".format(e, output, cmd), self.LEVEL_ERR)

        ext_id = int(output, 10)
        return ext_id

    def get_hw_rev_id(self):
        # get hardware revision
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s6301_56stp_lpc/mb_cpld/board_hw_id"

        output="0"
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("Get hwr rev id from LPC failed, exception={}, output={}, cmd={}\n".format(e, output, cmd), self.LEVEL_ERR)

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

    def get_gpio_max(self):
        cmd = "cat {}/bsp/bsp_gpio_max".format(self.lpc_sysfs_path)

        output="511"
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("Get gpio max failed, exception={}, output={}, cmd={}\n".format(e, output, cmd), self.LEVEL_ERR) 

        gpio_max = int(output, 10)

        return gpio_max

    def update_pci_device(self, driver, device, action):
        driver_path = os.path.join("/sys/bus/pci/drivers", driver, action)

        if os.path.exists(driver_path):
            with open(driver_path, "w") as file:
                file.write(device)


    def init_i2c_bus_order(self):
        device_actions = [
            #driver_name   bus_address     action
            ("i801_smbus", "0000:00:1f.4", "unbind"),
            ("ismt_smbus", "0000:00:12.0", "unbind"),
            ("i801_smbus", "0000:00:1f.4", "bind"),
            ("ismt_smbus", "0000:00:12.0", "bind")
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
        os.system("modprobe eeprom")

        #CPLD
        self.insmod("x86-64-ufispace-s6301-56stp-lpc")

        # check i2c bus status
        self.check_i2c_status()

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # get hw rev id and hw ext id
        self.hw_rev_id = self.get_hw_rev_id()
        self.hw_ext_id = self.get_hw_ext_id()

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
        self.init_gpio()

        # init HWM/Temp
        self.bsp_pr("Init HWMON");
        self.init_hwmon()

        # init poe
        self.bsp_pr("Init POE");
        os.system("modprobe x86-64-ufispace-s6301-56stp-poe")
        self.new_i2c_devices(
            [
                ('s6301_56stp_poe ', 0x20, 4),
            ]
        )
        os.system("echo 1 >  /sys/bus/i2c/devices/i2c-4/4-0020/sys/poe_init")

        # reset port led
        os.system("echo 0 > {}/mb_cpld/port_led_clear".format(self.lpc_sysfs_path))
        time.sleep(0.5)
        os.system("echo 1 > {}/mb_cpld/port_led_clear".format(self.lpc_sysfs_path))

        # set sys led to solid green
        os.system("echo 0x09 > {}/mb_cpld/led_sys".format(self.lpc_sysfs_path))

        self.bsp_pr("Init done");
        return True

    def init_gpio(self):

        # init GPIO sysfs
        if self.hw_rev_id > self.HW_REV_ALPHA:
            gpio_exp = [
                ('pca9535', 0x20, 6), #9535_MOD_ABS_0_7
                ('pca9535', 0x21, 7), #9535_RS_TXDIS_0_7
                ('pca9535', 0x22, 8), #9535_TXFLT_RXLOS_0_7
                ('pca9554', 0x21, 3), #9554_FAN_DIR
            ]

            gpio_map = {
                511:{'offset':  0  , 'dir': 'in'  , 'desc': "reserve"},
                510:{'offset': -1  , 'dir': 'in'  , 'desc': "reserve"},
                509:{'offset': -2  , 'dir': 'in'  , 'desc': "reserve"},
                508:{'offset': -3  , 'dir': 'in'  , 'desc': "reserve"},
                507:{'offset': -4  , 'dir': 'in'  , 'desc': "reserve"},
                506:{'offset': -5  , 'dir': 'in'  , 'desc': "reserve"},
                505:{'offset': -6  , 'dir': 'in'  , 'desc': "reserve"},
                504:{'offset': -7  , 'dir': 'in'  , 'desc': "reserve"},
                503:{'offset': -8  , 'dir': 'in'  , 'desc': "SFP0_ABS"},
                502:{'offset': -9  , 'dir': 'in'  , 'desc': "SFP1_ABS"},
                501:{'offset': -10 , 'dir': 'in'  , 'desc': "SFP2_ABS"},
                500:{'offset': -11 , 'dir': 'in'  , 'desc': "SFP3_ABS"},
                499:{'offset': -12 , 'dir': 'in'  , 'desc': "SFP4_ABS"},
                498:{'offset': -13 , 'dir': 'in'  , 'desc': "SFP5_ABS"},
                497:{'offset': -14 , 'dir': 'in'  , 'desc': "SFP6_ABS"},
                496:{'offset': -15 , 'dir': 'in'  , 'desc': "SFP7_ABS"},
                495:{'offset': -16 , 'dir': 'low' , 'desc': "SFP0_TXDIS"},
                494:{'offset': -17 , 'dir': 'low' , 'desc': "SFP1_TXDIS"},
                493:{'offset': -18 , 'dir': 'low' , 'desc': "SFP2_TXDIS"},
                492:{'offset': -19 , 'dir': 'low' , 'desc': "SFP3_TXDIS"},
                491:{'offset': -20 , 'dir': 'low' , 'desc': "SFP4_TXDIS"},
                490:{'offset': -21 , 'dir': 'low' , 'desc': "SFP5_TXDIS"},
                489:{'offset': -22 , 'dir': 'low' , 'desc': "SFP6_TXDIS"},
                488:{'offset': -23 , 'dir': 'low' , 'desc': "SFP7_TXDIS"},
                487:{'offset': -24 , 'dir': 'low' , 'desc': "SFP0_RS"},
                486:{'offset': -25 , 'dir': 'low' , 'desc': "SFP1_RS"},
                485:{'offset': -26 , 'dir': 'low' , 'desc': "SFP2_RS"},
                484:{'offset': -27 , 'dir': 'low' , 'desc': "SFP3_RS"},
                483:{'offset': -28 , 'dir': 'low' , 'desc': "SFP4_RS"},
                482:{'offset': -29 , 'dir': 'low' , 'desc': "SFP5_RS"},
                481:{'offset': -30 , 'dir': 'low' , 'desc': "SFP6_RS"},
                480:{'offset': -31 , 'dir': 'low' , 'desc': "SFP7_RS"},
                479:{'offset': -32 , 'dir': 'in'  , 'desc': "SFP0_RXLOS"},
                478:{'offset': -33 , 'dir': 'in'  , 'desc': "SFP1_RXLOS"},
                477:{'offset': -34 , 'dir': 'in'  , 'desc': "SFP2_RXLOS"},
                476:{'offset': -35 , 'dir': 'in'  , 'desc': "SFP3_RXLOS"},
                475:{'offset': -36 , 'dir': 'in'  , 'desc': "SFP4_RXLOS"},
                474:{'offset': -37 , 'dir': 'in'  , 'desc': "SFP5_RXLOS"},
                473:{'offset': -38 , 'dir': 'in'  , 'desc': "SFP6_RXLOS"},
                472:{'offset': -39 , 'dir': 'in'  , 'desc': "SFP7_RXLOS"},
                471:{'offset': -40 , 'dir': 'in'  , 'desc': "SFP0_TXFLT"},
                470:{'offset': -41 , 'dir': 'in'  , 'desc': "SFP1_TXFLT"},
                469:{'offset': -42 , 'dir': 'in'  , 'desc': "SFP2_TXFLT"},
                468:{'offset': -43 , 'dir': 'in'  , 'desc': "SFP3_TXFLT"},
                467:{'offset': -44 , 'dir': 'in'  , 'desc': "SFP4_TXFLT"},
                466:{'offset': -45 , 'dir': 'in'  , 'desc': "SFP5_TXFLT"},
                465:{'offset': -46 , 'dir': 'in'  , 'desc': "SFP6_TXFLT"},
                464:{'offset': -47 , 'dir': 'in'  , 'desc': "SFP7_TXFLT"},
                463:{'offset': -48 , 'dir': 'in'  , 'desc': "reserve"},
                462:{'offset': -49 , 'dir': 'in'  , 'desc': "reserve"},
                461:{'offset': -50 , 'dir': 'in'  , 'desc': "reserve"},
                460:{'offset': -51 , 'dir': 'in'  , 'desc': "FAN0_DIR"},
                459:{'offset': -52 , 'dir': 'in'  , 'desc': "FAN1_DIR"},
                458:{'offset': -53 , 'dir': 'in'  , 'desc': "reserve"},
                457:{'offset': -54 , 'dir': 'in'  , 'desc': "reserve"},
                456:{'offset': -55 , 'dir': 'in'  , 'desc': "reserve"},
            }

        else:
            gpio_exp = [
                ('pca9535', 0x20, 6), #9535_MOD_ABS_0_7
                ('pca9535', 0x21, 7), #9535_RS_TXDIS_0_7
                ('pca9535', 0x22, 8), #9535_TXFLT_RXLOS_0_7
            ]
            
            gpio_map = {
                511:{'offset':  0  , 'dir': 'in'  , 'desc': "reserve"},
                510:{'offset': -1  , 'dir': 'in'  , 'desc': "reserve"},
                509:{'offset': -2  , 'dir': 'in'  , 'desc': "reserve"},
                508:{'offset': -3  , 'dir': 'in'  , 'desc': "reserve"},
                507:{'offset': -4  , 'dir': 'in'  , 'desc': "reserve"},
                506:{'offset': -5  , 'dir': 'in'  , 'desc': "reserve"},
                505:{'offset': -6  , 'dir': 'in'  , 'desc': "reserve"},
                504:{'offset': -7  , 'dir': 'in'  , 'desc': "reserve"},
                503:{'offset': -8  , 'dir': 'in'  , 'desc': "SFP0_ABS"},
                502:{'offset': -9  , 'dir': 'in'  , 'desc': "SFP1_ABS"},
                501:{'offset': -10 , 'dir': 'in'  , 'desc': "SFP2_ABS"},
                500:{'offset': -11 , 'dir': 'in'  , 'desc': "SFP3_ABS"},
                499:{'offset': -12 , 'dir': 'in'  , 'desc': "SFP4_ABS"},
                498:{'offset': -13 , 'dir': 'in'  , 'desc': "SFP5_ABS"},
                497:{'offset': -14 , 'dir': 'in'  , 'desc': "SFP6_ABS"},
                496:{'offset': -15 , 'dir': 'in'  , 'desc': "SFP7_ABS"},
                495:{'offset': -16 , 'dir': 'low' , 'desc': "SFP0_TXDIS"},
                494:{'offset': -17 , 'dir': 'low' , 'desc': "SFP1_TXDIS"},
                493:{'offset': -18 , 'dir': 'low' , 'desc': "SFP2_TXDIS"},
                492:{'offset': -19 , 'dir': 'low' , 'desc': "SFP3_TXDIS"},
                491:{'offset': -20 , 'dir': 'low' , 'desc': "SFP4_TXDIS"},
                490:{'offset': -21 , 'dir': 'low' , 'desc': "SFP5_TXDIS"},
                489:{'offset': -22 , 'dir': 'low' , 'desc': "SFP6_TXDIS"},
                488:{'offset': -23 , 'dir': 'low' , 'desc': "SFP7_TXDIS"},
                487:{'offset': -24 , 'dir': 'low' , 'desc': "SFP0_RS"},
                486:{'offset': -25 , 'dir': 'low' , 'desc': "SFP1_RS"},
                485:{'offset': -26 , 'dir': 'low' , 'desc': "SFP2_RS"},
                484:{'offset': -27 , 'dir': 'low' , 'desc': "SFP3_RS"},
                483:{'offset': -28 , 'dir': 'low' , 'desc': "SFP4_RS"},
                482:{'offset': -29 , 'dir': 'low' , 'desc': "SFP5_RS"},
                481:{'offset': -30 , 'dir': 'low' , 'desc': "SFP6_RS"},
                480:{'offset': -31 , 'dir': 'low' , 'desc': "SFP7_RS"},
                479:{'offset': -32 , 'dir': 'in'  , 'desc': "SFP0_RXLOS"},
                478:{'offset': -33 , 'dir': 'in'  , 'desc': "SFP1_RXLOS"},
                477:{'offset': -34 , 'dir': 'in'  , 'desc': "SFP2_RXLOS"},
                476:{'offset': -35 , 'dir': 'in'  , 'desc': "SFP3_RXLOS"},
                475:{'offset': -36 , 'dir': 'in'  , 'desc': "SFP4_RXLOS"},
                474:{'offset': -37 , 'dir': 'in'  , 'desc': "SFP5_RXLOS"},
                473:{'offset': -38 , 'dir': 'in'  , 'desc': "SFP6_RXLOS"},
                472:{'offset': -39 , 'dir': 'in'  , 'desc': "SFP7_RXLOS"},
                471:{'offset': -40 , 'dir': 'in'  , 'desc': "SFP0_TXFLT"},
                470:{'offset': -41 , 'dir': 'in'  , 'desc': "SFP1_TXFLT"},
                469:{'offset': -42 , 'dir': 'in'  , 'desc': "SFP2_TXFLT"},
                468:{'offset': -43 , 'dir': 'in'  , 'desc': "SFP3_TXFLT"},
                467:{'offset': -44 , 'dir': 'in'  , 'desc': "SFP4_TXFLT"},
                466:{'offset': -45 , 'dir': 'in'  , 'desc': "SFP5_TXFLT"},
                465:{'offset': -46 , 'dir': 'in'  , 'desc': "SFP6_TXFLT"},
                464:{'offset': -47 , 'dir': 'in'  , 'desc': "SFP7_TXFLT"},
            }

        self.new_i2c_devices(gpio_exp)

        # get gpio max
        gpio_max = self.get_gpio_max()
        self.bsp_pr("GPIO MAX: {}".format(gpio_max))

        # export gpio
        for _, conf in gpio_map.items():
            gpio_num=gpio_max+conf['offset']
            gpio_dir=conf['dir']
            os.system("echo {} > {}/export".format(gpio_num, self.gpio_sys_path))
            os.system("echo {}   > {}/gpio{}/direction".format(gpio_dir, self.gpio_sys_path, gpio_num))

    def init_eeprom(self):

        # check ext_id for port index base
        if self.hw_ext_id == self.EXT_1BASE:
            port = 49
        else:
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

    def init_hwmon(self):

        os.system("modprobe ucd9000")
        os.system("modprobe lm75")

        # init hwmon/tmp
        if self.hw_rev_id > self.HW_REV_ALPHA:
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

