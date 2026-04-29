from struct import *
from ctypes import c_int, sizeof
import yaml
import os
import sys
import subprocess
import errno
import shutil
import copy
import glob
import re
import fcntl
import time
from collections import OrderedDict
import stat

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

def load_platform_config(cls):
    try:
        path = getattr(cls, 'PATH_PLT_CFG', None)
        if path and os.path.exists(path):
            with open(path, 'r') as _f:
                config = yaml.safe_load(_f)
                for key, val in config.items():
                    setattr(cls, key, val)
                path_lpc = getattr(cls, 'PATH_LPC', None)
                if path_lpc:
                    grp_bsp = "{}/bsp".format(path_lpc)
                    grp_cpld = "{}/mb_cpld".format(path_lpc)
                    setattr(cls, 'PATH_LPC_GRP_BSP', grp_bsp)
                    setattr(cls, 'PATH_LPC_GRP_CPLD', grp_cpld)
                    setattr(cls, 'PATH_MUX_RESET_ALL', "{}/mux_reset_all".format(grp_cpld))
                    setattr(cls, 'PATH_BOARD_HW_ID', "{}/board_hw_id".format(grp_cpld))
                    setattr(cls, 'PATH_BOARD_DEPH_ID', "{}/board_deph_id".format(grp_cpld))
                    setattr(cls, 'PATH_BOARD_BUILD_ID', "{}/board_build_id".format(grp_cpld))
                    setattr(cls, 'PATH_I2C_STUCK_STATUS', "{}/i2c_stuck".format(grp_bsp))

    except (IOError, OSError) as e:
            if e.errno == errno.ENOENT:
                pass
            else:
                msg("I/O Exception occurred: {}\n".format(e))
    except Exception as e:
        msg("Exception occurred while loading platform config: {}\n".format(e))

    return cls

