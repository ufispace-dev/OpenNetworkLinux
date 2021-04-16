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

class OnlPlatform_x86_64_ufispace_s9510_28dc_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9510-28dc-r0'
    MODEL="S9510-28DC"
    SYS_OBJECT_ID=".9510.28"
    PORT_COUNT=28
    PORT_CONFIG="24x25 + 2x100 + 2x400"
     
    def check_bmc_enable(self):
        return 1

    def init_eeprom(self, bus_ismt):    
        port = 0
        
        # init QSFPDD EEPROM
        for bus in range(bus_ismt+11, bus_ismt+13):
            self.new_i2c_device('optoe1', 0x50, bus)
            # update port_name            
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1
            
        # init QSFP28 EEPROM
        for bus in range(bus_ismt+9, bus_ismt+11):        
            self.new_i2c_device('sff8436', 0x50, bus)
            # update port_name            
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1
            
        # init SFP28 EEPROM
        for bus in range(bus_ismt+13, bus_ismt+37):
            self.new_i2c_device('optoe2', 0x50, bus)
            # update port_name            
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
            port = port + 1
            
    def init_gpio(self):
        
        # init GPIO sysfs
        self.new_i2c_devices(
            [
                ('pca9535', 0x20, 4), #9535_IO_EXP_01 (9535_BOARD_ID)
                ('pca9535', 0x21, 6), #9535_IO_EXP_02 (9535_QSFPX_0_3)
                ('pca9535', 0x22, 6), #9535_IO_EXP_03 (9535_TX_DIS_4_19)
                ('pca9535', 0x24, 6), #9535_IO_EXP_04 (9535_TX_DIS_20_27)
                ('pca9535', 0x26, 7), #9535_IO_EXP_05 (9535_TX_FLT_4_19)
                ('pca9535', 0x27, 7), #9535_IO_EXP_06 (9535_TX_FLT_20_27)
                ('pca9535', 0x25, 7), #9535_IO_EXP_07 (9535_RATE_SELECT_4_19)
                ('pca9535', 0x23, 7), #9535_IO_EXP_08 (9535_RATE_SELECT_20_27)
                ('pca9535', 0x20, 8), #9535_IO_EXP_11 (9535_MOD_ABS_4_19)
                ('pca9535', 0x22, 8), #9535_IO_EXP_09 (9535_MOD_ABS_20_27)
                ('pca9535', 0x21, 8), #9535_IO_EXP_12 (9535_RX_LOS_4_19)
                ('pca9535', 0x24, 8)  #9535_IO_EXP_10 (9535_RX_LOS_20_27)
            ]
        )
        
        # init all GPIO direction to "in"
        gpio_dir = ["in"] * 512 

        # get board id
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s9510_28dc_lpc/mb_cpld/board_hw_id"
        status, output = commands.getstatusoutput(cmd)
        if status != 0:
            msg("Get board id from LPC failed, status={}, output={}, cmd={}\n", status, output, cmd)
            return
            
        #board_id = int(output, 10)
        #model_id = board_id & 0b00001111
        #hw_rev = (board_id & 0b00110000) >> 4
        #build_rev = (board_id & 0b11000000) >> 6
        #hw_build_rev = (hw_rev << 2) | (build_rev)
        board_hw_id = int(output, 10)
        
        #Alpha 1
        if board_hw_id >= 0:        
            # init GPIO direction to output low
            for i in range(491, 487, -1) + \
                     range(479, 463, -1) + \
                     range(455, 447, -1) + \
                     range(395, 391, -1):
                gpio_dir[i] = "low"
            
            # init GPIO direction to output high
            for i in range(495, 491, -1) + \
                     range(415, 399, -1) + \
                     range(391, 383, -1):
                gpio_dir[i] = "high"
                
            msg("GPIO init\n")                
        else:
            msg("GPIO init failed, unknown board_hw_id=\n".format(board_hw_id))
            
        # export GPIO and configure direction
        for i in range(352, 512):
            os.system("echo {} > /sys/class/gpio/export".format(i))
            os.system("echo {} > /sys/class/gpio/gpio{}/direction".format(gpio_dir[i], i))

    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()
            
        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("Current IPMI_MAINTENANCE_MODE=%d\n" % (mode) )
            
        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)
            
        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("After IPMI_IOCTL IPMI_MAINTENANCE_MODE=%d\n" % (mode) )
        
    def baseconfig(self):

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))
        
        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        ########### initialize I2C bus 0 ###########
        # i2c_i801 is built-in

        # add i2c_ismt
        #self.insmod("i2c-ismt") #module not found
        os.system("modprobe i2c-ismt")

        #CPLD
        self.insmod("x86-64-ufispace-s9510-28dc-lpc")
        
        # init PCA9548
        bus_i801=0
        bus_ismt=1
        self.new_i2c_devices(
            [
                ('pca9546', 0x75, bus_ismt),   #9546_ROOT_TIMING
                ('pca9546', 0x76, bus_ismt),   #9546_ROOT_SFP
                ('pca9546', 0x70, bus_ismt+8), #9548_CHILD_QSFPX_0_3
                ('pca9548', 0x71, bus_ismt+8), #9548_CHILD_SFP_4_11
                ('pca9548', 0x72, bus_ismt+8), #9548_CHILD_SFP_12_19
                ('pca9548', 0x73, bus_ismt+8), #9548_CHILD_SFP_20_27
            ]
        )

        self.insmod("x86-64-ufispace-eeprom-mb")
        self.insmod("optoe")

        # init SYS EEPROM devices
        self.new_i2c_devices(
            [
                #  on cpu board
                ('mb_eeprom', 0x57, bus_ismt),
            ]
        )
        
        # init EEPROM
        self.init_eeprom(bus_ismt)

        # init Temperature        
        os.system("modprobe jc42")
        
        # init GPIO sysfs
        self.init_gpio()

        # onie syseeprom
        self.insmod("x86-64-ufispace-s9510-28dc-onie-syseeprom.ko")

        #enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        return True        

