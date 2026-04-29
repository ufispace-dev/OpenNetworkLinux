class UFispacePlatformBase:
    PATH_CPLD1_EVT_CTRL = "/sys/bus/i2c/devices/2-0032/event_detect_ctrl"
    PATH_CPLD2_EVT_CTRL = "/sys/bus/i2c/devices/2-0033/event_detect_ctrl"

    i2c_muxs = [
        ('pca9546',          0x75, 0),  # 9548_ROOT_FPGA_CPLD
        ('pca9546',          0x77, 3),  # 9548_CHILD_CPLD_4
        ('s9520_28xc_cpld2', 0x33, 2),  # CPLD2 Mux for QSFP port 0-3, SFP port 4-27
    ]

    cpld = [
        ('s9520_28xc_cpld1', 0x32, 2),  # CPLD1
    ]

    sys_eeprom = [
        ('sys_eeprom', 0x56, 1),
    ]

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
        self.new_i2c_devices(self.sys_eeprom)

    def start_sys_eeprom_deinit(self, board):
        self.bsp_pr("Deinit sys eeprom")
        self.del_i2c_devices(self.sys_eeprom)

    def start_event_ctrl_init(self, board):
        # enable event ctrl
        self.bsp_pr("Enable event control")
        self._write(self.PATH_CPLD1_EVT_CTRL, 1)
        self._write(self.PATH_CPLD2_EVT_CTRL, 1)

        return
