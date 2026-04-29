from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
import os
import sys
import subprocess
import fcntl
import yaml

# Standard error messages
def msg(s, fatal=False):
    sys.stderr.write(s)
    sys.stderr.flush()
    if fatal:
        sys.exit(1)


class OnlPlatform_x86_64_ufispace_s9511_20ct_r0(OnlPlatformUfiSpace):
    # Meta Information
    PLATFORM='x86-64-ufispace-s9511-20ct-r0'
    MODEL="S9511-20CT"
    SYS_OBJECT_ID=".9511.20"
    PORT_COUNT=20
    PORT_CONFIG="4x1 + 8x10 + 8x25"
    LEVEL_INFO=1
    LEVEL_ERR=2
    BSP_VERSION='0.1.51'

    # Path
    PATH_SYS_I2C_DEV_ATTR="/sys/bus/i2c/devices/{}-{:0>4x}/{}"
    PATH_LPC="/sys/devices/platform/x86_64_ufispace_s9511_20ct_lpc"
    PATH_LPC_GRP_BSP=PATH_LPC+"/bsp"
    PATH_LPC_GRP_MB_CPLD=PATH_LPC+"/mb_cpld"
    PATH_LPC_GRP_EC=PATH_LPC+"/ec"
    PATH_PORT_CONFIG="/lib/platform-config/current/onl/port_config.yml"
    PATH_FILE_PSU_TYPE="/tmp/psu_type"
    PATH_STAT_LED_BlINKING_SYSFS="/sys/bus/i2c/devices/10-0033/sys_led_blinking"
    PATH_STAT_LED_COLOR_SYSFS="/sys/bus/i2c/devices/10-0033/sys_led_color"

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


    def check_i2c_status(self, bus_i801):
        sysfs_mux_reset = self.PATH_LPC_GRP_MB_CPLD + "/mux_reset_all"

        bus=bus_i801

        # Check I2C status,assume
        retcode = os.system("i2cget -f -y {} 0x76 > /dev/null 2>&1".format(bus))
        if retcode != 0:

            # read mux failed, i2c bus may be stuck
            msg("Warning: Read I2C Mux Failed!! (ret=%d)\n" % (retcode))

            # Recovery I2C
            if os.path.exists(sysfs_mux_reset):
                with open(sysfs_mux_reset, "w") as f:
                    #write 0 to sysfs
                    f.write("{}".format(0))
                    msg("I2C bus recovery done.\n")
            else:
                msg("Warning: I2C recovery sysfs does not exist!! (path=%s)\n" % (sysfs_mux_reset))


    # BSP version settings
    def config_bsp_ver(self, bsp_ver):
            bsp_version_path=self.PATH_LPC_GRP_BSP+"/bsp_version"
            if os.path.exists(bsp_version_path):
                with open(bsp_version_path, "w") as f:
                    f.write(bsp_ver)


    def get_board_version(self):
        board = {}
        board_attrs = {
            "hw_rev": {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/hw_rev"},
            "deph_id": {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/deph_id"},
            "hw_build": {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/build_id"},
            "ext_id": {"sysfs": self.PATH_LPC_GRP_MB_CPLD+"/extend_id"},
        }

        for key, val in board_attrs.items():
            cmd = "cat {}".format(val["sysfs"])
            output = ""
            try:
                output = subprocess.check_output(cmd.split())
            except Exception as e:
                msg("get_board_version() failed, exception={}\n".format(e))
                msg("Use default value -1\n")
                output="-1"
            board[key] = int(output, 16)

        return board

    def get_psu_type(self):
        cmd = "cat {}/psu_type".format(self.PATH_LPC_GRP_MB_CPLD)
        output = subprocess.check_output(cmd.split())
        hex_value_str = output.decode().strip()
        psu_type = int(hex_value_str, 16)

        self.bsp_pr("Save PSU type ({}) into {}".format(psu_type, self.PATH_FILE_PSU_TYPE))
        os.system("echo {} > {}".format(psu_type, self.PATH_FILE_PSU_TYPE))

        if psu_type == 1:
            self.bsp_pr("Check PSU type done! PSU type = {} is AC PSU".format(psu_type))
        elif psu_type == 0:
            self.bsp_pr("Check PSU type done! PSU type = {} is DC PSU".format(psu_type))
        else:
            self.bsp_pr("Check PSU type done! Unknown PSU type = {}".format(psu_type), self.LEVEL_ERR)

        return psu_type

    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = self.PATH_SYS_I2C_DEV_ATTR.format(i2c_bus, i2c_addr, "idle_state")
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))


    def init_mux(self, bus_i801, bus_ismt):
        os.system("modprobe i2c_mux_pca954x")
        i2c_muxs = [
            ('pca9546', 0x75, bus_ismt),
            ('pca9548', 0x76, bus_i801),
            ('pca9548', 0x73, 7),  #SFP P0~3/ SFP+ P4~7
            ('pca9548', 0x73, 8),  #SFP+ P8~15
            ('pca9546', 0x73, 9),  #SFP28 P16~19
        ]

        self.new_i2c_devices(i2c_muxs)

        # init idel state on mux
        self.init_i2c_mux_idle_state(i2c_muxs)


    def init_sys_eeprom(self, bus_ismt):
        sys_eeprom = [
            ('sys_eeprom', 0x57, bus_ismt),
        ]

        self.new_i2c_devices(sys_eeprom)


    def init_cpld(self):
        cpld = [
            ('s9511_20ct_cpld1', 0x33, 10),
            ('s9511_20ct_cpld2', 0x27, 6),
        ]

        self.new_i2c_devices(cpld)


    def init_temp(self):
        os.system("modprobe lm75")
        os.system("modprobe lm90")
        temp = [
            ('tmp75'    ,0x49   ,10),
            ('tmp451'   ,0x4E   ,10),
        ]
        self.new_i2c_devices(temp)


    def init_eeprom(self):
        data = None
        port_eeprom = {
            0:  {"type":"SFP",     "bus":14,   "driver":"optoe2"},  #SFP P0
            1:  {"type":"SFP",     "bus":15,   "driver":"optoe2"},  #SFP P1
            2:  {"type":"SFP",     "bus":16,   "driver":"optoe2"},  #SFP P2
            3:  {"type":"SFP",     "bus":17,   "driver":"optoe2"},  #SFP P3
            4:  {"type":"SFP+",    "bus":18,   "driver":"optoe2"},  #SFP+ P4
            5:  {"type":"SFP+",    "bus":19,   "driver":"optoe2"},  #SFP+ P5
            6:  {"type":"SFP+",    "bus":20,   "driver":"optoe2"},  #SFP+ P6
            7:  {"type":"SFP+",    "bus":21,   "driver":"optoe2"},  #SFP+ P7
            8:  {"type":"SFP+",    "bus":22,   "driver":"optoe2"},  #SFP+ P8
            9:  {"type":"SFP+",    "bus":23,   "driver":"optoe2"},  #SFP+ P9
            10: {"type":"SFP+",    "bus":24,   "driver":"optoe2"},  #SFP+ P10
            11: {"type":"SFP+",    "bus":25,   "driver":"optoe2"},  #SFP+ P11
            12: {"type":"SFP28",   "bus":26,   "driver":"optoe2"},  #SFP+ P12
            13: {"type":"SFP28",   "bus":27,   "driver":"optoe2"},  #SFP+ P13
            14: {"type":"SFP28",   "bus":28,   "driver":"optoe2"},  #SFP+ P14
            15: {"type":"SFP28",   "bus":29,   "driver":"optoe2"},  #SFP+ P15
            16: {"type":"SFP28",   "bus":30,   "driver":"optoe2"},  #SFP28 P16
            17: {"type":"SFP28",   "bus":31,   "driver":"optoe2"},  #SFP28 P17
            18: {"type":"SFP28",   "bus":32,   "driver":"optoe2"},  #SFP28 P18
            19: {"type":"SFP28",   "bus":33,   "driver":"optoe2"},  #SFP28 P19
        }

        with open(self.PATH_PORT_CONFIG, 'r') as yaml_file:
            data = yaml.safe_load(yaml_file)

        # config eeprom
        for port, config in port_eeprom.items():
            addr=0x50
            self.new_i2c_device(config["driver"], addr, config["bus"])
            port_name = data[config["type"]][port]["port_name"]
            sysfs=self.PATH_SYS_I2C_DEV_ATTR.format(config["bus"], addr, "port_name")
            self._write(sysfs, port_name)

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
            ("ismt_smbus", "0000:00:12.0", "unbind"),
            ("i801_smbus", "0000:00:1f.4", "bind"),
            ("ismt_smbus", "0000:00:12.0", "bind"),
        ]
        # Iterate over the list and call modify_device for each tuple
        for driver_name, bus_address, action in device_actions:
            self.update_pci_device(driver_name, bus_address, action)

    def _write(self, path, val, perm="w"):
        if os.path.exists(path):
            try:
                with open(path, perm) as f:
                    f.write(str(val))
            except Exception as e:
                self.bsp_pr("Open file failed, exception={}".format(e))
        else:
            self.bsp_pr("File not found: {}".format(path))

    def baseconfig(self):
        # Load default kernel drivers
        os.system("modprobe i2c_i801")
        os.system("modprobe i2c_ismt")
        os.system("modprobe i2c_dev")
        os.system("modprobe coretemp")
        self.init_i2c_bus_order()

        # Init bus number
        bus_i801=0
        bus_ismt=1

        # Load LPC driver
        self.insmod("x86-64-ufispace-s9511-20ct-lpc")

        # Show BSP version setting
        self.bsp_pr("BSP version {}".format(self.BSP_VERSION))
        self.config_bsp_ver(self.BSP_VERSION)

        # Get hardware (board) revision
        #board = self.get_board_version()

        # Check i2c bus status
        self.check_i2c_status(bus_i801)

        # Init MUX sysfs
        self.bsp_pr("Init i2c")
        self.init_mux(bus_i801, bus_ismt)

        # Init SYS EEPROM devices
        self.bsp_pr("Init sys eeprom")
        self.insmod("x86-64-ufispace-sys-eeprom")
        self.init_sys_eeprom(bus_ismt)

        # Load CPLD
        self.bsp_pr("Load CPLD Driver")
        self.insmod("x86-64-ufispace-s9511-20ct-cpld")

        self.bsp_pr("Init CPLD")
        self.init_cpld()

        # Init temperature
        self.bsp_pr("Init TEMP")
        self.init_temp()

        # Init Port EEPROM
        self.bsp_pr("Init port eeprom")
        self.insmod("optoe")
        self.init_eeprom()

        # Check PSU Type
        self.bsp_pr("Check PSU Type")
        psu_type = self.get_psu_type()
        msg("PSU_TYPE = %d\n" % psu_type)

        # Init PSU eeprom
        # AC PSU has two psu slots(0x51, 0x53), while DC PSU is only one dual PSU slot(0x51)
        # [0: DC PSU/ 1: AC PSU]
        if psu_type == 1:
            self.new_i2c_device('sys_eeprom', 0x51, 13)
            self.new_i2c_device('sys_eeprom', 0x53, 13)
            self.bsp_pr("PSU Type 1 (AC PSU), PSU EEPROM 0x51, 0x53")
        elif psu_type == 0:
            #self.new_i2c_device('sys_eeprom', 0x51, 13)
            self.bsp_pr("PSU Type 0 (DC PSU), PSU EEPROM 0x51")
        else:
            self.bsp_pr("Unknown PSU Type {}".format(psu_type), self.LEVEL_ERR)

        # Set Front Panel STAT LED to green_solid
        no_blinking=0
        color=1
        os.system("echo {} > {}".format(color, self.PATH_STAT_LED_COLOR_SYSFS))
        os.system("echo {} > {}".format(no_blinking, self.PATH_STAT_LED_BlINKING_SYSFS))

        # enable event ctrl
        self._write("/sys/bus/i2c/devices/10-0033/event_ctrl", 1)
        self._write("/sys/bus/i2c/devices/6-0027/event_ctrl", 1)

        # Finished
        self.bsp_pr("Init done")

        return True