class UFispaceBase:
    LEVEL_INFO = 1
    LEVEL_ERR = 2
    PATH_LPC = None
    LEVEL_INFO = 1
    LEVEL_ERR = 2
    PATH_SYS_I2C_DEV_ATTR = "/sys/bus/i2c/devices/{}-{:0>4x}/{}"
    PATH_SYS_GPIO =  "/sys/bus/platform/devices/x86_64_ufispace_gpio/gpio_function"
    PATH_SYSTEM_LED = "/sys_switch/sysled/sys_led_status"
    PATH_PORT_PRESENCE = "/sys_switch/transceiver/eth{}/present"
    PATH_PORT_DEV_CLASS = "/sys_switch/transceiver/eth{}/dev_class"

    I2C_STUCK_STATUS_NORMAL = "0"
    I2C_STUCK_STATUS_ROOT_BUS = "1"
    I2C_STUCK_STATUS_TRANSCEIVER = "2"

    port_type_dict = {
        0x03: [2, 'SFP/SFP+/SFP28'],  # [dev_class, type_str]
        0x0B: [2, 'DWDM-SFP/SFP+'],
        0x0C: [1, 'QSFP'],
        0x0D: [1, 'QSFP+'],
        0x11: [1, 'QSFP28'],
        0x18: [3, 'QSFP-DD Double Density 8x (INF-8628)'],
        0x19: [3, 'OSFP 8x Pluggable Transceiver'],
        0x1E: [3, 'QSFP+ or later with CMIS spec'],
        0x1F: [3, 'SFP-DD Double Density 2X Pluggable Transceiver with CMIS spec'],
    }

    def del_i2c_device(self, addr, bus_number):
        bus = '/sys/bus/i2c/devices/i2c-%d' % bus_number
        devdir = "%d-%4.4x" % (bus_number, addr)
        return self.del_device(addr, bus, devdir)

    def del_i2c_devices(self, del_device_list):
        for (_, addr, bus_number) in del_device_list:
            self.del_i2c_device(addr, bus_number)

    def del_device(self, addr, bus, devdir):
        if os.path.exists(os.path.join(bus, devdir)):
            try:
                with open("%s/delete_device" % bus, "w") as f:
                    f.write("0x%x\n" % (addr))
            except Exception as e:
                print("Unexpected error deleting device 0x%x:%s: %s" % (addr, bus, e))

    def _write(self, path, val, perm="w"):
        if os.path.exists(path):
            try:
                with open(path, perm) as f:
                    f.write(str(val))
            except Exception as e:
                self.bsp_pr("Open file failed, exception={}".format(e))
        else:
            self.bsp_pr("File not found: {}".format(path))

    def is_integer(self, s):
        try:
            int(s)
            return True
        except (ValueError, TypeError):
            return False

    def twos_complement(self, n, w):
        if n & (1 << (w - 1)):
            n = n - (1 << w)
        return n

    def bsp_pr(self, pr_msg, level = None):
        level_default = getattr(self, 'LEVEL_DEFAULT', None)
        if level is None and level_default is None:
            level = self.LEVEL_INFO
        elif level is None:
            level = level_default

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
            msg(pr_msg+"\n")

    def get_board_version(self):
        board = {}
        board_attrs = {
            "hw_rev"  : self.PATH_BOARD_HW_ID,
            "deph_id" : self.PATH_BOARD_DEPH_ID,
            "hw_build": self.PATH_BOARD_BUILD_ID,
        }

        for key, sysfs in board_attrs.items():
            if os.path.exists(sysfs):
                try:
                    with open(sysfs, 'r') as f:
                        val = f.read().strip()
                except Exception as e:
                    self.bsp_pr("Get {} from LPC failed, exception={}".format(key, e), self.LEVEL_ERR)
                    val="0"
            else:
                val="0"
            board[key] = int(val, 0)

        return board

    def load_platform_config(self, board):
        """
        Load platform S3IP Configuration

        Args:
            board: The board SKU information.
        """
        # Load Configuration
        try:
            s3ip_cfg_file = self.PATH_S3IP_CFG
            with open(s3ip_cfg_file, 'r') as f:
                config = yaml.safe_load(f)
            return config
        except IOError:
            self.bsp_pr("FATAL: Configuration file '{}' not found.".format(s3ip_cfg_file))
            return

    def driver_install(self, driver):
        self.insmod(driver)

    def driver_install_params(self, driver, params):
        self.insmod(driver, True, params)

    def driver_install_optional(self, driver):
        self.insmod(driver, False)

    def driver_install_optional_params(self, driver, params):
        self.insmod(driver, False, params)

    def driver_install_modprobe(self, driver):
        os.system("modprobe {}".format(driver))

    def driver_install_modprobe_params(self, driver, params):
        os.system("modprobe {} {}".format(driver, " ".join([ "%s=%s" % (k,v) for (k,v) in params.items() ])))

    def driver_install_insmod_modprobe(self, driver):
        self.insmod(driver, False)
        os.system("modprobe {}".format(driver))

    def driver_install_insmod_modprobe_params(self, driver, params):
        self.insmod(driver, False, params)
        os.system("modprobe {} {}".format(driver, " ".join([ "%s=%s" % (k,v) for (k,v) in params.items() ])))

    def hook_driver_install(self, board, config, driver):
        """
        Hook the platform-specific driver install.

        Args:
            config: The S3IP configuration object/dictionary.
            board: The board SKU information.

        Returns:
            status: Integer indicating result: 0 (success, no params), 1 (success, with params), or -1 (skipped).
            params: The install parameters (valid only if status is 1).
        """
        if (driver['func'] == 'driver_install_params' or
                driver['func'] == 'driver_install_optional_params'):
            return (1, driver['func'], driver['params'])
        else:
            return (0, driver['func'], None)

    def stage_driver_init(self, stage, board=None, config=None):
        for driver in self.DRIVER_SUPPORT:
            if driver['stage'] != stage:
                continue

            status, func_str, params = self.hook_driver_install(board, config, driver)

            if status < 0:
                continue

            driver_name = driver['name']

            func = getattr(self, func_str, None)
            if func:
                if callable(func):
                    if (status == 1):
                        func(driver_name, params)
                    else:
                        func(driver_name)
                else:
                    continue
            else:
                os.system("modprobe {}".format(driver_name))

        # Certain signals (e.g., qsfp_reset) need time to settle after being set.
        # A delay is added here to prevent failures in subsequent operations and ensure
        # the configuration is applied correctly.
        time.sleep(0.5)

    def stage_driver_deinit(self, stage, board=None, config=None):
        for driver in reversed(self.DRIVER_SUPPORT):
            if driver['stage'] != stage:
                continue

            driver_name = driver['name']
            os.system("modprobe -rq {}".format(driver_name))

    def get_i2c_status(self, board):
        """
        Gets the I2C status.

        Args:
            board: The board SKU information.

        Returns:
            0 on success, or an error code on failure.
        """
        if os.path.exists(self.PATH_I2C_STUCK_STATUS):
            with open(self.PATH_I2C_STUCK_STATUS, "r") as f:
                status = f.read().strip()
                # Since the CPLD handles the port stuck status,
                # we only need to monitor the root port stuck status.
                if status == self.I2C_STUCK_STATUS_ROOT_BUS:
                    return -errno.EIO
                else:
                    return 0
        else:
            return -errno.ENOENT

    def check_i2c_status(self, board):
        sysfs_mux_reset = self.PATH_MUX_RESET_ALL

        retcode = self.get_i2c_status(board)

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

    def update_pci_device(self, driver, device, action):
        driver_path = os.path.join("/sys/bus/pci/drivers", driver, action)

        if os.path.exists(driver_path):
            try:
                with open(driver_path, "w") as file:
                    file.write(device)
            except Exception as e:
                print("Open file failed, error: {}".format(e))

    def update_pci_devices(self, device_actions = []):
        # Iterate over the list and call modify_device for each tuple
        for driver_name, bus_address, action in device_actions:
            self.update_pci_device(driver_name, bus_address, action)

    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = self.PATH_SYS_I2C_DEV_ATTR.format(i2c_bus, i2c_addr, "idle_state")
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def start_i2c_bus_order_init(self, board):
        """
        Starts the I2C bus order initialization process.

        Args:
            board: The board SKU information.
        """
        return

    def start_port_eeprom_init(self, config, board):
        port_conf = config['ports']['layer1']['layer2']
        if len(port_conf) > 1:
            self.bsp_pr("Init legacy I2C port eeprom")

        for key, vals in port_conf.items():
            if not isinstance(vals, dict):
                continue

            eeprom_attr = vals['eeprom']

            dev_type = eeprom_attr['dev_type']
            dev_name = eeprom_attr['dev_name']
            dev_config = config[dev_type]
            dev_attr = dev_config[dev_name]

            self.new_i2c_device(eeprom_attr["driver"], dev_attr['addr'], dev_attr["bus"])
            port_name = eeprom_attr["alias"]
            self._write(self.PATH_SYS_I2C_DEV_ATTR.format(dev_attr["bus"], dev_attr['addr'], "port_name"), port_name)

    def start_port_eeprom_deinit(self, config, board):
        port_conf = config['ports']['layer1']['layer2']
        if len(port_conf) > 1:
            self.bsp_pr("Deinit legacy I2C port eeprom")

        for key, vals in port_conf.items():
            if not isinstance(vals, dict):
                continue

            eeprom_attr = vals['eeprom']

            dev_type = eeprom_attr['dev_type']
            dev_name = eeprom_attr['dev_name']
            dev_config = config[dev_type]
            dev_attr = dev_config[dev_name]

            self.del_i2c_device(dev_attr['addr'], dev_attr["bus"])

    def start_mux_init(self, board):
        """
        Starts the I2C mux initialization process.

        Args:
            board: The board SKU information.
        """
        i2c_muxs= []
        self.new_i2c_devices(i2c_muxs)
        self.init_i2c_mux_idle_state(i2c_muxs)

    def start_mux_deinit(self, board):
        return

    def start_sys_eeprom_init(self, board):
        """
        Starts the system eeprom initialization process.

        Args:
            board: The board SKU information.
        """
        sys_eeprom= []
        self.new_i2c_devices(sys_eeprom)

    def start_sys_eeprom_deinit(self, board):
        return

    def start_gpio_init(self, board):
        """
        Starts the GPIO initialization process.

        Args:
            board: The board SKU information.
        """
        gpio_dev = []
        self.new_i2c_devices(gpio_dev)

    def start_gpio_deinit(self, board):
        """
        Starts the GPIO deinitialization process.

        Args:
            board: The board SKU information.
        """
        gpio_dev = []
        self.del_i2c_devices(gpio_dev)

    def start_cpld_init(self, board):
        """
        Starts the CPLD initialization process.

        Args:
            board: The board SKU information.
        """
        cpld = []
        self.new_i2c_devices(cpld)

    def start_cpld_deinit(self, board):
        """
        Starts the CPLD deinitialization process.

        Args:
            board: The board SKU information.
        """
        return

    def start_mac_rov_init(self, board):
        """
        Starts the MAC ROV initialization process.

        Args:
            board: The board SKU information.
        """
        return

    def start_system_led_init(self, board):
        """
        Starts the system led initialization process.

        Args:
            board: The board SKU information.
        """
        if not os.path.exists(self.PATH_SYSTEM_LED):
            return

        try:
            file_stat = os.stat(self.PATH_SYSTEM_LED)
            if not (file_stat.st_mode & stat.S_IWUSR):
                return

            with open(self.PATH_SYSTEM_LED, "r+") as f:
                led = f.read().strip()
                f.seek(0)
                f.write("1")
                f.flush()

                self.bsp_pr("Current System LED: {} -> 1".format(led))
        except (IOError, OSError) as e:
            self.bsp_pr("Failed to init System LED: {}".format(e))

    def start_event_ctrl_init(self, board):
        """
        Starts the event control initialization process.

        Args:
            board: The board SKU information.
        """
        return

    def start_mgmt_phy_init(self, board):
        """
        Starts the management PHY initialization process.

        Args:
            board: The board SKU information.
        """
        return

    # dev_class Functions
    def get_port_presence(self, port):
        try:
            with open(self.PATH_PORT_PRESENCE.format(port), "r") as f:
                raw = f.read().strip()

            val = int(raw, 0)
            status = True if val == 1 else False
            return status
        except:
            return False

    def update_dev_class(self, config, board):
        port_conf = config['ports']['layer1']['layer2']
        for key, vals in port_conf.items():
            if not isinstance(vals, dict):
                continue

            eeprom_attr = vals['eeprom']
            eeprom_attr['type'] = vals['type']['params']

            if eeprom_attr['type'] not in ['QSFPDD', 'QSFP']:
                continue

            # check module presence
            if not self.get_port_presence(key):
                continue

            alias_name = eeprom_attr['alias']
            dev_type = eeprom_attr['dev_type']
            dev_name = eeprom_attr['dev_name']
            dev_config = config[dev_type]
            dev_attr = dev_config[dev_name]

            # get dev_class
            sysfs=self.PATH_PORT_DEV_CLASS.format(key)
            with open(sysfs, "r") as f:
                dev_class_str = f.read().strip()
                dev_class = int(dev_class_str, 0)

            # get port type
            sysfs = self.PATH_SYS_I2C_DEV_ATTR.format(dev_attr["bus"], 0x50, "eeprom")
            cmd = ["dd", "if={}".format(sysfs), "bs=1", "count=1", "skip=0", "status=none"]
            output = subprocess.check_output(cmd)
            hex_str = unpack('B', output)[0]
            type_str = "{:02x}".format(hex_str)
            if type_str == "": #i2c maybe stuck
                self.check_i2c_status(board)
                continue
            port_type = int(type_str, 16)

            # check if port_type is in port_type_dict
            if port_type not in self.port_type_dict:
                self.bsp_pr("Port[{}] Type: {} is Unknown.".format(alias_name, hex(port_type)))
                continue

            sysfs=self.PATH_PORT_DEV_CLASS.format(key)
            # check if dev_class matches port_type_dev_class
            port_type_dev_class = self.port_type_dict.get(port_type)[0]
            if dev_class != port_type_dev_class:
                with open(sysfs, "w") as f:
                    f.write("{}".format(port_type_dev_class))
                self.bsp_pr("Port[{}] dev_class is changed from {} to {}".format(alias_name, dev_class, port_type_dev_class))

        self.bsp_pr("Please run ONLP API onlp_sfpi_dev_class_update() after inserting QSFP/QSFPDD modules at runtime")

    # S3IP Functions
    def ref_link_dev(self, path):
        source = glob.glob(path)
        return source

    def static_link_dev_normal(self, msg):
        return msg

    def static_link_dev_number(self, config, path):
        obj = self.get_obj_from_config(config, path)
        # remove xxxx_dir_type
        return len(obj) - 1

    def static_link_dev_threshold(self, attrs):
        source = glob.glob(attrs['params'])
        new_attrs = copy.deepcopy(attrs)
        if 'offset' not in new_attrs:
            new_attrs['offset'] = -5000

        try:
            offset = int(new_attrs['offset'], 0)
        except (ValueError, TypeError):
            offset = new_attrs['offset']

        try:
            # Only support one element
            sysfs = source[0]
            with open(sysfs, "r") as f:
                val = f.read().strip()
                val = int(val, 0) + offset
                return val
        except Exception as e:
            return "NA"

    def static_link_dev_to_dev_attr(self, config, path):
        obj = self.get_obj_from_config(config, path)
        # remove xxxx_dir_type
        return int(obj)

    def static_link_dev_number_component(self, config, attrs):
        if attrs == 'temperature':
            count = 0
            temp_number_path = config['temperature_sensors']['layer1']['number']['params']
            temp_obj = self.get_obj_from_config(config, temp_number_path)
            count += len(temp_obj) - 1

            psu_number_path = config['psus']['layer1']['number']['params']
            psu_obj = self.get_obj_from_config(config, psu_number_path)
            for psu_key, psu_attrs in psu_obj.items():
                if 'num_temp_sensors' in psu_attrs:
                    psu_temp_number_path = psu_attrs['num_temp_sensors']['params']
                    obj = self.get_obj_from_config(config, psu_temp_number_path)
                    count += len(obj) - 1
            return count
        elif attrs == 'fan':
            count = 0
            fan_tray_number_path = config['fan_trays']['layer1']['number']['params']
            fan_tray_obj = self.get_obj_from_config(config, fan_tray_number_path)
            for fan_tray_key, fan_tray_attrs in fan_tray_obj.items():
                if 'motor_number' in fan_tray_attrs:
                    fan_tray_motor_number_path = fan_tray_attrs['motor_number']['params']
                    obj = self.get_obj_from_config(config, fan_tray_motor_number_path)
                    count += len(obj) - 1

            psu_number_path = config['psus']['layer1']['number']['params']
            psu_obj = self.get_obj_from_config(config, psu_number_path)
            for psu_key, psu_attrs in psu_obj.items():
                if 'motor_number' in psu_attrs:
                    psu_motor_number_path = psu_attrs['motor_number']['params']
                    obj = self.get_obj_from_config(config, psu_motor_number_path)
                    count += len(obj) - 1

                if 'fan_speed' in psu_attrs:
                    count += 1
            return count
        elif attrs == 'psu':
            psu_number_path = config['psus']['layer1']['number']['params']
            obj = self.get_obj_from_config(config, psu_number_path)
            count = len(obj) - 1
            return count
        elif attrs == 'led':
            sysled_number_path = config['sys_leds']['layer1']['number']['params']
            obj = self.get_obj_from_config(config, sysled_number_path)
            count = len(obj) - 1
            return count
        elif attrs == 'sfp':
            sfp_number_path = config['ports']['layer1']['number']['params']
            obj = self.get_obj_from_config(config, sfp_number_path)
            count = len(obj) - 1
            return count

    def get_obj_from_config(self, config, path):
        try:
            keys = path.split('.')
            obj = config[keys[0]]
            for key in keys[1:]:
                if self.is_integer(key):
                    try:
                        obj = obj[int(key)]
                    except KeyError:
                        obj = obj[key]
                else:
                    obj = obj[key]
            return obj
        except Exception as e:
            print("Get object failed, error: {}".format(e))

    def get_source(self, config, attrs):
        dev_type = attrs['dev_type']
        dev_name = attrs['dev_name']
        dev_config = config[dev_type]
        dev_attr = dev_config[dev_name]
        ret = {}
        ret['dev'] = {'dev_type': dev_type, 'dev_name': dev_name}
        if dev_type in ['static_link_dev']:
            if dev_name in ['normal']:
                params = getattr(self, dev_attr['func'])(attrs['params'])
            elif dev_name in ['threshold']:
                params = getattr(self, dev_attr['func'])(attrs)
            elif dev_name in [
                'number',
                'to_attr',
                'number_component'
            ]:
                params = getattr(self, dev_attr['func'])(config, attrs['params'])
            else:
                params = getattr(self, dev_attr['func'])(attrs['params'])
            ret['src_attrs'] =  {'params': params}
        elif dev_type in ['ref_link_dev']:
            if dev_name in ['same_name', 'rename']:
                source = getattr(self, dev_attr['func'])(attrs['source'])
                ret['src_attrs'] =  {'src': source}
            else:
                ret['src_attrs'] = {'src': []}
        else:
            source = attrs['source']
            if dev_type in ['i2c_dev']:
                source_dir = dev_config['path_template'].format(bus=dev_attr['bus'], addr=dev_attr['addr'])
            elif dev_type in ['lpc_dev']:
                source_dir = dev_config['path_template'].format(dev=dev_attr['name'])
            elif dev_type in ['bmc_dev']:
                source_dir = dev_config['path_template'].format(dev=dev_attr['name'])
            elif dev_type in ['gpio_dev']:
                source_dir = dev_config['path_template'].format(dev=dev_attr['name'])
            elif dev_type in ['cpu_temp_dev']:
                source_dir = dev_config['path_template'].format(dev=dev_attr['name'])
            else:
                source_dir = ""
            ret['src_attrs'] = {'src': source, 'src_dir': source_dir}

        return ret

    def pre_target_link(self, target):
        try:
            # Create parent directories
            parent_dir = os.path.dirname(target)

            # Create parent directories if they don't exist.
            # This try/except block handles a race condition if another process
            # creates the directory between the check and the creation.
            if parent_dir and not os.path.exists(parent_dir):
                try:
                    os.makedirs(parent_dir)
                except OSError as e:
                    # Ignore error if the directory already exists
                    if e.errno != errno.EEXIST:
                        raise

            # Remove existing link/file
            # os.path.lexists() is important as it doesn't follow the symlink
            if os.path.lexists(target):
                os.remove(target)
        except Exception as e:
            self.bsp_pr("ERROR: Check target link {} fail). Reason: {}".format(target, e))

    def create_static_link(self, params, target):
        try:
            self.pre_target_link(target)

            # Create the new static link
            with open(target, "w") as f:
                f.write("{}\n".format(params))
                self.bsp_pr("Created link: {} -> static val({})".format(target, params))

            if os.path.exists(target):
                mode = stat.S_IREAD | stat.S_IRGRP | stat.S_IROTH
                os.chmod(target, mode)

        except Exception as e:
            self.bsp_pr("ERROR: Could not create static link {} -> val({}). Reason: {}".format(target, params, e))

    def create_symlink(self, source, target):
        try:
            if not os.path.exists(source):
                self.bsp_pr("Warning: Source not found: {}".format(source))

            self.pre_target_link(target)

            # Create the new symlink
            os.symlink(source, target)
            self.bsp_pr("Created link: {} -> {}".format(target, source))

        except Exception as e:
            self.bsp_pr("ERROR: Could not create link {} -> {}. Reason: {}".format(target, source, e))

    def scan_components(self, config, obj, parent_dir):
        if not isinstance(obj, dict):
            return 0

        if obj.get('normal_dir_type'):
            target_dir = os.path.join(parent_dir, obj.get('normal_dir_type').lstrip('/'))
        elif obj.get('one_param_dir_type'):
            target_dir = os.path.join(parent_dir, obj.get('one_param_dir_type').lstrip('/'))
        else:
            target_dir = parent_dir

        for key, vals in obj.items():
            new_target_dir = target_dir.format(key)

            if not isinstance(vals, dict):
                continue

            dev_type = vals.get('dev_type')
            if dev_type is None:
                self.scan_components(config, vals, new_target_dir)
            else:
                target_file = os.path.join(new_target_dir, key)
                source = self.get_source(config, vals)
                source_dev = source.get('dev', {})
                source_dev_type = source_dev.get('dev_type')
                source_dev_name = source_dev.get('dev_name')
                source_attrs = source.get('src_attrs', {})

                if source_dev_type == 'static_link_dev':
                    self.create_static_link(source_attrs.get('params'), target_file)
                elif source_dev_type == 'ref_link_dev':
                    if source_dev_name == 'same_name':
                        for source_file in source_attrs.get('src', []):
                            file_name = re.sub(r'^(\D+)(\d+)$', r'{}\2'.format(key), os.path.basename(source_file))
                            target_file = os.path.join(new_target_dir, file_name)
                            self.create_symlink(source_file, target_file)
                    elif source_dev_name == 'rename':
                        # Only support one element
                        source_file_list = source_attrs.get('src', [])
                        if source_file_list:
                            source_file = source_file_list[0]
                            self.create_symlink(source_file, target_file)
                else:
                    source_file = os.path.join(source_attrs.get('src_dir', ''), source_attrs.get('src', ''))
                    self.create_symlink(source_file, target_file)

    def init_s3ip(self, config):
        self.init_s3ip_symlink(config)

    def init_s3ip_symlink(self, config):

        sys_switch_path = config['sys_switch_path']
        components = OrderedDict([
            ("temperature_sensors", "Temperature sensors"),
            ("fan_trays", "FANs"),
            ("psus", "PSUs"),
            ("system_links", "System wide"),
            ("ports", "Ports"),
            ("sys_leds", "System LED"),
            ("cplds", "CPLDs"),
            ("vols", "Voltages sensors"),
            ("curr_sensors", "Current sensors"),
            ("fpgas", "FPGAs"),
            ("slots", "Slots"),
        ])

        # Clean up old directory structure
        self.bsp_pr("[S3IP] Cleaning up old directory '{}'...".format(sys_switch_path))
        if os.path.exists(sys_switch_path):
            shutil.rmtree(sys_switch_path)

        # Process components
        for component, desc in components.items():
            self.bsp_pr("[S3IP] Setting up {}...".format(desc))
            component_config = config[component]
            self.scan_components(config, component_config['layer1'], config['sys_switch_path'])

    # BMC Functions
    def get_bmc_support(self):
        if hasattr(self, "BMC_SUPPORT") and self.BMC_SUPPORT == 1:
            return 1
        else:
            return 0

    def check_bmc_support(self):
        bmc_support = self.get_bmc_support()
        self.bsp_pr("bmc support : {}".format(True if bmc_support else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_support" % bmc_support)

    def enable_bmc_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        self.bsp_pr("Current IPMI_MAINTENANCE_MODE={}".format(mode))

        # Currently, maintenance mode is enabled via the Ufispace BMC driver
        # As a precaution, we check and set it again here.
        if mode != IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON:
            ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

            mode=ipmi_ioctl.get_ipmi_maintenance_mode()
            self.bsp_pr("After IPMI_IOCTL IPMI_MAINTENANCE_MODE={}".format(mode))

    def disable_bmc_watchdog(self):
        os.system("ipmitool mc watchdog off")

    def set_bmc_time(self):
        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

    def disable_watchdog(self):
        sysfs_watchdog = "/dev/watchdog"

        if os.path.exists(sysfs_watchdog):
            os.remove(sysfs_watchdog)

    def pre_driver_remove(self):
        """
        Remove driver during pre-init phase
        """
        if self.DRIVER_CLEAN is None:
            return

        for driver in self.DRIVER_CLEAN:
            os.system("modprobe -rq {}".format(driver))

        return

    # Platform Initialization Functions
    def pre_init(self):
        self.pre_driver_remove()
        self.stage_driver_init('pre')
        board = self.get_board_version()
        config = self.load_platform_config(board)
        self.stage_driver_init('pre_phase2', board, config)
        self.start_i2c_bus_order_init(board)
        return {'board': board, 'config': config}

    def pre_deinit(self):
        board = self.get_board_version()
        config = self.load_platform_config(board)
        self.stage_driver_deinit('post', board, config)

        return {'board': board, 'config': config}

    def hook_main_init(self, config, board):
        """
        Starts the platform-specific initialization process.

        Args:
            config: The S3IP configuration object/dictionary.
            board: The board SKU information.
        """
        return

    def hook_main_deinit(self, config, board):
        """
        Starts the platform-specific deinitialization process.

        Args:
            config: The S3IP configuration object/dictionary.
            board: The board SKU information.
        """
        return

    def main_init(self, params):
        board = params['board']
        config = params['config']
        self.check_bmc_support()
        # Check i2c status of root bus (transceiver bus are not ready before mux init)
        self.check_i2c_status(board)
        self.start_mux_init(board)
        # Check i2c status of root bus and transceiver bus after mux init
        self.check_i2c_status(board)
        self.start_sys_eeprom_init(board)
        self.start_cpld_init(board)
        self.start_port_eeprom_init(config, board)
        self.start_gpio_init(board)
        self.hook_main_init(config, board)

    def main_deinit(self, params):
        board = params['board']
        config = params['config']

        self.hook_main_deinit(config, board)
        self.start_gpio_deinit(board)
        self.start_port_eeprom_deinit(config, board)
        self.start_cpld_deinit(board)
        self.start_sys_eeprom_deinit(board)
        self.start_mux_deinit(board)

    def hook_post_init(self, config, board):
        """
        Starts the platform-specific initialization process.

        Args:
            config: The S3IP configuration object/dictionary.
            board: The board SKU information.
        """
        return

    def post_init(self, params):

        board = params['board']
        config = params['config']

        self.stage_driver_init('post', board, config)

         # BMC post init
        if self.get_bmc_support():
            self.enable_bmc_maintenance_mode()
            self.disable_bmc_watchdog()
            self.set_bmc_time()

        self.disable_watchdog()
        self.init_s3ip(config)

        # init dev_class for CMIS/non-CMIS modules
        self.update_dev_class(config, board)

        self.start_mac_rov_init(board)
        self.start_system_led_init(board)
        self.start_event_ctrl_init(board)
        self.start_mgmt_phy_init(board)
        self.hook_post_init(config, board)

    def post_deinit(self, params):
        board = params['board']
        config = params['config']

        self.stage_driver_deinit('pre_phase2', board, config)
        self.stage_driver_deinit('pre')
        self.pre_driver_remove()

    def baseconfig(self):
        params = self.pre_init()
        self.main_init(params)
        self.post_init(params)
        self.bsp_pr("Init done")
        return True

    def basedeconfig(self):
        params = self.pre_deinit()
        self.main_deinit(params)
        self.post_deinit(params)
        self.bsp_pr("Deinit done")
        return True
