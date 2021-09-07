from onl.platform.base import *
from onl.platform.ingrasys import *
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


class OnlPlatform_x86_64_ingrasys_s9280_64x_4bwb_r0(OnlPlatformIngrasys):
    PLATFORM='x86-64-ingrasys-s9280-64x-4bwb-r0'
    MODEL="S9280-64X-4BWB"
    SYS_OBJECT_ID=".9280.64"
    PORT_COUNT=64
    PORT_CONFIG="64x100"

    # fp port to phy port mapping
    fp2phy_array=( 0,  1,  4,  5,  8,  9, 12, 13, 16, 17, 20, 21, 24, 25, 28, 29,
                   32, 33, 36, 37, 40, 41, 44, 45, 48, 49, 52, 53, 56, 57, 60, 61,
                   2,  3,  6,  7, 10, 11, 14, 15, 18, 19, 22, 23, 26, 27, 30, 31,
                   34, 35, 38, 39, 42, 43, 46, 47, 50, 51, 54, 55, 58, 59, 62, 63)
    # fp port to led port mapping
    fp2led_array=( 1,  2,  5,  6,  9, 10, 13, 14,  1,  2,  5,  6,  9, 10, 13, 14,
                   1,  2,  5,  6,  9, 10, 13, 14,  1,  2,  5,  6,  9, 10, 13, 14,
                   3,  4,  7,  8, 11, 12, 15, 16,  3,  4,  7,  8, 11, 12, 15, 16,
                   3,  4,  7,  8, 11, 12, 15, 16,  3,  4,  7,  8, 11, 12, 15, 16)
                

    def init_i2c_mux_idle_state(self, muxs):        
        IDLE_STATE_DISCONNECT = -2
        
        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = "/sys/bus/i2c/devices/%d-%s/idle_state" % (i2c_bus, hex(i2c_addr)[2:].zfill(4))
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def init_eeprom(self):
        port = 0
                    
        # init SYS EEPROM devices
        self.new_i2c_devices(
            [
                #  on cpu board
                ('mb_eeprom', 0x53, 0),
            ]
        )

        # init QSFP EEPROM
        for i in range(1, 65):
            phy_port = self.fp2phy_array[i-1] + 1
            port_group = (phy_port-1)/8
            eeprom_busbase = 29 + (port_group * 8)
            eeprom_busshift = (phy_port-1)%8            
            eeprom_bus = eeprom_busbase + eeprom_busshift
            self.new_i2c_device('optoe1', 0x50, eeprom_bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, eeprom_bus), shell=True)
            port = port + 1

        # init SFP EEPROM
        for i in range(1, 3):
            eeprom_bus = 16 + i
            self.new_i2c_device('sff8436', 0x50, eeprom_bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, eeprom_bus), shell=True)
            port = port + 1
            
    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()
            
        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("Current IPMI_MAINTENANCE_MODE=%d\n" % (mode) )
            
        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)
            
        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("After IPMI_IOCTL IPMI_MAINTENANCE_MODE=%d\n" % (mode) )


    def baseconfig(self):

        self.insmod("eeprom_mb") 
        os.system("modprobe gpio_pca953x")
        self.insmod("optoe") 
        
        ########### initialize I2C bus 0 ###########
        # i2c mux init
        i2c_muxs = [
            ('pca9548', 0x70, 0),  #pca9548_0
            ('pca9548', 0x71, 0),  #pca9548_2
            ('pca9546', 0x72, 0),  #pca9546_1
            ('pca9548', 0x75, 0),  #pca9548_11
            ('pca9548', 0x74, 9),  #pca9548_3
            ('pca9548', 0x74, 10), #pca9548_4
            ('pca9548', 0x74, 11), #pca9548_5
            ('pca9548', 0x74, 12), #pca9548_6
            ('pca9548', 0x74, 13), #pca9548_7
            ('pca9548', 0x74, 14), #pca9548_8
            ('pca9548', 0x74, 15), #pca9548_9
            ('pca9548', 0x74, 16), #pca9548_10
        ]

        self.new_i2c_devices(i2c_muxs)
        
        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

        # io expander init
        # Init System SEL and RST IO Expander
        os.system("i2cset -y -r 20 0x76 2 0x04")
        os.system("i2cset -y -r 20 0x76 3 0xDF")
        os.system("i2cset -y -r 20 0x76 6 0x09")
        os.system("i2cset -y -r 20 0x76 7 0x3F")
        os.system("i2cset -y -r 20 0x76 4 0x00")
        os.system("i2cset -y -r 20 0x76 5 0x00")

        # init eeprom
        self.init_eeprom()

        # thermal init 
        os.system("modprobe coretemp")

        self.new_i2c_devices(
            [
                # tmp75 CPU board thermal, hwmon1
                ('tmp75', 0x4F, 0),
            ]
        )

        # cpld init
        self.insmod("ingrasys_s9280_64x_4bwb_i2c_cpld")

        # add cpld 1~5 to sysfs
        for i in range(1, 6):
            self.new_i2c_device('ingrasys_cpld%d' % i, 0x33, i)

        # reset ports led shift register 
        os.system("i2cset -m 0x04 -y -r 20 0x76 2 0x00")
        os.system("sleep 1")
        os.system("i2cset -m 0x04 -y -r 20 0x76 2 0xFF")

        self.enable_ipmi_maintenance_mode()

        return True
        
