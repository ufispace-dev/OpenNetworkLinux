from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
from ctypes import c_int, sizeof
import os
import sys
import subprocess
import yaml
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
    IPMICTL_SET_MAINTENANCE_MODE_CMD = _IOWRITE << 30 | sizeof(c_int) << 16| \
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

class OnlPlatform_x86_64_ufispace_s9720_56ed_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9720-56ed-r0'
    MODEL="S9720-56ED"
    SYS_OBJECT_ID=".9720.56"
    PORT_COUNT=56
    PORT_CONFIG="36x400 + 20x800"
    LEVEL_INFO=1
    LEVEL_ERR=2
    BSP_VERSION='1.0.0'
    PATH_I2C_CPLD1="/sys/bus/i2c/devices/1-0030"
    PATH_I2C_CPLD2="/sys/bus/i2c/devices/1-0031"
    PATH_I2C_FPGA="/sys/bus/i2c/devices/1-0037"
    PATH_I2C_CPLD4="/sys/bus/i2c/devices/24-0032"
    PATH_I2C_CPLD5="/sys/bus/i2c/devices/24-0033"
    PATH_SYS_I2C_DEV_ATTR="/sys/bus/i2c/devices/{}-{:0>4x}/{}"
    PATH_SYS_GPIO = "/sys/class/gpio"
    PATH_SYSTEM_LED="/sys/bus/i2c/devices/1-0030/system_led_status"
    SYSTEM_LED_GREEN=0b00001001
    PATH_LPC="/sys/devices/platform/x86_64_ufispace_s9720_56ed_lpc"
    PATH_LPC_GRP_BSP=PATH_LPC+"/bsp"
    PATH_LPC_GRP_MB_CPLD=PATH_LPC+"/mb_cpld"
    PATH_PORT_CONFIG="/lib/platform-config/"+PLATFORM+"/onl/port_config.yml"
    PATH_EPDM_CLI="/lib/platform-config/current/onl/epdm_cli"
    PATH_CPLD1_EVT_CTRL=PATH_I2C_CPLD1+"/event_detect_ctrl"
    PATH_CPLD2_EVT_CTRL=PATH_I2C_CPLD2+"/event_detect_ctrl"
    PATH_FPGA_EVT_CTRL=PATH_I2C_FPGA+"/event_detect_ctrl"
    PATH_CPLD4_EVT_CTRL=PATH_I2C_CPLD4+"/event_detect_ctrl"
    PATH_CPLD5_EVT_CTRL=PATH_I2C_CPLD5+"/event_detect_ctrl"
    PATH_BSP_GPIO_MAX=PATH_LPC_GRP_BSP+"/bsp_gpio_max"


    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self, bus_i801):
        sysfs_mux_reset = self.PATH_LPC_GRP_MB_CPLD + "/mb_i2c_mux_rst"

        bus=bus_i801

        # Check I2C status,assume
        retcode = os.system("i2cget -f -y {} 0x72 > /dev/null 2>&1".format(bus))
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
            sysfs_bsp_logging = self.PATH_LPC_GRP_BSP+"/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            sysfs_bsp_logging = self.PATH_LPC_GRP_BSP+"/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            sysfs_bsp_logging = self.PATH_LPC_GRP_BSP+"/bsp_pr_info"

        if os.path.exists(sysfs_bsp_logging):
            with open(sysfs_bsp_logging, "w") as f:
                f.write(pr_msg)
        else:
            msg("Warning: bsp logging sys is not exist\n")

    def config_bsp_ver(self, bsp_ver):
        bsp_version_path=self.PATH_LPC_GRP_BSP+"/bsp_version"
        if os.path.exists(bsp_version_path):
            with open(bsp_version_path, "w") as f:
                f.write(bsp_ver)

    def get_board_version(self):
        board = {}
        board_attrs = {
            "hw_rev"  : {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/cpld_hw_rev"},
            "deph_id" : {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/cpld_deph_rev"},
            "hw_build": {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/cpld_build_rev"},
        }

        for key, val in board_attrs.items():
            cmd = ["cat", val["sysfs"]]
            try:
                output = subprocess.check_output(cmd)
            except Exception as e:
                self.bsp_pr("Get hw rev id from LPC failed, exception={}".format(e), self.LEVEL_ERR)
                output="1"
            board[key] = int(output, 10)

        return board

    def get_gpio_max(self):
        cmd = ["cat", self.PATH_BSP_GPIO_MAX]
        try:
            output = subprocess.check_output(cmd)
        except Exception as e:
            self.bsp_pr("Get gpio max failed, exception={}".format(e), self.LEVEL_ERR)
            output="511"

        gpio_max = int(output, 10)
        self.bsp_pr("GPIO MAX: {}".format(gpio_max))
        return gpio_max

    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = self.PATH_SYS_I2C_DEV_ATTR.format(i2c_bus, i2c_addr, "idle_state")
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def init_mux(self, bus_i801):

        i2c_muxs = [
            ('pca9548',       0x71, bus_i801),  #9548_ROOT_FPGA_CPLD
            ('pca9546',       0x72, bus_i801),  #9546_ROOT_CLK
            ('pca9548',       0x73, bus_i801),  #9548_ROOT_PWR
            ('pca9546',       0x76, 12),        #9546_CHILD_CPLD4_5
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

    def init_sys_eeprom(self):
        sys_eeprom = [
            ('sys_eeprom', 0x57, 0),
        ]

        self.new_i2c_devices(sys_eeprom)

    def init_cpld(self):
        cpld = [
            ('s9720_56ed_cpld4', 0x32, 24),
            ('s9720_56ed_cpld5', 0x33, 24),
            ('s9720_56ed_cpld1', 0x30, 1),
            ('s9720_56ed_cpld2', 0x31, 1),
            ('s9720_56ed_fpga' , 0x37, 1),
        ]

        self.new_i2c_devices(cpld)

    def init_eeprom(self):
        data = None
        port_eeprom = {
            0:   {"type": "QSFPDD_NIF" , "bus": 35 , "driver": "optoe3"},
            1:   {"type": "QSFPDD_NIF" , "bus": 36 , "driver": "optoe3"},
            2:   {"type": "QSFPDD_NIF" , "bus": 37 , "driver": "optoe3"},
            3:   {"type": "QSFPDD_NIF" , "bus": 38 , "driver": "optoe3"},
            4:   {"type": "QSFPDD_NIF" , "bus": 39 , "driver": "optoe3"},
            5:   {"type": "QSFPDD_NIF" , "bus": 40 , "driver": "optoe3"},
            6:   {"type": "QSFPDD_NIF" , "bus": 41 , "driver": "optoe3"},
            7:   {"type": "QSFPDD_NIF" , "bus": 42 , "driver": "optoe3"},
            8:   {"type": "QSFPDD_NIF" , "bus": 43 , "driver": "optoe3"},
            9:   {"type": "QSFPDD_NIF" , "bus": 44 , "driver": "optoe3"},
            10:  {"type": "QSFPDD_NIF" , "bus": 45 , "driver": "optoe3"},
            11:  {"type": "QSFPDD_NIF" , "bus": 46 , "driver": "optoe3"},
            12:  {"type": "QSFPDD_NIF" , "bus": 47 , "driver": "optoe3"},
            13:  {"type": "QSFPDD_NIF" , "bus": 48 , "driver": "optoe3"},
            14:  {"type": "QSFPDD_NIF" , "bus": 49 , "driver": "optoe3"},
            15:  {"type": "QSFPDD_NIF" , "bus": 50 , "driver": "optoe3"},
            16:  {"type": "QSFPDD_NIF" , "bus": 51 , "driver": "optoe3"},
            17:  {"type": "QSFPDD_NIF" , "bus": 52 , "driver": "optoe3"},
            18:  {"type": "QSFPDD_NIF" , "bus": 53 , "driver": "optoe3"},
            19:  {"type": "QSFPDD_NIF" , "bus": 54 , "driver": "optoe3"},
            20:  {"type": "QSFPDD_NIF" , "bus": 65 , "driver": "optoe3"},
            21:  {"type": "QSFPDD_NIF" , "bus": 66 , "driver": "optoe3"},
            22:  {"type": "QSFPDD_NIF" , "bus": 67 , "driver": "optoe3"},
            23:  {"type": "QSFPDD_NIF" , "bus": 68 , "driver": "optoe3"},
            24:  {"type": "QSFPDD_NIF" , "bus": 69 , "driver": "optoe3"},
            25:  {"type": "QSFPDD_NIF" , "bus": 70 , "driver": "optoe3"},
            26:  {"type": "QSFPDD_NIF" , "bus": 71 , "driver": "optoe3"},
            27:  {"type": "QSFPDD_NIF" , "bus": 72 , "driver": "optoe3"},
            28:  {"type": "QSFPDD_NIF" , "bus": 73 , "driver": "optoe3"},
            29:  {"type": "QSFPDD_NIF" , "bus": 74 , "driver": "optoe3"},
            30:  {"type": "QSFPDD_NIF" , "bus": 75 , "driver": "optoe3"},
            31:  {"type": "QSFPDD_NIF" , "bus": 76 , "driver": "optoe3"},
            32:  {"type": "QSFPDD_NIF" , "bus": 77 , "driver": "optoe3"},
            33:  {"type": "QSFPDD_NIF" , "bus": 78 , "driver": "optoe3"},
            34:  {"type": "QSFPDD_NIF" , "bus": 79 , "driver": "optoe3"},
            35:  {"type": "QSFPDD_NIF" , "bus": 80 , "driver": "optoe3"},
            36:  {"type": "SFP"        , "bus": 83 , "driver": "optoe2"},
            37:  {"type": "SFP"        , "bus": 84 , "driver": "optoe2"},
            38:  {"type": "QSFPDD_FAB" , "bus": 25 , "driver": "optoe3"},
            39:  {"type": "QSFPDD_FAB" , "bus": 26 , "driver": "optoe3"},
            40:  {"type": "QSFPDD_FAB" , "bus": 27 , "driver": "optoe3"},
            41:  {"type": "QSFPDD_FAB" , "bus": 28 , "driver": "optoe3"},
            42:  {"type": "QSFPDD_FAB" , "bus": 29 , "driver": "optoe3"},
            43:  {"type": "QSFPDD_FAB" , "bus": 30 , "driver": "optoe3"},
            44:  {"type": "QSFPDD_FAB" , "bus": 31 , "driver": "optoe3"},
            45:  {"type": "QSFPDD_FAB" , "bus": 32 , "driver": "optoe3"},
            46:  {"type": "QSFPDD_FAB" , "bus": 33 , "driver": "optoe3"},
            47:  {"type": "QSFPDD_FAB" , "bus": 34 , "driver": "optoe3"},
            48:  {"type": "QSFPDD_FAB" , "bus": 55 , "driver": "optoe3"},
            49:  {"type": "QSFPDD_FAB" , "bus": 56 , "driver": "optoe3"},
            50:  {"type": "QSFPDD_FAB" , "bus": 57 , "driver": "optoe3"},
            51:  {"type": "QSFPDD_FAB" , "bus": 58 , "driver": "optoe3"},
            52:  {"type": "QSFPDD_FAB" , "bus": 59 , "driver": "optoe3"},
            53:  {"type": "QSFPDD_FAB" , "bus": 60 , "driver": "optoe3"},
            54:  {"type": "QSFPDD_FAB" , "bus": 61 , "driver": "optoe3"},
            55:  {"type": "QSFPDD_FAB" , "bus": 62 , "driver": "optoe3"},
            56:  {"type": "QSFPDD_FAB" , "bus": 63 , "driver": "optoe3"},
            57:  {"type": "QSFPDD_FAB" , "bus": 64 , "driver": "optoe3"},
            58:  {"type": "MGMT"       , "bus": 81 , "driver": "optoe2"},
            59:  {"type": "MGMT"       , "bus": 82 , "driver": "optoe2"},     
        }

        with open(self.PATH_PORT_CONFIG, 'r') as yaml_file:
            data = yaml.safe_load(yaml_file)

        # config eeprom
        for port, config in port_eeprom.items():
            addr=0x50
            self.new_i2c_device(config["driver"], addr, config["bus"])
            port_name = data[config["type"]][port]["port_name"]
            sysfs=self.PATH_SYS_I2C_DEV_ATTR.format( config["bus"], addr, "port_name")
            os.system("echo {} > {}".format(port_name, sysfs))

    def init_gpio(self, gpio_max):
        gpio_map = {
            511:{'offset':  0  , 'dir': 'in'   , 'desc': "reserve"},
            510:{'offset': -1  , 'dir': 'low'  , 'desc': "7SEG_RD"},
            509:{'offset': -2  , 'dir': 'low'  , 'desc': "7SEG_RC"},
            508:{'offset': -3  , 'dir': 'low'  , 'desc': "7SEG_RE"},
            507:{'offset': -4  , 'dir': 'low'  , 'desc': "7SEG_RB"},
            506:{'offset': -5  , 'dir': 'high' , 'desc': "7SEG_RG"},
            505:{'offset': -6  , 'dir': 'low'  , 'desc': "7SEG_RF"},
            504:{'offset': -7  , 'dir': 'low'  , 'desc': "7SEG_RA"},
            503:{'offset': -8  , 'dir': 'in'   , 'desc': "reserve"},
            502:{'offset': -9  , 'dir': 'low'  , 'desc': "7SEG_LA"},
            501:{'offset': -10 , 'dir': 'low'  , 'desc': "7SEG_LB"},
            500:{'offset': -11 , 'dir': 'low'  , 'desc': "7SEG_LF"},
            499:{'offset': -12 , 'dir': 'high' , 'desc': "7SEG_LG"},
            498:{'offset': -13 , 'dir': 'low'  , 'desc': "7SEG_LD"},
            497:{'offset': -14 , 'dir': 'low'  , 'desc': "7SEG_LE"},
            496:{'offset': -15 , 'dir': 'low'  , 'desc': "7SEG_LC"},
        }

        self.new_i2c_devices(
            [
                ('pca9555', 0x20, 8), #9555_IO_EXP_TCA9555_1 (9555_LED_BOARD)
            ]
        )

        for _, conf in gpio_map.items():
            gpio_num=gpio_max+conf['offset']
            gpio_dir=conf['dir']
            os.system("echo {} > {}/export".format(gpio_num, self.PATH_SYS_GPIO))
            os.system("echo {} > {}/gpio{}/direction".format(gpio_dir, self.PATH_SYS_GPIO, gpio_num))

    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("Current IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("After IPMI_IOCTL IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

    def disable_bmc_watchdog(self):
        os.system("ipmitool mc watchdog off")

    def set_bmc_sel_time(self):
        os.system("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

    def enable_event_ctrl(self):
        # enable event ctrl
        os.system("echo 1 > "+ self.PATH_CPLD1_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_CPLD2_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_CPLD4_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_CPLD5_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_FPGA_EVT_CTRL)


    def set_system_led_green(self):
        raise NotImplementedError("Not Support")
        # if os.path.exists(self.PATH_SYSTEM_LED):
        #     with open(self.PATH_SYSTEM_LED, "r+") as f:
        #         led_reg = f.read()
        #  
        #         #write green to system led
        #         f.write("{}".format(self.SYSTEM_LED_GREEN))
        #  
        #         self.bsp_pr("Current System LED: {} -> 0x{:02x}".format(led_reg, self.SYSTEM_LED_GREEN))
        # else:
        #     self.bsp_pr("System LED sysfs not exist")
    
    def update_pci_device(self, driver, device, action):
        driver_path = os.path.join("/sys/bus/pci/drivers", driver, action)

        if os.path.exists(driver_path):
            try:
                with open(driver_path, "w") as file:
                    file.write(device)
            except Exception as e:
                print("Open file failed, error: {}".format(e))

    def init_i2c_bus_order(self):
        device_actions = [
            #driver_name   bus_address     action
            ("i801_smbus", "0000:00:1f.4", "unbind"),
            ("ismt_smbus", "0000:00:0f.0", "unbind"),
            ("i801_smbus", "0000:00:1f.4", "bind")
        ]

        # Iterate over the list and call modify_device for each tuple
        for driver_name, bus_address, action in device_actions:
            self.update_pci_device(driver_name, bus_address, action)

    def baseconfig(self):

        # load default kernel driver
        self.init_i2c_bus_order()
        os.system("rmmod gpio_ich")
        os.system("rmmod pinctrl_cedarfork")
        os.system("rmmod ee1004")
        os.system("modprobe i2c_i801")
        os.system("modprobe i2c_dev")
        os.system("modprobe gpio_pca953x")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")

        bus_i801=0

        #lpc driver
        self.insmod("x86-64-ufispace-s9720-56ed-lpc")

        # init interrupt handler for IRQ 17
        self.insmod("x86-64-ufispace-irq-handler", params={"irq_num": 17})

        # version setting
        self.bsp_pr("BSP version {}".format(self.BSP_VERSION))
        self.config_bsp_ver(self.BSP_VERSION)

        # get board version
        board = self.get_board_version()

        # get gpio max
        gpio_max = self.get_gpio_max()
        self.bsp_pr("GPIO MAX: {}".format(gpio_max))

        # check i2c bus status
        self.check_i2c_status(bus_i801)

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        # init MUX sysfs
        self.bsp_pr("Init i2c")
        self.init_mux(bus_i801)

        # init SYS EEPROM devices
        self.bsp_pr("Init sys eeprom")
        self.insmod("x86-64-ufispace-sys-eeprom")
        self.init_sys_eeprom()

        # init CPLD
        self.bsp_pr("Init CPLD")
        self.insmod("x86-64-ufispace-s9720-56ed-cpld", params={"mux_en":1})
        self.init_cpld()

        # enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        # set bmc sel time now
        self.set_bmc_sel_time()

        # init gpio
        self.bsp_pr("Init gpio")
        self.init_gpio(gpio_max)

        # init EEPROM
        self.bsp_pr("Init port eeprom")
        self.insmod("optoe")
        self.init_eeprom()

        self.bsp_pr("Enable event control")
        self.enable_event_ctrl()

        #self.bsp_pr("Init bcm88860")
        self.insmod("intel_auxiliary", False)
        self.insmod("ice")
        # init BCM82399
        os.system("timeout 120s {} init -s 10G".format(self.PATH_EPDM_CLI))

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        self.bsp_pr("Init done")
        return True

