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

class OnlPlatform_x86_64_ufispace_s9600_30dx_r0(OnlPlatformUfiSpace):
    PLATFORM="x86-64-ufispace-s9600-30dx-r0"
    MODEL="S9600-30DX"
    SYS_OBJECT_ID=".9600.30"
    PORT_COUNT=30
    PORT_CONFIG="24x100 + 6x400"
    LEVEL_INFO=1
    LEVEL_ERR=2
    SYSFS_LPC="/sys/devices/platform/x86_64_ufispace_s9600_30dx_lpc"
    SYSFS_DEPH_ID=SYSFS_LPC + "/mb_cpld/board_deph_id"
    SYSFS_HW_ID=SYSFS_LPC + "/mb_cpld/board_hw_id"
    FS_PLTM_CFG="/lib/platform-config/current/onl"

    def check_i2c_status(self):
        sysfs_mux_reset = self.SYSFS_LPC + "/mb_cpld/mux_reset"

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

    def init_eeprom(self):
        port = 0
        data = None

        with open(self.FS_PLTM_CFG + "/port_config.yml", 'r') as yaml_file:
            data = yaml.safe_load(yaml_file)

        # init QSFP EEPROM
        for bus in range(25, 41):
            self.new_i2c_device('optoe1', 0x50, bus)
            # update port_name
            if data is not None:
                port = bus - 25
                port_name = data["QSFP"][port]["port_name"]
                subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port_name, bus), shell=True)

        # init QSFPDD FAB EEPROM
        for bus in range(41, 55):
            self.new_i2c_device('optoe3', 0x50, bus)
            # update port_name
            if data is not None:
                port = bus - 25
                port_name = data["QSFPDD"][port]["port_name"]
                subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port_name, bus), shell=True)

    def init_sfp_eeprom(self):
        port = 0
        data = None

        with open(self.FS_PLTM_CFG + "/port_config.yml", 'r') as yaml_file:
            data = yaml.safe_load(yaml_file)

        # init SFP EEPROM
        for bus in range(23, 25):
            self.new_i2c_device('optoe2', 0x50, bus)
            # update port_name
            if data is not None:
                port = bus + 7
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

    def get_gpio_max(self):
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s9600_30dx_lpc/bsp/bsp_gpio_max"
        status, output = commands.getstatusoutput(cmd)
        if status != 0:
            self.bsp_pr("Get gpio max failed, status={}, output={}, cmd={}\n".format(status, output, cmd), self.LEVEL_ERR);
            self.bsp_pr("Use default GPIO MAX value 511\n".format(status, output, cmd), self.LEVEL_ERR);
            output="511"

        gpio_max = int(output, 10)
        self.bsp_pr("GPIO MAX: {}".format(gpio_max));

        return gpio_max

    def is_pvt_or_later(self):
        #read deph_id
        if os.path.exists(self.SYSFS_DEPH_ID):
            with open(self.SYSFS_DEPH_ID, "r") as f:
                deph_id = int(f.read())
        else:
            self.bsp_pr("SYSFS_DEPH_ID does not exist: {}".format(self.SYSFS_DEPH_ID));
            return False

        #read hw_id
        if os.path.exists(self.SYSFS_HW_ID):
            with open(self.SYSFS_HW_ID, "r") as f:
                hw_id = int(f.read())
        else:
            self.bsp_pr("SYSFS_HW_ID does not exist: {}".format(self.SYSFS_DEPH_ID));
            return False

        # is PVT or later
        if deph_id == 1 or (deph_id == 0 and hw_id >= 3):
            return True
        else:
            self.bsp_pr("device is not PVT or later, deph_id={}, hw_id={}".format(deph_id, hw_id));
            return False

    def enable_sfp_mux(self):
        cmd = "echo 0x1 > /sys/bus/i2c/devices/1-0031/cpld_sfp_mux_ctrl"
        status, output = commands.getstatusoutput(cmd)
        if status != 0:
            self.bsp_pr("enable_sfp_mux() failed, status={}, output={}, cmd={}\n".format(status, output, cmd), self.LEVEL_ERR);

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
        self.insmod("x86-64-ufispace-s9600-30dx-lpc")

        # check i2c bus status
        self.check_i2c_status()

        # Golden Finger to show CPLD
        os.system("i2cset -y 0 0x75 0x1")
        os.system("i2cget -y 0 0x30 0x2 > /dev/null 2>&1")
        os.system("i2cget -y 0 0x31 0x2 > /dev/null 2>&1")
        os.system("i2cget -y 0 0x32 0x2 > /dev/null 2>&1")
        os.system("i2cset -y 0 0x75 0x0 > /dev/null 2>&1")

        ########### initialize I2C bus 0 ###########
        # init PCA9548
        self.bsp_pr("Init PCA9548")
        i2c_muxs = [
            ('pca9548', 0x75, 0),  # 9548_CPLD
            ('pca9548', 0x73, 0),  # 9548_CPLD_UPG
            ('pca9548', 0x72, 0),  # 9548_PORT_ROOT
            ('pca9548', 0x76, 17), # 9548_QSFP_0_7
            ('pca9548', 0x76, 18), # 9548_QSFP_8_15
            ('pca9548', 0x76, 19), # 9548_QSFPDD_16_23
            ('pca9548', 0x76, 20), # 9548_QSFPDD_24_29
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

        self.insmod("x86-64-ufispace-sys-eeprom")
        self.insmod("optoe")

        # init SYS EEPROM devices
        self.bsp_pr("Init system eeprom")
        self.new_i2c_devices(
            [
                ('sys_eeprom', 0x57, 0),
            ]
        )

        # init EEPROM
        self.bsp_pr("Init port eeprom")
        self.init_eeprom()

        # init GPIO sysfs

        #9539_CPU_I2C
        self.new_i2c_device('pca9539', 0x77, 0)

        #get gpio_max
        gpio_max = self.get_gpio_max()

        # export GPIO
        for i in range(gpio_max-15, gpio_max+1):
            os.system("echo {} > /sys/class/gpio/export".format(i))

        # init GPIO direction
        # 9539_CPU_I2C 0x77
        for i in range(gpio_max-15, gpio_max+1):
            os.system("echo in > /sys/class/gpio/gpio{}/direction".format(i))

        # init CPLD
        self.bsp_pr("Init CPLD")
        self.insmod("x86-64-ufispace-s9600-30dx-cpld")
        for i, addr in enumerate((0x30, 0x31, 0x32)):
            self.new_i2c_device("s9600_30dx_cpld" + str(i+1), addr, 1)

        # enable event ctrl
        subprocess.call("echo 1 > /sys/bus/i2c/devices/1-0030/cpld_evt_ctrl", shell=True)
        subprocess.call("echo 1 > /sys/bus/i2c/devices/1-0031/cpld_evt_ctrl", shell=True)
        subprocess.call("echo 1 > /sys/bus/i2c/devices/1-0032/cpld_evt_ctrl", shell=True)

        # enable sfp mux and init sfp eeprom if pvt build or later
        if self.is_pvt_or_later():
            # enable sfp mux
            self.enable_sfp_mux()

            # init sfp eeprom
            self.init_sfp_eeprom()

        # enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        # init i40e (need to have i40e before phy init to avoid failure)
        self.bsp_pr("Init i40e")
        self.insmod("intel_auxiliary", False)
        self.insmod("i40e")

        self.bsp_pr("Init done")

        return True
