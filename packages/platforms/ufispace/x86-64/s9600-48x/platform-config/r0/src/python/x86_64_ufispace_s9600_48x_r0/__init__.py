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

class OnlPlatform_x86_64_ufispace_s9600_48x_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9600-48x-r0'
    MODEL="S9600-48X"
    SYS_OBJECT_ID=".9600.48"
    PORT_COUNT=48
    PORT_CONFIG="48x100"
     
    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self): 
        sysfs_mux_reset = "/sys/devices/platform/x86_64_ufispace_s9600_48x_lpc/cpu_cpld/mux_reset"

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
                    
        # init QSFP28 EEPROM
        for bus in range(25, 73):
            self.new_i2c_device('sff8436', 0x50, bus)
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
        self.insmod("x86-64-ufispace-s9600-48x-lpc")

        # check i2c bus status
        # FIXME: not ready
        #self.check_i2c_status()

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))
        
        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        # Golden Finger to show CPLD
        os.system("i2cset -y 0 0x75 0x1")
        os.system("i2cget -y 0 0x30 0x2")
        os.system("i2cget -y 0 0x31 0x2")
        os.system("i2cget -y 0 0x32 0x2")
        os.system("i2cget -y 0 0x33 0x2")
        os.system("i2cset -y 0 0x75 0x0")
        
        ########### initialize I2C bus 0 ###########
        # init PCA9548

        i2c_muxs = [
            ('pca9548', 0x75, 0),  # 9548_CPLD
            ('pca9548', 0x72, 0),  # 9548_QSFP
            ('pca9548', 0x73, 0),  # 9548_CPLD_UPG
            ('pca9548', 0x76, 9),  # 9548_QSFP_CHILD0
            ('pca9548', 0x76, 10), # 9548_QSFP_CHILD1
            ('pca9548', 0x76, 11), # 9548_QSFP_CHILD2
            ('pca9548', 0x76, 12), # 9548_QSFP_CHILD3
            ('pca9548', 0x76, 13), # 9548_QSFP_CHILD4
            ('pca9548', 0x76, 14), # 9548_QSFP_CHILD5
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

        # init QSFP EEPROM
        self.init_eeprom()
            
        # init Temperature
        self.new_i2c_devices(
            [               
                # CPU Board Temp
                ('tmp75', 0x4F, 0),
            ]
        )

        # init GPIO sysfs
        #9555_BEACON_LED
        self.new_i2c_device('pca9555', 0x20, 7)        
        #9539_HOST_GPIO_I2C
        self.new_i2c_device('pca9539', 0x74, 0)
        #9539_CPU_I2C
        self.new_i2c_device('pca9539', 0x77, 0)

        # export GPIO
        for i in range(464, 512):
            os.system("echo {} > /sys/class/gpio/export".format(i))

        # init GPIO direction
        # 9555_BEACON_LED 0x20
        os.system("echo in > /sys/class/gpio/gpio511/direction")
        os.system("echo high > /sys/class/gpio/gpio510/direction")
        os.system("echo low > /sys/class/gpio/gpio509/direction")
        os.system("echo low > /sys/class/gpio/gpio508/direction")
        os.system("echo low > /sys/class/gpio/gpio507/direction")
        os.system("echo low > /sys/class/gpio/gpio506/direction")
        os.system("echo low > /sys/class/gpio/gpio505/direction")
        os.system("echo low > /sys/class/gpio/gpio504/direction")
        os.system("echo in > /sys/class/gpio/gpio503/direction")
        os.system("echo low > /sys/class/gpio/gpio502/direction")
        os.system("echo low > /sys/class/gpio/gpio501/direction")
        os.system("echo high > /sys/class/gpio/gpio500/direction")
        os.system("echo low > /sys/class/gpio/gpio499/direction")
        os.system("echo low > /sys/class/gpio/gpio498/direction")
        os.system("echo low > /sys/class/gpio/gpio497/direction")
        os.system("echo low > /sys/class/gpio/gpio496/direction")

        # init GPIO direction
        # 9539_HOST_GPIO_I2C 0x74
        # 9539_CPU_I2C 0x77
        for i in range(464, 496):
            os.system("echo in > /sys/class/gpio/gpio{}/direction".format(i))

        # init CPLD
        self.insmod("x86-64-ufispace-s9600-48x-cpld")
        for i, addr in enumerate((0x30, 0x31, 0x32, 0x33)):
            self.new_i2c_device("s9600_48x_cpld" + str(i+1), addr, 1)

        # enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        return True

