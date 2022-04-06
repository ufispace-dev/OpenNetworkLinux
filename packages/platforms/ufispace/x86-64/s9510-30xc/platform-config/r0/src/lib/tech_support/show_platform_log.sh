#!/bin/bash

# TRUE=0, FALSE=1
TRUE=0
FALSE=1


# DATESTR: The format of log folder and log file
DATESTR=$(date +"%Y%m%d%H%M%S")
LOG_FOLDER_NAME="log_platform_${DATESTR}"
LOG_FILE_NAME="log_platform_${DATESTR}.log"

# LOG_FOLDER_ROOT: The root folder of log files
LOG_FOLDER_ROOT="/tmp/log"
LOG_FOLDER_PATH="${LOG_FOLDER_ROOT}/${LOG_FOLDER_NAME}"
LOG_FILE_PATH="${LOG_FOLDER_PATH}/${LOG_FILE_NAME}"


# MODEL_NAME: set by function _board_info
MODEL_NAME=""
# HW_REV: set by function _board_info
HW_REV=""
# BSP_INIT_FLAG: set bu function _check_bsp_init
BSP_INIT_FLAG=""

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"
IOGET="${SCRIPTPATH}/ioget"

# LOG_FILE_ENABLE=1: Log all the platform info to log files (${LOG_FILE_NAME})
# LOG_FILE_ENABLE=0: Print all the platform info in console
LOG_FILE_ENABLE=1

# HEADER_PROMPT=1 :print file name at log file first line
# HEADER_PROMPT=0 :don't print file name at log file first line
HEADER_PROMPT=1

# ls option
# LS_OPTION="-alu"                 : show all file, file permission, list time and sort it by name
# LS_OPTION="-a | cat | sort"      : list one filename per output line and sort it
LS_OPTION="-alu"

# Log Redirection
# LOG_REDIRECT="2> /dev/null": remove the error message from console
# LOG_REDIRECT=""            : show the error message in console
# LOG_REDIRECT="2>&1"        : show the error message in stdout, then stdout may send to console or file in _echo()
LOG_REDIRECT="2>&1"

# GPIO_OFFSET: update by function _update_gpio_offset
GPIO_OFFSET=0

# I2C Bus
i801_bus=""
ismt_bus=""

function _echo {
    str="$@"

    if [ "${LOG_FILE_ENABLE}" == "1" ] && [ -f "${LOG_FILE_PATH}" ]; then
        echo "${str}" >> "${LOG_FILE_PATH}"
    else
        echo "${str}"
    fi
}

function _banner {
   banner="$1"

   if [ ! -z "${banner}" ]; then
       _echo ""
       _echo "##############################"
       _echo "#   ${banner}"
       echo  "#   ${banner}..."
       _echo "##############################"
   fi
}

function _pkg_version {
    _banner "Package Version = 1.0.0"
}

function _update_gpio_offset {
    _banner "Update GPIO Offset"

    max_gpiochip=`ls /sys/class/gpio/ | sort -r | grep -m1 gpiochip`
    max_gpiochip_num="${max_gpiochip#*gpiochip}"

    if [ -z "${max_gpiochip_num}" ]; then
        GPIO_OFFSET=0
    elif [ ${max_gpiochip_num} -lt 256 ]; then
        GPIO_OFFSET=256
    else
        GPIO_OFFSET=0
    fi

    _echo "[GPIOCHIP MAX    ]: ${max_gpiochip}"
    _echo "[GPIOCHIP MAX NUM]: ${max_gpiochip_num}"
    _echo "[GPIO OFFSET     ]: ${GPIO_OFFSET}"
}

