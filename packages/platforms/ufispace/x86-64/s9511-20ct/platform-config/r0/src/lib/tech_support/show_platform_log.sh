#!/bin/bash

#Tech Support script version
TS_VERSION="1.0.1"

# TRUE=0, FALSE=1
TRUE=0
FALSE=1

# Device Serial Number
SN=$(dmidecode -s chassis-serial-number)
if [ ! $? -eq 0 ]; then
    SN=""
elif [[ $SN = *" "* ]]; then
    #SN contains space charachater inside
    SN=""
fi

# DATESTR: The format of log folder and log file
DATESTR=$(date +"%Y%m%d%H%M%S")
LOG_FOLDER_NAME=""
LOG_FILE_NAME=""

# LOG_FOLDER_ROOT: The root folder of log files
LOG_FOLDER_ROOT=""
LOG_FOLDER_PATH=""
LOG_FILE_PATH=""
LOG_FAST=${FALSE}

# PLAT: This script is compatible with the platform.
PLAT="S9511-20CT"
# MODEL_NAME: set by function _board_info
MODEL_NAME=""
# HW_REV: set by function _board_info
HW_REV=""
# HW_EXT: set by function _board_info
HW_EXT=""
# BSP_INIT_FLAG: set by function _check_bsp_init
BSP_INIT_FLAG=""

SCRIPTPATH="$( cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

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
# LOG_REDIRECT="2> /dev/null"        : remove the error message from console
# LOG_REDIRECT=""                    : show the error message in console
# LOG_REDIRECT="2>> $LOG_FILE_PATH"  : show the error message in stdout, then stdout may send to console or file in _echo()
LOG_REDIRECT=""

# GPIO_MAX: update by function _update_gpio_max
GPIO_MAX=0
GPIO_MAX_INIT_FLAG=0
GPIO_BASE=0
GPIO_BASE_INIT_FLAG=0

# I2C Bus
i801_bus=""
ismt_bus=""

# Sysfs
SYSFS_DEV="/sys/bus/i2c/devices"
SYSFS_CPLD1="${SYSFS_DEV}/10-0033"
SYSFS_CPLD2="${SYSFS_DEV}/6-0027"
SYSFS_LPC="/sys/devices/platform/x86_64_ufispace_s9511_20ct_lpc"

# Port Type
PORT_T_QSFPDD=1
PORT_T_QSFP=2
PORT_T_SFP=3
PORT_T_MGMT=4

# Execution Time
start_time=$(date +%s)
end_time=0
elapsed_time=0

# Options
OPT_BYPASS_I2C_COMMAND=${FALSE}

function _echo {
    str="$@"

    if [ "${LOG_FILE_ENABLE}" == "1" ] && [ -f "${LOG_FILE_PATH}" ]; then
        echo "${str}" >> "${LOG_FILE_PATH}"
    else
        echo "${str}"
    fi
}

function _printf {
    if [ "${LOG_FILE_ENABLE}" == "1" ] && [ -f "${LOG_FILE_PATH}" ]; then
        printf "$@" >> "${LOG_FILE_PATH}"
    else
        printf "$@"
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
    _banner "Package Version = ${TS_VERSION}"
}

function _show_ts_version {
    echo "Package Version = ${TS_VERSION}"
}

function _update_gpio_max {
    _banner "Update GPIO MAX and GPIO BASE"
    local sysfs_gpio_max="${SYSFS_LPC}/bsp/bsp_gpio_max"
    local sysfs_gpio_base="${SYSFS_LPC}/bsp/bsp_gpio_base"

    GPIO_MAX=$(cat ${sysfs_gpio_max})
    if [ $? -eq 1 ]  || [ "$GPIO_MAX" == "-1" ]; then
        GPIO_MAX_INIT_FLAG=0
    else
        GPIO_MAX_INIT_FLAG=1
    fi

    GPIO_BASE=$(cat ${sysfs_gpio_base})
    if [ $? -eq 1 ] || [ "$GPIO_BASE" == "-1" ]; then
        GPIO_BASE_INIT_FLAG=0
    else
        GPIO_BASE_INIT_FLAG=1
    fi

    _echo "[GPIO_MAX_INIT_FLAG]: ${GPIO_MAX_INIT_FLAG}"
    _echo "[GPIO_MAX]: ${GPIO_MAX}"

    _echo "[GPIO_BASE_INIT_FLAG]: ${GPIO_BASE_INIT_FLAG}"
    _echo "[GPIO_BASE]: ${GPIO_BASE}"
}

function _dd_read_byte {
    reg=$1
    echo "0x"`dd if=/dev/port bs=1 count=1 skip=$((reg)) status=none | xxd -g 1 | cut -d ' ' -f 2`
}

function _check_env {
    # _banner "Check Environment"

    # check basic commands
    cmd_array=("i2cdump" "lsusb" "dmidecode")
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

        if [ ! -d "${LOG_FOLDER_PATH}" ]; then
            _echo "[ERROR] invalid log path: ${LOG_FOLDER_PATH}"
            exit 1
        fi

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
    _update_gpio_max
}

function _check_filepath {
    filepath=$1
    if [ -z "${filepath}" ]; then
        _echo "ERROR, the ipnut string is empty!!!"
        return ${FALSE}
    elif [ ! -f "$filepath" ]; then
        _echo "ERROR: No such file: ${filepath}"
        return ${FALSE}
    else
        # _echo "File Path: ${filepath}"
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
        _echo "ERROR: No such device: Bus:${i2c_bus}, Address: ${i2c_addr}"
        return ${FALSE}
    fi
}

function _check_bsp_init {
    _banner "Check BSP Init"

    # As our bsp init status, we look at bsp_version.
    if [ -f "${SYSFS_LPC}/bsp/bsp_version" ]; then
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

    if _check_filepath "/sys/bus/i2c/devices/i2c-1/name" ;then
        i2c_1=`cat /sys/bus/i2c/devices/i2c-1/name`
    fi

    if echo "$i2c_0" | grep -q "I801"; then
        i801_bus=0
    fi

    if echo "$i2c_1" | grep -q "I801"; then
        i801_bus=1
    fi

    if echo "$i2c_0" | grep -q "iSMT"; then
        ismt_bus=0
    fi

    if echo "$i2c_1" | grep -q "iSMT"; then
        ismt_bus=1
    fi
}

