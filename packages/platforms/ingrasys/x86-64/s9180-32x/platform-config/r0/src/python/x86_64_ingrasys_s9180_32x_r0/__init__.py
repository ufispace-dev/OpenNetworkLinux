from onl.platform.base import *
from onl.platform.ingrasys import *
import os
import sys
import subprocess

def msg(s, fatal=False):
    sys.stderr.write(s)
    sys.stderr.flush()
    if fatal:
        sys.exit(1)

class OnlPlatform_x86_64_ingrasys_s9180_32x_r0(OnlPlatformIngrasys):
    PLATFORM='x86-64-ingrasys-s9180-32x-r0'
    MODEL="S9180-32X"
    SYS_OBJECT_ID=".9180.32"
    PORT_COUNT=32
    PORT_CONFIG="32x100"

    def check_bmc_enable(self):
        # check if main mux accessable, if no, bmc enabled
        retcode = subprocess.call('i2cget -y 0 0x76 0x0 2>/dev/null', shell=True)
        # fail if retrun code not 0
        if retcode:
            return 1
        return 0

    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = "/sys/bus/i2c/devices/%d-%s/idle_state" % (i2c_bus, hex(i2c_addr)[2:].zfill(4))
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def init_gpio_sysfs(self, bmc_enable=1):
        # init devices
        if bmc_enable:
            ioexps = [
                ('pca9535', 0x20, 5), #ABS Port 0-15
                ('pca9535', 0x21, 5), #ABS Port 16-31
                ('pca9535', 0x22, 5), #INT Port 0-15
                ('pca9535', 0x23, 5), #INT Port 16-31
                ('pca9535', 0x27, 5), #SFP status
                ('pca9535', 0x20, 6), #LP Mode Port 0-15
                ('pca9535', 0x21, 6), #LP Mode Port 16-31
                ('pca9535', 0x22, 6), #RST Port 0-15
                ('pca9535', 0x23, 6)  #RST Port 16-31
        ]
        else:
            ioexps = [
                ('pca9535', 0x20, 5), #ABS Port 0-15
                ('pca9535', 0x21, 5), #ABS Port 16-31
                ('pca9535', 0x22, 5), #INT Port 0-15
                ('pca9535', 0x23, 5), #INT Port 16-31
                ('pca9535', 0x27, 5), #SFP status
                ('pca9535', 0x20, 6), #LP Mode Port 0-15
                ('pca9535', 0x21, 6), #LP Mode Port 16-31
                ('pca9535', 0x22, 6), #RST Port 0-15
                ('pca9535', 0x23, 6), #RST Port 16-31
                ('pca9535', 0x25, 0)  #PSU I/O status
            ]

        self.new_i2c_devices(ioexps)
        # get gpio base
        max_gpiochip_num = int(subprocess.check_output("""ls /sys/class/gpio/gpiochip*/base | tail -1 | xargs cat""", shell=True))
        gpios_num = int(subprocess.check_output("""ls /sys/class/gpio/gpiochip*/ngpio | tail -1 | xargs cat""", shell=True))
        gpio_base = max_gpiochip_num + gpios_num
        msg("GPIO_BASE = %d\n" % gpio_base)

        # export gpio
        # ABS Port 0-15
        index_end = gpio_base
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)
            os.system("echo 1 > /sys/class/gpio/gpio%d/active_low" % i)

        # ABS Port 16-31
        index_end = index_start
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)
            os.system("echo 1 > /sys/class/gpio/gpio%d/active_low" % i)

        # INT Port 0-15
        index_end = index_start
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)
            os.system("echo 1 > /sys/class/gpio/gpio%d/active_low" % i)

        # INT Port 16-31
        index_end = index_start
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)
            os.system("echo 1 > /sys/class/gpio/gpio%d/active_low" % i)

        # SFP status
        index_end = index_start
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)
            if i == index_start+4 or i == index_start+5 or i == index_start+8 or \
               i == index_start+9 or i == index_start+10 or i == index_start+11:
                os.system("echo out > /sys/class/gpio/gpio%d/direction" % i)
            else:
                os.system("echo 1 > /sys/class/gpio/gpio%d/active_low" % i)

        # LP Mode Port 0-15
        index_end = index_start
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)
            os.system("echo out > /sys/class/gpio/gpio%d/direction" % i)

        # LP Mode Port 16-31
        index_end = index_start
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)
            os.system("echo out > /sys/class/gpio/gpio%d/direction" % i)

        # RST Port 0-15
        index_end = index_start
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)
            os.system("echo out > /sys/class/gpio/gpio%d/direction" % i)
            os.system("echo 1 > /sys/class/gpio/gpio%d/active_low" % i)
            os.system("echo 0 > /sys/class/gpio/gpio%d/value" % i)

        # RST Port 16-31
        index_end = index_start
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)
            os.system("echo out > /sys/class/gpio/gpio%d/direction" % i)
            os.system("echo 1 > /sys/class/gpio/gpio%d/active_low" % i)
            os.system("echo 0 > /sys/class/gpio/gpio%d/value" % i)

        if bmc_enable:
            return

        # apply only on platform without bmc
        # PSU I/O status
        index_end = index_start
        index_start = index_end - gpios_num
        for i in range(index_start, index_end):
            os.system("echo %d > /sys/class/gpio/export" % i)
            if i == index_start+1 or i == index_start+2 or i == index_start+4 or \
               i == index_start+5 or i == index_start+6 or i == index_start+9 or \
               i == index_start+10 or i == index_start+12:
                os.system("echo 1 > /sys/class/gpio/gpio%d/active_low" % i)

    def port_get_bus_id(self, port):
        bus_base = 0
        bus_id = 0

        if port >= 1 and port <= 8:
            bus_base = 9
        elif port >= 9 and port <= 16:
            bus_base = 17
        elif port >= 17 and port <= 24:
            bus_base = 25
        elif port >= 25 and port <= 32:
            bus_base = 33
        elif port == 33:
            return 45
        elif port == 34:
            return 46
        else:
            msg("Invalid port number %d\n" % port )

        idx = port % 8
        if idx == 1:
            bus_id = bus_base + 1
        elif idx == 2:
            bus_id = bus_base + 0
        elif idx == 3:
            bus_id = bus_base + 3
        elif idx == 4:
            bus_id = bus_base + 2
        elif idx == 5:
            bus_id = bus_base + 5
        elif idx == 6:
            bus_id = bus_base + 4
        elif idx == 7:
            bus_id = bus_base + 7
        else:
            bus_id = bus_base + 6

        return bus_id

    def init_port_eeprom(self):
        # init QSFP EEPROM
        for port in range(1, 33):
            bus = self.port_get_bus_id(port)
            self.new_i2c_device('optoe1', 0x50, bus)
            # update port_name
            subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)

        # init SFP(0/1) EEPROM
        port = 33
        bus = self.port_get_bus_id(port)
        self.new_i2c_device('sff8436', 0x50, bus)
        subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)
        port = 34
        bus = self.port_get_bus_id(port)
        self.new_i2c_device('sff8436', 0x50, bus)
        subprocess.call("echo {} > /sys/bus/i2c/devices/{}-0050/port_name".format(port, bus), shell=True)

    def baseconfig(self):

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        if bmc_enable:
            return self.baseconfig_bmc()

        # vid to mac vdd value mapping
        vdd_val_array=( 0.85,  0.82,  0.77,  0.87,  0.74,  0.84,  0.79,  0.89 )
        # vid to rov reg value mapping
        rov_reg_array=( 0x24,  0x21,  0x1C,  0x26,  0x19, 0x23, 0x1E, 0x28 )

        self.insmod("eeprom_mb")
        # init SYS EEPROM devices
        self.new_i2c_devices(
            [
                #  on main board
                ('mb_eeprom', 0x55, 0),
                #  on cpu board
                ('mb_eeprom', 0x51, 0),
            ]
        )

        os.system("modprobe w83795")
        os.system("modprobe eeprom")
        os.system("modprobe gpio_pca953x")
        self.insmod("optoe")

        ########### initialize I2C bus 0 ###########
        # init PCA9548/PCA9545
        i2c_muxs = [
            ('pca9548', 0x70, 0),
            ('pca9548', 0x71, 1),
            ('pca9548', 0x71, 2),
            ('pca9548', 0x71, 3),
            ('pca9548', 0x71, 4),
            ('pca9548', 0x71, 7),
            ('pca9548', 0x76, 0),
            ('pca9545', 0x72, 0),
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

        # Golden Finger to show CPLD
        os.system("i2cget -y 44 0x74 2")

        # Reset BMC Dummy Board
        os.system("i2cset -y -r 0 0x26 4 0x00")
        os.system("i2cset -y -r 0 0x26 5 0x00")
        os.system("i2cset -y -r 0 0x26 2 0x3F")
        os.system("i2cset -y -r 0 0x26 3 0x1F")
        os.system("i2cset -y -r 0 0x26 6 0xC0")
        os.system("i2cset -y -r 0 0x26 7 0x00")

        # CPU Baord
        os.system("i2cset -y -r 0 0x77 6 0xFF")
        os.system("i2cset -y -r 0 0x77 7 0xFF")

        # init SMBUS1 ABS
        os.system("i2cset -y -r 5 0x20 4 0x00")
        os.system("i2cset -y -r 5 0x20 5 0x00")
        os.system("i2cset -y -r 5 0x20 6 0xFF")
        os.system("i2cset -y -r 5 0x20 7 0xFF")

        os.system("i2cset -y -r 5 0x21 4 0x00")
        os.system("i2cset -y -r 5 0x21 5 0x00")
        os.system("i2cset -y -r 5 0x21 6 0xFF")
        os.system("i2cset -y -r 5 0x21 7 0xFF")

        os.system("i2cset -y -r 5 0x22 4 0x00")
        os.system("i2cset -y -r 5 0x22 5 0x00")
        os.system("i2cset -y -r 5 0x22 6 0xFF")
        os.system("i2cset -y -r 5 0x22 7 0xFF")

        os.system("i2cset -y -r 5 0x23 4 0x00")
        os.system("i2cset -y -r 5 0x23 5 0x00")
        os.system("i2cset -y -r 5 0x23 6 0xFF")
        os.system("i2cset -y -r 5 0x23 7 0xFF")

        # init SFP
        os.system("i2cset -y -r 5 0x27 4 0x00")
        os.system("i2cset -y -r 5 0x27 5 0x00")
        os.system("i2cset -y -r 5 0x27 2 0x00")
        os.system("i2cset -y -r 5 0x27 3 0x00")
        os.system("i2cset -y -r 5 0x27 6 0xCF")
        os.system("i2cset -y -r 5 0x27 7 0xF0")

        # set ZQSFP LP_MODE = 0
        os.system("i2cset -y -r 6 0x20 4 0x00")
        os.system("i2cset -y -r 6 0x20 5 0x00")
        os.system("i2cset -y -r 6 0x20 2 0x00")
        os.system("i2cset -y -r 6 0x20 3 0x00")
        os.system("i2cset -y -r 6 0x20 6 0x00")
        os.system("i2cset -y -r 6 0x20 7 0x00")

        os.system("i2cset -y -r 6 0x21 4 0x00")
        os.system("i2cset -y -r 6 0x21 5 0x00")
        os.system("i2cset -y -r 6 0x21 2 0x00")
        os.system("i2cset -y -r 6 0x21 3 0x00")
        os.system("i2cset -y -r 6 0x21 6 0x00")
        os.system("i2cset -y -r 6 0x21 7 0x00")

        # set ZQSFP RST = 1
        os.system("i2cset -y -r 6 0x22 4 0x00")
        os.system("i2cset -y -r 6 0x22 5 0x00")
        os.system("i2cset -y -r 6 0x22 2 0xFF")
        os.system("i2cset -y -r 6 0x22 3 0xFF")
        os.system("i2cset -y -r 6 0x22 6 0x00")
        os.system("i2cset -y -r 6 0x22 7 0x00")

        os.system("i2cset -y -r 6 0x23 4 0x00")
        os.system("i2cset -y -r 6 0x23 5 0x00")
        os.system("i2cset -y -r 6 0x23 2 0xFF")
        os.system("i2cset -y -r 6 0x23 3 0xFF")
        os.system("i2cset -y -r 6 0x23 6 0x00")
        os.system("i2cset -y -r 6 0x23 7 0x00")

        # init Host GPIO
        os.system("i2cset -y -r 0 0x74 4 0x00")
        os.system("i2cset -y -r 0 0x74 5 0x00")
        os.system("i2cset -y -r 0 0x74 2 0x0F")
        os.system("i2cset -y -r 0 0x74 3 0xDF")
        os.system("i2cset -y -r 0 0x74 6 0x08")
        os.system("i2cset -y -r 0 0x74 7 0x1F")

        # init LED board after PVT (BAREFOOT_IO_EXP_LED_ID)
        #os.system("i2cset -y -r 50 0x75 4 0x00")
        #os.system("i2cset -y -r 50 0x75 5 0x00")
        #os.system("i2cset -y -r 50 0x75 6 0x00")
        #os.system("i2cset -y -r 50 0x75 7 0xFF")

        # init Board ID
        os.system("i2cset -y -r 51 0x27 4 0x00")
        os.system("i2cset -y -r 51 0x27 5 0x00")
        os.system("i2cset -y -r 51 0x27 6 0xFF")
        os.system("i2cset -y -r 51 0x27 7 0xFF")

        # init Board ID of Dummy BMC Board
        os.system("i2cset -y -r 0 0x24 4 0x00")
        os.system("i2cset -y -r 0 0x24 5 0x00")
        os.system("i2cset -y -r 0 0x24 6 0xFF")
        os.system("i2cset -y -r 0 0x24 7 0xFF")

        # init PSU I/O (BAREFOOT_IO_EXP_PSU_ID)
        os.system("i2cset -y -r 0 0x25 4 0x00")
        os.system("i2cset -y -r 0 0x25 5 0x00")
        os.system("i2cset -y -r 0 0x25 2 0x00")
        os.system("i2cset -y -r 0 0x25 3 0x1D")
        os.system("i2cset -y -r 0 0x25 6 0xDB")
        os.system("i2cset -y -r 0 0x25 7 0x03")

        # init FAN I/O (BAREFOOT_IO_EXP_FAN_ID)
        os.system("i2cset -y -r 59 0x20 4 0x00")
        os.system("i2cset -y -r 59 0x20 5 0x00")
        os.system("i2cset -y -r 59 0x20 2 0x11")
        os.system("i2cset -y -r 59 0x20 3 0x11")
        os.system("i2cset -y -r 59 0x20 6 0xCC")
        os.system("i2cset -y -r 59 0x20 7 0xCC")

        # init Fan
        # select bank 0
        os.system("i2cset -y -r 56 0x2F 0x00 0x80")
        # enable FANIN 1-8
        os.system("i2cset -y -r 56 0x2F 0x06 0xFF")
        # disable FANIN 9-14
        os.system("i2cset -y -r 56 0x2F 0x07 0x00")
        # select bank 2
        os.system("i2cset -y -r 56 0x2F 0x00 0x82")
        # set PWM mode in FOMC
        os.system("i2cset -y -r 56 0x2F 0x0F 0x00")

        # init VOLMON
        os.system("i2cset -y -r 56 0x2F 0x00 0x80")
        os.system("i2cset -y -r 56 0x2F 0x01 0x1C")
        os.system("i2cset -y -r 56 0x2F 0x00 0x80")
        os.system("i2cset -y -r 56 0x2F 0x02 0xFF")
        os.system("i2cset -y -r 56 0x2F 0x03 0x50")
        os.system("i2cset -y -r 56 0x2F 0x04 0x0A")
        os.system("i2cset -y -r 56 0x2F 0x00 0x80")
        os.system("i2cset -y -r 56 0x2F 0x01 0x1D")
        self.new_i2c_device('w83795adg', 0x2F, 56)

        # init Fan Speed
        os.system("echo 120 > /sys/class/hwmon/hwmon1/device/pwm1")
        os.system("echo 120 > /sys/class/hwmon/hwmon1/device/pwm2")

        # init Temperature
        self.new_i2c_devices(
            [
                # ASIC Coretemp and Front MAC
                ('lm86', 0x4C, 53),

                # CPU Board
                ('tmp75', 0x4F, 0),

                # Near PSU1
                ('tmp75', 0x48, 53),

                # Rear MAC
                ('tmp75', 0x4A, 53),

                # Near Port 32
                ('tmp75', 0x4B, 53),

                # Near PSU2
                ('tmp75', 0x4D, 53),
            ]
        )

        # init gpio sysfs
        self.init_gpio_sysfs(bmc_enable)

        # init port eeprom
        self.init_port_eeprom()

        # init PSU(0/1) EEPROM devices
        self.new_i2c_device('eeprom', 0x50, 57)
        self.new_i2c_device('eeprom', 0x50, 58)

        # _mac_vdd_init
        try:
            reg_val_str = subprocess.check_output("""i2cget -y 44 0x33 0x42 2>/dev/null""", shell=True)
            reg_val = int(reg_val_str, 16)
            vid = reg_val & 0x7
            mac_vdd_val = vdd_val_array[vid]
            rov_reg = rov_reg_array[vid]
            subprocess.check_output("""i2cset -y -r 55 0x22 0x21 0x%x w 2>/dev/null""" % rov_reg, shell=True)
            msg("Setting mac vdd %1.2f with rov register value 0x%x\n" % (mac_vdd_val, rov_reg) )
        except subprocess.CalledProcessError:
            pass
        # init SYS LED
        os.system("i2cset -y -r 50 0x75 2 0x01")
        os.system("i2cset -y -r 50 0x75 4 0x00")
        os.system("i2cset -y -r 50 0x75 5 0x00")
        os.system("i2cset -y -r 50 0x75 6 0x00")
        os.system("i2cset -y -r 50 0x75 7 0x00")

        return True

    def baseconfig_bmc(self):

        # vid to mac vdd value mapping
        vdd_val_array=( 0.85,  0.82,  0.77,  0.87,  0.74,  0.84,  0.79,  0.89 )
        # vid to rov reg value mapping
        rov_reg_array=( 0x24,  0x21,  0x1C,  0x26,  0x19, 0x23, 0x1E, 0x28 )

        self.insmod("eeprom_mb")
        # init SYS EEPROM devices
        self.new_i2c_devices(
            [
                #  on cpu board
                ('mb_eeprom', 0x51, 0),
            ]
        )

        os.system("modprobe eeprom")
        os.system("modprobe gpio_pca953x")
        self.insmod("optoe")

        ########### initialize I2C bus 0 ###########
        # init PCA9548
        self.new_i2c_devices(
            [
                ('pca9548', 0x70, 0),
                ('pca9548', 0x71, 1),
                ('pca9548', 0x71, 2),
                ('pca9548', 0x71, 3),
                ('pca9548', 0x71, 4),
                ('pca9548', 0x71, 7),
            ]
        )

        # Golden Finger to show CPLD
        os.system("i2cget -y 44 0x74 2")

        # CPU Baord
        os.system("i2cset -y -r 0 0x77 6 0xFF")
        os.system("i2cset -y -r 0 0x77 7 0xFF")

        # init SMBUS1 ABS
        os.system("i2cset -y -r 5 0x20 4 0x00")
        os.system("i2cset -y -r 5 0x20 5 0x00")
        os.system("i2cset -y -r 5 0x20 6 0xFF")
        os.system("i2cset -y -r 5 0x20 7 0xFF")

        os.system("i2cset -y -r 5 0x21 4 0x00")
        os.system("i2cset -y -r 5 0x21 5 0x00")
        os.system("i2cset -y -r 5 0x21 6 0xFF")
        os.system("i2cset -y -r 5 0x21 7 0xFF")

        os.system("i2cset -y -r 5 0x22 4 0x00")
        os.system("i2cset -y -r 5 0x22 5 0x00")
        os.system("i2cset -y -r 5 0x22 6 0xFF")
        os.system("i2cset -y -r 5 0x22 7 0xFF")

        os.system("i2cset -y -r 5 0x23 4 0x00")
        os.system("i2cset -y -r 5 0x23 5 0x00")
        os.system("i2cset -y -r 5 0x23 2 0xCF")
        os.system("i2cset -y -r 5 0x23 3 0xF0")
        os.system("i2cset -y -r 5 0x23 6 0xCF")
        os.system("i2cset -y -r 5 0x23 7 0xF0")

        # init SFP
        os.system("i2cset -y -r 5 0x27 4 0x00")
        os.system("i2cset -y -r 5 0x27 5 0x00")
        os.system("i2cset -y -r 5 0x27 2 0x00")
        os.system("i2cset -y -r 5 0x27 3 0x00")
        os.system("i2cset -y -r 5 0x27 6 0xCF")
        os.system("i2cset -y -r 5 0x27 7 0xF0")

        # set ZQSFP LP_MODE = 0
        os.system("i2cset -y -r 6 0x20 4 0x00")
        os.system("i2cset -y -r 6 0x20 5 0x00")
        os.system("i2cset -y -r 6 0x20 2 0x00")
        os.system("i2cset -y -r 6 0x20 3 0x00")
        os.system("i2cset -y -r 6 0x20 6 0x00")
        os.system("i2cset -y -r 6 0x20 7 0x00")

        os.system("i2cset -y -r 6 0x21 4 0x00")
        os.system("i2cset -y -r 6 0x21 5 0x00")
        os.system("i2cset -y -r 6 0x21 2 0x00")
        os.system("i2cset -y -r 6 0x21 3 0x00")
        os.system("i2cset -y -r 6 0x21 6 0x00")
        os.system("i2cset -y -r 6 0x21 7 0x00")

        # set ZQSFP RST = 1
        os.system("i2cset -y -r 6 0x22 4 0x00")
        os.system("i2cset -y -r 6 0x22 5 0x00")
        os.system("i2cset -y -r 6 0x22 2 0xFF")
        os.system("i2cset -y -r 6 0x22 3 0xFF")
        os.system("i2cset -y -r 6 0x22 6 0x00")
        os.system("i2cset -y -r 6 0x22 7 0x00")

        os.system("i2cset -y -r 6 0x23 4 0x00")
        os.system("i2cset -y -r 6 0x23 5 0x00")
        os.system("i2cset -y -r 6 0x23 2 0xFF")
        os.system("i2cset -y -r 6 0x23 3 0xFF")
        os.system("i2cset -y -r 6 0x23 6 0x00")
        os.system("i2cset -y -r 6 0x23 7 0x00")

        # init Host GPIO
        os.system("i2cset -y -r 0 0x74 4 0x00")
        os.system("i2cset -y -r 0 0x74 5 0x00")
        os.system("i2cset -y -r 0 0x74 2 0x0F")
        os.system("i2cset -y -r 0 0x74 3 0xDF")
        os.system("i2cset -y -r 0 0x74 6 0x08")
        os.system("i2cset -y -r 0 0x74 7 0x1F")

        # init Temperature
        self.new_i2c_devices(
            [
                # CPU Board
                ('tmp75', 0x4F, 0),
            ]
        )

        # init gpio sysfs
        self.init_gpio_sysfs()

        # init port eeprom
        self.init_port_eeprom()

        # _mac_vdd_init
        try:
            reg_val_str = subprocess.check_output("""i2cget -y 44 0x33 0x42 2>/dev/null""", shell=True)
            reg_val = int(reg_val_str, 16)
            vid = reg_val & 0x7
            mac_vdd_val = vdd_val_array[vid]
            rov_reg = rov_reg_array[vid]
            subprocess.check_output("""i2cset -y -r 8 0x22 0x21 0x%x w 2>/dev/null""" % rov_reg, shell=True)
            msg("Setting mac vdd %1.2f with rov register value 0x%x\n" % (mac_vdd_val, rov_reg) )
        except subprocess.CalledProcessError:
            pass
        return True
