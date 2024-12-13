from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
from ctypes import c_int, sizeof
import os
import sys
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

class OnlPlatform_x86_64_ufispace_s9600_28dx_r0(OnlPlatformUfiSpace):
    PLATFORM="x86-64-ufispace-s9600-28dx-r0"
    MODEL="S9600-28DX"
    SYS_OBJECT_ID=".9600.28"
    PORT_COUNT=28
    PORT_CONFIG="48x100 + 8x400"
    LEVEL_INFO=1
    LEVEL_ERR=2
    SYSFS_LPC="/sys/devices/platform/x86_64_ufispace_s9600_28dx_lpc"
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

        # init SFP+ EEPROM
        for bus in range(25, 29):
            self.new_i2c_device('optoe2', 0x50, bus)
            # update port_name
            if data is not None:
                port = bus + 3
                port_name = data["SFP"][port]["port_name"]
                subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port_name, bus), shell=True)

        # init QSFP EEPROM
        for bus in range(29, 53):
            self.new_i2c_device('optoe1', 0x50, bus)
            # update port_name
            if data is not None:
                port = bus - 29
                port_name = data["QSFP"][port]["port_name"]
                subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port_name, bus), shell=True)

        # init QSFPDD FAB EEPROM
        for bus in range(53, 57):
            self.new_i2c_device('optoe3', 0x50, bus)
            # update port_name
            if data is not None:
                port = bus - 29
                port_name = data["QSFPDD"][port]["port_name"]
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
        self.insmod("x86-64-ufispace-s9600-28dx-lpc")

        # check i2c bus status
        self.check_i2c_status()

        # Golden Finger to show CPLD #TODO
        #os.system("i2cset -y 0 0x75 0x1")
        #os.system("i2cget -y 0 0x30 0x2 > /dev/null 2>&1")
        #os.system("i2cget -y 0 0x31 0x2 > /dev/null 2>&1")
        #os.system("i2cget -y 0 0x32 0x2 > /dev/null 2>&1")
        #os.system("i2cset -y 0 0x75 0x0 > /dev/null 2>&1")

        ########### initialize I2C bus 0 ###########
        # init PCA9548
        self.bsp_pr("Init PCA9548")
        i2c_muxs = [
            ('pca9548', 0x75, 0),  # 9548_1_CPLD
            ('pca9548', 0x73, 0),  # 9548_2_NTM
            ('pca9548', 0x72, 0),  # 9548_PORT_ROOT
            ('pca9546', 0x71, 17), # 9546_SFP+_0_3
            ('pca9548', 0x76, 17), # 9548_QSFP_0_7
            ('pca9548', 0x76, 18), # 9548_QSFP_8_15
            ('pca9548', 0x76, 19), # 9548_QSFP_16_23
            ('pca9548', 0x76, 23), # 9548_QSFPDD_0_7
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

        self.insmod("x86-64-ufispace-eeprom-mb")
        self.insmod("optoe")

        # init SYS EEPROM devices
        self.bsp_pr("Init cpu eeprom")
        self.new_i2c_devices(
            [
                #  on cpu board
                ('mb_eeprom', 0x57, 0),
            ]
        )

        # init EEPROM
        self.bsp_pr("Init port eeprom")
        self.init_eeprom()

        # init GPIO sysfs

        '''
        #TODO
        #9539_CPU_I2C
        self.new_i2c_device('pca9539', 0x77, 0)
        #9539_0x74_TBD
        #self.new_i2c_device('pca9539', 0x74, 0)

        # export GPIO
        #for i in range(464, 512):
        #    os.system("echo {} > /sys/class/gpio/export".format(i))

        # init GPIO direction
        # 9555_BEACON_LED 0x20
        os.system("echo in > /sys/class/gpio/gpio511/direction")
        os.system("echo low > /sys/class/gpio/gpio510/direction")
        os.system("echo low > /sys/class/gpio/gpio509/direction")
        os.system("echo low > /sys/class/gpio/gpio508/direction")
        os.system("echo low > /sys/class/gpio/gpio507/direction")
        os.system("echo high > /sys/class/gpio/gpio506/direction")
        os.system("echo low > /sys/class/gpio/gpio505/direction")
        os.system("echo low > /sys/class/gpio/gpio504/direction")
        os.system("echo in > /sys/class/gpio/gpio503/direction")
        os.system("echo low > /sys/class/gpio/gpio502/direction")
        os.system("echo low > /sys/class/gpio/gpio501/direction")
        os.system("echo low > /sys/class/gpio/gpio500/direction")
        os.system("echo high > /sys/class/gpio/gpio499/direction")
        os.system("echo low > /sys/class/gpio/gpio498/direction")
        os.system("echo low > /sys/class/gpio/gpio497/direction")
        os.system("echo low > /sys/class/gpio/gpio496/direction")

        # init GPIO direction
        # 9539_CPU_I2C 0x77
        for i in range(480, 496):
            os.system("echo in > /sys/class/gpio/gpio{}/direction".format(i))
        '''

        # init CPLD
        self.bsp_pr("Init CPLD")
        self.insmod("x86-64-ufispace-s9600-28dx-cpld")
        for i, addr in enumerate((0x30, 0x31, 0x32)):
            self.new_i2c_device("s9600_28dx_cpld" + str(i+1), addr, 1)

        '''
        #TODO
        #config mac rov
        cpld_addr=30
        cpld_bus=1
        rov_addrs=[0x60, 0x62]
        rov_reg=0x21
        rov_bus=4
        
        # vid to mac vdd value mapping 
        vdd_val_array=( 0.82, 0.82, 0.76, 0.78, 0.80, 0.84, 0.86, 0.88 )
        # vid to rov reg value mapping 
        rov_reg_array=( 0x73, 0x73, 0x67, 0x6B, 0x6F, 0x77, 0x7B, 0x7F )

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
        subprocess.call("echo 1 > /sys/bus/i2c/devices/30-0033/cpld_evt_ctrl", shell=True)
        subprocess.call("echo 1 > /sys/bus/i2c/devices/30-0034/cpld_evt_ctrl", shell=True)
        '''

        # enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        self.bsp_pr("Init done")

        return True
