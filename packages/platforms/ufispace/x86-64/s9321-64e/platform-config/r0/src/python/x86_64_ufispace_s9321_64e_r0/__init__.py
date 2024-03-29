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

class OnlPlatform_x86_64_ufispace_s9321_64e_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9321-64e-r0'
    MODEL="S9321-64E"
    SYS_OBJECT_ID=".9321.64"
    PORT_COUNT=48
    PORT_CONFIG="40x100 + 8x400"
    LEVEL_INFO=1
    LEVEL_ERR=2
    PATH_SYS_I2C_DEV_ATTR="/sys/bus/i2c/devices/{}-{:0>4x}/{}"
    PATH_SYS_GPIO = "/sys/class/gpio"
    PATH_I2C_CPLD1="/sys/bus/i2c/devices/1-0030"
    PATH_I2C_CPLD2="/sys/bus/i2c/devices/1-0031"
    PATH_I2C_CPLD3="/sys/bus/i2c/devices/1-0032"
    PATH_I2C_FPGA="/sys/bus/i2c/devices/1-0037"
    PATH_SYSTEM_LED=PATH_I2C_CPLD1+"/cpld_system_led_sys"
    PATH_MAC_ROV=PATH_I2C_CPLD1+"/cpld_mac_rov"
    PATH_CPLD1_EVT_CTRL=PATH_I2C_CPLD1+"/cpld_evt_ctrl"
    PATH_CPLD2_EVT_CTRL=PATH_I2C_CPLD2+"/cpld_evt_ctrl"
    PATH_CPLD3_EVT_CTRL=PATH_I2C_CPLD3+"/cpld_evt_ctrl"
    PATH_PORT_LED_CTRL=PATH_I2C_CPLD1+"/cpld_port_led_clr"
    SYSTEM_LED_GREEN=0b00001001
    PATH_LPC="/sys/devices/platform/x86_64_ufispace_s9321_64e_lpc"
    PATH_LPC_GRP_BSP=PATH_LPC+"/bsp"
    PATH_LPC_GRP_MB_CPLD=PATH_LPC+"/mb_cpld"
    PATH_BSP_GPIO_MAX=PATH_LPC_GRP_BSP+"/bsp_gpio_max"
    PATH_MUX_RESET_ALL=PATH_LPC_GRP_MB_CPLD + "/mux_reset_all"
    PATH_BOARD_HW_ID=PATH_LPC_GRP_MB_CPLD+"/board_hw_id"
    PATH_BOARD_DEPH_ID=PATH_LPC_GRP_MB_CPLD+"/board_deph_id"
    PATH_BOARD_BUILD_ID=PATH_LPC_GRP_MB_CPLD+"/board_build_id"
    PATH_PORT_CONFIG="/lib/platform-config/"+PLATFORM+"/onl/port_config.yml"
    PATH_EPDM_CLI="/lib/platform-config/current/onl/epdm_cli"
    PATH_FPGA_PCI_EN=PATH_LPC_GRP_BSP+"/bsp_fpga_pci_enable"
    FPGA_PCI_ENABLE=0

    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self, board):
        sysfs_mux_reset = self.PATH_MUX_RESET_ALL

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
            msg("Warning: bsp logging sys is not exist\n")

    def get_board_version(self):
        board = {}
        board_attrs = {
            "hw_rev"  : {"sysfs": self.PATH_BOARD_HW_ID},
            "deph_id" : {"sysfs": self.PATH_BOARD_DEPH_ID},
            "hw_build": {"sysfs": self.PATH_BOARD_BUILD_ID},
        }

        for key, val in board_attrs.items():
            cmd = "cat {}".format(val["sysfs"])
            status, output = commands.getstatusoutput(cmd)
            if status != 0:
                self.bsp_pr("Get hwr rev id from LPC failed, status={}, output={}, cmd={}".format(status, output, cmd), self.LEVEL_ERR)
                output="1"
            board[key] = int(output, 10)

        return board

    def get_gpio_max(self):
        cmd = "cat " + self.PATH_BSP_GPIO_MAX
        status, output = commands.getstatusoutput(cmd)
        if status != 0:
            self.bsp_pr("Get gpio max failed, status={}, output={}, cmd={}".format(status, output, cmd), self.LEVEL_ERR)
            output="511"

        gpio_max = int(output, 10)

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

    def init_mux(self, board):

        i2c_muxs = [
            ('pca9548', 0x72, 0),  # 9548_CPLD
            ('pca9548', 0x73, 0),  # 9548_DC
        ]

        self.new_i2c_devices(i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)

    def init_sys_eeprom(self, board):
        sys_eeprom = [
            ('sys_eeprom', 0x57, 0),
        ]

        self.new_i2c_devices(sys_eeprom)

    def init_cpld(self, board):
        cpld = [
            ('s9321_64e_cpld1', 0x30, 1),
            ('s9321_64e_cpld2', 0x31, 1),
            ('s9321_64e_cpld3', 0x32, 1),
            ('s9321_64e_fpga' , 0x37, 1),
        ]

        self.new_i2c_devices(cpld)

    def init_eeprom(self, board):
        data = None
        port_eeprom = {
            0:  {"type": "QSFPDD" , "bus": 17, "driver": "optoe3"},
            1:  {"type": "QSFPDD" , "bus": 18, "driver": "optoe3"},
            2:  {"type": "QSFPDD" , "bus": 19, "driver": "optoe3"},
            3:  {"type": "QSFPDD" , "bus": 20, "driver": "optoe3"},
            4:  {"type": "QSFPDD" , "bus": 21, "driver": "optoe3"},
            5:  {"type": "QSFPDD" , "bus": 22, "driver": "optoe3"},
            6:  {"type": "QSFPDD" , "bus": 23, "driver": "optoe3"},
            7:  {"type": "QSFPDD" , "bus": 24, "driver": "optoe3"},
            8:  {"type": "QSFPDD" , "bus": 25, "driver": "optoe3"},
            9:  {"type": "QSFPDD" , "bus": 26, "driver": "optoe3"},
            10: {"type": "QSFPDD" , "bus": 27, "driver": "optoe3"},
            11: {"type": "QSFPDD" , "bus": 28, "driver": "optoe3"},
            12: {"type": "QSFPDD" , "bus": 29, "driver": "optoe3"},
            13: {"type": "QSFPDD" , "bus": 30, "driver": "optoe3"},
            14: {"type": "QSFPDD" , "bus": 31, "driver": "optoe3"},
            15: {"type": "QSFPDD" , "bus": 32, "driver": "optoe3"},
            16: {"type": "QSFPDD" , "bus": 49, "driver": "optoe3"},
            17: {"type": "QSFPDD" , "bus": 50, "driver": "optoe3"},
            18: {"type": "QSFPDD" , "bus": 51, "driver": "optoe3"},
            19: {"type": "QSFPDD" , "bus": 52, "driver": "optoe3"},
            20: {"type": "QSFPDD" , "bus": 53, "driver": "optoe3"},
            21: {"type": "QSFPDD" , "bus": 54, "driver": "optoe3"},
            22: {"type": "QSFPDD" , "bus": 55, "driver": "optoe3"},
            23: {"type": "QSFPDD" , "bus": 56, "driver": "optoe3"},
            24: {"type": "QSFPDD" , "bus": 57, "driver": "optoe3"},
            25: {"type": "QSFPDD" , "bus": 58, "driver": "optoe3"},
            26: {"type": "QSFPDD" , "bus": 59, "driver": "optoe3"},
            27: {"type": "QSFPDD" , "bus": 60, "driver": "optoe3"},
            28: {"type": "QSFPDD" , "bus": 61, "driver": "optoe3"},
            29: {"type": "QSFPDD" , "bus": 62, "driver": "optoe3"},
            30: {"type": "QSFPDD" , "bus": 63, "driver": "optoe3"},
            31: {"type": "QSFPDD" , "bus": 64, "driver": "optoe3"},
            32: {"type": "QSFPDD" , "bus": 33, "driver": "optoe3"},
            33: {"type": "QSFPDD" , "bus": 34, "driver": "optoe3"},
            34: {"type": "QSFPDD" , "bus": 35, "driver": "optoe3"},
            35: {"type": "QSFPDD" , "bus": 36, "driver": "optoe3"},
            36: {"type": "QSFPDD" , "bus": 37, "driver": "optoe3"},
            37: {"type": "QSFPDD" , "bus": 38, "driver": "optoe3"},
            38: {"type": "QSFPDD" , "bus": 39, "driver": "optoe3"},
            39: {"type": "QSFPDD" , "bus": 40, "driver": "optoe3"},
            40: {"type": "QSFPDD" , "bus": 41, "driver": "optoe3"},
            41: {"type": "QSFPDD" , "bus": 42, "driver": "optoe3"},
            42: {"type": "QSFPDD" , "bus": 43, "driver": "optoe3"},
            43: {"type": "QSFPDD" , "bus": 44, "driver": "optoe3"},
            44: {"type": "QSFPDD" , "bus": 45, "driver": "optoe3"},
            45: {"type": "QSFPDD" , "bus": 46, "driver": "optoe3"},
            46: {"type": "QSFPDD" , "bus": 47, "driver": "optoe3"},
            47: {"type": "QSFPDD" , "bus": 48, "driver": "optoe3"},
            48: {"type": "QSFPDD" , "bus": 65, "driver": "optoe3"},
            49: {"type": "QSFPDD" , "bus": 66, "driver": "optoe3"},
            50: {"type": "QSFPDD" , "bus": 67, "driver": "optoe3"},
            51: {"type": "QSFPDD" , "bus": 68, "driver": "optoe3"},
            52: {"type": "QSFPDD" , "bus": 69, "driver": "optoe3"},
            53: {"type": "QSFPDD" , "bus": 70, "driver": "optoe3"},
            54: {"type": "QSFPDD" , "bus": 71, "driver": "optoe3"},
            55: {"type": "QSFPDD" , "bus": 72, "driver": "optoe3"},
            56: {"type": "QSFPDD" , "bus": 73, "driver": "optoe3"},
            57: {"type": "QSFPDD" , "bus": 74, "driver": "optoe3"},
            58: {"type": "QSFPDD" , "bus": 75, "driver": "optoe3"},
            59: {"type": "QSFPDD" , "bus": 76, "driver": "optoe3"},
            60: {"type": "QSFPDD" , "bus": 77, "driver": "optoe3"},
            61: {"type": "QSFPDD" , "bus": 78, "driver": "optoe3"},
            62: {"type": "QSFPDD" , "bus": 79, "driver": "optoe3"},
            63: {"type": "QSFPDD" , "bus": 80, "driver": "optoe3"},
            64: {"type": "MGMT"   , "bus": 81, "driver": "optoe2"},
            65: {"type": "MGMT"   , "bus": 82, "driver": "optoe2"},
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

    def init_gpio(self, gpio_max, board):
        gpio_map_alpha = {
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

        gpio_map_beta = {
            511:{'offset':  0  , 'dir': 'low'   , 'desc': "ID LED"},
        }

        self.new_i2c_devices(
            [
                ('pca9555', 0x20, 4), #9555_IO_EXP_TCA9555_1 (9555_LED_BOARD)
            ]
        )

        if board['hw_rev'] == 1:
            gpio_map = gpio_map_alpha
        else:
            gpio_map = gpio_map_beta

        for _, conf in gpio_map.items():
            gpio_num=gpio_max+conf['offset']
            gpio_dir=conf['dir']
            os.system("echo {} > {}/export".format(gpio_num, self.PATH_SYS_GPIO))
            os.system("echo {}   > {}/gpio{}/direction".format(gpio_dir, self.PATH_SYS_GPIO, gpio_num))

    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        # self.bsp_pr("Current IPMI_MAINTENANCE_MODE={}".format(mode) )

        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        # self.bsp_pr("After IPMI_IOCTL IPMI_MAINTENANCE_MODE={}".format(mode) )

    def disable_bmc_watchdog(self):
        os.system("ipmitool mc watchdog off")

    def set_system_led_green(self):
        if os.path.exists(self.PATH_SYSTEM_LED):
            with open(self.PATH_SYSTEM_LED, "r+") as f:
                led_reg = f.read()

                #write green to system led
                f.write("{}".format(self.SYSTEM_LED_GREEN))

                self.bsp_pr("Current System LED: {} -> 0x{:02x}".format(led_reg, self.SYSTEM_LED_GREEN))
        else:
            self.bsp_pr("System LED sysfs not exist")

    def init_rov(self):
        rov_addrs=[0x64]
        rov_reg=0x21
        rov_bus=13

        rov_avs_array={
            '0x7A': {'vdd_val': '0.85'   ,'vout_cmd':'0x06b2'},
            '0x7C': {'vdd_val': '0.8375' ,'vout_cmd':'0x0699'},
            '0x7E': {'vdd_val': '0.825'  ,'vout_cmd':'0x0681'},
            '0x80': {'vdd_val': '0.8125' ,'vout_cmd':'0x0669'},
            '0x82': {'vdd_val': '0.8'    ,'vout_cmd':'0x0651'},
            '0x84': {'vdd_val': '0.7875' ,'vout_cmd':'0x0639'},
            '0x86': {'vdd_val': '0.775'  ,'vout_cmd':'0x0621'},
            '0x88': {'vdd_val': '0.7625' ,'vout_cmd':'0x0609'},
            '0x8A': {'vdd_val': '0.75'   ,'vout_cmd':'0x05F0'},
            '0x8C': {'vdd_val': '0.7375' ,'vout_cmd':'0x05D8'},
            '0x8E': {'vdd_val': '0.725'  ,'vout_cmd':'0x05C0'},
            '0x90': {'vdd_val': '0.7125' ,'vout_cmd':'0x05A8'},
            '0x92': {'vdd_val': '0.7'    ,'vout_cmd':'0x0590'},
            '0x94': {'vdd_val': '0.6875' ,'vout_cmd':'0x0578'},
            '0x96': {'vdd_val': '0.675'  ,'vout_cmd':'0x055F'},
            '0x98': {'vdd_val': '0.6625' ,'vout_cmd':'0x0547'},
            '0x9A': {'vdd_val': '0.65'   ,'vout_cmd':'0x052F'}
        }

        #get rov from cpld
        reg_val_str = subprocess.check_output("cat {}".format(self.PATH_MAC_ROV), shell=True)
        reg_val = int(reg_val_str, 0)
        self.bsp_pr("{}={}".format(self.PATH_MAC_ROV, reg_val_str))

        for index, rov_addr in enumerate(rov_addrs):
            val_key="0x{:0>2X}".format(reg_val)
            if val_key in rov_avs_array:
                mac_vdd_val=rov_avs_array[val_key]['vdd_val']
                rov_reg_val=rov_avs_array[val_key]['vout_cmd']
                self.bsp_pr("Setting mac[{}] vdd {} with rov register value {}".format(index, mac_vdd_val, rov_reg_val) )
                os.system("i2cset -y {} {} {} {} w".format(rov_bus, rov_addr, rov_reg, rov_reg_val))

    def enable_event_ctrl(self, board):
        # enable event ctrl
        os.system("echo 1 > "+ self.PATH_CPLD1_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_CPLD2_EVT_CTRL)
        os.system("echo 1 > "+ self.PATH_CPLD3_EVT_CTRL)

    def enable_port_led_ctrl(self, board):
        # port led enable
        os.system("echo 1 > "+ self.PATH_PORT_LED_CTRL)

    def config_fpga_pci_enable(self, en):
        path=self.PATH_FPGA_PCI_EN
        if os.path.exists(path):
            with open(path, "w") as f:
                f.write("{}".format(en))

    def baseconfig(self):

        # load default kernel driver
        os.system("modprobe i2c_i801")
        os.system("modprobe i2c_dev")
        os.system("modprobe i2c_mux")
        os.system("modprobe gpio_pca953x")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe lm75")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")

        self.insmod("x86-64-ufispace-s9321-64e-lpc")

        board = self.get_board_version()

        gpio_max = self.get_gpio_max()
        self.bsp_pr("GPIO MAX: {}".format(gpio_max))

        self.check_i2c_status(board)

        bmc_enable = self.check_bmc_enable()
        self.bsp_pr("bmc enable : {}".format(True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        self.bsp_pr("Init I2C")
        self.init_mux(board)

        self.bsp_pr("Init sys eeprom")
        self.insmod("x86-64-ufispace-sys-eeprom")
        self.init_sys_eeprom(board)

        self.bsp_pr("Init CPLD")
        self.config_fpga_pci_enable(self.FPGA_PCI_ENABLE)

        if self.FPGA_PCI_ENABLE == 1:
            self.insmod("x86-64-ufispace-s9321-64e-cpld")
        else:
            self.insmod("x86-64-ufispace-s9321-64e-cpld", params={'mux_en': 1})
        
        self.init_cpld(board)

        # init EEPROM
        if self.FPGA_PCI_ENABLE == 1:
            self.bsp_pr("Init FPGA PCI port eeprom")
            os.system("setpci -s 18:00.0 COMMAND=0x02")
        else:
            self.bsp_pr("Init legacy I2C port eeprom")
            self.insmod("optoe")
            self.init_eeprom(board)

        self.bsp_pr("Init gpio");
        self.init_gpio(gpio_max, board)

        self.enable_ipmi_maintenance_mode()

        self.disable_bmc_watchdog()

        self.bsp_pr("Enable event control");
        self.enable_event_ctrl(board)

        self.bsp_pr("Enable port led control");
        self.enable_port_led_ctrl(board)

        # init ice (need to have ice before bcm81381 init to avoid failure)
        self.bsp_pr("Init ice")
        self.insmod("intel_auxiliary", False)
        self.insmod("ice")

        # init bcm82399
        self.bsp_pr("Init bcm82399")
        os.system("timeout 120s {} init -s 10G".format(self.PATH_EPDM_CLI))

        self.bsp_pr("Init done")

        return True