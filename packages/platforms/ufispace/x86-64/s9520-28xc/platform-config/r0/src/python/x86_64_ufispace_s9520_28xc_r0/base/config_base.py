import os
import yaml
import subprocess

class UFispacePlatformBase:
    PATH_CPLD1_EVT_CTRL = "/sys/bus/i2c/devices/2-0032/event_detect_ctrl"
    PATH_CPLD2_EVT_CTRL = "/sys/bus/i2c/devices/2-0033/event_detect_ctrl"
    PATH_CPLD1_SKU_ID   = "/sys/bus/i2c/devices/2-0032/cpld_sku_id"
    PATH_BOARD_SKU_ID   = "/sys/bus/platform/devices/x86_64_ufispace_s9520_28xc_lpc/mb_cpld/board_sku_id"

    # CPU SKU ID Raw Values
    SKU_DENVERTON = 0xED  # 1110_1101
    SKU_LCC       = 0xEB  # 1110_1011

    # CPU Type
    CPU_TYPE_UNKNOWN   = -1
    CPU_TYPE_DENVERTON = 1
    CPU_TYPE_LCC       = 2

    i2c_muxs = [
        ('pca9546',          0x75, 0),  # 9548_ROOT_FPGA_CPLD
        ('pca9546',          0x77, 3),  # 9548_CHILD_CPLD_4
        ('s9520_28xc_cpld2', 0x33, 2),  # CPLD2 Mux for QSFP port 0-3, SFP port 4-27
    ]

    cpld = [
        ('s9520_28xc_cpld1', 0x32, 2),  # CPLD1
    ]

    sys_eeprom_denverton = [
        ('sys_eeprom', 0x56, 1),
    ]

    sys_eeprom_lcc = [
        ('sys_eeprom', 0x57, 1),
    ]

    def get_cpld_sku_id(self, board=None):
        sysfs_path = getattr(self, 'PATH_BOARD_SKU_ID', None)
        if sysfs_path and os.path.exists(sysfs_path):
            try:
                with open(sysfs_path, 'r') as f:
                    val = int(f.read().strip(), 0)
                    if val not in (0, 0xFF):
                        return val
            except Exception as e:
                self.bsp_pr("ERR: Read SKU ID from LPC sysfs failed: {}".format(e))

        try:
            with open("/dev/port", "rb", buffering=0) as f:
                f.seek(0xE00)
                val = f.read(1)
                if val and val[0] not in (0, 0xFF):
                    sku = val[0]
                    self.bsp_pr("Check CPU SKU ID via LPC 0xE00: 0x{:02X}".format(sku))
                    return sku
        except Exception as e:
            self.bsp_pr("ERR: Read LPC port 0xE00 failed: {}".format(e))

        try:
            cmd = ["i2cget", "-f", "-y", "2", "0x32", "0x00"]
            out = subprocess.check_output(cmd).decode('utf-8').strip()
            return int(out, 0)
        except Exception as e:
            self.bsp_pr("ERR: Read SKU ID via i2cget failed: {}".format(e))
            return None

    def get_cpu_type(self, board=None):
        """
        Identify CPU SKU type.

        Returns:
            CPU_TYPE_DENVERTON (1)  : Denverton CPU
            CPU_TYPE_LCC       (2)  : LCC CPU
            CPU_TYPE_UNKNOWN   (-1) : Unknown SKU or Error reading SKU ID
        """
        sku_id = self.get_cpld_sku_id(board)

        if sku_id == self.SKU_DENVERTON:
            self.bsp_pr("Denverton CPU Detected, BOARD_SKU_ID = 0x{:02X}".format(sku_id))
            return self.CPU_TYPE_DENVERTON
        elif sku_id == self.SKU_LCC:
            self.bsp_pr("LCC CPU Detected, BOARD_SKU_ID = 0x{:02X}".format(sku_id))
            return self.CPU_TYPE_LCC

        sku_str = "0x{:02X}".format(sku_id) if sku_id is not None else "None"
        self.bsp_pr("ERR: Unknown or invalid SKU ID ({})".format(sku_str))
        return self.CPU_TYPE_UNKNOWN

    def start_i2c_bus_order_init(self, board):

        device_actions = [
            #driver_name   bus_address     action
            ("i801_smbus", "0000:00:1f.4", "unbind"),
            ("ismt_smbus", "0000:00:12.0", "unbind"),
            ("i801_smbus", "0000:00:1f.4", "bind"),
            ("ismt_smbus", "0000:00:12.0", "bind"),
        ]

        self.update_pci_devices(device_actions)

    def start_mux_init(self, board):
        self.bsp_pr("Init I2C Mux")
        self.new_i2c_devices(self.i2c_muxs)

        #init idle state on mux
        self.init_i2c_mux_idle_state(self.i2c_muxs)

    def start_mux_deinit(self, board):
        self.bsp_pr("Deinit I2C Mux")
        self.del_i2c_devices(reversed(self.i2c_muxs))

    def start_cpld_init(self, board):
        self.bsp_pr("Init CPLD")
        self.new_i2c_devices(self.cpld)

    def start_cpld_deinit(self, board):
        self.bsp_pr("Deinit CPLD")
        self.del_i2c_devices(reversed(self.cpld))

    def start_sys_eeprom_init(self, board):
        self.bsp_pr("Init sys eeprom")

        cpu_type = self.get_cpu_type(board)
        if cpu_type == self.CPU_TYPE_DENVERTON:
            self.runtime_syseeprom = self.sys_eeprom_denverton
        elif cpu_type == self.CPU_TYPE_LCC:
            self.runtime_syseeprom = self.sys_eeprom_lcc
        else:
            self.bsp_pr("ERR: Failed to initialize EEPROM due to unknown CPU SKU ID")
            return

        self.new_i2c_devices(self.runtime_syseeprom)

    def start_sys_eeprom_deinit(self, board):
        self.bsp_pr("Deinit sys eeprom")
        eeprom_to_del = getattr(self, "runtime_syseeprom", None)

        if eeprom_to_del is None:
            cpu_type = self.get_cpu_type(board)
            if cpu_type == self.CPU_TYPE_DENVERTON:
                eeprom_to_del = self.sys_eeprom_denverton
            elif cpu_type == self.CPU_TYPE_LCC:
                eeprom_to_del = self.sys_eeprom_lcc
            else:
                self.bsp_pr("WARNING: Unknown CPU SKU ID during deinit, skipping EEPROM deletion")
                return

        self.del_i2c_devices(eeprom_to_del)

    def start_event_ctrl_init(self, board):
        # enable event ctrl
        self.bsp_pr("Enable event control")
        self._write(self.PATH_CPLD1_EVT_CTRL, 1)
        self._write(self.PATH_CPLD2_EVT_CTRL, 1)

        return

    def load_platform_config(self, board):
        """
        Load platform S3IP Configuration

        Args:
            board: The board SKU information.
        """

        # Load Configuration
        s3ip_cfg_file = "Unknown"

        if not os.path.exists(s3ip_cfg_file):
            cpu_type = self.get_cpu_type(board)
            if cpu_type == self.CPU_TYPE_DENVERTON:
                s3ip_cfg_file = self.PATH_S3IP_CFG_DENVERTON
            elif cpu_type == self.CPU_TYPE_LCC:
                s3ip_cfg_file = self.PATH_S3IP_CFG_LCC
            else:
                self.bsp_pr("FATAL: Cannot determine S3IP config file for unknown CPU SKU ID")
                return None

        try:
            with open(s3ip_cfg_file, 'r') as f:
                config = yaml.safe_load(f)
            return config

        except (IOError, OSError) as e:
            self.bsp_pr("FATAL: Failed to open S3IP config file '{}'. Error: {}".format(s3ip_cfg_file, e))
            return None
        except Exception as e:
            self.bsp_pr("FATAL: Unexpected error while loading S3IP config: {}".format(e))
            return None
