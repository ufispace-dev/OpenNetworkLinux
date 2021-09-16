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
    IPMI_MAINTENANCE_MODE_OFF = 1
    IPMI_MAINTENANCE_MODE_ON = 2

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
        input_buffer = pack('i', 0)
        out_buffer = fcntl.ioctl(self.ipmidev, self.IPMICTL_GET_MAINTENANCE_MODE_CMD, input_buffer)
        maintanence_mode = unpack('i', out_buffer)[0]

        return maintanence_mode

    def set_ipmi_maintenance_mode(self, mode):
        fcntl.ioctl(self.ipmidev, self.IPMICTL_SET_MAINTENANCE_MODE_CMD, c_int(mode))


class OnlPlatform_x86_64_ufispace_s9300_32d_r0(OnlPlatformUfiSpace):
    PLATFORM = 'x86-64-ufispace-s9300-32d-r0'
    MODEL = "s9300-32d"
    SYS_OBJECT_ID = ".9300.32"
    PORT_COUNT = 32
    PORT_CONFIG = "32x400"
     
    def check_bmc_enable(self):
        return 1

    def init_i2c_mux_idle_state(self, muxs):        
        IDLE_STATE_DISCONNECT = -2
        
        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = "/sys/bus/i2c/devices/%d-%s/idle_state" % (i2c_bus, hex(i2c_addr)[2:].zfill(4))
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def check_i2c_status(self): 
        sysfs_mux_reset = "/sys/devices/platform/x86_64_ufispace_s9300_32d_lpc/mb_cpld/mux_reset"

        # Check I2C status
        retcode = os.system("i2cget -f -y 0 0x73 > /dev/null 2>&1")
        if retcode != 0:

            # read mux failed, i2c bus may be stuck
            msg("Warning: Read I2C Mux Failed!! (ret=%d)\n" % (retcode) )

            # Recovery I2C
            if os.path.exists(sysfs_mux_reset):
                with open(sysfs_mux_reset, "w") as f:
                    # write 0 to sysfs
                    f.write("{}".format(0))
                    msg("I2C bus recovery done.\n")
            else:
                msg("Warning: I2C recovery sysfs does not exist!! (path=%s)\n" % (sysfs_mux_reset) )

    def init_eeprom(self):
        port = 0
                    
        # init SYS EEPROM devices
        self.new_i2c_devices(
            [
                #  on cpu board
                ('mb_eeprom', 0x57, 0),
            ]
        )

        # init QSFPDD EEPROM
        for bus in range(17, 49):
            self.new_i2c_device('optoe3', 0x50, bus)
            # update port_name            
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1

        # init SFP EEPROM
        for bus in range(13, 17):
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

    def baseconfig(self):

        # lpc driver
        self.insmod("x86-64-ufispace-s9300-32d-lpc")

        # check i2c bus status
        self.check_i2c_status()

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))
        
        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        # initialize I210 I2C bus 0 #
        # init PCA9548
        i2c_muxs = [
            ('pca9548', 0x73, 0),
            ('pca9548', 0x72, 0),
            ('pca9548', 0x76, 9),
            ('pca9548', 0x76, 10),
            ('pca9548', 0x76, 11),
            ('pca9548', 0x76, 12),
        ]
                    
        self.new_i2c_devices(i2c_muxs)
        
        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)
        
        self.insmod("x86-64-ufispace-eeprom-mb")
        self.insmod("optoe")

        # init eeprom
        self.init_eeprom()

        # init CPLD
        self.insmod("x86-64-ufispace-s9300-32d-cpld")
        for i, addr in enumerate((0x30, 0x31, 0x32)):
            self.new_i2c_device("s9300_32d_cpld" + str(i+1), addr, 2)

        # config mac rov
        # done bye CPLD at power on          

        self.enable_ipmi_maintenance_mode()

        # init i40e
        self.insmod("i40e")
        
        # enable port led 
        os.system("echo 1 > /sys/bus/i2c/devices/2-0030/cpld_port_led_clr_ctrl")

        return True