function _show_system_info {
    _banner "Show System Info"

    x86_date=`date`
    x86_uptime=`uptime`
    last_login=`last`

    _echo "[X86 Date Time ]: ${x86_date}"
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
    hw_ext_name_array=("S9511-20CT")
    model_id_array=($((2#11101111)))
    model_name_array=("S9511-20CT")

    model_id=$(_dd_read_byte 0x700)
    ret=$?
    if [ $ret -eq 0 ]; then
        model_id=$((model_id))
    else
        _echo "Get board model id failed ($ret), Exit!!"
        exit $ret
    fi

    board_rev_id=$(_dd_read_byte 0x701)
    ret=$?
    if [ $ret -eq 0 ]; then
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
    model_id=$(((model_id & 2#11111111) >> 0))
    if [ $model_id -eq ${model_id_array[0]} ]; then
        model_name=${model_name_array[0]}
    else
        _echo "Invalid model_id: ${model_id}"
    fi

    hw_ext_id=$(_dd_read_byte 0x706)
    ret=$?
    if [ $ret -eq 0 ]; then
        hw_ext_id=$(((hw_ext_id & 2#00000111) >> 0))
    else
        _echo "Get extended id failed ($ret), Exit!!"
        exit $ret
    fi
    hw_ext_name=${hw_ext_name_array[${hw_ext_id}]}

    MODEL_NAME=${model_name}
    HW_REV=${hw_rev}
    HW_EXT=${hw_ext_id}
    _echo "[CPLD 0x0/0x1/0x6 Reg Raw ]: ${model_id} ${board_rev_id} ${hw_ext_id}"
    _echo "[Board Type               ]: ${model_name}"
    _echo "[Extended ID              ]: ${hw_ext_name}"
    _echo "[Design Phase             ]: ${deph_name}"
    _echo "[Hardware Revision        ]: ${hw_rev}"
    _echo "[BUILD_ID                 ]: ${build_rev}"
}

function _bios_version {
    _banner "Show BIOS Version"

    bios_ver=$(eval "cat /sys/class/dmi/id/bios_version ${LOG_REDIRECT}")
    bios_boot_sel=$(_dd_read_byte 0xE30C)

    # EC BIOS BOOT SEL D[6]
    bios_boot_sel=$(((bios_boot_sel & 2#01000000) >> 6))

    _echo "[BIOS Vesion  ]: ${bios_ver}"
    _echo "[BIOS Boot ROM]: ${bios_boot_sel}"
}

function _cpld_version_lpc {
    # Not Support
    return 0
    # _banner "Show CPLD Version (LPC)"
}

function _cpld_version_i2c {
    if [ "${OPT_BYPASS_I2C_COMMAND}" == "${TRUE}" ]; then
        _banner "Show CPLD Version (I2C) (Bypass)"
        return
    fi

    _banner "Show CPLD Version (I2C)"

    if [[ $MODEL_NAME == "${PLAT}" ]]; then

        local cpld1_i2c_bus=${10}
        local cpld2_i2c_bus=${6}
        local mux_i2c_addr=0x33

        # MB CPLD
        mb_cpld1_ver=""
        mb_cpld2_ver=""
        mb_cpld1_build=""
        mb_cpld2_build=""

        # CPLD 1-2

        _check_i2c_device ${cpld1_i2c_bus} ${mux_i2c_addr}
        ret=$?

        if [ ${ret} -eq 0 ]; then
            if _check_i2c_device ${cpld1_i2c_bus} "0x33"; then
                mb_cpld1_ver=$(eval "i2cget -y -f ${cpld1_i2c_bus} 0x33 0x2 ${LOG_REDIRECT}")
                mb_cpld1_build=$(eval "i2cget -y -f ${cpld1_i2c_bus} 0x33 0x1 ${LOG_REDIRECT}")
                _printf "[MB CPLD1 Version]: %d.%02d.%03d\n" $(( (mb_cpld1_ver & 2#11000000) >> 6)) $(( mb_cpld1_ver & 2#00111111 )) $((mb_cpld1_build))
            fi

            if _check_i2c_device ${cpld2_i2c_bus} "0x27"; then
                mb_cpld2_ver=$(eval "i2cget -y -f ${cpld2_i2c_bus} 0x27 0x2 ${LOG_REDIRECT}")
                mb_cpld2_build=$(eval "i2cget -y -f ${cpld2_i2c_bus} 0x27 0x4 ${LOG_REDIRECT}")
                _printf "[MB CPLD2 Version]: %d.%02d.%03d\n" $(( (mb_cpld2_ver & 2#11000000) >> 6)) $(( mb_cpld2_ver & 2#00111111 )) $((mb_cpld2_build))
            fi
            i2cset -y -f ${cpld2_i2c_bus} ${mux_i2c_addr} 0x0
        fi
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
    fi
}

function _cpld_version_sysfs {
    _banner "Show CPLD Version (Sysfs)"

    if [ "${MODEL_NAME}" == "${PLAT}" ]; then
        if _check_filepath "$SYSFS_CPLD1/cpld_version_h"; then
            mb_cpld_ver=$(eval "cat $SYSFS_CPLD1/cpld_version_h ${LOG_REDIRECT}")
            _echo "[MB CPLD1 Version]: ${mb_cpld_ver}"
        fi

        if _check_filepath "$SYSFS_CPLD2/cpld_version_h"; then
            mb_cpld_ver=$(eval "cat $SYSFS_CPLD2/cpld_version_h ${LOG_REDIRECT}")
            _echo "[MB CPLD2 Version]: ${mb_cpld_ver}"
        fi
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
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
    _cpld_version
}

function _show_psu_type {
    _banner "Show PSU Type"

    if _check_filepath "/tmp/psu_type"; then
        psu_type_file=$(cat /tmp/psu_type)
        _echo "Read PSU Type from file= $((psu_type_file)) [0:DC PSU, 1:AC PSU]"
    else
        _echo "Failed to read PSU Type from /tmp/psu_type file(${psu_type_file})!!!"
    fi

    if _check_filepath "${SYSFS_CPLD1}/psu_type" ;then
        psu_type_cpld=$(cat ${SYSFS_CPLD1}/psu_type)
        _echo "Read PSU Type from cpld= ${psu_type_cpld} [0:DC PSU, 1:AC PSU]"
    else
        _echo "Failed to read PSU Type from cpld(${psu_type_cpld})!!!"
    fi

    psu_type=$((psu_type_cpld))
}

function _show_i2c_tree_bus {
    _banner "Show I2C Tree Bus 0"

    ret=$(eval "(time i2cdetect -y 0) ${LOG_REDIRECT}")

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
    local is_ch=${5:-false}
    local i=0;
    local startc=0;
    local endc=0;

    if [ -z "${chip_addr}" ] || [ -z "${channel_num}" ] || [ -z "${chip_dev_desc}" ]; then
        _echo "ERROR: parameter cannot be empty!!!"
        exit 99
    fi

    if $is_ch; then
        startc=${channel_num}
        endc=$(( ${channel_num} + 1 ))
    else
        startc=0
        endc=${channel_num}
    fi

    _check_i2c_device "$bus" "$chip_addr"
    ret=$?
    if [ "$ret" == "0" ]; then
        _echo "TCA9548 Mux ${chip_dev_desc}"
        _echo "---------------------------------------------------"
        for (( i=startc; i<${endc}; i++ ))
        do
            _echo "TCA9548 Mux ${chip_dev_desc} - Channel ${i}"
            # open mux channel
            i2cset -y ${bus} ${chip_addr} $(( 2 ** ${i} ))
            # dump i2c tree            
            ret=$(eval "(time i2cdetect -y ${bus}) ${LOG_REDIRECT}")
            _echo "${ret}"
            # close mux channel
            i2cset -y ${bus} ${chip_addr} 0x0
            _echo ""
        done
    fi
}

function _show_i2c_tree_bus_mux_i2c {
    if [ "${OPT_BYPASS_I2C_COMMAND}" == "${TRUE}" ]; then
        _banner "Show I2C Tree Bus MUX (I2C) (Bypass)"
        return
    fi

    _banner "Show I2C Tree Bus MUX (I2C)"

    local i=0
    local chip_addr1=""
    local bus=""

    if [ "${MODEL_NAME}" == "${PLAT}" ]; then
        # i801_bus
        bus="${i801_bus}"
        # 9546_ROOT
        chip_addr1="0x76"
        _check_i2c_device "${bus}" "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            ## (9548_ROOT_PORT)-0x76
            _show_i2c_mux_devices "${bus}" "${chip_addr1}" "8" "9548_ROOT_PORT-${chip_addr1}"
        fi

        # ismt_bus
        bus="${ismt_bus}"
        # 9548_CPLD
        chip_addr1="0x75"
        _check_i2c_device "${bus}" "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            local cpld_chann=0
            _show_i2c_mux_devices "${bus}" "${chip_addr1}" ${cpld_chann} "9546_ROOT_CPLD-${chip_addr1}" true
        fi
    else
        echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
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
    local pca954x_device_bus=("")
    if [ "${MODEL_NAME}" == "${PLAT}" ]; then
        pca954x_device_id=("0x76" "0x75")
        pca954x_device_bus=("${i801_bus}" "${ismt_bus}")
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
    fi

    for ((i=0;i<5;i++))
    do
        _echo "[DEV PCA9548 (${i})]"
        for (( j=0; j<${#pca954x_device_id[@]}; j++ ))
        do
            ret=`i2cget -f -y ${pca954x_device_bus[$j]} ${pca954x_device_id[$j]}`
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

function _show_sys_eeprom_i2c {
    _banner "Show System EEPROM"

    eeprom_addr="0x57"
    eeprom_mux=""
    eeprom_ch=""
    under_mux=false

    # read six times return empty content
    if [ "${eeprom_mux}" != "" ] && [ "${eeprom_ch}" != "" ]; then
        under_mux=true
    fi

    if $under_mux; then
        i2cset -y ${ismt_bus} ${eeprom_mux} $(( 2 ** ${eeprom_ch} ))
    fi
    sys_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")
    sys_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")
    sys_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")
    sys_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")
    sys_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")
    sys_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")

    #seventh read return correct content
    sys_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")

    if $under_mux; then
        i2cset -y -f ${ismt_bus} ${eeprom_mux} 0x0
    fi
    _echo "[System EEPROM]:"
    _echo "${sys_eeprom}"
}

function _show_sys_eeprom_sysfs {
    _banner "Show System EEPROM"

    sys_eeprom=$(eval "cat /sys/bus/i2c/devices/1-0057/eeprom ${LOG_REDIRECT} | hexdump -C")
    _echo "[System EEPROM]:"
    _echo "${sys_eeprom}"
}

function _show_sys_eeprom {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_sys_eeprom_sysfs
    else
        _show_sys_eeprom_i2c
    fi
}

function _show_psu_status_cpld_sysfs {
    _show_psu_type

    _banner "Show PSU Status (CPLD)"

    # Read PSU Status
    # 0: DC PSU, 1: AC PSU
    #### AC PSU ##############################################################################################
    if [ "${psu_type}" == "1" ]; then
        # Read PSU0 Absent Status (0: psu present, 1: psu absent)
        if _check_filepath "${SYSFS_CPLD1}/psu0_present"; then
            psu0_absent_l=$(eval "cat ${SYSFS_CPLD1}/psu0_present ${LOG_REDIRECT}")
            _echo "[PSU0 Absent Status (L)    ]: ${psu0_absent_l}"
        else
            _echo "[Error] ${SYSFS_CPLD1}/psu0_present not exist!!!"
        fi

        # Read PSU0 Power Good Status (1: power good, 0: power not ok)
        if _check_filepath "${SYSFS_CPLD1}/psu0_pwok"; then
            psu0_power_ok=$(eval "cat ${SYSFS_CPLD1}/psu0_pwok ${LOG_REDIRECT}")
            _echo "[PSU0 Power Good Status    ]: ${psu0_power_ok}"
        else
            _echo "[Error] ${SYSFS_CPLD1}/psu0_pwok not exist!!!"
        fi

        # Read PSU0 Vin Power Good Status (1: power good, 0: not providing power)
        if _check_filepath "${SYSFS_CPLD1}/psu0_vin_pwok"; then
            psu0_vin_power_ok=$(eval "cat ${SYSFS_CPLD1}/psu0_vin_pwok ${LOG_REDIRECT}")
            _echo "[PSU0 Vin Power Good Status]: ${psu0_vin_power_ok}"
        else
            _echo "[Error] ${SYSFS_CPLD1}/psu0_vin_pwok not exist!!!"
        fi

        # Read PSU1 Absent Status (0: psu present, 1: psu absent)
        if _check_filepath "${SYSFS_CPLD1}/psu1_present"; then
            psu1_absent_l=$(eval "cat ${SYSFS_CPLD1}/psu1_present ${LOG_REDIRECT}")
            _echo "[PSU1 Absent Status (L)    ]: ${psu1_absent_l}"
        else
            _echo "[Error] ${SYSFS_CPLD1}/psu1_present not exist!!!"
        fi

        # Read PSU1 Power Good Status (1: power good, 0: power not ok)
        if _check_filepath "${SYSFS_CPLD1}/psu1_pwok"; then
            psu1_power_ok=$(eval "cat ${SYSFS_CPLD1}/psu1_pwok ${LOG_REDIRECT}")
            _echo "[PSU1 Power Good Status    ]: ${psu1_power_ok}"
        else
            _echo "[Error] ${SYSFS_CPLD1}/psu1_pwok not exist!!!"
        fi

        # Read PSU1 Vin Power Good Status (1: power good, 0: not providing power)
        if _check_filepath "${SYSFS_CPLD1}/psu1_vin_pwok"; then
            psu1_vin_power_ok=$(eval "cat ${SYSFS_CPLD1}/psu1_vin_pwok ${LOG_REDIRECT}")
            _echo "[PSU1 Vin Power Good Status]: ${psu1_vin_power_ok}"
        else
            _echo "[Error] ${SYSFS_CPLD1}/psu1_vin_pwok not exist!!!"
        fi
    #### DC PSU ##############################################################################################
    elif [ "${psu_type}" == "0" ]; then
        # Read PSU0 Absent Status (0: psu present, 1: psu absent)
        if _check_filepath "${SYSFS_CPLD1}/psu0_present"; then
            psu0_absent_l=$(eval "cat ${SYSFS_CPLD1}/psu0_present ${LOG_REDIRECT}")
            _echo "[PSU0 Absent Status (L)    ]: ${psu0_absent_l}"
        else
            _echo "[Error] ${SYSFS_CPLD1}/psu0_present not exist!!!"
        fi

        # Read PSU0 Power Good Status (1: power good, 0: power not ok)
        if _check_filepath "${SYSFS_CPLD1}/psu0_pwok"; then
            psu0_power_ok=$(eval "cat ${SYSFS_CPLD1}/psu0_pwok ${LOG_REDIRECT}")
            _echo "[PSU0 Power Good Status    ]: ${psu0_power_ok}"
        else
            _echo "[Error] ${SYSFS_CPLD1}/psu0_pwok not exist!!!"
        fi

        # Read PSU0 Vin Power Good Status (1: power good, 0: not providing power)
        if _check_filepath "${SYSFS_CPLD1}/psu0_vin_pwok"; then
            psu0_vin_power_ok=$(eval "cat ${SYSFS_CPLD1}/psu0_vin_pwok ${LOG_REDIRECT}")
            _echo "[PSU0 Vin Power Good Status]: ${psu0_vin_power_ok}"
        else
            _echo "[Error] ${SYSFS_CPLD1}/psu0_vin_pwok not exist!!!"
        fi
    else
        _echo "[Error] Wrong PSU Type = ${psu_type}!!!"
    fi
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

function _eeprom_page_desc {
    eeprom_page=$1

    if [ "${eeprom_page}" == "0" ]; then
        echo "Lower Page 0 (00h)"
    else
        hex_page=$(printf "%02X" $((eeprom_page - 1)))
        echo "Upper Page $((eeprom_page - 1)) (${hex_page}h)"
    fi
}

function _eeprom_page_repeat_desc {
    loop_idx=$1
    loop_max=$2

    if [ "${loop_max}" == "1" ]; then
        echo ""
    else
        if [ "${loop_idx}" == "0" ]; then
            echo "(1st)"
        elif [ "${loop_idx}" == "1" ]; then
            echo "(2nd)"
        else
            echo "($((loop_idx + 1)))"
        fi
    fi
}

function _eeprom_pages_dump {
    local port=$1
    local type=$2
    local sysfs=$3

    local eeprom_page_array=(0  1  2  3  4  5
                             17 18 19
                             33 34 35 36 37 38 39 40
                             41 42 43 44 45 46 47 48 )

    local eeprom_repeat_array=(2 2 2 1 1 1
                               1 1 1
                               1 1 1 1 1 1 1 1
                               1 1 1 1 1 1 1 1 )

    if [ "${type}" !=  "${PORT_T_QSFP}" ] && [ "${type}" !=  "${PORT_T_QSFP}" ]; then
        # Only support QSFP & QSFPDD
        return 0
    fi

    for (( page_i=0; page_i<${#eeprom_page_array[@]}; page_i++ ))
    do
        for (( repeate_i=0; repeate_i<${eeprom_repeat_array[page_i]}; repeate_i++ ))
        do
            eeprom_page=$(eval  "dd if=${sysfs} bs=128 count=1 skip=${eeprom_page_array[${page_i}]}  status=none ${LOG_REDIRECT} | hexdump -C")

            if [ -z "$eeprom_page" ] && [ "${eeprom_repeat_array[page_i]}" == "0" ]; then
                eeprom_page="ERROR!!! The result is empty. It should read failed ${sysfs}!!"
            fi

            echo "[Port${port} EEPROM $(_eeprom_page_desc ${eeprom_page_array[page_i]}) $(_eeprom_page_repeat_desc ${repeate_i} ${eeprom_repeat_array[page_i]})]:" >> ${LOG_FOLDER_PATH}/port${port}_eeprom.log 2>&1
            echo "${eeprom_page}" >> ${LOG_FOLDER_PATH}/port${port}_eeprom.log 2>&1
        done
    done
}

function _show_port_status_sysfs {
    _banner "Show Port Status / EEPROM"

    if [ "${MODEL_NAME}" == "${PLAT}" ]; then

        port_absent_sysfs_array=(    "${SYSFS_CPLD2}/port_0_abs"
                                     "${SYSFS_CPLD2}/port_1_abs"
                                     "${SYSFS_CPLD2}/port_2_abs"
                                     "${SYSFS_CPLD2}/port_3_abs"
                                     "${SYSFS_CPLD2}/port_4_abs"
                                     "${SYSFS_CPLD2}/port_5_abs"
                                     "${SYSFS_CPLD2}/port_6_abs"
                                     "${SYSFS_CPLD2}/port_7_abs"
                                     "${SYSFS_CPLD2}/port_8_abs"
                                     "${SYSFS_CPLD2}/port_9_abs"
                                     "${SYSFS_CPLD2}/port_10_abs"
                                     "${SYSFS_CPLD2}/port_11_abs"
                                     "${SYSFS_CPLD2}/port_12_abs"
                                     "${SYSFS_CPLD2}/port_13_abs"
                                     "${SYSFS_CPLD2}/port_14_abs"
                                     "${SYSFS_CPLD2}/port_15_abs"
                                     "${SYSFS_CPLD2}/port_16_abs"
                                     "${SYSFS_CPLD2}/port_17_abs"
                                     "${SYSFS_CPLD2}/port_18_abs"
                                     "${SYSFS_CPLD2}/port_19_abs")

        port_rx_los_sysfs_array=(    "${SYSFS_CPLD2}/port_0_rx_los"
                                     "${SYSFS_CPLD2}/port_1_rx_los"
                                     "${SYSFS_CPLD2}/port_2_rx_los"
                                     "${SYSFS_CPLD2}/port_3_rx_los"
                                     "${SYSFS_CPLD2}/port_4_rx_los"
                                     "${SYSFS_CPLD2}/port_5_rx_los"
                                     "${SYSFS_CPLD2}/port_6_rx_los"
                                     "${SYSFS_CPLD2}/port_7_rx_los"
                                     "${SYSFS_CPLD2}/port_8_rx_los"
                                     "${SYSFS_CPLD2}/port_9_rx_los"
                                     "${SYSFS_CPLD2}/port_10_rx_los"
                                     "${SYSFS_CPLD2}/port_11_rx_los"
                                     "${SYSFS_CPLD2}/port_12_rx_los"
                                     "${SYSFS_CPLD2}/port_13_rx_los"
                                     "${SYSFS_CPLD2}/port_14_rx_los"
                                     "${SYSFS_CPLD2}/port_15_rx_los"
                                     "${SYSFS_CPLD2}/port_16_rx_los"
                                     "${SYSFS_CPLD2}/port_17_rx_los"
                                     "${SYSFS_CPLD2}/port_18_rx_los"
                                     "${SYSFS_CPLD2}/port_19_rx_los")

        port_tx_fault_sysfs_array=(  "${SYSFS_CPLD2}/port_0_tx_fault"
                                     "${SYSFS_CPLD2}/port_1_tx_fault"
                                     "${SYSFS_CPLD2}/port_2_tx_fault"
                                     "${SYSFS_CPLD2}/port_3_tx_fault"
                                     "${SYSFS_CPLD2}/port_4_tx_fault"
                                     "${SYSFS_CPLD2}/port_5_tx_fault"
                                     "${SYSFS_CPLD2}/port_6_tx_fault"
                                     "${SYSFS_CPLD2}/port_7_tx_fault"
                                     "${SYSFS_CPLD2}/port_8_tx_fault"
                                     "${SYSFS_CPLD2}/port_9_tx_fault"
                                     "${SYSFS_CPLD2}/port_10_tx_fault"
                                     "${SYSFS_CPLD2}/port_11_tx_fault"
                                     "${SYSFS_CPLD2}/port_12_tx_fault"
                                     "${SYSFS_CPLD2}/port_13_tx_fault"
                                     "${SYSFS_CPLD2}/port_14_tx_fault"
                                     "${SYSFS_CPLD2}/port_15_tx_fault"
                                     "${SYSFS_CPLD2}/port_16_tx_fault"
                                     "${SYSFS_CPLD2}/port_17_tx_fault"
                                     "${SYSFS_CPLD2}/port_18_tx_fault"
                                     "${SYSFS_CPLD2}/port_19_tx_fault")

        port_tx_disable_sysfs_array=("${SYSFS_CPLD2}/port_0_tx_disable"
                                     "${SYSFS_CPLD2}/port_1_tx_disable"
                                     "${SYSFS_CPLD2}/port_2_tx_disable"
                                     "${SYSFS_CPLD2}/port_3_tx_disable"
                                     "${SYSFS_CPLD2}/port_4_tx_disable"
                                     "${SYSFS_CPLD2}/port_5_tx_disable"
                                     "${SYSFS_CPLD2}/port_6_tx_disable"
                                     "${SYSFS_CPLD2}/port_7_tx_disable"
                                     "${SYSFS_CPLD2}/port_8_tx_disable"
                                     "${SYSFS_CPLD2}/port_9_tx_disable"
                                     "${SYSFS_CPLD2}/port_10_tx_disable"
                                     "${SYSFS_CPLD2}/port_11_tx_disable"
                                     "${SYSFS_CPLD2}/port_12_tx_disable"
                                     "${SYSFS_CPLD2}/port_13_tx_disable"
                                     "${SYSFS_CPLD2}/port_14_tx_disable"
                                     "${SYSFS_CPLD2}/port_15_tx_disable"
                                     "${SYSFS_CPLD2}/port_16_tx_disable"
                                     "${SYSFS_CPLD2}/port_17_tx_disable"
                                     "${SYSFS_CPLD2}/port_18_tx_disable"
                                     "${SYSFS_CPLD2}/port_19_tx_disable")

        port_rate_sel_sysfs_array=(  "${SYSFS_CPLD2}/port_0_rate_sel"
                                     "${SYSFS_CPLD2}/port_1_rate_sel"
                                     "${SYSFS_CPLD2}/port_2_rate_sel"
                                     "${SYSFS_CPLD2}/port_3_rate_sel"
                                     "${SYSFS_CPLD2}/port_4_rate_sel"
                                     "${SYSFS_CPLD2}/port_5_rate_sel"
                                     "${SYSFS_CPLD2}/port_6_rate_sel"
                                     "${SYSFS_CPLD2}/port_7_rate_sel"
                                     "${SYSFS_CPLD2}/port_8_rate_sel"
                                     "${SYSFS_CPLD2}/port_9_rate_sel"
                                     "${SYSFS_CPLD2}/port_10_rate_sel"
                                     "${SYSFS_CPLD2}/port_11_rate_sel"
                                     "${SYSFS_CPLD2}/port_12_rate_sel"
                                     "${SYSFS_CPLD2}/port_13_rate_sel"
                                     "${SYSFS_CPLD2}/port_14_rate_sel"
                                     "${SYSFS_CPLD2}/port_15_rate_sel"
                                     "${SYSFS_CPLD2}/port_16_rate_sel"
                                     "${SYSFS_CPLD2}/port_17_rate_sel"
                                     "${SYSFS_CPLD2}/port_18_rate_sel"
                                     "${SYSFS_CPLD2}/port_19_rate_sel")

        port_pwr_en_sysfs_array=(    "${SYSFS_CPLD2}/port_0_pwr_en"
                                     "${SYSFS_CPLD2}/port_1_pwr_en"
                                     "${SYSFS_CPLD2}/port_2_pwr_en"
                                     "${SYSFS_CPLD2}/port_3_pwr_en"
                                     "${SYSFS_CPLD2}/port_4_pwr_en"
                                     "${SYSFS_CPLD2}/port_5_pwr_en"
                                     "${SYSFS_CPLD2}/port_6_pwr_en"
                                     "${SYSFS_CPLD2}/port_7_pwr_en"
                                     "${SYSFS_CPLD2}/port_8_pwr_en"
                                     "${SYSFS_CPLD2}/port_9_pwr_en"
                                     "${SYSFS_CPLD2}/port_10_pwr_en"
                                     "${SYSFS_CPLD2}/port_11_pwr_en"
                                     "${SYSFS_CPLD2}/port_12_pwr_en"
                                     "${SYSFS_CPLD2}/port_13_pwr_en"
                                     "${SYSFS_CPLD2}/port_14_pwr_en"
                                     "${SYSFS_CPLD2}/port_15_pwr_en"
                                     "${SYSFS_CPLD2}/port_16_pwr_en"
                                     "${SYSFS_CPLD2}/port_17_pwr_en"
                                     "${SYSFS_CPLD2}/port_18_pwr_en"
                                     "${SYSFS_CPLD2}/port_19_pwr_en")

        port_name_array=(
            "0"   "1"   "2"   "3"   "4"   "5"   "6"   "7"
            "8"   "9"   "10"  "11"  "12"  "13"  "14"  "15"
            "16"  "17"  "18"  "19"
        )

        desc_absent=("Present" "Absence")
        desc_rx_los=("Normal" "RX Los Detected")
        desc_tx_fault=("Normal" "TX Fault")
        desc_tx_disable=("Normal" "TX Disable")
        desc_rate_sel=("Low Rate" "Full Rate")
        desc_pwr_en=("Enable" "Disable")

        local QSFPDD=${PORT_T_QSFPDD}
        local QSFP=${PORT_T_QSFP}
        local SFP=${PORT_T_SFP}
        local MGMT=${PORT_T_MGMT}
        port_type_array=(
        #   0        1        2        3        4        5        6        7
            ${SFP}   ${SFP}   ${SFP}   ${SFP}   ${SFP}   ${SFP}   ${SFP}   ${SFP}
        #   8        9        10       11       12       13       14       15
            ${SFP}   ${SFP}   ${SFP}   ${SFP}   ${SFP}   ${SFP}   ${SFP}   ${SFP}
        #   16       17       18       19
            ${SFP}   ${SFP}   ${SFP}   ${SFP}
        )

        # ref sys_port_array
        port_eeprom_bus_array=(
        #   0     1     2     3     4     5     6     7
            14    15    16    17    18    19    20    21
        #   8     9     10    11    12    13    14    15
            22    23    24    25    26    27    28    29
        #   16    17    18    19
            30    31    32    33
        )

        for (( i=0; i<${#port_name_array[@]}; i++ ))
        do
            local attr_val=""
            local sysfs_path=${port_absent_sysfs_array[${i}]}
            # Port Absent Status (0: Present, 1:Absence)
            if _check_filepath "${port_absent_sysfs_array[${i}]}"; then
                attr_val=$(eval "cat ${port_absent_sysfs_array[${i}]}")
                port_absent="$attr_val"
                _echo "[Port${i} Absent Status]: ${attr_val} (${desc_absent[${attr_val}]})"
            else
                _echo "[Error] Absent Unknown port number: ${i}"
            fi

            # Port RX LOS (0:Normal, 1:RX Los Detected)
            if [[ ${port_type_array[${i}]} == ${SFP} && -e "${port_rx_los_sysfs_array[${i}]}" ]]; then
                attr_val=$(cat "${port_rx_los_sysfs_array[${i}]}")
                _echo "[Port${i} RX LOS       ]: ${attr_val} (${desc_rx_los[${attr_val}]})"
            else
                _echo "[Error] RX LOS Unknown port number: ${i}"
            fi


            # Port TX Fault (0:Normal, 1:TX Fault)
            if [[ ${port_type_array[${i}]} == ${SFP} && -e "${port_tx_fault_sysfs_array[${i}]}" ]]; then
                attr_val=$(cat "${port_tx_fault_sysfs_array[${i}]}")
                _echo "[Port${i} TX Fault     ]: ${attr_val} (${desc_tx_fault[${attr_val}]})"
            else
                _echo "[Error] TX Fault Unknown port number: ${i}"
            fi


            # Port TX Disable (0: Normal, 1:TX Disable)
            if [[ ${port_type_array[${i}]} == ${SFP} && -e "${port_tx_disable_sysfs_array[${i}]}" ]]; then
                attr_val=$(cat "${port_tx_disable_sysfs_array[${i}]}")
                _echo "[Port${i} TX Disable   ]: ${attr_val} (${desc_tx_disable[${attr_val}]})"
            else
                _echo "[Error] TX Disable Unknown port number: ${i}"
            fi

            # Port Rate Select
            if [[ ${port_type_array[${i}]} == ${SFP} && -e "${port_rate_sel_sysfs_array[${i}]}" ]]; then
                attr_val=$(cat "${port_rate_sel_sysfs_array[${i}]}")
                _echo "[Port${i} Rate Select  ]: ${attr_val} (${desc_rate_sel[${attr_val}]})"
            else
                _echo "[Error] Rate Select Unknown port number: ${i}"
            fi

            # Port PWR Enable
            if [[ ${port_type_array[${i}]} == ${SFP} && -e "${port_pwr_en_sysfs_array[${i}]}" ]]; then
                attr_val=$(cat "${port_pwr_en_sysfs_array[${i}]}")
                _echo "[Port${i} PWR Enable   ]: ${attr_val} (${desc_pwr_en[${attr_val}]})"
            else
                _echo "[Error] PWR Enable Unknown port number: ${i}"
            fi

            local eeprom_path="/sys/bus/i2c/devices/${port_eeprom_bus_array[${i}]}-0050/eeprom"

            if [[ "${port_absent}" == "0x00" && -e "${eeprom_path}" ]]; then
                port_eeprom=$(dd if="${eeprom_path}" bs=128 count=2 skip=0 status=none | hexdump -C)
                if [ "${LOG_FILE_ENABLE}" == "1" ]; then
                    if [ "${port_type_array[${i}]}" == "${SFP}" ]; then
                        hexdump -C "${eeprom_path}" > "${LOG_FOLDER_PATH}/port${i}_eeprom.log" 2>&1
                    else
                        _eeprom_pages_dump "${i}" "${port_type_array[${i}]}" "${eeprom_path}"
                    fi
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
    fi
}

function _show_port_status {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_port_status_sysfs
    fi
}

function _show_cpu_temperature_sysfs {
    _banner "show CPU Temperature"

    cpu_temp_array=("1")

    for (( i=0; i<${#cpu_temp_array[@]}; i++ ))
    do
        if [ -f "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_input" ]; then
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_label"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_input"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_max"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_crit"
            temp_label=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_label ${LOG_REDIRECT}")
            temp_input=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_input ${LOG_REDIRECT}")
            temp_max=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_max ${LOG_REDIRECT}")
            temp_crit=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_crit ${LOG_REDIRECT}")
        elif [ -f "/sys/devices/platform/coretemp.0/hwmon/hwmon2/temp${cpu_temp_array[${i}]}_input" ]; then
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_label"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_input"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_max"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_crit"
            temp_label=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_label ${LOG_REDIRECT}")
            temp_input=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_input ${LOG_REDIRECT}")
            temp_max=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_max ${LOG_REDIRECT}")
            temp_crit=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon1/temp${cpu_temp_array[${i}]}_crit ${LOG_REDIRECT}")
        elif [ -f "/sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_input" ]; then
            _check_filepath "/sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_label"
            _check_filepath "/sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_input"
            _check_filepath "/sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_max"
            _check_filepath "/sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_crit"
            temp_label=$(eval "cat /sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_label ${LOG_REDIRECT}")
            temp_input=$(eval "cat /sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_input ${LOG_REDIRECT}")
            temp_max=$(eval "cat /sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_max ${LOG_REDIRECT}")
            temp_crit=$(eval "cat /sys/devices/platform/coretemp.0/temp${cpu_temp_array[${i}]}_crit ${LOG_REDIRECT}")
        else
            _echo "sysfs of CPU core temperature not found!!!"
        fi

        _echo "[CPU Core Temp${cpu_temp_array[${i}]} Label   ]: ${temp_label}"
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

    if [ $MODEL_NAME == "${PLAT}" ]; then
        cpld1_sysfs_array=( "intr_0"      # Timimg_Card
                            "intr_1"      # DPLL_1588
                            "intr_2"      # DPLL_SYNC
                            "intr_3"      # SI5395
                            "intr_4"      # SI5395_LOL
                            "intr_5"      # BITS
                            "intr_6"      # PSU0
                            "intr_7"      # PSU1
                            "intr_8"      # HWM_NMI
                            "intr_9"      # TSEN_NMI
                            "intr_10"     # TSEN_ALRT
                            "intr_11"     # TSEN_ALRT1
                            "intr_12"     # VDD_CORE_VRHOT
                            "intr_13"     # VDD_CORE_PINALRT
                            "intr_14"     # Fan0
                            "intr_15"     # Fan1
                            "intr_16"     # Fan2
                            "intr_17"     # Fan3
                            "intr_18"     # MAC
                            "intr_19"     # I210_ARLT
                            "intr_20"     # PHY
                            "intr_21"     # CPLD_IO_CPLD2
                            "intr_22"     # CPU_NMI
                            "intr_23"     # CPLD_to_CPU_I2C
                            "intr_24"     # CPLD_to_CPU_NMI
                            "intr_25"     # CPLD_to_CPU_PTP
                            "intr_26"     # CPLD_to_CPU_ETH
                            "intr_27"   ) # CPLD_to_CPU_thermal

        cpld1_sysfs_desc=(  "Timimg_Card"
                            "DPLL_1588"
                            "DPLL_SYNC"
                            "SI5395"
                            "SI5395_LOL"
                            "BITS"
                            "PSU0"
                            "PSU1"
                            "HWM_NMI"
                            "TSEN_NMI"
                            "TSEN_ALRT"
                            "TSEN_ALRT1"
                            "VDD_CORE_VRHOT"
                            "VDD_CORE_PINALRT"
                            "Fan0"
                            "Fan1"
                            "Fan2"
                            "Fan3"
                            "MAC"
                            "I210_ARLT"
                            "PHY"
                            "CPLD_IO_CPLD2"
                            "CPU_NMI"
                            "CPLD_to_CPU_I2C"
                            "CPLD_to_CPU_NMI"
                            "CPLD_to_CPU_PTP"
                            "CPLD_to_CPU_ETH"
                            "CPLD_to_CPU_Thermal"   )

        cpld2_sysfs_array=( "port_0_abs"
                            "port_1_abs"
                            "port_2_abs"
                            "port_3_abs"
                            "port_4_abs"
                            "port_5_abs"
                            "port_6_abs"
                            "port_7_abs"
                            "port_8_abs"
                            "port_9_abs"
                            "port_10_abs"
                            "port_11_abs"
                            "port_12_abs"
                            "port_13_abs"
                            "port_14_abs"
                            "port_15_abs"
                            "port_16_abs"
                            "port_17_abs"
                            "port_18_abs"
                            "port_19_abs"
                            "port_0_rx_los"
                            "port_1_rx_los"
                            "port_2_rx_los"
                            "port_3_rx_los"
                            "port_4_rx_los"
                            "port_5_rx_los"
                            "port_6_rx_los"
                            "port_7_rx_los"
                            "port_8_rx_los"
                            "port_9_rx_los"
                            "port_10_rx_los"
                            "port_11_rx_los"
                            "port_12_rx_los"
                            "port_13_rx_los"
                            "port_14_rx_los"
                            "port_15_rx_los"
                            "port_16_rx_los"
                            "port_17_rx_los"
                            "port_18_rx_los"
                            "port_19_rx_los"
                            "port_0_tx_fault"
                            "port_1_tx_fault"
                            "port_2_tx_fault"
                            "port_3_tx_fault"
                            "port_4_tx_fault"
                            "port_5_tx_fault"
                            "port_6_tx_fault"
                            "port_7_tx_fault"
                            "port_8_tx_fault"
                            "port_9_tx_fault"
                            "port_10_tx_fault"
                            "port_11_tx_fault"
                            "port_12_tx_fault"
                            "port_13_tx_fault"
                            "port_14_tx_fault"
                            "port_15_tx_fault"
                            "port_16_tx_fault"
                            "port_17_tx_fault"
                            "port_18_tx_fault"
                            "port_19_tx_fault"
                            "port_0_tx_disable"
                            "port_1_tx_disable"
                            "port_2_tx_disable"
                            "port_3_tx_disable"
                            "port_4_tx_disable"
                            "port_5_tx_disable"
                            "port_6_tx_disable"
                            "port_7_tx_disable"
                            "port_8_tx_disable"
                            "port_9_tx_disable"
                            "port_10_tx_disable"
                            "port_11_tx_disable"
                            "port_12_tx_disable"
                            "port_13_tx_disable"
                            "port_14_tx_disable"
                            "port_15_tx_disable"
                            "port_16_tx_disable"
                            "port_17_tx_disable"
                            "port_18_tx_disable"
                            "port_19_tx_disable"
                            "port_0_rate_sel"
                            "port_1_rate_sel"
                            "port_2_rate_sel"
                            "port_3_rate_sel"
                            "port_4_rate_sel"
                            "port_5_rate_sel"
                            "port_6_rate_sel"
                            "port_7_rate_sel"
                            "port_8_rate_sel"
                            "port_9_rate_sel"
                            "port_10_rate_sel"
                            "port_11_rate_sel"
                            "port_12_rate_sel"
                            "port_13_rate_sel"
                            "port_14_rate_sel"
                            "port_15_rate_sel"
                            "port_16_rate_sel"
                            "port_17_rate_sel"
                            "port_18_rate_sel"
                            "port_19_rate_sel"
                            "port_0_pwr_en"
                            "port_1_pwr_en"
                            "port_2_pwr_en"
                            "port_3_pwr_en"
                            "port_4_pwr_en"
                            "port_5_pwr_en"
                            "port_6_pwr_en"
                            "port_7_pwr_en"
                            "port_8_pwr_en"
                            "port_9_pwr_en"
                            "port_10_pwr_en"
                            "port_11_pwr_en"
                            "port_12_pwr_en"
                            "port_13_pwr_en"
                            "port_14_pwr_en"
                            "port_15_pwr_en"
                            "port_16_pwr_en"
                            "port_17_pwr_en"
                            "port_18_pwr_en"
                            "port_19_pwr_en"  )

        cpld2_sysfs_desc=(  "Port_0_SFP_ABS"
                            "Port_1_SFP_ABS"
                            "Port_2_SFP_ABS"
                            "Port_3_SFP_ABS"
                            "Port_4_SFPPLUS_ABS"
                            "Port_5_SFPPLUS_ABS"
                            "Port_6_SFPPLUS_ABS"
                            "Port_7_SFPPLUS_ABS"
                            "Port_8_SFPPLUS_ABS"
                            "Port_9_SFPPLUS_ABS"
                            "Port_10_SFPPLUS_ABS"
                            "Port_11_SFPPLUS_ABS"
                            "Port_12_SFP28_ABS"
                            "Port_13_SFP28_ABS"
                            "Port_14_SFP28_ABS"
                            "Port_15_SFP28_ABS"
                            "Port_16_SFP28_ABS"
                            "Port_17_SFP28_ABS"
                            "Port_18_SFP28_ABS"
                            "Port_19_SFP28_ABS"
                            "Port_0_SFP_Rx_Los"
                            "Port_1_SFP_Rx_Los"
                            "Port_2_SFP_Rx_Los"
                            "Port_3_SFP_Rx_Los"
                            "Port_4_SFPPLUS_Rx_Los"
                            "Port_5_SFPPLUS_Rx_Los"
                            "Port_6_SFPPLUS_Rx_Los"
                            "Port_7_SFPPLUS_Rx_Los"
                            "Port_8_SFPPLUS_Rx_Los"
                            "Port_9_SFPPLUS_Rx_Los"
                            "Port_10_SFPPLUS_Rx_Los"
                            "Port_11_SFPPLUS_Rx_Los"
                            "Port_12_SFP28_Rx_Los"
                            "Port_13_SFP28_Rx_Los"
                            "Port_14_SFP28_Rx_Los"
                            "Port_15_SFP28_Rx_Los"
                            "Port_16_SFP28_Rx_Los"
                            "Port_17_SFP28_Rx_Los"
                            "Port_18_SFP28_Rx_Los"
                            "Port_19_SFP28_Rx_Los"
                            "Port_0_SFP_Tx_Fault"
                            "Port_1_SFP_Tx_Fault"
                            "Port_2_SFP_Tx_Fault"
                            "Port_3_SFP_Tx_Fault"
                            "Port_4_SFPPLUS_Tx_Fault"
                            "Port_5_SFPPLUS_Tx_Fault"
                            "Port_6_SFPPLUS_Tx_Fault"
                            "Port_7_SFPPLUS_Tx_Fault"
                            "Port_8_SFPPLUS_Tx_Fault"
                            "Port_9_SFPPLUS_Tx_Fault"
                            "Port_10_SFPPLUS_Tx_Fault"
                            "Port_11_SFPPLUS_Tx_Fault"
                            "Port_12_SFP28_Tx_Fault"
                            "Port_13_SFP28_Tx_Fault"
                            "Port_14_SFP28_Tx_Fault"
                            "Port_15_SFP28_Tx_Fault"
                            "Port_16_SFP28_Tx_Fault"
                            "Port_17_SFP28_Tx_Fault"
                            "Port_18_SFP28_Tx_Fault"
                            "Port_19_SFP28_Tx_Fault"
                            "Port_0_SFP_Tx_Disable"
                            "Port_1_SFP_Tx_Disable"
                            "Port_2_SFP_Tx_Disable"
                            "Port_3_SFP_Tx_Disable"
                            "Port_4_SFPPLUS_Tx_Disable"
                            "Port_5_SFPPLUS_Tx_Disable"
                            "Port_6_SFPPLUS_Tx_Disable"
                            "Port_7_SFPPLUS_Tx_Disable"
                            "Port_8_SFPPLUS_Tx_Disable"
                            "Port_9_SFPPLUS_Tx_Disable"
                            "Port_10_SFPPLUS_Tx_Disable"
                            "Port_11_SFPPLUS_Tx_Disable"
                            "Port_12_SFP28_Tx_Disable"
                            "Port_13_SFP28_Tx_Disable"
                            "Port_14_SFP28_Tx_Disable"
                            "Port_15_SFP28_Tx_Disable"
                            "Port_16_SFP28_Tx_Disable"
                            "Port_17_SFP28_Tx_Disable"
                            "Port_18_SFP28_Tx_Disable"
                            "Port_19_SFP28_Tx_Disable"
                            "Port_0_SFP_Rate_Sel"
                            "Port_1_SFP_Rate_Sel"
                            "Port_2_SFP_Rate_Sel"
                            "Port_3_SFP_Rate_Sel"
                            "Port_4_SFPPLUS_Rate_Sel"
                            "Port_5_SFPPLUS_Rate_Sel"
                            "Port_6_SFPPLUS_Rate_Sel"
                            "Port_7_SFPPLUS_Rate_Sel"
                            "Port_8_SFPPLUS_Rate_Sel"
                            "Port_9_SFPPLUS_Rate_Sel"
                            "Port_10_SFPPLUS_Rate_Sel"
                            "Port_11_SFPPLUS_Rate_Sel"
                            "Port_12_SFP28_Rate_Sel"
                            "Port_13_SFP28_Rate_Sel"
                            "Port_14_SFP28_Rate_Sel"
                            "Port_15_SFP28_Rate_Sel"
                            "Port_16_SFP28_Rate_Sel"
                            "Port_17_SFP28_Rate_Sel"
                            "Port_18_SFP28_Rate_Sel"
                            "Port_19_SFP28_Rate_Sel"
                            "Port_0_SFP_PWR_EN"
                            "Port_1_SFP_PWR_EN"
                            "Port_2_SFP_PWR_EN"
                            "Port_3_SFP_PWR_EN"
                            "Port_4_SFPPLUS_PWR_EN"
                            "Port_5_SFPPLUS_PWR_EN"
                            "Port_6_SFPPLUS_PWR_EN"
                            "Port_7_SFPPLUS_PWR_EN"
                            "Port_8_SFPPLUS_PWR_EN"
                            "Port_9_SFPPLUS_PWR_EN"
                            "Port_10_SFPPLUS_PWR_EN"
                            "Port_11_SFPPLUS_PWR_EN"
                            "Port_12_SFP28_PWR_EN"
                            "Port_13_SFP28_PWR_EN"
                            "Port_14_SFP28_PWR_EN"
                            "Port_15_SFP28_PWR_EN"
                            "Port_16_SFP28_PWR_EN"
                            "Port_17_SFP28_PWR_EN"
                            "Port_18_SFP28_PWR_EN"
                            "Port_19_SFP28_PWR_EN"  )

        # CPLD 1 MB Interrupt
        for ((j=0; j<${#cpld1_sysfs_array[@]}; j++))
        do
            cpld1_sysfs_reg=$(eval "cat ${SYSFS_CPLD1}/${cpld1_sysfs_array[${j}]} ${LOG_REDIRECT}")
            _echo "[CPLD1 ${cpld1_sysfs_desc[${j}]} Interrupt(L) ]: ${cpld1_sysfs_reg}"
        done

        # CPLD 2 port interrupt
        for (( j=0; j<${#cpld2_sysfs_array[@]}; j++ ))
        do
            cpld2_sysfs_reg=$(eval "cat ${SYSFS_CPLD2}/${cpld2_sysfs_array[${j}]} ${LOG_REDIRECT}")
            _echo "[CPLD2 ${cpld2_sysfs_desc[${j}]} Interrupt(L) )]: ${cpld2_sysfs_reg}"
        done

    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
    fi
}

function _show_cpld_interrupt {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_cpld_interrupt_sysfs
    fi
}

function _show_system_led_sysfs {
    _banner "Show System LED"

    if [ "${MODEL_NAME}" == "${PLAT}" ]; then
        local sys_led_sysfs_attr=(  "sys_led_status"
                                    "sys_led_blinking"
                                    "sys_led_color"     )

        local gnss_led_sysfs_attr=( "gnss_led_status"
                                    "gnss_led_blinking"
                                    "gnss_led_color"    )

        local sync_led_sysfs_attr=( "sync_led_status"
                                    "sync_led_blinking"
                                    "sync_led_color"    )

        local pwr_led_sysfs_attr=(  "pwr_led_status"
                                    "pwr_led_blinking"
                                    "pwr_led_color"     )

        local fan_led_sysfs_attr=(  "fan_led_status"
                                    "fan_led_blinking"
                                    "fan_led_color"     )

        local system_led_attr_descrption=(  "LED Status  "
                                            "LED Blinking"
                                            "LED Color   "     )

        local desc_onoff=("OFF" "ON")
        local desc_blink=("Solid" "Blink")
        local desc_color=("Yellow" "Green")

        local desc_attr=(   "${desc_onoff[@]}"
                            "${desc_blink[@]}"
                            "${desc_color[@]}"   )

        local led=( "SYS "
                    "GNSS"
                    "SYNC"
                    "PWR "
                    "FAN " )

        for (( i=0; i<${#led[@]}; i++ ))
        do
            case ${i} in
            0)
                # sys_led_sysfs_attr
                for (( j=0; j<3; j++))
                do
                    if _check_filepath "${SYSFS_CPLD1}/${sys_led_sysfs_attr[j]}"; then
                        attr_val=$(eval "cat ${SYSFS_CPLD1}/${sys_led_sysfs_attr[j]} ${LOG_REDIRECT}")

                        case ${j} in
                        0)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_onoff[${attr_val}]})"
                            ;;
                        1)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_blink[${attr_val}]})"
                            ;;
                        2)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_color[${attr_val}]})"
                            ;;
                        esac
                    else
                        _echo "${led[$i]}: ${SYSFS_CPLD1}/${sys_led_sysfs_attr[${j}]} not exist!!!"
                    fi
                done
                ;;
            1)
                # gnss_led_sysfs_attr
                for (( j=0; j<3; j++))
                do
                    if _check_filepath "${SYSFS_CPLD1}/${gnss_led_sysfs_attr[j]}"; then
                        attr_val=$(eval "cat ${SYSFS_CPLD1}/${gnss_led_sysfs_attr[j]} ${LOG_REDIRECT}")

                        case ${j} in
                        0)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_onoff[${attr_val}]})"
                            ;;
                        1)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_blink[${attr_val}]})"
                            ;;
                        2)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_color[${attr_val}]})"
                            ;;
                        esac
                    else
                        _echo "${led[$i]}: ${SYSFS_CPLD1}/${gnss_led_sysfs_attr[${j}]} not exist!!!"
                    fi
                done
                ;;
            2)
                # sync_led_sysfs_attr
                for (( j=0; j<3; j++))
                do
                    if _check_filepath "${SYSFS_CPLD1}/${sync_led_sysfs_attr[j]}"; then
                        attr_val=$(eval "cat ${SYSFS_CPLD1}/${sync_led_sysfs_attr[j]} ${LOG_REDIRECT}")

                        case ${j} in
                        0)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_onoff[${attr_val}]})"
                            ;;
                        1)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_blink[${attr_val}]})"
                            ;;
                        2)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_color[${attr_val}]})"
                            ;;
                        esac
                    else
                        _echo "${led[$i]}: ${SYSFS_CPLD1}/${sync_led_sysfs_attr[${j}]} not exist!!!"
                    fi
                done
                ;;
            3)
                # pwr_led_sysfs_attr
                for (( j=0; j<3; j++))
                do
                    if _check_filepath "${SYSFS_CPLD1}/${pwr_led_sysfs_attr[j]}"; then
                        attr_val=$(eval "cat ${SYSFS_CPLD1}/${pwr_led_sysfs_attr[j]} ${LOG_REDIRECT}")

                        case ${j} in
                        0)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_onoff[${attr_val}]})"
                            ;;
                        1)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_blink[${attr_val}]})"
                            ;;
                        2)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_color[${attr_val}]})"
                            ;;
                        esac
                    else
                        _echo "${led[$i]}: ${SYSFS_CPLD1}/${pwr_led_sysfs_attr[${j}]} not exist!!!"
                    fi
                done
                ;;
            4)
                # fan_led_sysfs_attr
                for (( j=0; j<3; j++))
                do
                    if _check_filepath "${SYSFS_CPLD1}/${fan_led_sysfs_attr[j]}"; then
                        attr_val=$(eval "cat ${SYSFS_CPLD1}/${fan_led_sysfs_attr[j]} ${LOG_REDIRECT}")

                        case ${j} in
                        0)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_onoff[${attr_val}]})"
                            ;;
                        1)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_blink[${attr_val}]})"
                            ;;
                        2)
                            _echo "[${led[$i]} ${system_led_attr_descrption[${j}]}]: ${attr_val} (${desc_color[${attr_val}]})"
                            ;;
                        esac
                    else
                        _echo "${led[$i]}: ${SYSFS_CPLD1}/${fan_led_sysfs_attr[${j}]} not exist!!!"
                    fi
                done
                ;;
            esac
        done
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
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
        ret=$(_dd_read_byte ${reg})
        _echo "The value of address ${reg} is ${ret}"
        offset=$(( ${offset} + 1 ))
        reg=$(( ${base} + ${offset} ))
        reg=`printf "0x%X\n" ${reg}`
    done
}

