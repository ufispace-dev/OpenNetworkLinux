from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
from ctypes import c_int, sizeof
import os
import sys
import subprocess
import fcntl

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

class OnlPlatform_x86_64_ufispace_s9700_23d_r4(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9700-23d-r4'
    MODEL="S9700-23D"
    SYS_OBJECT_ID=".9700.23"
    PORT_COUNT=23
    PORT_CONFIG="10x400 + 13x400"
    port_type_dict = {
        0x03: [2, 'SFP/SFP+/SFP28'],  # [dev_class, type_str]
        0x0B: [2, 'DWDM-SFP/SFP+'],
        0x0C: [1, 'QSFP'],
        0x0D: [1, 'QSFP+'],
        0x11: [1, 'QSFP28'],
        0x18: [3, 'QSFP-DD Double Density 8x (INF-8628)'],
        0x19: [3, 'OSFP 8x Pluggable Transceiver'],
        0x1E: [3, 'QSFP+ or later with CMIS spec'],
        0x1F: [3, 'SFP-DD Double Density 2X Pluggable Transceiver with CMIS spec'],
    }

    sysfs_port_present = {
        0:  {"sysfs": "/sys/bus/i2c/devices/1-0030/cpld_qsfpdd_nif_port_status_0",  "bit": 1, "bus": 25,},
        1:  {"sysfs": "/sys/bus/i2c/devices/1-0030/cpld_qsfpdd_nif_port_status_1",  "bit": 1, "bus": 26,},
        2:  {"sysfs": "/sys/bus/i2c/devices/1-0030/cpld_qsfpdd_nif_port_status_2",  "bit": 1, "bus": 27,},
        3:  {"sysfs": "/sys/bus/i2c/devices/1-0030/cpld_qsfpdd_nif_port_status_3",  "bit": 1, "bus": 28,},
        4:  {"sysfs": "/sys/bus/i2c/devices/1-0030/cpld_qsfpdd_nif_port_status_4",  "bit": 1, "bus": 29,},
        5:  {"sysfs": "/sys/bus/i2c/devices/1-0030/cpld_qsfpdd_nif_port_status_5",  "bit": 1, "bus": 30,},
        6:  {"sysfs": "/sys/bus/i2c/devices/1-0030/cpld_qsfpdd_nif_port_status_6",  "bit": 1, "bus": 31,},
        7:  {"sysfs": "/sys/bus/i2c/devices/1-0030/cpld_qsfpdd_nif_port_status_7",  "bit": 1, "bus": 32,},
        8:  {"sysfs": "/sys/bus/i2c/devices/1-0030/cpld_qsfpdd_nif_port_status_8",  "bit": 1, "bus": 33,},
        9:  {"sysfs": "/sys/bus/i2c/devices/1-0030/cpld_qsfpdd_nif_port_status_9",  "bit": 1, "bus": 34,},
        10: {"sysfs": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_fab_port_status_0",  "bit": 1, "bus": 41,},
        11: {"sysfs": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_fab_port_status_1",  "bit": 1, "bus": 42,},
        12: {"sysfs": "/sys/bus/i2c/devices/1-0031/cpld_qsfpdd_fab_port_status_2",  "bit": 1, "bus": 43,},
        13: {"sysfs": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_fab_port_status_0",  "bit": 1, "bus": 44,},
        14: {"sysfs": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_fab_port_status_1",  "bit": 1, "bus": 45,},
        15: {"sysfs": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_fab_port_status_2",  "bit": 1, "bus": 46,},
        16: {"sysfs": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_fab_port_status_3",  "bit": 1, "bus": 47,},
        17: {"sysfs": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_fab_port_status_4",  "bit": 1, "bus": 48,},
        18: {"sysfs": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_fab_port_status_5",  "bit": 1, "bus": 49,},
        19: {"sysfs": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_fab_port_status_6",  "bit": 1, "bus": 50,},
        20: {"sysfs": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_fab_port_status_7",  "bit": 1, "bus": 51,},
        21: {"sysfs": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_fab_port_status_8",  "bit": 1, "bus": 52,},
        22: {"sysfs": "/sys/bus/i2c/devices/1-0032/cpld_qsfpdd_fab_port_status_9",  "bit": 1, "bus": 53,},
        23: {"sysfs": "/sys/bus/i2c/devices/1-0030/cpld_sfp_port_status",           "bit": 0, "bus": -1,},
        24: {"sysfs": "/sys/bus/i2c/devices/1-0030/cpld_sfp_port_status",           "bit": 4, "bus": -1,},
    }

    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self):
        sysfs_mux_reset = "/sys/devices/platform/x86_64_ufispace_s9700_23d_lpc/cpu_cpld/mux_reset"

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

        # init QSFPDD NIF EEPROM
        for bus in range(25, 35):
            self.new_i2c_device('optoe3', 0x50, bus)
            # update port_name
            self._write("/sys/bus/i2c/devices/{}-0050/port_name".format(bus), port)
            port = port + 1

        # init QSFPDD FAB EEPROM
        for bus in range(41, 54):
            self.new_i2c_device('optoe3', 0x50, bus)
            # update port_name
            self._write("/sys/bus/i2c/devices/{}-0050/port_name".format(bus), port)
            port = port + 1

    def ufi_port_present_get(self, port):
        present_raw = ""

        if port not in self.sysfs_port_present:
            msg("Port {} is not valid.\n".format(port), self.LEVEL_ERR)
            return False

        sysfs = self.sysfs_port_present[port]["sysfs"]
        bit = self.sysfs_port_present[port]["bit"]

        with open(sysfs, "r") as f:
            present_raw = f.read().strip()

        reg_val = int(present_raw, 16)
        pres_status = True if ((reg_val >> bit) & 0x1) == 0 else False

        return pres_status

    def update_dev_class(self):
        for port in range(0, 23):  # QSFPX ports
            bus = self.sysfs_port_present[port]["bus"]

            # check module presence
            if not self.ufi_port_present_get(port):
                continue

            # get dev_class
            sysfs = "/sys/bus/i2c/devices/{}-0050/dev_class".format(bus)
            cmd = ["cat", sysfs]
            dev_class_str = subprocess.check_output(cmd)
            dev_class = int(dev_class_str, 10)

            # get port type
            cmd = ["dd", "if=/sys/bus/i2c/devices/{}-0050/eeprom".format(bus), "bs=1", "count=1", "skip=0", "status=none"]
            output = subprocess.check_output(cmd)
            hex_str = unpack('B', output)[0]
            type_str = "{:02x}".format(hex_str)
            if type_str == "": #i2c maybe stuck
                self.check_i2c_status()
                continue
            port_type = int(type_str, 16)


            # check if port_type is in port_type_dict
            if port_type not in self.port_type_dict:
                msg("Port[{}] Type: {} is Unknown.\n".format(port, hex(port_type)))
                continue

            # check if dev_class matches port_type_dev_class
            port_type_dev_class = self.port_type_dict.get(port_type)[0]
            if dev_class != port_type_dev_class:
                with open(sysfs, "w") as f:
                    f.write("{}".format(port_type_dev_class))
                msg("Port[{}] dev_class is changed from {} to {}\n".format(port, dev_class, port_type_dev_class))

        msg("Please run ONLP API onlp_sfpi_dev_class_update() after inserting QSFP/QSFPDD modules at runtime\n")

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
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s9700_23d_lpc/bsp/bsp_gpio_max"
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
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s9700_23d_lpc/bsp/bsp_gpio_base"
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
        #9539_HOST_GPIO_I2C
        self.new_i2c_device('pca9539', 0x74, 0)
        #9555_BEACON_LED
        self.new_i2c_device('pca9555', 0x20, 7)
        #9555_BOARD_ID
        self.new_i2c_device('pca9555', 0x20, 3)
        #9539_CPU_I2C
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
            for i in range(base, base+64):
                os.system("echo {} > /sys/class/gpio/export".format(i))

            # init GPIO direction
            # 9539_HOST_GPIO_I2C 0x74
            os.system("echo high > /sys/class/gpio/gpio" + str(base)    + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+1)  + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base+2)  + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+3)  + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+4)  + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+5)  + "/direction")
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
            # 9555_BEACON_LED 0x20
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+16) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+17) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+18) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+19) + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base+20) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+21) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+22) + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+23) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+24) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+25) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+26) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+27) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+28) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base+29) + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base+30) + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base+31) + "/direction")

            # init GPIO direction
            # 9555_BOARD_ID 0x20, 9539_CPU_I2C 0x77
            for i in range(base+32, base+64):
                os.system("echo in > /sys/class/gpio/gpio{}/direction".format(i))
        else:
            # export GPIO
            for i in range(base-63, base+1):
                os.system("echo {} > /sys/class/gpio/export".format(i))

            # init GPIO direction
            # 9539_HOST_GPIO_I2C 0x74
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
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-10) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-11) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-12) + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base-13) + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-14) + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base-15) + "/direction")

            # init GPIO direction
            # 9555_BEACON_LED 0x20
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-16) + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base-17) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-18) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-19) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-20) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-21) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-22) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-23) + "/direction")
            os.system("echo in   > /sys/class/gpio/gpio" + str(base-24) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-25) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-26) + "/direction")
            os.system("echo high > /sys/class/gpio/gpio" + str(base-27) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-28) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-29) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-30) + "/direction")
            os.system("echo low  > /sys/class/gpio/gpio" + str(base-31) + "/direction")

            # init GPIO direction
            # 9555_BOARD_ID 0x20, 9539_CPU_I2C 0x77
            for i in range(base-63, base-31):
                os.system("echo in > /sys/class/gpio/gpio{}/direction".format(i))

    def _write(self, path, val, perm="w"):
        if os.path.exists(path):
            try:
                with open(path, perm) as f:
                    f.write(str(val))
            except Exception as e:
                msg("Open file failed, exception={}".format(e))
        else:
            msg("File not found: {}".format(path))

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
        self.insmod("x86-64-ufispace-s9700-23d-lpc")

        # check i2c bus status
        self.check_i2c_status()

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        ########### initialize I2C bus 0 ###########
        # init PCA9548

        i2c_muxs = [
            ('pca9548', 0x75, 0),
            ('pca9548', 0x72, 0),
            ('pca9548', 0x73, 0),
            ('pca9548', 0x76, 9),
            ('pca9548', 0x76, 10),
            ('pca9548', 0x76, 15),
            ('pca9548', 0x76, 16),
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
        self.insmod("x86-64-ufispace-s9700-23d-cpld")
        for i, addr in enumerate((0x30, 0x31, 0x32)):
            self.new_i2c_device("s9700_23d_cpld" + str(i+1), addr, 1)

        #config mac rov

        cpld_addr=[30]
        cpld_bus=1
        rov_addr=0x76
        rov_reg=0x21
        rov_bus=[4]

        # vid to mac vdd value mapping
        vdd_val_array=( 0.82,  0.82,  0.76,  0.78,  0.80,  0.84,  0.86,  0.88 )
        # vid to rov reg value mapping
        rov_reg_array=( 0x73, 0x73, 0x67, 0x6b, 0x6f, 0x77, 0x7b, 0x7f )

        for index, cpld in enumerate(cpld_addr):
            #get rov from cpld
            cmd = "cat /sys/bus/i2c/devices/{}-00{}/cpld_psu_status_0".format(cpld_bus, cpld)
            reg_val_str = subprocess.check_output(cmd.split())
            reg_val = int(reg_val_str, 16)
            vid = (reg_val & 0xe) >> 1
            mac_vdd_val = vdd_val_array[vid]
            rov_reg_val = rov_reg_array[vid]
            #set rov to mac
            msg("Setting mac vdd %1.2f with rov register value 0x%x\n" % (mac_vdd_val, rov_reg_val) )
            os.system("i2cset -y {} {} {} {} w".format(rov_bus[index], rov_addr, rov_reg, rov_reg_val))

        # enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        #disable watchdog
        self.disable_bmc_watchdog()

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        # init dev_class for CMIS/non-CMIS modules
        self.update_dev_class()

        msg("Init done\n")

        return True
