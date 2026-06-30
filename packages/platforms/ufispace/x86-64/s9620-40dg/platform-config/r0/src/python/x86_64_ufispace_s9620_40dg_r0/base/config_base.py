import os
import yaml

class UFispacePlatformBase:
    PATH_I2C_CPLD1 = "/sys/bus/i2c/devices/2-0026"
    PATH_I2C_CPLD2 = "/sys/bus/i2c/devices/2-0027"
    PATH_CPLD1_EVT_CTRL = PATH_I2C_CPLD1 + "/event_detect_ctrl"
    PATH_CPLD2_EVT_CTRL = PATH_I2C_CPLD2 + "/event_detect_ctrl"

    # --- I2C MUX Configurations ---
    i2c_muxs_alpha = [
        ('pca9546', 0x76, 0),   # 9546_MB_MUX
        ('pca9548', 0x75, 5),   # 9548_MB_MUX
        ('pca9548', 0x73, 4),   # 9548_QSFP28_0_5
        ('pca9548', 0x73, 7),   # 9548_QSFP28_6_11
        ('pca9548', 0x73, 3),   # 9548_QSFPDD_12_15
        ('pca9548', 0x73, 8),   # 9548_SFP56_16_23
        ('pca9548', 0x73, 9),   # 9548_SFP56_24_31
        ('pca9548', 0x73, 10),  # 9548_SFP56_32_39
        ('pca9546', 0x75, 1),   # 9546_ROOT_ISMT
        ('pca9546', 0x76, 63),  # 9546_ROOT_I801
    ]

    i2c_muxs = [
        ('pca9546',          0x76, 0),  # 9548_ROOT_FPGA_CPLD
        ('s9620_40dg_cpld2', 0x27, 2),  # CPLD2 Mux
        ('s9620_40dg_cpld3', 0x25, 5),  # CPLD3 Mux
        ('s9620_40dg_cpld4', 0x24, 5),  # CPLD4 Mux
        ('pca9546',          0x75, 1),
        ('pca9546',          0x76, 47),
    ]

    # --- CPLD Configurations ---
    cpld_alpha = [
        ('s9620_40dg_cpld1', 0x26, 2),  # CPLD1
        ('s9620_40dg_cpld2', 0x27, 2),  # CPLD2
        ('s9620_40dg_cpld3', 0x25, 6),  # CPLD3 on Bus 6 for Alpha
        ('s9620_40dg_cpld4', 0x24, 6),  # CPLD4 on Bus 6 for Alpha
    ]

    cpld = [
        ('s9620_40dg_cpld1', 0x26, 2),  # CPLD1
        # ('s9620_40dg_cpld2', 0x27, 2),  # CPLD2
        # ('s9620_40dg_cpld3', 0x25, 5),  # CPLD3 on Bus 5 for Beta
        # ('s9620_40dg_cpld4', 0x24, 5),  # CPLD4 on Bus 5 for Beta
    ]

    sys_eeprom = [
        ('sys_eeprom', 0x57, 1),
    ]

    def start_i2c_bus_order_init(self, board):

        device_actions = [
            # driver_name   bus_address     action
            ("i801_smbus", "0000:00:1f.4", "unbind"),
            ("ismt_smbus", "0000:00:0f.0", "unbind"),
            ("i801_smbus", "0000:00:1f.4", "bind"),
            ("ismt_smbus", "0000:00:0f.0", "bind"),
        ]

        self.update_pci_devices(device_actions)

    def start_mux_init(self, board):
        self.bsp_pr("Init I2C Mux")
        hw_rev = board.get("hw_rev", 0)

        if hw_rev <= 1:
            self.runtime_i2c_muxs = self.i2c_muxs_alpha
        else:
            self.runtime_i2c_muxs = self.i2c_muxs

        self.new_i2c_devices(self.runtime_i2c_muxs)

        self.init_i2c_mux_idle_state(self.runtime_i2c_muxs)

    def start_mux_deinit(self, board):
        self.bsp_pr("Deinit I2C Mux")
        muxs_to_del = getattr(self, "runtime_i2c_muxs", None)
        if muxs_to_del is None:
            hw_rev = board.get("hw_rev", 0)
            if hw_rev <= 1:
                muxs_to_del = self.i2c_muxs_alpha
            else:
                muxs_to_del = self.i2c_muxs
        self.del_i2c_devices(reversed(muxs_to_del))

    def start_cpld_init(self, board):
        self.bsp_pr("Init CPLD")
        hw_rev = board.get("hw_rev", 0)

        if hw_rev <= 1:
            self.runtime_cpld = self.cpld_alpha
        else:
            self.runtime_cpld = self.cpld

        self.new_i2c_devices(self.runtime_cpld)

    def start_cpld_deinit(self, board):
        self.bsp_pr("Deinit CPLD")
        cpld_to_del = getattr(self, "runtime_cpld", None)

        if cpld_to_del is None:
            hw_rev = board.get("hw_rev", 0)
            if hw_rev <= 1:
                cpld_to_del = self.cpld_alpha
            else:
                cpld_to_del = self.cpld

        self.del_i2c_devices(reversed(cpld_to_del))

    def start_sys_eeprom_init(self, board):
        self.bsp_pr("Init sys eeprom")
        self.new_i2c_devices(self.sys_eeprom)

    def start_sys_eeprom_deinit(self, board):
        self.bsp_pr("Deinit sys eeprom")
        self.del_i2c_devices(self.sys_eeprom)

    def start_event_ctrl_init(self, board):
        self.bsp_pr("Enable event control")
        hw_rev = board.get("hw_rev", 0)
        cpld_3_4_bus = 6 if hw_rev <= 1 else 5

        path_cpld3_evt = "/sys/bus/i2c/devices/{}-0025/event_detect_ctrl".format(cpld_3_4_bus)
        path_cpld4_evt = "/sys/bus/i2c/devices/{}-0024/event_detect_ctrl".format(cpld_3_4_bus)

        self._write(self.PATH_CPLD1_EVT_CTRL, 1)
        self._write(self.PATH_CPLD2_EVT_CTRL, 1)
        self._write(path_cpld3_evt, 1)
        self._write(path_cpld4_evt, 1)
        return

    def load_platform_config(self, board):
        """
        Load platform S3IP Configuration

        Args:
            board: The board SKU information.
        """

        # Load Configuration
        s3ip_cfg_file = "Unknown"

        try:
            hw_rev = board.get("hw_rev", 0)

            if hw_rev <= 1:
                s3ip_cfg_file = self.PATH_S3IP_CFG_ALPHA
            else:
                s3ip_cfg_file = self.PATH_S3IP_CFG

            with open(s3ip_cfg_file, 'r') as f:
                config = yaml.safe_load(f)
            return config

        except (IOError, OSError) as e:
            self.bsp_pr("FATAL: Failed to open S3IP config file '{}'. Error: {}".format(s3ip_cfg_file, e))
            return None
        except Exception as e:
            self.bsp_pr("FATAL: Unexpected error while loading S3IP config: {}".format(e))
            return None