function _show_onlpdump {
    _banner "Show onlpdump"

    which onlpdump > /dev/null 2>&1
    ret_onlpdump=$?
    timeout_cmd="timeout 20s"

    if [ ${ret_onlpdump} -eq 0 ]; then
        cmd_array=("${timeout_cmd} onlpdump -d" \
                   "${timeout_cmd} onlpdump -s" \
                   "${timeout_cmd} onlpdump -r" \
                   "${timeout_cmd} onlpdump -e" \
                   "${timeout_cmd} onlpdump -o" \
                   "${timeout_cmd} onlpdump -x" \
                   "${timeout_cmd} onlpdump -i" \
                   "${timeout_cmd} onlpdump -p" \
                   "${timeout_cmd} onlpdump -S")
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

function _show_onlpd {
    _banner "Show onlpd"

    which onlpd > /dev/null 2>&1
    ret_onlpd=$?
    timeout_cmd="timeout 20s"

    if [ ${ret_onlpd} -eq 0 ]; then
        cmd_array=("${timeout_cmd} onlpd -d" \
                   "${timeout_cmd} onlpd -s" \
                   "${timeout_cmd} onlpd -r" \
                   "${timeout_cmd} onlpd -e" \
                   "${timeout_cmd} onlpd -o" \
                   "${timeout_cmd} onlpd -x" \
                   "${timeout_cmd} onlpd -i" \
                   "${timeout_cmd} onlpd -p" \
                   "${timeout_cmd} onlpd -S")
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
    timeout_cmd="timeout 20s"

    if [ ${ret_onlps} -eq 0 ]; then
        cmd_array=("${timeout_cmd} onlps chassis onie show -" \
                   "${timeout_cmd} onlps chassis asset show -" \
                   "${timeout_cmd} onlps chassis env -" \
                   "${timeout_cmd} onlps sfp inventory -" \
                   "${timeout_cmd} onlps sfp bitmaps -" \
                   "${timeout_cmd} onlps chassis debug show -")
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

    _echo "[Command]: lsusb -v"
    ret=$(eval "lsusb -v ${LOG_REDIRECT}")
    _echo "${ret}"
    _echo ""

    _echo ""
    _echo "[Command]: find /sys/devices -name authorized* -exec tail -n +1 {} +"
    ret=$(eval "find /sys/devices -name authorized* -exec tail -n +1 {} + ${LOG_REDIRECT}")
    _echo "${ret}"
    _echo ""
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

    cmd_array=("lsblk"
               "lsblk -O"
               "parted -s -l"
               "fdisk -l /dev/mmcblk0"
               "find /sys/fs/ -name errors_count -print -exec cat {} \;"
               "find /sys/fs/ -name first_error_time -print -exec cat {} \; -exec echo '' \;"
               "find /sys/fs/ -name last_error_time -print -exec cat {} \; -exec echo '' \;"                "df -h")

    for (( i=0; i<${#cmd_array[@]}; i++ ))
    do
        _echo "[Command]: ${cmd_array[$i]}"
        ret=$(eval "${cmd_array[$i]} ${LOG_REDIRECT}")
        _echo "${ret}"
        _echo ""
    done

    # check smartctl command
    cmd="smartctl -a /dev/mmcblk0"
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

    cmd_array=( "dmidecode -t 0" \
                "dmidecode -t 1" \
                "dmidecode -t 2" \
                "dmidecode -t 3"    )

    for (( i=0; i<${#cmd_array[@]}; i++ ))
    do
        _echo "[Command]: ${cmd_array[$i]} "
        ret=$(eval "${cmd_array[$i]} ${LOG_REDIRECT}")
        _echo "${ret}"
        _echo ""
    done
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

usage() {
    local f=$(basename "$0")
    echo ""
    echo "Usage:"
    echo "    $f [-b] [-d D_DIR] [-h] [-i identifier] [-v]"
    echo "Description:"
    echo "  -b                bypass i2c command (required when NOS vendor use their own platform bsp to control i2c devices)"
    echo "  -d                specify D_DIR as log destination instead of default path /tmp/log"
    echo "  -h                show tech support script usage"
    echo "  -i                insert an identifier in the log file name"
    echo "  -v                show tech support script version"
    echo "Example:"
    echo "    $f -b"
    echo "    $f -d /var/log"
    echo "    $f -h"
    echo "    $f -i identifier"
    echo "    $f -v"
}

function _getopts {
    local OPTSTRING=":bd:fhi:v"
    # default log dir
    local log_folder_root="/tmp/log"
    local identifier=$SN

    while getopts ${OPTSTRING} opt; do
        case ${opt} in
            b)
                OPT_BYPASS_I2C_COMMAND=${TRUE}
                ;;
            d)
                log_folder_root=${OPTARG}
                ;;
            f)
                LOG_FAST=${TRUE}
                ;;
            h)
                usage
                exit 0
                ;;
            i)
                identifier=${OPTARG}
                ;;
            v)
                _show_ts_version
                exit 0
                ;;
            ?)
                echo "Invalid option: -${OPTARG}."
                usage
                exit -1
                ;;
        esac
    done

    LOG_FOLDER_ROOT=${log_folder_root}
    LOG_FOLDER_NAME="log_platform_${identifier}_${DATESTR}"
    LOG_FILE_NAME="log_platform_${identifier}_${DATESTR}.log"
    LOG_FOLDER_PATH="${LOG_FOLDER_ROOT}/${LOG_FOLDER_NAME}"
    LOG_FILE_PATH="${LOG_FOLDER_PATH}/${LOG_FILE_NAME}"
    LOG_REDIRECT="2>> $LOG_FILE_PATH"
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
    _show_sys_eeprom

    _show_psu_status_cpld
    _show_port_status
    _show_cpu_temperature
    _show_cpld_interrupt
    _show_system_led
    _show_ioport
    _show_onlpdump
    _show_onlpd
    _show_onlps
    _show_system_info
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
    _show_dmesg
    _additional_log_collection
    _show_time
    _compression

    echo "#   The tech-support collection is completed. Please share the tech support log file."
}

_getopts $@
_main
