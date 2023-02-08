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


# Log Redirection
# LOG_REDIRECT="2> /dev/null": remove the error message from console
# LOG_REDIRECT=""            : show the error message in console
# LOG_REDIRECT="2>&1"        : show the error message in stdout, then stdout may send to console or file in _echo()
LOG_REDIRECT="2>&1"

# GPIO_MAX: update by function _update_gpio_max
GPIO_MAX=0
GPIO_MAX_INIT_FLAG=0

# I2C Bus
i801_bus=""
ismt_bus=""

# CPLD max index
MAX_CPLD=3
CPLD_BUS=2

# Execution Time
start_time=$(date +%s)
end_time=0
elapsed_time=0

SYSFS_LPC="/sys/bus/platform/devices/x86_64_ufispace_s9500_54cf_lpc"

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

function _update_gpio_max {
    _banner "Update GPIO MAX"
    local sysfs="/sys/devices/platform/x86_64_ufispace_s9500_54cf_lpc/bsp/bsp_gpio_max"

    GPIO_MAX=$(cat ${sysfs})
    if [ $? -eq 1 ]; then
        GPIO_MAX_INIT_FLAG=0
    else
        GPIO_MAX_INIT_FLAG=1
    fi

    _echo "[GPIO_MAX_INIT_FLAG]: ${GPIO_MAX_INIT_FLAG}"
    _echo "[GPIO_MAX]: ${GPIO_MAX}"
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
        echo "${LOG_FILE_NAME}" > "${LOG_FILE_PATH}"
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
    #_update_gpio_max #Not Support
}

function _check_filepath {
    filepath=$1
    if [ -z "${filepath}" ]; then
        _echo "ERROR, the ipnut string is empty!!"
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
        _echo "ERROR, the ipnut string is empty!!!"
        return ${FALSE}
    fi

    value=$(eval "i2cget -y -f ${i2c_bus} ${i2c_addr} ${LOG_REDIRECT}")
    ret=$?

    if [ $ret -eq 0 ]; then
        return ${TRUE}
    else
        _echo "ERROR: No such device: ${i2c_addr}"
        return ${FALSE}
    fi
}

function _check_bsp_init {
    _banner "Check BSP Init"

    i2c_bus_0=$(eval "i2cdetect -y ${i801_bus} ${LOG_REDIRECT} | grep UU")
    ret=$?
    if [ $ret -eq 0 ] && [ ! -z "${i2c_bus_0}" ] ; then
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

    if [ ! -f "/boot/grub/grub.cfg" ]; then
        return 0
    fi

    _banner "Show GRUB Info"

    grub_info=`cat /boot/grub/grub.cfg`

    _echo "[GRUB Info     ]:"
    _echo "${grub_info}"

}

function _show_driver {
    _banner "Show Kernel Driver"

    cmd_array=("lsmod" \
               "cat /lib/modules/$(uname -r)/modules.builtin")

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

    _show_i2c_tree_bus_0
    _show_i2c_tree_bus_0
    _show_i2c_tree_bus_0
}

