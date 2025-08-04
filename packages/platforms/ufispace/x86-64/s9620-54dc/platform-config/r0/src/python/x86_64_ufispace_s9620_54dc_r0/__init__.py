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

class OnlPlatform_x86_64_ufispace_s9620_54dc_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9620-54dc-r0'
    MODEL="S9620-54DC"
    SYS_OBJECT_ID=".9620.54"
    PORT_COUNT=54
    PORT_CONFIG="40x10 + 8x50 + 4x100 + 2x400"
    LEVEL_INFO=1
    LEVEL_ERR=2
    BSP_VERSION='1.0.0'
    PATH_I2C_CPLD1="/sys/bus/i2c/devices/2-0030"
    PATH_I2C_CPLD2="/sys/bus/i2c/devices/2-0031"
    PATH_I2C_CPLD3="/sys/bus/i2c/devices/2-0032"
    PATH_I2C_FPGA="/sys/bus/i2c/devices/2-0037"
    PATH_SYS_I2C_DEV_ATTR="/sys/bus/i2c/devices/{}-{:0>4x}/{}"
    PATH_SYS_GPIO = "/sys/class/gpio"
    PATH_SYSTEM_LED="/sys/bus/i2c/devices/2-0030/system_led_status"
    SYSTEM_LED_GREEN=0b00001001
    PATH_LPC="/sys/devices/platform/x86_64_ufispace_s9620_54dc_lpc"
    PATH_LPC_GRP_BSP=PATH_LPC+"/bsp"
    PATH_BSP_GPIO_BASE=PATH_LPC_GRP_BSP+"/bsp_gpio_base"
    PATH_LPC_GRP_MB_CPLD=PATH_LPC+"/mb_cpld"
    PATH_PORT_CONFIG="/lib/platform-config/"+PLATFORM+"/onl/port_config.yml"
    PATH_EPDM_CLI="/lib/platform-config/"+PLATFORM+"/onl/epdm_cli"
    PATH_CPLD1_EVT_CTRL=PATH_I2C_CPLD1+"/event_detect_ctrl"
    PATH_CPLD2_EVT_CTRL=PATH_I2C_CPLD2+"/event_detect_ctrl"
    PATH_CPLD3_EVT_CTRL=PATH_I2C_CPLD3+"/event_detect_ctrl"
    PATH_FPGA_EVT_CTRL=PATH_I2C_FPGA+"/event_detect_ctrl"
    PATH_BSP_GPIO_MAX=PATH_LPC_GRP_BSP+"/bsp_gpio_max"


    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self):
        sysfs_mux_reset = self.PATH_LPC_GRP_MB_CPLD + "/mux_reset_all"

        # Check I2C status
        retcode = os.system("i2cget -f -y 0 0x72 > /dev/null 2>&1")
        if retcode != 0:

            #read mux failed, i2c bus may be stuck
            self.bsp_pr("Warning: Read I2C Mux Failed!! (ret={})".format(retcode))

            # Not supporting I2C recovery
            if 0:
                #Recovery I2C
                if os.path.exists(sysfs_mux_reset):
                    with open(sysfs_mux_reset, "w") as f:
                        #write 0 to sysfs
                        f.write("{}".format(0))
                        self.bsp_pr("I2C bus recovery done.")
                else:
                    self.bsp_pr("Warning: I2C recovery sysfs does not exist!! (path={})".format(sysfs_mux_reset))


    def bsp_pr(self, pr_msg, level = LEVEL_INFO):
        if level == self.LEVEL_INFO:
            bsp_pr = self.PATH_LPC_GRP_BSP+"/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            bsp_pr = self.PATH_LPC_GRP_BSP+"/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            bsp_pr = self.PATH_LPC_GRP_BSP+"/bsp_pr_info"

        if os.path.exists(bsp_pr):
            with open(bsp_pr, "w") as f:
                f.write(pr_msg)
        else:
            msg("Warning: bsp_pr sysfs does not exist\n")

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
    
    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = self.PATH_SYS_I2C_DEV_ATTR.format(i2c_bus, i2c_addr, "idle_state")
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f: 
                    f.write(str(IDLE_STATE_DISCONNECT))

    def get_gpio_max(self):
        cmd = " ".join(["cat", self.PATH_BSP_GPIO_MAX])
        output = ""
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("get_gpio_max() failed, exception={}".format(e), self.LEVEL_ERR)
            self.bsp_pr("Use default GPIO MAX value -1", self.LEVEL_ERR)
            output="-1"

        gpio_max = int(output, 10)
        self.bsp_pr("GPIO MAX: {}".format(gpio_max))

        return gpio_max

    def get_gpio_base(self):
        cmd = " ".join(["cat", self.PATH_BSP_GPIO_BASE])
        output = ""
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("get_gpio_base() failed, exception={}".format(e), self.LEVEL_ERR)
            self.bsp_pr("Use default GPIO Base value -1", self.LEVEL_ERR)
            output="-1"

        gpio_base = int(output, 10)
        self.bsp_pr("GPIO Base: {}".format(gpio_base))

        return gpio_base

    def init_gpio(self):
        #get gpio_max/gpio_base
        gpio_max = self.get_gpio_max()
        gpio_base = self.get_gpio_base()

        return

    def init_mux(self, bus_i801):

        i2c_muxs = [
            ('pca9548',   0x72, bus_i801),  #9548_MUX1
            ('pca9548',   0x73, bus_i801),  #9548_MUX2
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

    def init_sys_eeprom(self):
        sys_eeprom = [
            ('sys_eeprom', 0x57, 1),
        ]

        self.new_i2c_devices(sys_eeprom)

    def init_cpld(self):
        cpld = [
            ('s9620_54dc_cpld1', 0x30, 2),
            ('s9620_54dc_cpld2', 0x31, 2),
            ('s9620_54dc_cpld3', 0x32, 2),
            ('s9620_54dc_fpga' , 0x37, 2),
        ]

        self.new_i2c_devices(cpld)

    def init_eeprom(self):
        data = None
        port_eeprom = {
            0:   {"type": "SFP"        , "bus": 18 , "driver": "optoe2"},
            1:   {"type": "SFP"        , "bus": 19 , "driver": "optoe2"},
            2:   {"type": "SFP"        , "bus": 20 , "driver": "optoe2"},
            3:   {"type": "SFP"        , "bus": 21 , "driver": "optoe2"},
            4:   {"type": "SFP"        , "bus": 22 , "driver": "optoe2"},
            5:   {"type": "SFP"        , "bus": 23 , "driver": "optoe2"},
            6:   {"type": "SFP"        , "bus": 24 , "driver": "optoe2"},
            7:   {"type": "SFP"        , "bus": 25 , "driver": "optoe2"},
            8:   {"type": "SFP"        , "bus": 26 , "driver": "optoe2"},
            9:   {"type": "SFP"        , "bus": 27 , "driver": "optoe2"},
            10:  {"type": "SFP"        , "bus": 28 , "driver": "optoe2"},
            11:  {"type": "SFP"        , "bus": 29 , "driver": "optoe2"},
            12:  {"type": "SFP"        , "bus": 30 , "driver": "optoe2"},
            13:  {"type": "SFP"        , "bus": 31 , "driver": "optoe2"},
            14:  {"type": "SFP"        , "bus": 32 , "driver": "optoe2"},
            15:  {"type": "SFP"        , "bus": 33 , "driver": "optoe2"},
            16:  {"type": "SFP"        , "bus": 34 , "driver": "optoe2"},
            17:  {"type": "SFP"        , "bus": 35 , "driver": "optoe2"},
            18:  {"type": "SFP"        , "bus": 36 , "driver": "optoe2"},
            19:  {"type": "SFP"        , "bus": 37 , "driver": "optoe2"},
            20:  {"type": "SFP"        , "bus": 38 , "driver": "optoe2"},
            21:  {"type": "SFP"        , "bus": 39 , "driver": "optoe2"},
            22:  {"type": "SFP"        , "bus": 40 , "driver": "optoe2"},
            23:  {"type": "SFP"        , "bus": 41 , "driver": "optoe2"},
            24:  {"type": "SFP"        , "bus": 42 , "driver": "optoe2"},
            25:  {"type": "SFP"        , "bus": 43 , "driver": "optoe2"},
            26:  {"type": "SFP"        , "bus": 44 , "driver": "optoe2"},
            27:  {"type": "SFP"        , "bus": 45 , "driver": "optoe2"},
            28:  {"type": "SFP"        , "bus": 46 , "driver": "optoe2"},
            29:  {"type": "SFP"        , "bus": 47 , "driver": "optoe2"},
            30:  {"type": "SFP"        , "bus": 48 , "driver": "optoe2"},
            31:  {"type": "SFP"        , "bus": 49 , "driver": "optoe2"},
            32:  {"type": "SFP"        , "bus": 50 , "driver": "optoe2"},
            33:  {"type": "SFP"        , "bus": 51 , "driver": "optoe2"},
            34:  {"type": "SFP"        , "bus": 52 , "driver": "optoe2"},
            35:  {"type": "SFP"        , "bus": 53 , "driver": "optoe2"},
            36:  {"type": "SFP"        , "bus": 54 , "driver": "optoe2"},
            37:  {"type": "SFP"        , "bus": 55 , "driver": "optoe2"},
            38:  {"type": "SFP"        , "bus": 56 , "driver": "optoe2"},
            39:  {"type": "SFP"        , "bus": 57 , "driver": "optoe2"},
            40:  {"type": "SFP"        , "bus": 58 , "driver": "optoe2"},
            41:  {"type": "SFP"        , "bus": 59 , "driver": "optoe2"},
            42:  {"type": "SFP"        , "bus": 60 , "driver": "optoe2"},
            43:  {"type": "SFP"        , "bus": 61 , "driver": "optoe2"},
            44:  {"type": "SFP"        , "bus": 62 , "driver": "optoe2"},
            45:  {"type": "SFP"        , "bus": 63 , "driver": "optoe2"},
            46:  {"type": "SFP"        , "bus": 64 , "driver": "optoe2"},
            47:  {"type": "SFP"        , "bus": 65 , "driver": "optoe2"},
            48:  {"type": "QSFP"       , "bus": 66 , "driver": "optoe1"},
            49:  {"type": "QSFP"       , "bus": 67 , "driver": "optoe1"},
            50:  {"type": "QSFP"       , "bus": 68 , "driver": "optoe1"},
            51:  {"type": "QSFP"       , "bus": 69 , "driver": "optoe1"},
            52:  {"type": "QSFPDD"     , "bus": 70 , "driver": "optoe3"},
            53:  {"type": "QSFPDD"     , "bus": 71 , "driver": "optoe3"},
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


    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        # self.bsp_pr("Current IPMI_MAINTENANCE_MODE={}".format(mode) )

        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        # self.bsp_pr("After IPMI_IOCTL IPMI_MAINTENANCE_MODE={}".format(mode) )

    def disable_bmc_watchdog(self):
        os.system("ipmitool mc watchdog off")
    
    def set_bmc_sel_time(self):
        os.system("ipmitool sel time set now > /dev/null 2>&1")

    def enable_event_ctrl(self):
        # enable event ctrl
        os.system("echo 1 > "+ self.PATH_CPLD1_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_CPLD2_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_CPLD3_EVT_CTRL)
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
            ("i801_smbus", "0000:00:1f.4", "bind"),
            ("ismt_smbus", "0000:00:0f.0", "bind")
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
        self.insmod("i2c-smbus", False)
        os.system("modprobe i2c_i801")
        self.insmod("i2c-ismt", False)
        os.system("modprobe i2c_ismt")
        os.system("modprobe i2c_dev")
        os.system("modprobe gpio_pca953x")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")

        bus_i801=0

        #lpc driver
        self.insmod("x86-64-ufispace-s9620-54dc-lpc")

        # init interrupt handler for IRQ 17
        self.insmod("x86-64-ufispace-irq-handler", params={"irq_num": 17})

        # version setting
        self.bsp_pr("BSP version {}".format(self.BSP_VERSION))
        self.config_bsp_ver(self.BSP_VERSION)

        # get board version
        board = self.get_board_version()

        # check i2c bus status 
        # self.check_i2c_status() # Q3U Not Support

        bmc_enable = self.check_bmc_enable()
        self.bsp_pr("bmc enable : {}".format(True if bmc_enable else False))

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
        self.insmod("x86-64-ufispace-s9620-54dc-cpld", params={'mux_en':1})
        self.init_cpld()

        #enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        # set bmc sel time now
        self.set_bmc_sel_time()

        # init gpio
        # self.bsp_pr("Init gpio")
        # self.init_gpio()

        # init EEPROM
        self.bsp_pr("Init port eeprom")
        self.insmod("optoe")
        self.init_eeprom()

        self.bsp_pr("Enable event control")
        self.enable_event_ctrl()

        #self.bsp_pr("Init bcm88860")
        self.insmod("intel_auxiliary", False)
        self.insmod("ice")
        # init bcm82399
        # os.system("timeout 120s {} init -s auto_25G -d mdio".format(self.PATH_EPDM_CLI))

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        self.bsp_pr("Init done")
        return True

