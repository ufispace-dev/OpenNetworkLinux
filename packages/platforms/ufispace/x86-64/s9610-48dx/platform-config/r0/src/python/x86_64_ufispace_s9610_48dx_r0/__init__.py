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

class OnlPlatform_x86_64_ufispace_s9610_48dx_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9610-48dx-r0'
    MODEL="S9610-48DX"
    SYS_OBJECT_ID=".9610.48"
    PORT_COUNT=48
    PORT_CONFIG="40x100 + 8x400"
    LEVEL_INFO=1
    LEVEL_ERR=2
    SYSFS_LPC="/sys/devices/platform/x86_64_ufispace_s9610_48dx_lpc"
    FS_PLTM_CFG="/lib/platform-config/current/onl"
    SYS_FMT_OFFSET = "/sys/bus/i2c/devices/{}-{:04x}/{}_{}"
    SYS_FMT = "/sys/bus/i2c/devices/{}-{:04x}/{}"
    SYSFS_QSFP_PRESENT = "cpld_qsfp_intr_present"
    SYSFS_QSFPDD_PRESENT = "cpld_qsfpdd_intr_present"
    SYSFS_SFPDD_PRESENT = "cpld_sfpdd_intr_present"
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

    def check_i2c_status(self):
        sysfs_mux_reset = self.SYSFS_LPC + "/mb_cpld/mux_reset"

        # Check I2C status
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

    def ufi_port_to_cpld_addr(self, port):
        CPLD_BASE_ADDR = [0x30, 0x31, 0x32, 0x33]

        if port in range(0, 10):
            return CPLD_BASE_ADDR[1]
        elif port in range(10, 20):
            return CPLD_BASE_ADDR[2]
        elif port in range(20, 30):
            return CPLD_BASE_ADDR[1]
        elif port in range(30, 40):
            return CPLD_BASE_ADDR[2]
        elif port in range(40, 52):
            return CPLD_BASE_ADDR[3]

    def ufi_qsfp_port_to_sysfs_attr_offset(self, port):
        if port in range(0, 4):
            return 0
        elif port in range(4, 8):
            return 1
        elif port in range(8, 10):
            return 2
        elif port in range(10, 14):
            return 0
        elif port in range(14, 18):
            return 1
        elif port in range(18, 20):
            return 2
        elif port in range(20, 24):
            return 0
        elif port in range(24, 28):
            return 1
        elif port in range(28, 30):
            return 2
        elif port in range(30, 34):
            return 0
        elif port in range(34, 38):
            return 1
        elif port in range(38, 40):
            return 2

    def ufi_port_to_bit_offset(self, port):
        if 0 <= port < 10:
            return port % 4
        elif 10 <= port < 14:
            return port % 5
        elif 14 <= port < 18:
            return port % 14
        elif 18 <= port < 20:
            return port % 18
        elif 20 <= port < 24:
            return port % 8
        elif 24 <= port < 28:
            return port % 20
        elif 28 <= port < 30:
            return port % 8
        elif 30 <= port < 34:
            return port % 26
        elif 34 <= port < 38:
            return port % 30
        elif 38 <= port < 40:
            return port % 34
        elif 40 <= port < 48:  # QSFPDD
            return port % 40
        elif 48 <= port < 52:  # SFPDD
            return port % 48
        elif 52 <= port < 56:  # SFP
            return port % 48
        else:
            return -1  # Default value for an invalid port

    def ufi_port_present_get(self, port):
        cpld_bus = 1
        cpld_addr = self.ufi_port_to_cpld_addr(port)
        if 0 <= port < 40:
            attr_offset = self.ufi_qsfp_port_to_sysfs_attr_offset(port)
            sysfs_present = self.SYS_FMT_OFFSET.format(cpld_bus, cpld_addr, self.SYSFS_QSFP_PRESENT, attr_offset)
        elif 40 <= port < 48:  # QSFPDD
            sysfs_present = self.SYS_FMT.format(cpld_bus, cpld_addr, self.SYSFS_QSFPDD_PRESENT)
        elif 48 <= port < 52:  # SFPDD
            sysfs_present = self.SYS_FMT.format(cpld_bus, cpld_addr, self.SYSFS_SFPDD_PRESENT)
        with open(sysfs_present, "r") as f:
            present_raw = f.read().strip()

        reg_val = int(present_raw, 16)

        pres_val = not ((reg_val >> self.ufi_port_to_bit_offset(port)) & 0x1)
        return pres_val

    def init_dev_class(self):
        # init dev_class
        for bus in range(25, 65):  # QSFP
            # get dev_class
            port = bus - 25
            dev_class_sysfs_path = "/sys/bus/i2c/devices/{}-0050/dev_class".format(bus)
            dev_class_str = subprocess.check_output("cat {}".format(dev_class_sysfs_path), shell=True)
            dev_class = int(dev_class_str, 10)
            # get module type
            if self.ufi_port_present_get(port) == 1:
                type_str = subprocess.check_output(
                    "dd if=/sys/bus/i2c/devices/{}-0050/eeprom bs=1 count=1 skip=0 status=none | hexdump -n 1 -e '1/1 \"%02x\"'"
                    .format(bus), shell=True)
                if type_str == "": #i2c maybe stuck
                    self.check_i2c_status()
                    continue
                type = int(type_str, 16)
            else:
                continue
            # compare dev_class and type
            if type not in self.port_type_dict:
                self.bsp_pr("Port[{}] Type: {} is Unknown.".format(str(port), hex(type)))
                continue
            port_types = self.port_type_dict.get(type)
            if dev_class != port_types[0]:
                with open(dev_class_sysfs_path, "w") as f:
                    f.write("{}".format(port_types[0]))
                self.bsp_pr("Port(" + str(port) + ") dev_class=" + dev_class_str +" change to " + str(port_types[0]) +".")
    def init_eeprom(self):
        port = 0
        data = None

        with open(self.FS_PLTM_CFG + "/port_config.yml", 'r') as yaml_file:
            data = yaml.safe_load(yaml_file)

        # init QSFP NIF EEPROM
        for bus in range(25, 65):
            self.new_i2c_device('optoe1', 0x50, bus)
            # update port_name
            if data is not None:
                port = bus - 25
                port_name = data["QSFP"][port]["port_name"]
                subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port_name, bus), shell=True)

        # init QSFPDD NIF EEPROM
        for bus in range(65, 73):
            self.new_i2c_device('optoe3', 0x50, bus)
            # update port_name
            if data is not None:
                port = bus - 25
                port_name = data["QSFPDD"][port]["port_name"]
                subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port_name, bus), shell=True)

        # init SFPDD NIF EEPROM
        for bus in range(73, 77):
            self.new_i2c_device('optoe2', 0x50, bus)
            # update port_name
            if data is not None:
                port = bus - 25
                port_name = data["SFPDD"][port]["port_name"]
                subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port_name, bus), shell=True)

        # init SFP+ EEPROM
        for bus in range(77, 81):
            self.new_i2c_device('optoe2', 0x50, bus)
            # update port_name
            if data is not None:
                port = bus - 25
                port_name = data["SFP"][port]["port_name"]
                subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port_name, bus), shell=True)

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

    def baseconfig(self):

        # load default kernel driver
        os.system("modprobe i2c_i801")
        os.system("modprobe i2c_dev")
        os.system("modprobe gpio_pca953x")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe lm75")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")

        # lpc driver
        self.insmod("x86-64-ufispace-s9610-48dx-lpc")

        # check i2c bus status
        self.check_i2c_status()

        # Golden Finger to show CPLD #TBD
        #os.system("i2cset -y 0 0x71 0x1")
        #os.system("i2cget -y 0 0x30 0x2")
        #os.system("i2cget -y 0 0x31 0x2")
        #os.system("i2cget -y 0 0x32 0x2")
        #os.system("i2cset -y 0 0x71 0x0")

        ########### initialize I2C bus 0 ###########
        # init PCA9548
        self.bsp_pr("Init PCA9548")
        i2c_muxs = [
            ('pca9548', 0x71, 0),  # 9548_CPLD
            ('pca9548', 0x72, 0),  # 9548_PORT_ROOT
            ('pca9548', 0x73, 0),  # 9548_DC
            ('pca9548', 0x76, 10), # 9548_QSFP_1
            ('pca9548', 0x76, 11), # 9548_QSFP_2
            ('pca9548', 0x76, 12), # 9548_QSFP_3
            ('pca9548', 0x76, 13), # 9548_QSFP_4
            ('pca9548', 0x76, 14), # 9548_QSFP_5
            ('pca9548', 0x76, 9),  # 9548_QSFPDD_1
            ('pca9546', 0x76, 15), # 9546_SFPDD_1
            ('pca9546', 0x76, 16), # 9546_SFP_1
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

        self.insmod("x86-64-ufispace-sys-eeprom")
        self.insmod("optoe")

        # init SYS EEPROM devices
        self.bsp_pr("Init cpu eeprom")
        self.new_i2c_devices(
            [
                #  on cpu board
                ('sys_eeprom', 0x57, 0),
            ]
        )

        # init EEPROM
        self.bsp_pr("Init port eeprom")
        self.init_eeprom()


        # init CPLD
        self.bsp_pr("Init CPLD")
        self.insmod("x86-64-ufispace-s9610-48dx-cpld")
        for i, addr in enumerate((0x30, 0x31, 0x32, 0x33)):
            self.new_i2c_device("s9610_48dx_cpld" + str(i+1), addr, 1)

        #init dev_class
        self.bsp_pr("Init port dev_class")
        self.init_dev_class()

        #config mac rov #TBD
        self.bsp_pr("Init MAC ROV")
        cpld_addr=30
        cpld_bus=1
        rov_addrs=[0x60]
        rov_reg=0x21
        rov_bus=21

        # vid to mac vdd value mapping
        vdd_val_array=( 0.84, 0.80, 0.70, 0.72, 0.74, 0.76, 0.78, 0.82 )
        # vid to rov reg value mapping
        rov_reg_array=( 0x77, 0x6f, 0x5b, 0x5f, 0x63, 0x67, 0x6b, 0x73 )

        #get rov from cpld
        reg_val_str = subprocess.check_output("cat /sys/bus/i2c/devices/{}-00{}/cpld_mac_rov".format(cpld_bus, cpld_addr), shell=True)
        reg_val = int(reg_val_str, 16)
        msg("/sys/bus/i2c/devices/{}-00{}/cpld_mac_rov={}".format(cpld_bus, cpld_addr, reg_val_str))

        for index, rov_addr in enumerate(rov_addrs):
            vid = (reg_val >> 3*index) & 0x7
            mac_vdd_val = vdd_val_array[vid]
            rov_reg_val = rov_reg_array[vid]
            #set rov to mac
            msg("Setting mac[%d] vdd %1.2f with rov register value 0x%x\n" % (index, mac_vdd_val, rov_reg_val) )
            os.system("i2cset -y {} {} {} {} w".format(rov_bus, rov_addr, rov_reg, rov_reg_val))

        # enable event ctrl
        subprocess.call("echo 1 > /sys/bus/i2c/devices/1-0030/cpld_evt_ctrl", shell=True)
        subprocess.call("echo 1 > /sys/bus/i2c/devices/1-0031/cpld_evt_ctrl", shell=True)
        subprocess.call("echo 1 > /sys/bus/i2c/devices/1-0032/cpld_evt_ctrl", shell=True)
        subprocess.call("echo 1 > /sys/bus/i2c/devices/1-0033/cpld_evt_ctrl", shell=True)

        # enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        # init ice (need to have ice before bcm81381 init to avoid failure)
        self.bsp_pr("Init ice")
        self.insmod("intel_auxiliary", False)
        self.insmod("ice")

        # init bcm81381
        self.bsp_pr("Init bcm81381")
        os.system("timeout 120s " + self.FS_PLTM_CFG + "/epdm_cli init 1")

        self.bsp_pr("Init done")

        return True