function _show_board_info {
    _banner "Show Board Info"

    # CPLD1 0x700 Register Definition
    build_rev_id_array=(0 1 2 3)
    build_rev_array=(1 2 3 4)
    hw_rev_id_array=(0 1 2 3)
    deph_name_array=("NPI" "GA")
    hw_rev_array=("Proto" "Alpha" "Beta" "PVT")
    hw_rev_ga_array=("GA_1" "GA_2" "GA_3" "GA_4")
    model_id_array=($((2#11111110)))
    model_name_array=("S9500-54CF")

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

    # Build Rev D[3:5]
    build_rev_id=$(((board_rev_id & 2#00011000) >> 3))
    build_rev=${build_rev_array[${build_rev_id}]}

    # MODEL ID D[0:7]
    for (( i=0; i<${#model_id_array[@]}; i++ )); do
        if [ $model_id -eq ${model_id_array[$i]} ]; then
            model_name=${model_name_array[$i]}
            break
        fi
    done

    if [ -z "$model_name" ]; then
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
    bios_boot_rom=`${IOGET} 0x602`
    if [ $? -eq 0 ]; then
        bios_boot_rom=`echo ${bios_boot_rom} | awk -F" " '{print $NF}'`
    fi

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

function _cpld_version_i2c {
    _banner "Show CPLD Version (I2C)"

    if [[ "${MODEL_NAME}" == *"S9500-54CF"* ]]; then
        # MB CPLD
        mb_cpld_base_addr=0x30
        ver_reg=0x2
        build_reg=0x4
        ver_val=""
        build_val=""


        _check_i2c_device "${ismt_bus}" "0x72"
        value=$(eval "i2cget -y -f "${ismt_bus}" 0x72 ${LOG_REDIRECT}")
        ret=$?

        if [ ${ret} -eq 0 ]; then
            for (( i=0; i<MAX_CPLD; i++ )); do
                cpld_addr=$(($mb_cpld_base_addr+$i))
                i2cset -y -f "${ismt_bus}" 0x72 0x1
                _check_i2c_device "${ismt_bus}" "$cpld_addr"
                ver_val=$(eval "i2cget -y -f "${ismt_bus}" $cpld_addr $ver_reg ${LOG_REDIRECT}")
                build_val=$(eval "i2cget -y -f "${ismt_bus}" $cpld_addr $build_reg ${LOG_REDIRECT}")
                i2cset -y -f "${ismt_bus}" 0x72 0x0
                _echo "[MB CPLD$(( $i+1 )) Version]: $(( (ver_val & 2#11000000) >> 6 )).$(( ver_val & 2#00111111 )).$(( build_val & 2#11111111 ))"
            done
        fi
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _cpld_version_lpc {
    _banner "Show CPLD Version (LPC)"

    if [ "${MODEL_NAME}" == "S9500-54CF" ]; then
        # MB CPLD S9500-54CF
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

    if [[ "${MODEL_NAME}" == *"S9500-54CF"* ]]; then
        # MB CPLD
        mb_cpld_base_addr=30
        cpld_ver_h_attr="cpld_version_h"

        for (( i=0; i<MAX_CPLD; i++ )); do
            sysfs_path="/sys/bus/i2c/devices/${CPLD_BUS}-00$(($mb_cpld_base_addr+$i))"
            _check_filepath "$sysfs_path/$cpld_ver_h_attr"
            version_str=$(eval "cat $sysfs_path/$cpld_ver_h_attr ${LOG_REDIRECT}")
            _echo "[MB CPLD$(( $i+1 )) Version]: $version_str"
        done
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _cpld_version {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _cpld_version_sysfs
    else
        _cpld_version_i2c
    fi
}

function _show_version {
    _bios_version
    _bmc_version
    _cpld_version
}

function _show_i2c_tree_bus_0 {
    _banner "Show I2C Tree Bus 0"

    ret=$(eval "i2cdetect -y 0 ${LOG_REDIRECT}")

    _echo "[I2C Tree]:"
    _echo "${ret}"
}

function _show_i2c_mux_devices {
    local i2c_bus=$1
    local chip_addr=$2
    local channel_num=$3
    local chip_dev_desc=$4
    local i=0;

    if [ -z "${chip_addr}" ] || [ -z "${channel_num}" ] || [ -z "${chip_dev_desc}" ]; then
        _echo "ERROR: parameter cannot be empty!!!"
        exit 99
    fi

    _check_i2c_device "${i2c_bus}" "$chip_addr"
    ret=$?
    if [ "$ret" == "0" ]; then
        _echo "TCA9548 Mux ${chip_dev_desc}"
        _echo "---------------------------------------------------"
        for (( i=0; i<${channel_num}; i++ ))
        do
            _echo "TCA9548 Mux ${chip_dev_desc} - Channel ${i}"
            # open mux channel
            i2cset -y ${i2c_bus} ${chip_addr} $(( 2 ** ${i} ))
            # dump i2c tree
            ret=$(eval "i2cdetect -y ${i2c_bus} ${LOG_REDIRECT}")
            _echo "${ret}"
            # close mux channel
            i2cset -y ${i2c_bus} ${chip_addr} 0x0
            _echo ""
        done
    fi

}

function _show_i2c_tree_bus_mux_i2c {
    _banner "Show I2C Tree Bus MUX (I2C)"

    local i=0
    local chip_addr1=""
    local chip_addr2=""
    local chip_addr3=""
    local chip_addr1_chann=""
    local chip_addr2_chann=""

    if [[ "${MODEL_NAME}" == *"S9500-54CF"* ]]; then
        ## ROOT_SFP-0x75
        _show_i2c_mux_devices "${i801_bus}" "0x75" "8" "ROOT_SFP_0x75"

        ## ROOT_CPLD-0x72
        _show_i2c_mux_devices "${ismt_bus}" "0x72" "8" "ROOT_0x72"

        ## ROOT_SFP-0x72-Channel(0~7)-0x76-Channel(0~7)
        chip_addr1="0x75"
        chip_addr2="0x76"
        _check_i2c_device "${i801_bus}" "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            for (( chip_addr1_chann=0; chip_addr1_chann<=7; chip_addr1_chann++ ))
            do
                # open mux channel - 0x72 (chip_addr1)
                i2cset -y 0 ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
                _show_i2c_mux_devices "${i801_bus}" "${chip_addr2}" "8" "ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}"
                # close mux channel - 0x72 (chip_addr1)
                i2cset -y 0 ${chip_addr1} 0x0
            done
        fi

        ## ROOT_QSFP_QSFPDD-0x75-Channel(5~6)-0x76-Channel(0~7)
        chip_addr1="0x75"
        chip_addr2="0x76"
        _check_i2c_device "${i801_bus}" "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            for (( chip_addr1_chann=5; chip_addr1_chann<=6; chip_addr1_chann++ ))
            do
                # open mux channel - 0x75 (chip_addr1)
                i2cset -y 0 ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
                _show_i2c_mux_devices "${i801_bus}" "${chip_addr2}" "8" "ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}"
                # close mux channel - 0x75 (chip_addr1)
                i2cset -y 0 ${chip_addr1} 0x0
            done
        fi
    else
        echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _show_i2c_tree {
    _banner "Show I2C Tree"

    _show_i2c_tree_bus_0

    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _echo "TBD"
    else
        _show_i2c_tree_bus_mux_i2c
    fi

    _show_i2c_tree_bus_0
}

function _show_i2c_device_info {
    _banner "Show I2C Device Info"

    local pca954x_device_id=("")
    if [[ "${MODEL_NAME}" == *"S9500-54CF"* ]]; then
        pca954x_device_id=("0x75")
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

    _echo "[Command]: ls /sys/class/gpio/"
    #ret=($(ls /sys/class/gpio/))
    #_echo "#${ret[*]}"
    ret=`ls -al /sys/class/gpio/`
    _echo "${ret}"

    local file_path="/sys/kernel/debug/gpio"
    if [ -f "${file_path}" ]; then
        _echo ""
        _echo "[Command]: cat ${file_path}"
        _echo "$(cat ${file_path})"
    fi

    _echo ""
    _echo "[Command]: ls /sys/bus/i2c/devices/"
    #ret=($(ls /sys/bus/i2c/devices/))
    #_echo "#${ret[*]}"
    ret=`ls -al /sys/bus/i2c/devices/`
    _echo "${ret}"

    _echo ""
    _echo "[Command]: ls -al /dev/"
    #ret=($(ls -al /dev/))
    #_echo "#${ret[*]}"
    ret=`ls -al /dev/`
    _echo "${ret}"
}

function _show_cpu_eeprom_i2c {
    _banner "Show CPU EEPROM"

    #first read return empty content
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} 0x57 c")
    #second read return correct content
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
    if [[ "${MODEL_NAME}" == *"S9500-54CF"* ]]; then
        bus_id=${CPLD_BUS}
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    # Read PSU Status
    _check_filepath "/sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status"
    cpld_psu_status_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status ${LOG_REDIRECT}")

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

function _show_sfp_port_status_sysfs {
    _banner "Show SFP Port Status / EEPROM"
    echo "    Show SFP Port Status / EEPROM, please wait..."

    bus_id=""
    port_status_cpld_addr_array=""
    port_status_sysfs_idx_array=""
    port_status_bit_idx_array=""

    if [[ "${MODEL_NAME}" == *"S9500-54CF"* ]]; then
        bus_id="2"
        sfp_port_eeprom_bus_id_base=18

        port_status_cpld_addr_array=("0031" "0031" "0031" "0031" "0031" "0031" "0031" "0031" \
                                     "0031" "0031" "0031" "0031" "0031" "0031" "0031" "0031" \
                                     "0031" "0031" "0031" "0031" "0031" "0031" "0031" "0031" \
                                     "0032" "0032" "0032" "0032" "0032" "0032" "0032" "0032" \
                                     "0032" "0032" "0032" "0032" "0032" "0032" "0032" "0032" \
                                     "0032" "0032" "0032" "0032" "0032" "0032" "0032" "0032" \
                                     "0032" "0032" "0032" "0032" "0032" "0032")

        port_status_sysfs_idx_array=("0_7" "0_7" "0_7" "0_7" "0_7" "0_7" "0_7" "0_7" \
                                     "8_15" "8_15" "8_15" "8_15" "8_15" "8_15" "8_15" "8_15" \
                                     "16_23" "16_23" "16_23" "16_23" "16_23" "16_23" "16_23" "16_23" \
                                     "24_31" "24_31" "24_31" "24_31" "24_31" "24_31" "24_31" "24_31" \
                                     "32_39" "32_39" "32_39" "32_39" "32_39" "32_39" "32_39" "32_39" \
                                     "40_47" "40_47" "40_47" "40_47" "40_47" "40_47" "40_47" "40_47" \
                                     "48_53" "48_53" "48_53" "48_53" "48_53" "48_53")

        port_status_bit_idx_array=("0" "1" "2" "3" "4" "5" "6" "7" \
                                   "0" "1" "2" "3" "4" "5" "6" "7" \
                                   "0" "1" "2" "3" "4" "5" "6" "7" \
                                   "0" "1" "2" "3" "4" "5" "6" "7" \
                                   "0" "1" "2" "3" "4" "5" "6" "7" \
                                   "0" "1" "2" "3" "4" "5" "6" "7" \
                                   "0" "1" "2" "3" "4" "5")

        for (( i=0; i<${#port_status_cpld_addr_array[@]}; i++ ))
        do
            # Module SFP Port RX LOS (0:Undetected, 1:Detected)
            sysfs="/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_sfp_rx_los_${port_status_sysfs_idx_array[${i}]}"
            _check_filepath $sysfs
            port_rx_los_reg=$(eval "cat $sysfs ${LOG_REDIRECT}")

            port_rx_los=$(( (port_rx_los_reg & 1 << ${port_status_bit_idx_array[i]}) >> ${port_status_bit_idx_array[i]} ))

            # Module SFP Port Absent Status (0:Present, 1:Absence)
            sysfs="/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_sfp_present_${port_status_sysfs_idx_array[${i}]}"
            _check_filepath $sysfs
            port_absent_reg=$(eval "cat $sysfs ${LOG_REDIRECT}")
            port_absent=$(( (port_absent_reg & 1 << ${port_status_bit_idx_array[i]}) >> ${port_status_bit_idx_array[i]} ))

            # Module SFP Port TX FAULT (0:Undetected, 1:Detected)
            sysfs="/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_sfp_tx_fault_${port_status_sysfs_idx_array[${i}]}"
            _check_filepath $sysfs
            port_tx_fault_reg=$(eval "cat $sysfs ${LOG_REDIRECT}")
            port_tx_fault=$(( (port_tx_fault_reg & 1 << ${port_status_bit_idx_array[i]}) >> ${port_status_bit_idx_array[i]} ))

            # Module SFP Port TX DISABLE (0:Disabled, 1:Enabled)
            sysfs="/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_sfp_tx_disable_${port_status_sysfs_idx_array[${i}]}"
            _check_filepath $sysfs
            port_tx_disable_reg=$(eval "cat $sysfs ${LOG_REDIRECT}")
            port_tx_disable=$(( (port_tx_fault_reg & 1 << ${port_status_bit_idx_array[i]}) >> ${port_status_bit_idx_array[i]} ))

            # Module SFP Port Dump EEPROM
            if [ "${port_absent}" == "0" ]; then
                sysfs="/sys/bus/i2c/devices/$((sfp_port_eeprom_bus_id_base + i))-0050/eeprom"
                _check_filepath $sysfs

                port_eeprom_p0_1st=$(eval  "dd if=$sysfs bs=128 count=2 skip=0  status=none ${LOG_REDIRECT} | hexdump -C")
                port_eeprom_p0_2nd=$(eval  "dd if=$sysfs bs=128 count=2 skip=0  status=none ${LOG_REDIRECT} | hexdump -C")
                if [ -z "$port_eeprom_p0_1st" ]; then
                    port_eeprom_p0_1st="ERROR!!! The result is empty. It should read failed ($sysfs)!!"
                fi

                # Full EEPROM Log
                if [ "${LOG_FILE_ENABLE}" == "1" ]; then
                    hexdump -C $sysfs > ${LOG_FOLDER_PATH}/sfp_port${i}_eeprom.log 2>&1
                fi
            else
                port_eeprom_p0_1st="N/A"
                port_eeprom_p0_2nd="N/A"
            fi

            _echo "[SFP Port${i} RX LOS Reg Raw]: ${port_rx_los_reg}"
            _echo "[SFP Port${i} RX LOS]: ${port_rx_los}"
            _echo "[SFP Port${i} Module Absent Reg Raw]: ${port_absent_reg}"
            _echo "[SFP Port${i} Module Absent]: ${port_absent}"
            _echo "[SFP Port${i} TX FAULT Reg Raw]: ${port_tx_fault_reg}"
            _echo "[SFP Port${i} TX FAULT]: ${port_tx_fault}"
            _echo "[SFP Port${i} TX DISABLE Reg Raw]: ${port_tx_disable_reg}"
            _echo "[SFP Port${i} TX DISABLE]: ${port_tx_disable}"
            _echo "[Port${i} EEPROM Page0-0(1st)]:"
            _echo "${port_eeprom_p0_1st}"
            _echo "[Port${i} EEPROM Page0-0(2nd)]:"
            _echo "${port_eeprom_p0_2nd}"
            _echo ""
        done
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _show_sfp_port_status {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_sfp_port_status_sysfs
    fi
}

function _show_qsfp_port_status_sysfs {
    # Not Support
    return 0
}

function _show_qsfp_port_status {
    # Not Support
    return 0
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_qsfp_port_status_sysfs
    fi
}

function _show_mgmt_sfp_status_sysfs {
    # Not Support
    return 0
}

function _show_mgmt_sfp_status {
    # Not Support
    return 0
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_mgmt_sfp_status_sysfs
    fi
}

function _show_cpu_temperature_sysfs {
    _banner "show CPU Temperature"

    for ((i=1;i<=14;i++))
    do
        if [ -f "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${i}_input" ]; then
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${i}_input"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${i}_max"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${i}_crit"
            temp_input=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${i}_input ${LOG_REDIRECT}")
            temp_max=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${i}_max ${LOG_REDIRECT}")
            temp_crit=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${i}_crit ${LOG_REDIRECT}")
        elif [ -f "/sys/devices/platform/coretemp.0/temp${i}_input" ]; then
            _check_filepath "/sys/devices/platform/coretemp.0/temp${i}_input"
            _check_filepath "/sys/devices/platform/coretemp.0/temp${i}_max"
            _check_filepath "/sys/devices/platform/coretemp.0/temp${i}_crit"
            temp_input=$(eval "cat /sys/devices/platform/coretemp.0/temp${i}_input ${LOG_REDIRECT}")
            temp_max=$(eval "cat /sys/devices/platform/coretemp.0/temp${i}_max ${LOG_REDIRECT}")
            temp_crit=$(eval "cat /sys/devices/platform/coretemp.0/temp${i}_crit ${LOG_REDIRECT}")
        else
            _echo "sysfs of CPU Temp${i} temperature not found!!!"
            continue
        fi

        _echo "[CPU Core Temp${i} Input   ]: ${temp_input}"
        _echo "[CPU Core Temp${i} Max     ]: ${temp_max}"
        _echo "[CPU Core Temp${i} Crit    ]: ${temp_crit}"
        _echo ""
    done
}

function _show_cpu_temperature {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_cpu_temperature_sysfs
    fi
}

function _show_system_led_sysfs {
    _banner "Show System LED"

    if [[ "${MODEL_NAME}" == *"S9500-54CF"* ]]; then
        led_fan_path="/sys/bus/i2c/devices/${CPLD_BUS}-0030/cpld_system_led_fan"
        led_gnss_path="/sys/bus/i2c/devices/${CPLD_BUS}-0030/cpld_system_led_gnss"
        led_sync_path="/sys/bus/i2c/devices/${CPLD_BUS}-0030/cpld_system_led_sync"
        led_sys_path="/sys/bus/i2c/devices/${CPLD_BUS}-0030/cpld_system_led_sys"
        led_psu0_path="/sys/bus/i2c/devices/${CPLD_BUS}-0030/cpld_system_led_psu_0"
        led_psu1_path="/sys/bus/i2c/devices/${CPLD_BUS}-0030/cpld_system_led_psu_1"
        led_id_path="/sys/bus/i2c/devices/${CPLD_BUS}-0030/cpld_system_led_id"
        _check_filepath ${led_fan_path}
        _check_filepath ${led_gnss_path}
        _check_filepath ${led_sync_path}
        _check_filepath ${led_sys_path}
        _check_filepath ${led_psu0_path}
        _check_filepath ${led_psu1_path}
        _check_filepath ${led_id_path}
        system_led_fan=$(eval "cat ${led_fan_path} ${LOG_REDIRECT}")
        system_led_gnss=$(eval "cat ${led_gnss_path} ${LOG_REDIRECT}")
        system_led_sync=$(eval "cat ${led_sync_path} ${LOG_REDIRECT}")
        system_led_sys=$(eval "cat ${led_sys_path} ${LOG_REDIRECT}")
        system_led_psu0=$(eval "cat ${led_psu0_path} ${LOG_REDIRECT}")
        system_led_psu1=$(eval "cat ${led_psu1_path} ${LOG_REDIRECT}")
        system_led_id=$(eval "cat ${led_id_path} ${LOG_REDIRECT}")
        _echo "[System LED - Fan ]: ${system_led_fan}"
        _echo "[System LED - GNSS]: ${system_led_gnss}"
        _echo "[System LED - SYNC]: ${system_led_sync}"
        _echo "[System LED - SYS ]: ${system_led_sys}"
        _echo "[System LED - PSU0]: ${system_led_psu0}"
        _echo "[System LED - PSU1]: ${system_led_psu1}"
        _echo "[System LED - ID  ]: ${system_led_id}"
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

    base=0x600
    offset=0x0
    reg=$(( ${base} + ${offset} ))
    reg=`printf "0x%X\n" ${reg}`
    ret=""

    while [ "${reg}" != "0x700" ]
    do
        ret=`${IOGET} "${reg}"`
        offset=$(( ${offset} + 1 ))
        reg=$(( ${base} + ${offset} ))
        reg=`printf "0x%X\n" ${reg}`
        _echo "${ret}"
    done

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

    base=0x700
    offset=0x0
    reg=$(( ${base} + ${offset} ))
    reg=`printf "0x%X\n" ${reg}`
    ret=""

    while [ "${reg}" != "0xF00" ]
    do
        ret=`${IOGET} "${reg}"`
        offset=$(( ${offset} + 1 ))
        reg=$(( ${base} + ${offset} ))
        reg=`printf "0x%X\n" ${reg}`
        _echo "${ret}"
    done

    ret=$(eval "${IOGET} 0x501 ${LOG_REDIRECT}")
    _echo "${ret}"
    ret=$(eval "${IOGET} 0xf000 ${LOG_REDIRECT}")
    _echo "${ret}"
    ret=$(eval "${IOGET} 0xf011 ${LOG_REDIRECT}")
    _echo "${ret}"
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

#require skld cpu cpld 1.12.016 and later
function _show_cpld_error_log {
    # Not Support
    return 0
}

# Note: In order to prevent affecting MCE mechanism,
#       the function will not clear the 0x425 and 0x429 registers at step 1.1/1.2,
#       and only use to show the current correctable error count.
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

    _echo "[Command]: lsusb -v"
    ret=$(eval "lsusb -v ${LOG_REDIRECT}")
    _echo "${ret}"
    _echo ""

    _echo "[Command]: grep 046b /sys/bus/usb/devices/*/idVendor"
    ret=$(eval "grep 046b /sys/bus/usb/devices/*/idVendor ${LOG_REDIRECT}")
    _echo "${ret}"
    _echo ""

    # check usb auth
    _echo "[USB Port Authentication]: "
    usb_auth_file_array=("/sys/bus/usb/devices/usb1/authorized" \
                         "/sys/bus/usb/devices/usb1/authorized_default" \
                         "/sys/bus/usb/devices/1-3/authorized" \
                         "/sys/bus/usb/devices/1-3.1/authorized" \
                         "/sys/bus/usb/devices/1-3:1.0/authorized" )

    for (( i=0; i<${#usb_auth_file_array[@]}; i++ ))
    do
        _check_filepath "${usb_auth_file_array[$i]}"
        if [ -f "${usb_auth_file_array[$i]}" ]; then
            ret=$(eval "cat ${usb_auth_file_array[$i]} ${LOG_REDIRECT}")
            _echo "${usb_auth_file_array[$i]}: $ret"
        else
            _echo "${usb_auth_file_array[$i]}: -1"
        fi
    done
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
               "lsblk -O" \
               "parted -l /dev/sda" \
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

    #FIXME
    #ipmitool fru -v
    #ipmitool i2c bus=2 0x80 0x1 0x5c (mux ctrl)
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
    #Not Support
    return 0
}

function _show_bmc_sensors {
    _banner "Show BMC Sensors"

    ret=$(eval "ipmitool sensor ${LOG_REDIRECT}")
    _echo "[Sensors]:"
    _echo "${ret}"
}

function _show_bmc_sel_raw_data {
    _banner "Show BMC SEL Raw Data"
    echo "    Show BMC SEL Raw Data, please wait..."

    if [ "${LOG_FILE_ENABLE}" == "1" ]; then
        _echo "[SEL RAW Data]:"
        ret=$(eval "ipmitool sel save ${LOG_FOLDER_PATH}/sel_raw_data.log > /dev/null ${LOG_REDIRECT}")
        _echo "The file is located at ${LOG_FOLDER_NAME}/sel_raw_data.log"
    else
        _echo "[SEL RAW Data]:"
        ret=$(eval "ipmitool sel save /tmp/log/sel_raw_data.log > /dev/null ${LOG_REDIRECT}")
        cat /tmp/log/sel_raw_data.log
        rm /tmp/log/sel_raw_data.log
    fi
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
        #_echo "copy /var/log/syslog* to ${LOG_FOLDER_PATH}"
        #cp /var/log/syslog*  "${LOG_FOLDER_PATH}"

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

function _show_time {
    _banner "Show Execution Time"
    end_time=$(date +%s)
    elapsed_time=$(( end_time - start_time ))

    ret=`date -d @${start_time}`
    _echo "[Start Time ] ${ret}"

    ret=`date -d @${end_time}`
    _echo "[End Time   ] ${ret}"

    _echo "[Elapse Time] ${elapsed_time} seconds"
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
    _show_psu_status_cpld
    _show_rov
    _show_sfp_port_status
    _show_qsfp_port_status
    _show_mgmt_sfp_status
    _show_cpu_temperature
    _show_system_led
    _show_beacon_led
    _show_ioport
    _show_onlpdump
    _show_onlps
    _show_system_info
    _show_cpld_error_log
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
    _show_bmc_device_status
    _show_bmc_sel_raw_data
    _show_bmc_sel_elist
    _show_bmc_sel_elist_detail
    _show_dmesg
    _additional_log_collection
    _show_time
    _compression

    echo "#   done..."
}

_main