function _check_env {
    #_banner "Check Environment"

    # check utility
    if [ ! -f "${IOGET}" ]; then
        echo "Error!!! ioget(${IOGET}) file not found!!! Exit!!!"
        echo "Please update the ioget file path in script or put the ioget under ${IOGET}."
        exit 1
    fi

    # check basic commands
    cmd_array=("ipmitool" "lsusb" "dmidecode")
    for (( i=0; i<${#cmd_array[@]}; i++ ))
    do
        ret=`which ${cmd_array[$i]}`

        if [ ! $? -eq 0 ]; then
            _echo "${cmd_array[$i]} command not found!!"
            exit 1
        fi
    done

    if [ "${LOG_FILE_ENABLE}" == "1" ]; then
        mkdir -p "${LOG_FOLDER_PATH}"
        if [ "${HEADER_PROMPT}" == "1" ]; then
            echo "${LOG_FILE_NAME}" > "${LOG_FILE_PATH}"
        else
            touch "${LOG_FILE_PATH}"
        fi
    fi

    # get i2c root
    _get_i2c_root
    if [ ! -z ${i801_bus} ]; then
        _echo "[I801 bus ]: ${i801_bus}"
    fi

    if [ ! -z ${ismt_bus} ]; then
        _echo "[ismt bus ]: ${ismt_bus}"
    fi

    # check BSP init
    _check_bsp_init

    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _update_gpio_offset
    fi
}

function _check_filepath {
    filepath=$1
    if [ -z "${filepath}" ]; then
        _echo "ERROR, the ipnut string is empyt!!!"
        return ${FALSE}
    elif [ ! -f "$filepath" ]; then
        _echo "ERROR: No such file: ${filepath}"
        return ${FALSE}
    else
        #_echo "File Path: ${filepath}"
        return ${TRUE}
    fi
}

function _check_i2c_device {
    i2c_bus=$1
    i2c_addr=$2

    if [ -z "${i2c_addr}" ]; then
        _echo "ERROR, the ipnut string is empyt!!!"
        return ${FALSE}
    fi

    value=$(eval "i2cget -y -f ${i2c_bus} ${i2c_addr} ${LOG_REDIRECT}")
    ret=$?

    if [ $ret -eq 0 ]; then
        return ${TRUE}
    else
        _echo "ERROR: No such device: Bus:i2c_bus, Address: ${i2c_addr}"
        return ${FALSE}
    fi
}

function _check_bsp_init {
    _banner "Check BSP Init"
    
    # We use ismt bus device status (cpu eeprom, i2c mux 0 ...) to check bsp init status
    local bus=$(eval "i2cdetect -y ${ismt_bus} ${LOG_REDIRECT} | grep UU")
    ret=$?
    if [ $ret -eq 0 ] && [ ! -z "${bus}" ] ; then
        BSP_INIT_FLAG=1
    else
        BSP_INIT_FLAG=0
    fi

    _echo "[BSP_INIT_FLAG]: ${BSP_INIT_FLAG}"
}

function _get_i2c_root {
    if _check_filepath "/sys/bus/i2c/devices/i2c-0/name" ;then
        i2c_0=`cat /sys/bus/i2c/devices/i2c-0/name`
    fi

    if echo "$i2c_0" | grep -q "I801"; then
        i801_bus=0
        ismt_bus=1
    else
        i801_bus=1
        ismt_bus=0
    fi
}

function _show_system_info {
    _banner "Show System Info"

    x86_date=`date`
    x86_uptime=`uptime`
    bmc_date=$(eval "ipmitool sel time get ${LOG_REDIRECT}")
    last_login=`last`

    _echo "[X86 Date Time ]: ${x86_date}"
    _echo "[BMC Date Time ]: ${bmc_date}"
    _echo "[X86 Up Time   ]: ${x86_uptime}"
    _echo "[X86 Last Login]: "
    _echo "${last_login}"
    _echo ""

    cmd_array=("uname -a" "cat /proc/cmdline" "cat /proc/ioports" \
               "cat /proc/iomem" "cat /proc/meminfo" \
               "cat /proc/sys/kernel/printk" \
               "find /etc -name '*-release' -print -exec cat {} \;")

    for (( i=0; i<${#cmd_array[@]}; i++ ))
    do
        _echo "[Command]: ${cmd_array[$i]}"
        ret=$(eval "${cmd_array[$i]} ${LOG_REDIRECT}")
        _echo "${ret}"
        _echo ""
    done

}

function _show_grub {

    local grub_path="/boot/grub/grub.cfg"
    if [ ! -f "${grub_path}" ]; then
        grub_path="/mnt/onl/boot/grub/grub.cfg"
        if [ ! -f "${grub_path}" ]; then
            return 0
        fi
    fi

    _banner "Show GRUB Info"

    grub_info=`cat ${grub_path}`

    _echo "[GRUB Info     ]:"
    _echo "${grub_info}"

}

function _show_driver {
    _banner "Show Kernel Driver"

    cmd_array=("lsmod | sort" \
               "cat /lib/modules/$(uname -r)/modules.builtin | sort")

    for (( i=0; i<${#cmd_array[@]}; i++ ))
    do
        _echo "[Command]: ${cmd_array[$i]}"
        ret=$(eval "${cmd_array[$i]} ${LOG_REDIRECT}")
        _echo "${ret}"
        _echo ""
    done
}

function _pre_log {
    _banner "Pre Log"

    _show_i2c_tree_bus
    _show_i2c_tree_bus
    _show_i2c_tree_bus
}

function _show_board_info {
    _banner "Show Board Info"

    # CPLD 0x700 Register Definition
    build_rev_id_array=(0 1 2 3)
    build_rev_array=(1 2 3 4)
    hw_rev_id_array=(0 1 2 3)
    hw_rev_array=("Proto" "Alpha" "Beta" "PVT")
    hw_rev_ga_array=("GA_1" "GA_2" "GA_3" "GA_4")
    deph_name_array=("NPI" "GA")
    model_id_array=($((2#00000110)))
    model_name_array=("S9510-30XC")

    model_id=`${IOGET} 0x700`
    ret=$?
    if [ $ret -eq 0 ]; then
        model_id=`echo ${model_id} | awk -F" " '{print $NF}'`
        model_id=$((model_id))
    else
        _echo "Get board model id failed ($ret), Exit!!"
        exit $ret
    fi

    board_rev_id=`${IOGET} 0x701`
    ret=$?
    if [ $ret -eq 0 ]; then
        board_rev_id=`echo ${board_rev_id} | awk -F" " '{print $NF}'`
        board_rev_id=$((board_rev_id))
    else
        _echo "Get board hw/build revision id failed ($ret), Exit!!"
        exit $ret
    fi

    # DEPH D[2]
    deph_id=$(((board_rev_id & 2#00000100) >> 2))
    deph_name=${deph_name_array[${deph_id}]}

    # HW Rev D[0:1]
    hw_rev_id=$(((board_rev_id & 2#00000011) >> 0))
    hw_rev=${hw_rev_array[${hw_rev_id}]}
    if [ $deph_id -eq 0 ]; then
        hw_rev=${hw_rev_array[${hw_rev_id}]}
    else
        hw_rev=${hw_rev_ga_array[${hw_rev_id}]}
    fi

    # Build Rev D[3:4]
    build_rev_id=$(((board_rev_id & 2#00011000) >> 3))
    build_rev=${build_rev_array[${build_rev_id}]}

    # MODEL ID D[0:3]
    model_id=$(((model_id & 2#00001111) >> 0))
    if [ $model_id -eq ${model_id_array[0]} ]; then
       model_name=${model_name_array[0]}
    else
       _echo "Invalid model_id: ${model_id}"
       exit 1
    fi

    MODEL_NAME=${model_name}
    HW_REV=${hw_rev}
    _echo "[Board Type/Rev Reg Raw ]: ${model_id} ${board_rev_id}"
    _echo "[Board Type and Revision]: ${model_name} ${deph_name} ${hw_rev} ${build_rev}"
}

function _bios_version {
    _banner "Show BIOS Version"

    bios_ver=$(eval "dmidecode -s bios-version ${LOG_REDIRECT}")
    bios_boot_rom=`${IOGET} 0x75b`
    if [ $? -eq 0 ]; then
        bios_boot_rom=`echo ${bios_boot_rom} | awk -F" " '{print $NF}'`
    fi

    # BIOS BOOT SEL D[0:1]
    bios_boot_rom=$(((bios_boot_rom & 2#00000011) >> 0))

    _echo "[BIOS Vesion  ]: ${bios_ver}"
    _echo "[BIOS Boot ROM]: ${bios_boot_rom}"
}

function _bmc_version {
    _banner "Show BMC Version"

    bmc_rom1_ver=$(eval "ipmitool raw 0x32 0x8f 0x8 0x1 ${LOG_REDIRECT}")
    bmc_rom2_ver=$(eval "ipmitool raw 0x32 0x8f 0x8 0x2 ${LOG_REDIRECT}")
    bmc_active_rom=$(eval "ipmitool raw 0x32 0x8f 0x7 ${LOG_REDIRECT}")

    _echo "[BMC ROM1 Ver  ]: ${bmc_rom1_ver}"
    _echo "[BMC ROM2 Ver  ]: ${bmc_rom2_ver}"
    _echo "[BMC Active ROM]: ${bmc_active_rom}"
}

function _cpld_version_lpc {
    _banner "Show CPLD Version (LPC)"

    if [ "${MODEL_NAME}" == "S9510-30XC" ]; then
        # MB CPLD S9510-30XC
        mb_cpld_ver=`${IOGET} 0x702`
        ret=$?
        if [ ${ret} -eq 0 ]; then
            mb_cpld_ver=`echo ${mb_cpld_ver} | awk -F" " '{print $NF}'`
        fi

        mb_cpld_build=`${IOGET} 0x704`
        ret=$?
        if [ ${ret} -eq 0 ]; then
            mb_cpld_build=`echo ${mb_cpld_build} | awk -F" " '{print $NF}'`
        fi

        _echo "[MB CPLD Version]: $(( (mb_cpld_ver & 2#11000000) >> 6)).$(( mb_cpld_ver & 2#00111111 ))"
        _echo "[MB CPLD Build  ]: $(( mb_cpld_build ))"
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _cpld_version_sysfs {
    _banner "Show CPLD Version (Sysfs)"

    if [ "${MODEL_NAME}" == "S9510-30XC" ]; then
        if _check_filepath "/sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/cpld_version_h"; then
            mb_cpld_ver=$(eval "cat /sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/cpld_version_h ${LOG_REDIRECT}")
            _echo "[MB CPLD Version]: ${mb_cpld_ver}"
        fi
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _cpld_version {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _cpld_version_sysfs
    else
        _cpld_version_lpc
    fi
}

function _ucd_version {

    _banner "Show UCD Version"

    if [ "${MODEL_NAME}" != "S9510-30XC" ]; then
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    #get ucd version via BMC
    ucd_ver_raw=$(eval "ipmitool raw 0x3c 0x8 0x0 ${LOG_REDIRECT}")
    ret=$?

    #check return code
    if [ ! $ret -eq 0 ] ; then
        _echo "Require BMC v2.02 or later to get UCD version"
        return $ret
    fi

    #convert hex to ascii
    ucd_ver_ascii=`echo $ucd_ver_raw | xxd -r -p`

    #get ucd date via BMC
    ucd_date_raw=$(eval "ipmitool raw 0x3c 0x8 0x1 ${LOG_REDIRECT}")
    ret=$?

    #check return code
    if [ ! $ret -eq 0 ] ; then
        _echo "Require BMC v2.02 or later to get UCD version"
        return $ret
    fi

    #convert hex to ascii
    ucd_date_ascii=`echo $ucd_date_raw | xxd -r -p`

    _echo "[${brd[i]} UCD REVISION RAW]: ${ucd_ver_raw}"
    _echo "[${brd[i]} UCD DATE RAW    ]: ${ucd_date_raw}"
    _echo "[${brd[i]} MFR_REVISION    ]: ${ucd_ver_ascii}"
    _echo "[${brd[i]} MFR_DATE        ]: ${ucd_date_ascii}"
}


function _show_version {
    _bios_version
    _bmc_version
    _cpld_version
    # _ucd_version # Not support
}

function _show_i2c_tree_bus {
    _banner "Show I2C Tree Bus 0"

    ret=$(eval "i2cdetect -y 0 ${LOG_REDIRECT}")

    _echo "[I2C Tree 0]:"
    _echo "${ret}"

    _banner "Show I2C Tree Bus 1"

    ret=$(eval "i2cdetect -y 1 ${LOG_REDIRECT}")

    _echo "[I2C Tree 1]:"
    _echo "${ret}"
}

function _show_i2c_mux_devices {
    local bus=$1
    local chip_addr=$2
    local channel_num=$3
    local chip_dev_desc=$4
    local i=0;

    if [ -z "${chip_addr}" ] || [ -z "${channel_num}" ] || [ -z "${chip_dev_desc}" ]; then
        _echo "ERROR: parameter cannot be empty!!!"
        exit 99
    fi

    _check_i2c_device "$bus" "$chip_addr"
    ret=$?
    if [ "$ret" == "0" ]; then
        _echo "TCA9548 Mux ${chip_dev_desc}"
        _echo "---------------------------------------------------"
        for (( i=0; i<${channel_num}; i++ ))
        do
            _echo "TCA9548 Mux ${chip_dev_desc} - Channel ${i}"
            # open mux channel
            i2cset -y ${bus} ${chip_addr} $(( 2 ** ${i} ))
            # dump i2c tree
            ret=$(eval "i2cdetect -y ${bus} ${LOG_REDIRECT}")
            _echo "${ret}"
            # close mux channel
            i2cset -y ${bus} ${chip_addr} 0x0
            _echo ""
        done
    fi

}

function _show_i2c_tree_bus_mux_i2c {
    _banner "Show I2C Tree Bus MUX (I2C)"

    local i=0
    local chip_addr1=""
    local chip_addr2=""

    if [ "${MODEL_NAME}" == "S9510-30XC" ]; then

        chip_addr1="0x76"
        _check_i2c_device "${i801_bus}" "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then

            # Name                  | Alpha_MUX#
            # ----------------------|-------------
            # 9546_ROOT_TIMING      |  MUX0
            # 9546_ROOT_SFP         |  MUX2
            # 9546_CHILD_QSFP_0_1   |  MUX7
            # 9548_CHILD_SFP_2_9    |  MUX3
            # 9548_CHILD_SFP_10_17  |  MUX4
            # 9548_CHILD_SFP_18_25  |  MUX5
            # 9548_CHILD_SFP_26_29  |  MUX6

            ## (9546_ROOT_SFP)-0x76
            _show_i2c_mux_devices "${i801_bus}" "${chip_addr1}" "4" "(9546_ROOT_SFP)-0x76"

            ## (9546_ROOT_SFP)-0x76(3)
            #                                |-(9546_CHILD_QSFP_0_1)-0x70
            #                                |-(9548_CHILD_SFP_2_9)-0x71
            #                                |-(9548_CHILD_SFP_10_17)-0x72
            #                                |-(9548_CHILD_SFP_18_25)-0x73
            #                                |-(9548_CHILD_SFP_26_29)-0x74
            i2cset -y ${i801_bus} ${chip_addr1} $(( 2 ** 3 ))

            chip_addr2="0x70"
            _check_i2c_device "${i801_bus}" "${chip_addr2}"
            ret=$?
            if [ "$ret" == "0" ]; then
                _show_i2c_mux_devices "${i801_bus}" "${chip_addr2}" "4" "(9546_CHILD_QSFP_0_1)-0x70"
            fi

            chip_addr2="0x71"
            _check_i2c_device "${i801_bus}" "${chip_addr2}"
            ret=$?
            if [ "$ret" == "0" ]; then
            _show_i2c_mux_devices "${i801_bus}" "${chip_addr2}" "8" "(9548_CHILD_SFP_2_9)-0x71"
            fi

            chip_addr2="0x72"
            _check_i2c_device "${i801_bus}" "${chip_addr2}"
            ret=$?
            if [ "$ret" == "0" ]; then
            _show_i2c_mux_devices "${i801_bus}" "${chip_addr2}" "8" "(9548_CHILD_SFP_10_17)-0x72"
            fi

            chip_addr2="0x73"
            _check_i2c_device "${i801_bus}" "${chip_addr2}"
            ret=$?
            if [ "$ret" == "0" ]; then
            _show_i2c_mux_devices "${i801_bus}" "${chip_addr2}" "8" "(9548_CHILD_SFP_18_25)-0x73"
            fi

            chip_addr2="0x74"
            _check_i2c_device "${i801_bus}" "${chip_addr2}"
            ret=$?
            if [ "$ret" == "0" ]; then
            _show_i2c_mux_devices "${i801_bus}" "${chip_addr2}" "8" "(9548_CHILD_SFP_26_29)-0x74"
            fi

            i2cset -y ${i801_bus} ${chip_addr1} 0x0
        fi

    else
        echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _show_i2c_tree {
    _banner "Show I2C Tree"

    _show_i2c_tree_bus

    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _echo "TBD"
    else
        _show_i2c_tree_bus_mux_i2c
    fi
}

function _show_i2c_device_info {
    _banner "Show I2C Device Info"

    local pca954x_device_id=("")
    if [ "${MODEL_NAME}" == "S9510-30XC" ]; then
        pca954x_device_id=("0x76")
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    for ((i=0;i<5;i++))
    do
        _echo "[DEV PCA9548 (${i})]"
        for (( j=0; j<${#pca954x_device_id[@]}; j++ ))
        do
            ret=`i2cget -f -y ${i801_bus} ${pca954x_device_id[$j]}`
            _echo "[I2C Device ${pca954x_device_id[$j]}]: $ret"
        done
        sleep 0.4
    done
}

function _show_sys_devices {
    _banner "Show System Devices"

    _echo "[Command]: ls /sys/class/gpio/ ${LS_OPTION}"
    ret=$(eval "ls /sys/class/gpio/ ${LS_OPTION}")
    _echo "${ret}"

    local file_path="/sys/kernel/debug/gpio"
    if [ -f "${file_path}" ]; then
        _echo ""
        _echo "[Command]: cat ${file_path}"
        _echo "$(cat ${file_path})"
    fi

    _echo ""
    _echo "[Command]: ls /sys/bus/i2c/devices/ ${LS_OPTION}"
    ret=$(eval ls /sys/bus/i2c/devices/ ${LS_OPTION})
    _echo "${ret}"

    _echo ""
    _echo "[Command]: ls /dev/ ${LS_OPTION}"
    ret=$(eval ls /dev/ ${LS_OPTION})
    _echo "${ret}"
}


function _show_gpio_sysfs {

    _banner "Show GPIO Status"

    gpio_device=`ls /sys/class/gpio/ | sort -r | grep -m1 "gpio[[:digit:]]"`
    max_gpio=${gpio_device#*gpio}
    gpio_device=`ls /sys/class/gpio/ | sort | grep -m1 "gpio[[:digit:]]"`
    min_gpio=${gpio_device#*gpio}
    for (( i=${min_gpio}; i<=${max_gpio}; i++ ))
    do
        if [ ! -d "/sys/class/gpio/gpio${i}" ]; then
            continue
        fi

        if _check_filepath "/sys/class/gpio/gpio${i}/direction"; then
            gpio_dir=$(eval "cat /sys/class/gpio/gpio${i}/direction")
        else
            gpio_dir="N/A"
        fi

        if _check_filepath "/sys/class/gpio/gpio${i}/value"; then
            gpio_value=$(eval "cat /sys/class/gpio/gpio${i}/value")
        else
            gpio_value="N/A"
        fi

        if _check_filepath "/sys/class/gpio/gpio${i}/active_low"; then
            gpio_active=$(eval "cat /sys/class/gpio/gpio${i}/active_low")
        else
            gpio_active="N/A"
        fi

        _echo "[GPIO Pin]: ${i} ,[Direction]: ${gpio_dir} ,[Value]: ${gpio_value} ,[Active Low] : ${gpio_active}"
    done
}

function _show_gpio {

    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_gpio_sysfs
    fi
}

function _show_cpu_eeprom_i2c {
    _banner "Show CPU EEPROM"

    #read six times return empty content
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} 0x57 c")
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} 0x57 c")
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} 0x57 c")
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} 0x57 c")
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} 0x57 c")
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} 0x57 c")

    #seventh read return correct content
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} 0x57 c")
    _echo "[CPU EEPROM]:"
    _echo "${cpu_eeprom}"
}

function _show_cpu_eeprom_sysfs {
    _banner "Show CPU EEPROM"

    cpu_eeprom=$(eval "cat /sys/bus/i2c/devices/1-0057/eeprom ${LOG_REDIRECT} | hexdump -C")
    _echo "[CPU EEPROM]:"
    _echo "${cpu_eeprom}"
}

function _show_cpu_eeprom {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_cpu_eeprom_sysfs
    else
        _show_cpu_eeprom_i2c
    fi
}

function _show_psu_status_cpld_sysfs {
    _banner "Show PSU Status (CPLD)"

    bus_id=""
    if [ "${MODEL_NAME}" == "S9510-30XC" ]; then
        bus_id="1"
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    # Read PSU Status
    _check_filepath "/sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/psu_status"
    cpld_psu_status_reg=$(eval "cat /sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/psu_status ${LOG_REDIRECT}")

    # Read PSU0 Power Good Status (1: power good, 0: not providing power)
    psu0_power_ok=$(((cpld_psu_status_reg & 2#00010000) >> 4))

    # Read PSU0 Absent Status (0: psu present, 1: psu absent)
    psu0_absent_l=$(((cpld_psu_status_reg & 2#00000001) >> 0))

    # Read PSU1 Power Good Status (1: power good, 0: not providing power)
    psu1_power_ok=$(((cpld_psu_status_reg & 2#00100000) >> 5))

    # Read PSU1 Absent Status (0: psu present, 1: psu absent)
    psu1_absent_l=$(((cpld_psu_status_reg & 2#00000010) >> 1))

    _echo "[PSU  Status Reg Raw   ]: ${cpld_psu_status_reg}"
    _echo "[PSU0 Power Good Status]: ${psu0_power_ok}"
    _echo "[PSU0 Absent Status (L)]: ${psu0_absent_l}"
    _echo "[PSU1 Power Good Status]: ${psu1_power_ok}"
    _echo "[PSU1 Absent Status (L)]: ${psu1_absent_l}"
}

function _show_psu_status_cpld {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_psu_status_cpld_sysfs
    fi
}

function _show_rov_sysfs {
    # Not Support
    return 0
}

function _show_rov {
    # Not Support
    return 0
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_rov_sysfs
    fi
}

function _show_port_status_sysfs {
    _banner "Show Port Status / EEPROM"

    if [ "${MODEL_NAME}" == "S9510-30XC" ]; then

        port_name_array=("0" "1" \
                         "2"  "3"  "4"  "5"  "6"  "7"  "8"  "9"  "10" "11" \
                         "12" "13" "14" "15" "16" "17" "18" "19" "20" "21" \
                         "22" "23" "24" "25" "26" "27" "28" "29")

        port_absent_gpio_array=("485" "484" \
                                "375" "374" "373" "372" "371" "370" "369" "368" "383" "382" \
                                "381" "380" "379" "378" "377" "376" "359" "358" "357" "356" \
                                "355" "354" "353" "352" "367" "366" "365" "364")

        port_lp_mode_gpio_array=("489" "488" \
                                 "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" \
                                 "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1"\
                                 "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1")

        port_reset_gpio_array=("493" "492" \
                               "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" \
                               "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1"\
                               "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1")

        port_intr_gpio_array=("481" "480" \
                              "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" \
                              "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1"\
                              "-1" "-1" "-1" "-1" "-1" "-1" "-1" "-1")

        port_tx_fault_gpio_array=("-1" "-1" \
                                  "439" "438" "437" "436" "435" "434" "433" "432" "447" "446" \
                                  "445" "444" "443" "442" "441" "440" "423" "422" "421" "420" \
                                  "419" "418" "417" "416" "431" "430" "429" "428")

        port_rx_los_gpio_array=("-1" "-1" \
                                "343" "342" "341" "340" "339" "338" "337" "336" "351" "350" \
                                "349" "348" "347" "346" "345" "344" "327" "326" "325" "324" \
                                "323" "322" "321" "320" "335" "334" "333" "332")

        port_tx_dis_gpio_array=("-1" "-1" \
                                "471" "470" "469" "468" "467" "466" "465" "464" "479" "478" \
                                "477" "476" "475" "474" "473" "472" "455" "454" "453" "452" \
                                "451" "450" "449" "448" "463" "462" "461" "460")

        port_rate_sel_gpio_array=("-1" "-1" \
                                  "407" "406" "405" "404" "403" "402" "401" "400" "415" "414" \
                                  "413" "412" "411" "410" "409" "408" "391" "390" "389" "388" \
                                  "387" "386" "385" "384" "399" "398" "397" "396")

        port_eeprom_bus_array=("11" "10" \
                               "14" "15" "16" "17" "18" "19" "20" "21" "22" "23" \
                               "24" "25" "26" "27" "28" "29" "30" "31" "32" "33"\
                               "34" "35" "36" "37" "38" "39" "40" "41")

        for (( i=0; i<${#port_name_array[@]}; i++ ))
        do

            local gpio_path=0
            # Port Absent Status (0: Present, 1:Absence)
            gpio_path=$(( ${port_absent_gpio_array[${i}]} - ${GPIO_OFFSET} ))
            if [ "${port_absent_gpio_array[${i}]}" != "-1" ] && _check_filepath "/sys/class/gpio/gpio${gpio_path}/value"; then
                port_absent=$(eval "cat /sys/class/gpio/gpio${gpio_path}/value")
                _echo "[Port${i} Module Absent]: ${port_absent}"
            fi

            # Port Lower Power Mode Status (0: Normal Power Mode, 1:Low Power Mode)
            gpio_path=$(( ${port_lp_mode_gpio_array[${i}]} - ${GPIO_OFFSET} ))
            if [ "${port_lp_mode_gpio_array[${i}]}" != "-1" ] && _check_filepath "/sys/class/gpio/gpio${gpio_path}/value"; then

                port_lp_mode=$(eval "cat /sys/class/gpio/gpio${gpio_path}/value")

                _echo "[Port${i} Low Power Mode]: ${port_lp_mode}"
            fi

            # Port Reset Status (0:Reset, 1:Normal)
            gpio_path=$(( ${port_reset_gpio_array[${i}]} - ${GPIO_OFFSET} ))
            if [ "${port_reset_gpio_array[${i}]}" != "-1" ] && _check_filepath "/sys/class/gpio/gpio${gpio_path}/value"; then
                port_reset=$(eval "cat /sys/class/gpio/gpio${gpio_path}/value")
                _echo "[Port${i} Reset Status]: ${port_reset}"
            fi

            # Port Interrupt Status (0: Interrupted, 1:No Interrupt)
            gpio_path=$(( ${port_intr_gpio_array[${i}]} - ${GPIO_OFFSET} ))
            if [ "${port_intr_gpio_array[${i}]}" != "-1" ] && _check_filepath "/sys/class/gpio/gpio${gpio_path}/value"; then
                port_intr_l=$(eval "cat /sys/class/gpio/gpio${gpio_path}/value")
                _echo "[Port${i} Interrupt Status (L)]: ${port_intr_l}"
            fi

            # Port Tx Fault Status (0:normal, 1:tx fault)
            gpio_path=$(( ${port_tx_fault_gpio_array[${i}]} - ${GPIO_OFFSET} ))
            if [ "${port_tx_fault_gpio_array[${i}]}" != "-1" ] && _check_filepath "/sys/class/gpio/gpio${gpio_path}/value"; then
                port_tx_fault=$(eval "cat /sys/class/gpio/gpio${gpio_path}/value")
                _echo "[Port${i} Tx Fault Status]: ${port_tx_fault}"
            fi

            # Port Rx Loss Status (0:los undetected, 1: los detected)
            gpio_path=$(( ${port_rx_los_gpio_array[${i}]} - ${GPIO_OFFSET} ))
            if [ "${port_rx_los_gpio_array[${i}]}" != "-1" ] && _check_filepath "/sys/class/gpio/gpio${gpio_path}/value"; then
                port_rx_loss=$(eval "cat /sys/class/gpio/gpio${gpio_path}/value")
                _echo "[Port${i} Rx Loss Status]: ${port_rx_loss}"
            fi

            # Port Tx Disable Status (0:enable tx, 1: disable tx)
            gpio_path=$(( ${port_tx_dis_gpio_array[${i}]} - ${GPIO_OFFSET} ))
            if [ "${port_tx_dis_gpio_array[${i}]}" != "-1" ] && _check_filepath "/sys/class/gpio/gpio${gpio_path}/value"; then
                port_tx_dis=$(eval "cat /sys/class/gpio/gpio${gpio_path}/value")
                _echo "[Port${i} Tx Disable Status]: ${port_tx_dis}"
            fi

            # Port Rate Select (0: low rate, 1:full rate)
            gpio_path=$(( ${port_rate_sel_gpio_array[${i}]} - ${GPIO_OFFSET} ))
            if [ "${port_rate_sel_gpio_array[${i}]}" != "-1" ] && _check_filepath "/sys/class/gpio/gpio${gpio_path}/value"; then
                port_rate_sel=$(eval "cat /sys/class/gpio/gpio${gpio_path}/value")
                _echo "[Port${i} Port Rate Select]: ${port_rate_sel}"
            fi

            # Port Dump EEPROM
            local eeprom_path="/sys/bus/i2c/devices/${port_eeprom_bus_array[${i}]}-0050/eeprom"
            if [ "${port_absent}" == "0" ] && _check_filepath "${eeprom_path}"; then
                port_eeprom=$(eval "dd if=${eeprom_path} bs=128 count=2 skip=0 status=none ${LOG_REDIRECT} | hexdump -C")
                if [ "${LOG_FILE_ENABLE}" == "1" ]; then
                    hexdump -C "${eeprom_path}" > ${LOG_FOLDER_PATH}/port${i}_eeprom.log 2>&1
                fi
                if [ -z "$port_eeprom" ]; then
                    port_eeprom="ERROR!!! The result is empty. It should read failed (${eeprom_path})!!"
                fi
            else
                port_eeprom="N/A"
            fi

            _echo "[Port${i} EEPROM Page0-1]:"
            _echo "${port_eeprom}"
            _echo ""
        done
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _show_port_status {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_port_status_sysfs
    fi
}

function _show_cpu_temperature_sysfs {
    _banner "show CPU Temperature"

    cpu_temp_array=("1" "4" "6" "10" "16")

    for (( i=0; i<${#cpu_temp_array[@]}; i++ ))
    do
        if [ -f "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_input" ]; then
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_input"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_max"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_crit"
            temp_input=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_input ${LOG_REDIRECT}")
            temp_max=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_max ${LOG_REDIRECT}")
            temp_crit=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_crit ${LOG_REDIRECT}")
        elif [ -f "/sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_input" ]; then
            _check_filepath "/sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_input"
            _check_filepath "/sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_max"
            _check_filepath "/sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_crit"
            temp_input=$(eval "cat /sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_input ${LOG_REDIRECT}")
            temp_max=$(eval "cat /sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_max ${LOG_REDIRECT}")
            temp_crit=$(eval "cat /sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_crit ${LOG_REDIRECT}")
        else
            _echo "sysfs of CPU core temperature not found!!!"
        fi

        _echo "[CPU Core Temp${cpu_temp_array[${i}]} Input   ]: ${temp_input}"
        _echo "[CPU Core Temp${cpu_temp_array[${i}]} Max     ]: ${temp_max}"
        _echo "[CPU Core Temp${cpu_temp_array[${i}]} Crit    ]: ${temp_crit}"
        _echo ""
    done
}

function _show_cpu_temperature {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_cpu_temperature_sysfs
    fi
}

function _show_cpld_interrupt_sysfs {
    _banner "Show CPLD Interrupt"

    psu_interrupt=$(eval "cat /sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/psu_intr")
    fan_interrupt=$(eval "cat /sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/fan_intr")
    port_interrupt=$(eval "cat /sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/port_intr")

    if [ "${MODEL_NAME}" == "S9510-30XC" ]; then
        _echo "[PSU Interrupt Reg Raw    ]: ${psu_interrupt}"
        _echo "[FAN Interrupt Reg Raw    ]: ${fan_interrupt}"
        _echo "[Port Interrupt Reg Raw   ]: ${port_interrupt}"
        _echo ""
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

}

function _show_cpld_interrupt {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_cpld_interrupt_sysfs
    fi
}

function _show_system_led_sysfs {
    _banner "Show System LED"

    if [ "${MODEL_NAME}" == "S9510-30XC" ]; then
        if _check_filepath "/sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/led_ctrl_1" &&
           _check_filepath "/sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/led_ctrl_2" &&
           _check_filepath "/sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/led_status_1"; then

            led_ctrl_1=$(eval "cat /sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/led_ctrl_1 ${LOG_REDIRECT}")
            led_ctrl_2=$(eval "cat /sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/led_ctrl_2 ${LOG_REDIRECT}")
            led_status_1=$(eval "cat /sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/led_status_1 ${LOG_REDIRECT}")

            # System LED
            sys_color=$(((led_ctrl_1 & 2#00010000) >>4)) # (1: green, 0: yellow)
            sys_blink=$(((led_ctrl_1 & 2#01000000) >>6)) # (1: blinking, 0: solid)
            sys_onoff=$(((led_ctrl_1 & 2#10000000) >>7)) # (1: on, 0: off)

            LED_YELLOW_SOLID="YELLOW SOLID"
            LED_YELLOW_BLINKING="YELLOW BLINKING"
            LED_GREEN_SOLID="GREEN SOLID"
            LED_GREEN_BLINKING="GREEN BLINKING"
            LED_OFF="OFF"

            case "${sys_onoff}_${sys_color}_${sys_blink}" in
                '1_0_0')
                    sys_str=${YELLOW_SOLID}
                    ;;
                '1_0_1')
                    sys_str=${LED_YELLOW_BLINKING}
                    ;;
                '1_1_0')
                    sys_str=${LED_GREEN_SOLID}
                    ;;
                '1_1_1')
                    sys_str=${LED_GREEN_BLINKING}
                    ;;
                *)
                    sys_str=${LED_OFF}
            esac

            # FAN LED
            fan_color=$(((led_status_1 & 2#00000001) >>0)) # (1: green, 0: yellow)
            fan_blink=$(((led_status_1 & 2#00000100) >>2)) # (1: blinking, 0: solid)
            fan_onoff=$(((led_status_1 & 2#00001000) >>3)) # (1: on, 0: off)
            case "${fan_onoff}_${fan_color}_${fan_blink}" in
                '1_0_0')
                    fan_str=${YELLOW_SOLID}
                    ;;
                '1_0_1')
                    fan_str=${LED_YELLOW_BLINKING}
                    ;;
                '1_1_0')
                    fan_str=${LED_GREEN_SOLID}
                    ;;
                '1_1_1')
                    fan_str=${LED_GREEN_BLINKING}
                    ;;
                *)
                    fan_str=${LED_OFF}
            esac


            # PSU LED
            psu_color=$(((led_status_1 & 2#00010000) >>4)) # (1: green, 0: yellow)
            psu_blink=$(((led_status_1 & 2#01000000) >>6)) # (1: blinking, 0: solid)
            psu_onoff=$(((led_status_1 & 2#10000000) >>7)) # (1: on, 0: off)
            case "${psu_onoff}_${psu_color}_${psu_blink}" in
                '1_0_0')
                    psu_str=${YELLOW_SOLID}
                    ;;
                '1_0_1')
                    psu_str=${LED_YELLOW_BLINKING}
                    ;;
                '1_1_0')
                    psu_str=${LED_GREEN_SOLID}
                    ;;
                '1_1_1')
                    psu_str=${LED_GREEN_BLINKING}
                    ;;
                *)
                    psu_str=${LED_OFF}
            esac


            # Sync LED
            sync_color=$(((led_ctrl_2 & 2#00000001) >>0)) # (1: green, 0: yellow)
            sync_blink=$(((led_ctrl_2 & 2#00000100) >>2)) # (1: blinking, 0: solid)
            sync_onoff=$(((led_ctrl_2 & 2#00001000) >>3)) # (1: on, 0: off)
            case "${sync_onoff}_${sync_color}_${sync_blink}" in
                '1_0_0')
                    sync_str=${YELLOW_SOLID}
                    ;;
                '1_0_1')
                    sync_str=${LED_YELLOW_BLINKING}
                    ;;
                '1_1_0')
                    sync_str=${LED_GREEN_SOLID}
                    ;;
                '1_1_1')
                    sync_str=${LED_GREEN_BLINKING}
                    ;;
                *)
                    sync_str=${LED_OFF}
            esac


            # GNSS LED
            gnss_color=$(((led_ctrl_1 & 2#00000001) >>0)) # (1: green, 0: yellow)
            gnss_blink=$(((led_ctrl_1 & 2#00000100) >>2)) # (1: blinking, 0: solid)
            gnss_onoff=$(((led_ctrl_1 & 2#00001000) >>3)) # (1: on, 0: off)
            case "${gnss_onoff}_${gnss_color}_${gnss_blink}" in
                '1_0_0')
                    gnss_str=${YELLOW_SOLID}
                    ;;
                '1_0_1')
                    gnss_str=${LED_YELLOW_BLINKING}
                    ;;
                '1_1_0')
                    gnss_str=${LED_GREEN_SOLID}
                    ;;
                '1_1_1')
                    gnss_str=${LED_GREEN_BLINKING}
                    ;;
                *)
                    gnss_str=${LED_OFF}
            esac

            _echo "[System LED Color   ]: ${sys_color}"
            _echo "[System LED Blinking]: ${sys_blink}"
            _echo "[System LED ON-OFF  ]: ${sys_onoff}"
            _echo "[System LED STRING  ]: ${sys_str}"
            _echo "[FAN LED Color      ]: ${fan_color}"
            _echo "[FAN LED Blinking   ]: ${fan_blink}"
            _echo "[FAN LED ON-OFF     ]: ${fan_onoff}"
            _echo "[FAN LED STRING     ]: ${fan_str}"
            _echo "[PSU LED Color      ]: ${psu_color}"
            _echo "[PSU LED Blinking   ]: ${psu_blink}"
            _echo "[PSU LED ON-OFF     ]: ${psu_onoff}"
            _echo "[PSU LED STRING     ]: ${psu_str}"
            _echo "[SYNC LED Color     ]: ${sync_color}"
            _echo "[SYNC LED Blinking  ]: ${sync_blink}"
            _echo "[SYNC LED ON-OFF    ]: ${sync_onoff}"
            _echo "[SYNC LED STRING    ]: ${sync_str}"
            _echo "[GNSS LED Color     ]: ${gnss_color}"
            _echo "[GNSS LED Blinking  ]: ${gnss_blink}"
            _echo "[GNSS LED ON-OFF    ]: ${gnss_onoff}"
            _echo "[GNSS LED STRING    ]: ${gnss_str}"
        else
            _echo "FILE led_ctrl_1, led_ctrl_2, or led_status_1 not exist, exit!!!"
        fi
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _show_system_led {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_system_led_sysfs
    fi
}

function _show_beacon_led_sysfs {
    # Not Support
    return 0
}

function _show_beacon_led {
    # Not Support
    return 0
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_beacon_led_sysfs
    fi
}

function _show_ioport {
    _banner "Show ioport (LPC)"


    base=0x700
    offset=0x0
    reg=$(( ${base} + ${offset} ))
    reg=`printf "0x%X\n" ${reg}`
    ret=""

    while [ "${reg}" != "0x800" ]
    do
        ret=`${IOGET} "${reg}"`
        offset=$(( ${offset} + 1 ))
        reg=$(( ${base} + ${offset} ))
        reg=`printf "0x%X\n" ${reg}`
        _echo "${ret}"
    done
}

function _show_onlpdump {
    _banner "Show onlpdump"
    
    which onlpdump > /dev/null 2>&1
    ret_onlpdump=$?

    if [ ${ret_onlpdump} -eq 0 ]; then
        cmd_array=("onlpdump -d" \
                   "onlpdump -s" \
                   "onlpdump -r" \
                   "onlpdump -e" \
                   "onlpdump -o" \
                   "onlpdump -x" \
                   "onlpdump -i" \
                   "onlpdump -p" \
                   "onlpdump -S")
        for (( i=0; i<${#cmd_array[@]}; i++ ))
        do
            _echo "[Command]: ${cmd_array[$i]}"
            ret=$(eval "${cmd_array[$i]} ${LOG_REDIRECT} | tr -d '\0'")
            _echo "${ret}"
            _echo ""
        done
    else
        _echo "Not support!"
    fi
}

function _show_onlps {
    _banner "Show onlps"
    
    which onlps > /dev/null 2>&1
    ret_onlps=$?

    if [ ${ret_onlps} -eq 0 ]; then
        cmd_array=("onlps chassis onie show -" \
                   "onlps chassis asset show -" \
                   "onlps chassis env -" \
                   "onlps sfp inventory -" \
                   "onlps sfp bitmaps -" \
                   "onlps chassis debug show -")
        for (( i=0; i<${#cmd_array[@]}; i++ ))
        do
            _echo "[Command]: ${cmd_array[$i]}"
            ret=$(eval "${cmd_array[$i]} ${LOG_REDIRECT} | tr -d '\0'")
            _echo "${ret}"
            _echo ""
        done
    else
        _echo "Not support!"
    fi
}

function _show_cpld_error_log {
    # Not Support
    return 0
}

function _show_memory_correctable_error_count {
    # Not Support
    return 0
}

function _show_usb_info {
    _banner "Show USB Info"


    _echo "[Command]: lsusb -t"
    ret=$(eval "lsusb -t ${LOG_REDIRECT}")
    _echo "${ret}"
    _echo ""

    # FIXME
    # _echo "[Command]: lsusb -v"
    # ret=$(eval "lsusb -v ${LOG_REDIRECT}")
    # _echo "${ret}"
    # _echo ""

    # FIXME no /sys/bus/usb/devices/*/idVendor sysfs
    # _echo "[Command]: grep 046b /sys/bus/usb/devices/*/idVendor"
    # ret=$(eval "grep 046b /sys/bus/usb/devices/*/idVendor ${LOG_REDIRECT}")
    # _echo "${ret}"
    # _echo ""

    # FIXME no following sysfs
    # # check usb auth
    # _echo "[USB Port Authentication]: "
    # usb_auth_file_array=("/sys/bus/usb/devices/usb1/authorized" \
    #                      "/sys/bus/usb/devices/usb1/authorized_default" \
    #                      "/sys/bus/usb/devices/1-4/authorized" \
    #                      "/sys/bus/usb/devices/1-4.1/authorized" \
    #                      "/sys/bus/usb/devices/1-4:1.0/authorized" )

    # for (( i=0; i<${#usb_auth_file_array[@]}; i++ ))
    # do
    #     _check_filepath "${usb_auth_file_array[$i]}"
    #     if [ -f "${usb_auth_file_array[$i]}" ]; then
    #         ret=$(eval "cat ${usb_auth_file_array[$i]} ${LOG_REDIRECT}")
    #         _echo "${usb_auth_file_array[$i]}: $ret"
    #     else
    #         _echo "${usb_auth_file_array[$i]}: -1"
    #     fi
    # done
}

function _show_scsi_device_info {
    _banner "Show SCSI Device Info"

    scsi_device_info=$(eval "cat /proc/scsi/sg/device_strs ${LOG_REDIRECT}")
    _echo "[SCSI Device Info]: "
    _echo "${scsi_device_info}"
    _echo ""
}

function _show_onie_upgrade_info {
    _banner "Show ONIE Upgrade Info"

    if [ -d "/sys/firmware/efi" ]; then
        if [ ! -d "/mnt/onie-boot/" ]; then
            mkdir /mnt/onie-boot
        fi

        mount LABEL=ONIE-BOOT /mnt/onie-boot/
        onie_show_version=`/mnt/onie-boot/onie/tools/bin/onie-version`
        onie_show_pending=`/mnt/onie-boot/onie/tools/bin/onie-fwpkg show-pending`
        onie_show_result=`/mnt/onie-boot/onie/tools/bin/onie-fwpkg show-results`
        onie_show_log=`/mnt/onie-boot/onie/tools/bin/onie-fwpkg show-log`
        umount /mnt/onie-boot/

        _echo "[ONIE Show Version]:"
        _echo "${onie_show_version}"
        _echo ""
        _echo "[ONIE Show Pending]:"
        _echo "${onie_show_pending}"
        _echo ""
        _echo "[ONIE Show Result ]:"
        _echo "${onie_show_result}"
        _echo ""
        _echo "[ONIE Show Log    ]:"
        _echo "${onie_show_log}"
    else
        _echo "BIOS is in Legacy Mode!!!!!"
    fi
}

function _show_disk_info {
    _banner "Show Disk Info"

    cmd_array=("lsblk" \
               "parted -l" \
               "fdisk -l /dev/sda" \
               "find /sys/fs/ -name errors_count -print -exec cat {} \;" \
               "find /sys/fs/ -name first_error_time -print -exec cat {} \; -exec echo '' \;" \
			   "find /sys/fs/ -name last_error_time -print -exec cat {} \; -exec echo '' \;" \
               "df -h")

    for (( i=0; i<${#cmd_array[@]}; i++ ))
    do
        _echo "[Command]: ${cmd_array[$i]}"
        ret=$(eval "${cmd_array[$i]} ${LOG_REDIRECT}")
        _echo "${ret}"
        _echo ""
    done

    # check smartctl command
    cmd="smartctl -a /dev/sda"
    ret=`which smartctl`
    if [ ! $? -eq 0 ]; then
        _echo "[command]: ($cmd) not found (SKIP)!!"
    else
        ret=$(eval "$cmd ${LOG_REDIRECT}")
        _echo "[command]: $cmd"
        _echo "${ret}"
    fi

}

function _show_lspci {
    _banner "Show lspci Info"

    ret=`lspci`
    _echo "${ret}"
    _echo ""

    _echo "[PCI Bridge Hotplug Status]: "
    pci_device_id=($(lspci | grep "PLX Technology" | awk '{print $1}'))
    for i in "${pci_device_id[@]}"
    do
        ret=`lspci -vvv -s ${i} | grep HotPlug`
        _echo "${i} ${ret}"
    done
}

function _show_lspci_detail {
    _banner "Show lspci Detail Info"

    ret=$(eval "lspci -xxxx -vvv ${LOG_REDIRECT}")
    _echo "${ret}"
}

function _show_proc_interrupt {
    _banner "Show Proc Interrupts"

    for i in {1..5};
    do
        ret=$(eval "cat /proc/interrupts ${LOG_REDIRECT}")
        _echo "[Proc Interrupts ($i)]:"
        _echo "${ret}"
        _echo ""
        sleep 1
    done
}

function _show_ipmi_info {
    _banner "Show IPMI Info"

    ipmi_folder="/proc/ipmi/0/"

    if [ -d "${ipmi_folder}" ]; then
        ipmi_file_array=($(ls ${ipmi_folder}))
        for (( i=0; i<${#ipmi_file_array[@]}; i++ ))
        do
            _echo "[Command]: cat ${ipmi_folder}/${ipmi_file_array[$i]} "
            ret=$(eval "cat "${ipmi_folder}/${ipmi_file_array[$i]}" ${LOG_REDIRECT}")
            _echo "${ret}"
            _echo ""
        done
    else
        _echo "Warning, folder not found (${ipmi_folder})!!!"
    fi

    _echo "[Command]: lsmod | grep ipmi "
    ret=`lsmod | grep ipmi`
    _echo "${ret}"
}

function _show_bios_info {
    _banner "Show BIOS Info"

    cmd_array=("dmidecode -t 0" \
               "dmidecode -t 1" \
               "dmidecode -t 2" \
               "dmidecode -t 3")

    for (( i=0; i<${#cmd_array[@]}; i++ ))
    do
        _echo "[Command]: ${cmd_array[$i]} "
        ret=$(eval "${cmd_array[$i]} ${LOG_REDIRECT}")
        _echo "${ret}"
        _echo ""
    done
}

function _show_bmc_info {
    _banner "Show BMC Info"

    cmd_array=("ipmitool mc info" "ipmitool lan print" "ipmitool sel info" \
               "ipmitool fru -v" "ipmitool power status" \
               "ipmitool channel info 0xf" "ipmitool channel info 0x1" \
               "ipmitool sol info 0x1" \
               "ipmitool mc watchdog get" "ipmitool mc info -I usb")

    for (( i=0; i<${#cmd_array[@]}; i++ ))
    do
        _echo "[Command]: ${cmd_array[$i]} "
        ret=$(eval "${cmd_array[$i]} ${LOG_REDIRECT}")
        _echo "${ret}"
        _echo ""
    done

}

function _show_bmc_device_status {
    # Not Support
    return 0
}

function _show_bmc_sensors {
    _banner "Show BMC Sensors"

    ret=$(eval "ipmitool sensor ${LOG_REDIRECT}")
    _echo "[Sensors]:"
    _echo "${ret}"
}

function _show_bmc_sel_raw_data {
    # Not Support
    return 0
}

function _show_bmc_sel_elist {
    _banner "Show BMC SEL"

    ret=$(eval "ipmitool sel elist ${LOG_REDIRECT}")
    _echo "[SEL Record]:"
    _echo "${ret}"
}

function _show_bmc_sel_elist_detail {
    _banner "Show BMC SEL Detail -- Abnormal Event"

    _echo "    Show BMC SEL details, please wait..."
    sel_id_list=""

    readarray sel_array < <(ipmitool sel elist 2> /dev/null)

    for (( i=0; i<${#sel_array[@]}; i++ ))
    do
        if [[ "${sel_array[$i]}" == *"Undetermined"* ]] ||
           [[ "${sel_array[$i]}" == *"Bus"* ]] ||
           [[ "${sel_array[$i]}" == *"CATERR"* ]] ||
           [[ "${sel_array[$i]}" == *"OEM"* ]] ; then
            _echo  "${sel_array[$i]}"
            sel_id=($(echo "${sel_array[$i]}" | awk -F" " '{print $1}'))
            sel_id_list="${sel_id_list} 0x${sel_id}"
        fi
    done

    if [ ! -z "${sel_id_list}" ]; then
        sel_detail=$(eval "ipmitool sel get ${sel_id_list} ${LOG_REDIRECT}")
    else
        sel_detail=""
    fi

    _echo "[SEL Record ID]: ${sel_id_list}"
    _echo ""
    _echo "[SEL Detail   ]:"
    _echo "${sel_detail}"
}

function _show_dmesg {
    _banner "Show Dmesg"

    ret=$(eval "dmesg ${LOG_REDIRECT}")
    _echo "${ret}"
}

function _additional_log_collection {
    _banner "Additional Log Collection"

    if [ -z "${LOG_FOLDER_PATH}" ] || [ ! -d "${LOG_FOLDER_PATH}" ]; then
        _echo "LOG_FOLDER_PATH (${LOG_FOLDER_PATH}) not found!!!"
        _echo "do nothing..."
    else

        if [ -f "/var/log/kern.log" ]; then
            _echo "copy /var/log/kern.log* to ${LOG_FOLDER_PATH}"
            cp /var/log/kern.log*  "${LOG_FOLDER_PATH}"
        fi

        if [ -f "/var/log/dmesg" ]; then
            _echo "copy /var/log/dmesg* to ${LOG_FOLDER_PATH}"
            cp /var/log/dmesg*  "${LOG_FOLDER_PATH}"
        fi
    fi
}

function _compression {
    _banner "Compression"

    if [ ! -z "${LOG_FOLDER_PATH}" ] && [ -d "${LOG_FOLDER_PATH}" ]; then
        cd "${LOG_FOLDER_ROOT}"
        tar -zcf "${LOG_FOLDER_NAME}".tgz "${LOG_FOLDER_NAME}"

        echo "The tarball is ready at ${LOG_FOLDER_ROOT}/${LOG_FOLDER_NAME}.tgz"
        _echo "The tarball is ready at ${LOG_FOLDER_ROOT}/${LOG_FOLDER_NAME}.tgz"
    fi
}

function _main {
    echo "The script will take a few minutes, please wait..."
    _check_env
    _pkg_version
    _pre_log
    _show_board_info
    _show_version
    _show_i2c_tree
    _show_i2c_device_info
    _show_sys_devices
    _show_cpu_eeprom
    _show_gpio
    _show_psu_status_cpld
#   _show_rov # Not support
    _show_port_status
    _show_cpu_temperature
    _show_cpld_interrupt
    _show_system_led
#   _show_beacon_led # Not support
    _show_ioport
    _show_onlpdump
    _show_onlps
    _show_system_info
#   _show_cpld_error_log # Not support
#   _show_memory_correctable_error_count # Not support
    _show_grub
    _show_driver
    _show_usb_info
    _show_scsi_device_info
    _show_onie_upgrade_info
    _show_disk_info
    _show_lspci
    _show_lspci_detail
    _show_proc_interrupt
    _show_bios_info
    _show_bmc_info
    _show_bmc_sensors
#   _show_bmc_device_status # Not support
#   _show_bmc_sel_raw_data # Not support
    _show_bmc_sel_elist
    _show_bmc_sel_elist_detail
    _show_dmesg
    _additional_log_collection
    _compression

    echo "#   done..."
}

_main

