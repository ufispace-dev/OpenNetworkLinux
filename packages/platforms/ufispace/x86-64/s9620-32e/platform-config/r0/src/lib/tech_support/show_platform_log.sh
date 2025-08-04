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
PLAT="S9620-32E"
# MODEL_NAME: set by function _board_info
MODEL_NAME=""
# HW_REV: set by function _board_info
HW_REV=""
HW_REV_ID=0
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
SYSFS_CPLD1="${SYSFS_DEV}/2-0030"
SYSFS_CPLD2="${SYSFS_DEV}/2-0031"
SYSFS_CPLD3="${SYSFS_DEV}/2-0032"
SYSFS_CPLD4="${SYSFS_DEV}/16-0033"
SYSFS_FPGA="${SYSFS_DEV}/2-0037"
SYSFS_LPC="/sys/devices/platform/x86_64_ufispace_s9620_32e_lpc"

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
    #_banner "Check Environment"

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
        _echo "[iSMT bus ]: ${ismt_bus}"
    fi

    # check BSP init
    _check_bsp_init

    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _update_gpio_max
    fi
}

function _check_filepath {
    filepath=$1
    if [ -z "${filepath}" ]; then
        _echo "ERROR, the input string is empty!!!"
        return ${FALSE}
    elif [ ! -f "$filepath" ]; then
        _echo "ERROR: No such file: ${filepath}"
        return ${FALSE}
    else
        #_echo "File Path: ${filepath}"
        return ${TRUE}
    fi
}

function _check_dirpath {
    dirpath=$1
    if [ -z "${dirpath}" ]; then
        _echo "ERROR, the ipnut string is empty!!!"
        return ${FALSE}
    elif [ ! -d "$dirpath" ]; then
        _echo "ERROR: No such directory: ${dirpath}"
        return ${FALSE}
    else
        return ${TRUE}
    fi
}

function _check_i2c_device {
    i2c_bus=$1
    i2c_addr=$2

    if [ -z "${i2c_addr}" ]; then
        _echo "ERROR, the input string is empty!!!"
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

    # CPLD1 0xE00 Register Definition
    build_rev_id_array=(0 1 2 3 4 5 6 7)
    build_rev_array=(1 2 3 4 5 6 7 8)
    hw_rev_id_array=(0 1 2 3)
    hw_rev_array=("Proto" "Alpha" "Beta" "PVT")
    hw_rev_ga_array=("GA_1" "GA_2" "GA_3" "GA_4")
    deph_name_array=("NPI" "GA")
    model_id_array=($((2#00100100)) $((2#00101111)))
    model_name_array=("S9620-32E" "S9620-32E")
    model_name=""

    model_id=$(_dd_read_byte 0xE00)
    ret=$?
    if [ $ret -eq 0 ]; then
        model_id=`echo ${model_id} | awk -F" " '{print $NF}'`
        model_id=$((model_id))
    else
        _echo "Get board model id failed ($ret), Exit!!"
        exit $ret
    fi

    board_rev_id=$(_dd_read_byte 0xE01)
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
    HW_REV_ID=$(((board_rev_id & 2#00000011) >> 0))
    hw_rev=${hw_rev_array[${HW_REV_ID}]}
    if [ $deph_id -eq 0 ]; then
        hw_rev=${hw_rev_array[${HW_REV_ID}]}
    else
        hw_rev=${hw_rev_ga_array[${HW_REV_ID}]}
    fi

    # Build Rev D[3:5]
    build_rev_id=$(((board_rev_id & 2#00111000) >> 3))
    build_rev=${build_rev_array[${build_rev_id}]}

    # MODEL ID D[0:5]
    model_id=$(((model_id & 2#11111111) >> 0))
    if [ $model_id -eq ${model_id_array[0]} ]; then
       model_name=${model_name_array[0]}
    elif [ $model_id -eq ${model_id_array[1]} ]; then
       model_name=${model_name_array[1]}
    else
       _echo "Invalid model_id: ${model_id}"
       exit 1
    fi

    MODEL_NAME=${model_name}
    HW_REV=${hw_rev}

    _echo "[CPLD 0x0/0x1 Reg Raw     ]: ${model_id} ${board_rev_id}"
    _echo "[Board Type               ]: ${model_name}"
    _echo "[Design Phase             ]: ${deph_name}"
    _echo "[Hardware Revision        ]: ${hw_rev}"
    _echo "[BUILD_ID                 ]: ${build_rev}"
}

function _bios_version {
    _banner "Show BIOS Version"

    bios_ver=$(eval "cat /sys/class/dmi/id/bios_version ${LOG_REDIRECT}")
    bios_boot_sel=$(_dd_read_byte 0xE30C)
    if [ $? -eq 0 ]; then
        bios_boot_sel=`echo ${bios_boot_sel} | awk -F" " '{print $NF}'`
    fi

    # EC BIOS BOOT SEL D[6]
    bios_boot_sel=$(((bios_boot_sel & 2#01000000) >> 6))

    _echo "[BIOS Vesion  ]: ${bios_ver}"
    _echo "[BIOS Boot ROM]: ${bios_boot_sel}"
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
    # Not Support
    return 0
    # _banner "Show CPLD Version (LPC)"

    # if [ "${MODEL_NAME}" == "${PLAT}" ]; then
    #     # MB CPLD S9620-36D
    #     mb_cpld_ver=$(_dd_read_byte 0xE02)
    #     ret=$?
    #     if [ ${ret} -eq 0 ]; then
    #         mb_cpld_ver=`echo ${mb_cpld_ver} | awk -F" " '{print $NF}'`
    #     fi

    #     mb_cpld_build=$(_dd_read_byte 0xE04)
    #     ret=$?
    #     if [ ${ret} -eq 0 ]; then
    #         mb_cpld_build=`echo ${mb_cpld_build} | awk -F" " '{print $NF}'`
    #     fi

    #     _echo "[MB CPLD Version]: $(( (mb_cpld_ver & 2#11000000) >> 6)).$(( mb_cpld_ver & 2#00111111 ))"
    #     _echo "[MB CPLD Build  ]: $(( mb_cpld_build ))"
    # else
    #     _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
    #     exit 1
    # fi
}

function _cpld_version_i2c {
    if [ "${OPT_BYPASS_I2C_COMMAND}" == "${TRUE}" ]; then
        _banner "Show CPLD Version (I2C) (Bypass)"
        return
    fi

    _banner "Show CPLD Version (I2C)"

    if [[ $MODEL_NAME == "${PLAT}" ]]; then

        local mux_i2c_bus=${i801_bus}
        local mux_i2c_addr_0x71=0x71
        local mux_i2c_addr_0x76=0x76

        # MB CPLD
        mb_cpld1_ver=""
        mb_cpld2_ver=""
        mb_cpld3_ver=""
        mb_cpld4_ver=""
        mb_fpga_ver=""
        mb_cpld1_build=""
        mb_cpld2_build=""
        mb_cpld3_build=""
        mb_cpld4_build=""
        mb_fpga_build=""

        # CPLD 1-5

        _check_i2c_device ${mux_i2c_bus} ${mux_i2c_addr_0x71}
        ret=$?

        if [ ${ret} -eq ${TRUE} ]; then
            i2cset -y -f ${mux_i2c_bus} ${mux_i2c_addr_0x71} 0x1
            if _check_i2c_device ${mux_i2c_bus} "0x30"; then
                mb_cpld1_ver=$(eval "i2cget -y -f ${mux_i2c_bus} 0x30 0x2 ${LOG_REDIRECT}")
                mb_cpld1_build=$(eval "i2cget -y -f ${mux_i2c_bus} 0x30 0x4 ${LOG_REDIRECT}")
                _printf "[MB CPLD1 Version]: %d.%02d.%03d\n" $(( (mb_cpld1_ver & 2#11000000) >> 6)) $(( mb_cpld1_ver & 2#00111111 )) $((mb_cpld1_build))
            fi

            if _check_i2c_device ${mux_i2c_bus} "0x31"; then
                mb_cpld2_ver=$(eval "i2cget -y -f ${mux_i2c_bus} 0x31 0x2 ${LOG_REDIRECT}")
                mb_cpld2_build=$(eval "i2cget -y -f ${mux_i2c_bus} 0x31 0x4 ${LOG_REDIRECT}")
                _printf "[MB CPLD2 Version]: %d.%02d.%03d\n" $(( (mb_cpld2_ver & 2#11000000) >> 6)) $(( mb_cpld2_ver & 2#00111111 )) $((mb_cpld2_build))
            fi

            if _check_i2c_device ${mux_i2c_bus} "0x32"; then
                mb_cpld3_ver=$(eval "i2cget -y -f ${mux_i2c_bus} 0x32 0x2 ${LOG_REDIRECT}")
                mb_cpld3_build=$(eval "i2cget -y -f ${mux_i2c_bus} 0x32 0x4 ${LOG_REDIRECT}")
                _printf "[MB CPLD3 Version]: %d.%02d.%03d\n" $(( (mb_cpld3_ver & 2#11000000) >> 6)) $(( mb_cpld3_ver & 2#00111111 )) $((mb_cpld3_build))
            fi

            if _check_i2c_device ${mux_i2c_bus} "0x37"; then
                mb_fpga_ver=$(eval "i2cget -y -f ${mux_i2c_bus} 0x37 0x2 ${LOG_REDIRECT}")
                mb_fpga_build=$(eval "i2cget -y -f ${mux_i2c_bus} 0x37 0x4 ${LOG_REDIRECT}")
                _printf "[MB FPGA Version]: %d.%02d.%03d\n" $(( (mb_fpga_ver & 2#11000000) >> 6)) $(( mb_fpga_ver & 2#00111111 )) $((mb_fpga_build))
            fi

            i2cset -y -f ${mux_i2c_bus} ${mux_i2c_addr_0x71} 0x80
            i2cset -y -f ${mux_i2c_bus} ${mux_i2c_addr_0x76} 0x40
            if _check_i2c_device ${mux_i2c_bus} "0x33"; then
                mb_cpld4_ver=$(eval "i2cget -y -f ${mux_i2c_bus} 0x33 0x2 ${LOG_REDIRECT}")
                mb_cpld4_build=$(eval "i2cget -y -f ${mux_i2c_bus} 0x33 0x4 ${LOG_REDIRECT}")
                _printf "[MB CPLD4 Version]: %d.%02d.%03d\n" $(( (mb_cpld4_ver & 2#11000000) >> 6)) $(( mb_cpld4_ver & 2#00111111 )) $((mb_cpld4_build))
            fi

            i2cset -y -f ${mux_i2c_bus} ${mux_i2c_addr_0x76} 0x0
            i2cset -y -f ${mux_i2c_bus} ${mux_i2c_addr_0x71} 0x0
        fi

    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
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

        if _check_filepath "$SYSFS_CPLD3/cpld_version_h"; then
            mb_cpld_ver=$(eval "cat $SYSFS_CPLD3/cpld_version_h ${LOG_REDIRECT}")
            _echo "[MB CPLD3 Version]: ${mb_cpld_ver}"
        fi

        if _check_filepath "$SYSFS_CPLD4/cpld_version_h"; then
            mb_cpld_ver=$(eval "cat $SYSFS_CPLD4/cpld_version_h ${LOG_REDIRECT}")
            _echo "[MB CPLD4 Version]: ${mb_cpld_ver}"
        fi

        if _check_filepath "$SYSFS_FPGA/fpga_version_h"; then
            mb_cpld_ver=$(eval "cat $SYSFS_FPGA/fpga_version_h ${LOG_REDIRECT}")
            _echo "[MB FPGA Version]: ${mb_cpld_ver}"
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
        _cpld_version_i2c
    fi
}

function _ucd_version {

    _banner "Show UCD Version"

    if [ "${MODEL_NAME}" != "${PLAT}" ]; then
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
    _banner "Show I2C Tree Bus i801"

    ret=$(eval "(time i2cdetect -y ${i801_bus}) ${LOG_REDIRECT}")

    _echo "[I2C Tree ${i801_bus}]:"
    _echo "${ret}"

    _banner "Show I2C Tree Bus iSMT"

    ret=$(eval "i2cdetect -y "${ismt_bus}" ${LOG_REDIRECT}")

    _echo "[I2C Tree ${ismt_bus}]:"
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
        chip_addr1="0x71"
        _check_i2c_device "${bus}" "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then

            ## (9548_ROOT_PORT_1)-0x72
            _show_i2c_mux_devices "${bus}" "${chip_addr1}" "8" "9548_ROOT_PORT_2-${chip_addr1}"
            local chip_addr2_array=("" "" "" "" "" "" "0x76" "")
            local mux_name_array=("" "" "" "" "" "" "9548_CHILD_CPLD4" "")

            for (( chip_addr1_chann=0; chip_addr1_chann<${#chip_addr2_array[@]}; chip_addr1_chann++ ))
            do
                if [ -z "${chip_addr2_array[${chip_addr1_chann}]}" ]; then
                    continue
                fi

                local chip_addr2=${chip_addr2_array[${chip_addr1_chann}]}
                local mux_name=${mux_name_array[${chip_addr1_chann}]}
                # open mux channel - 0x72 (chip_addr1)
                i2cset -y ${bus} ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
                _show_i2c_mux_devices ${bus} "${chip_addr2}" "8" "${mux_name}-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}"
                # close mux channel - 0x72 (chip_addr1)
                i2cset -y ${bus} ${chip_addr1} 0x0
            done
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
    local pca954x_device_bus=("")
    if [ "${MODEL_NAME}" == "${PLAT}" ]; then
        pca954x_device_id=("0x71")
        pca954x_device_bus=("${i801_bus}")
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    for (( i=0; i<${#pca954x_device_id[@]}; i++ ))
    do
        for ((j=0;j<5;j++))
        do
            _echo "[DEV PCA9548 (${j})]"
            ret=`i2cget -f -y ${pca954x_device_bus[$i]} ${pca954x_device_id[$i]}`
            _echo "[I2C Device ${pca954x_device_id[$i]}]: $ret"
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

    local eeprom_addr="0x57"
    local eeprom_mux=""
    local eeprom_ch=""
    local under_mux=false
    local eeprom_bus=-1

    if (( ${HW_REV_ID} <= 1 )); then
        eeprom_bus=${i801_bus} # Alpha
    else
        eeprom_bus=${ismt_bus} # Beta and later
    fi

    if [ "${eeprom_mux}" != "" ] && [ "${eeprom_ch}" != "" ]; then
        under_mux=true
    fi

    if $under_mux; then
        i2cset -y ${eeprom_bus} ${eeprom_mux} $(( 2 ** ${eeprom_ch} ))
    fi

    #first read return empty content
    sys_eeprom=$(eval "i2cdump -y ${eeprom_bus} ${eeprom_addr} c")
    #second read return correct content
    sys_eeprom=$(eval "i2cdump -y ${eeprom_bus} ${eeprom_addr} c")

    if $under_mux; then
        i2cset -y -f ${eeprom_bus} ${eeprom_mux} 0x0
    fi
    _echo "[System EEPROM]:"
    _echo "${sys_eeprom}"
}

function _show_sys_eeprom_sysfs {
    _banner "Show System EEPROM"

    local eeprom_bus=-1

    if (( ${HW_REV_ID} <= 1 )); then
        eeprom_bus=${i801_bus} # Alpha
    else
        eeprom_bus=${ismt_bus} # Beta and later
    fi

    sys_eeprom=$(eval "cat /sys/bus/i2c/devices/${eeprom_bus}-0057/eeprom ${LOG_REDIRECT} | hexdump -C")
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

function _show_gpio_sysfs {

    _banner "Show GPIO Status"

    max_gpio=`ls /sys/class/gpio/ | grep "gpio[[:digit:]]" | sort -V | tail -n 1`
    min_gpio=`ls /sys/class/gpio/ | grep "gpio[[:digit:]]" | sort -V | head -n 1`

    if [ -z $max_gpio ] || [ -z $min_gpio ]  ;then
        _echo "No Contents!!!"
        return 0
    fi

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

function _show_psu_status_cpld_sysfs {
    _banner "Show PSU Status (CPLD)"

    bus_id=""
    if [ "${MODEL_NAME}" == "${PLAT}" ]; then
        bus_id="1"
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    # Read PSU Status
    if _check_filepath "${SYSFS_CPLD1}/psu_0_pg"; then
        # Read PSU0 Power Good Status (1: power good, 0: not providing power)
        psu0_power_ok=$(eval "cat ${SYSFS_CPLD1}/psu_0_pg ${LOG_REDIRECT}")

        # Read PSU0 Absent Status (0: psu present, 1: psu absent)
        psu0_absent_l=$(eval "cat ${SYSFS_CPLD1}/psu_0_prsnt ${LOG_REDIRECT}")

        # Read PSU1 Power Good Status (1: power good, 0: not providing power)
        psu1_power_ok=$(eval "cat ${SYSFS_CPLD1}/psu_1_pg ${LOG_REDIRECT}")

        # Read PSU1 Absent Status (0: psu present, 1: psu absent)
        psu1_absent_l=$(eval "cat ${SYSFS_CPLD1}/psu_1_prsnt ${LOG_REDIRECT}")

        _echo "[PSU0 Power Good Status]: ${psu0_power_ok}"
        _echo "[PSU0 Absent Status (L)]: ${psu0_absent_l}"
        _echo "[PSU1 Power Good Status]: ${psu1_power_ok}"
        _echo "[PSU1 Absent Status (L)]: ${psu1_absent_l}"
    else
        _echo "${SYSFS_CPLD1}/cpld_psu_status not exist!!!"
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
                             41 42 43 44 45 46 47 48)

    local eeprom_repeat_array=(2 2 2 1 1 1
                               1 1 1
                               1 1 1 1 1 1 1 1
                               1 1 1 1 1 1 1 1)

    if [ "${type}" !=  "${PORT_T_QSFPDD}" ] && [ "${type}" !=  "${PORT_T_QSFP}" ]; then
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

function _get_port_attr_bit {
    local attr=$1
    local bit_stream=$2
    local tmp_value=0
    case "${attr}" in
        "mgmt_abs" )
            tmp_value=$(( bit_stream >> 0 ))
            ;;
        "mgmt_rxlos" )
            tmp_value=$(( bit_stream >> 3 ))
            ;;
        "mgmt_txflt" )
            tmp_value=$(( bit_stream >> 6 ))
            ;;
        "mgmt_txdis" )
            tmp_value=$(( bit_stream >> 9 ))
            ;;
        *)
            tmp_value=$(( bit_stream ))
    esac

    echo $(( tmp_value & 2#111 ))
}

function _show_port_status_sysfs {
    _banner "Show Port Status / EEPROM"
    local show_sysfs=${TRUE}

    if [ "${MODEL_NAME}" == "${PLAT}" ]; then

        sys_port_array=( "${SYSFS_CPLD2}/qsfpdd_p0_abs"            #0
                         "${SYSFS_CPLD2}/qsfpdd_p1_abs"            #1
                         "${SYSFS_CPLD2}/qsfpdd_p2_abs"            #2
                         "${SYSFS_CPLD2}/qsfpdd_p3_abs"            #3
                         "${SYSFS_CPLD2}/qsfpdd_p4_abs"            #4
                         "${SYSFS_CPLD2}/qsfpdd_p5_abs"            #5
                         "${SYSFS_CPLD2}/qsfpdd_p6_abs"            #6
                         "${SYSFS_CPLD2}/qsfpdd_p7_abs"            #7
                         "${SYSFS_CPLD2}/qsfpdd_p8_abs"            #8
                         "${SYSFS_CPLD2}/qsfpdd_p9_abs"            #9
                         "${SYSFS_CPLD2}/qsfpdd_p10_abs"           #10
                         "${SYSFS_CPLD2}/qsfpdd_p11_abs"           #11
                         "${SYSFS_CPLD2}/qsfpdd_p12_abs"           #12
                         "${SYSFS_CPLD2}/qsfpdd_p13_abs"           #13
                         "${SYSFS_CPLD2}/qsfpdd_p14_abs"           #14
                         "${SYSFS_CPLD2}/qsfpdd_p15_abs"           #15
                         "${SYSFS_CPLD3}/qsfpdd_p16_abs"           #16
                         "${SYSFS_CPLD3}/qsfpdd_p17_abs"           #17
                         "${SYSFS_CPLD3}/qsfpdd_p18_abs"           #18
                         "${SYSFS_CPLD3}/qsfpdd_p19_abs"           #19
                         "${SYSFS_CPLD3}/qsfpdd_p20_abs"           #20
                         "${SYSFS_CPLD3}/qsfpdd_p21_abs"           #21
                         "${SYSFS_CPLD3}/qsfpdd_p22_abs"           #22
                         "${SYSFS_CPLD3}/qsfpdd_p23_abs"           #23
                         "${SYSFS_CPLD3}/qsfpdd_p24_abs"           #24
                         "${SYSFS_CPLD3}/qsfpdd_p25_abs"           #25
                         "${SYSFS_CPLD3}/qsfpdd_p26_abs"           #26
                         "${SYSFS_CPLD3}/qsfpdd_p27_abs"           #27
                         "${SYSFS_CPLD3}/qsfpdd_p28_abs"           #28
                         "${SYSFS_CPLD3}/qsfpdd_p29_abs"           #29
                         "${SYSFS_CPLD3}/qsfpdd_p30_abs"           #30
                         "${SYSFS_CPLD3}/qsfpdd_p31_abs"           #31
                         "${SYSFS_CPLD2}/qsfpdd_p0_lp_mode"        #32
                         "${SYSFS_CPLD2}/qsfpdd_p1_lp_mode"        #33
                         "${SYSFS_CPLD2}/qsfpdd_p2_lp_mode"        #34
                         "${SYSFS_CPLD2}/qsfpdd_p3_lp_mode"        #35
                         "${SYSFS_CPLD2}/qsfpdd_p4_lp_mode"        #36
                         "${SYSFS_CPLD2}/qsfpdd_p5_lp_mode"        #37
                         "${SYSFS_CPLD2}/qsfpdd_p6_lp_mode"        #38
                         "${SYSFS_CPLD2}/qsfpdd_p7_lp_mode"        #39
                         "${SYSFS_CPLD2}/qsfpdd_p8_lp_mode"        #40
                         "${SYSFS_CPLD2}/qsfpdd_p9_lp_mode"        #41
                         "${SYSFS_CPLD2}/qsfpdd_p10_lp_mode"       #42
                         "${SYSFS_CPLD2}/qsfpdd_p11_lp_mode"       #43
                         "${SYSFS_CPLD2}/qsfpdd_p12_lp_mode"       #44
                         "${SYSFS_CPLD2}/qsfpdd_p13_lp_mode"       #45
                         "${SYSFS_CPLD2}/qsfpdd_p14_lp_mode"       #46
                         "${SYSFS_CPLD2}/qsfpdd_p15_lp_mode"       #47
                         "${SYSFS_CPLD3}/qsfpdd_p16_lp_mode"       #48
                         "${SYSFS_CPLD3}/qsfpdd_p17_lp_mode"       #49
                         "${SYSFS_CPLD3}/qsfpdd_p18_lp_mode"       #50
                         "${SYSFS_CPLD3}/qsfpdd_p19_lp_mode"       #51
                         "${SYSFS_CPLD3}/qsfpdd_p20_lp_mode"       #52
                         "${SYSFS_CPLD3}/qsfpdd_p21_lp_mode"       #53
                         "${SYSFS_CPLD3}/qsfpdd_p22_lp_mode"       #54
                         "${SYSFS_CPLD3}/qsfpdd_p23_lp_mode"       #55
                         "${SYSFS_CPLD3}/qsfpdd_p24_lp_mode"       #56
                         "${SYSFS_CPLD3}/qsfpdd_p25_lp_mode"       #57
                         "${SYSFS_CPLD3}/qsfpdd_p26_lp_mode"       #58
                         "${SYSFS_CPLD3}/qsfpdd_p27_lp_mode"       #59
                         "${SYSFS_CPLD3}/qsfpdd_p28_lp_mode"       #60
                         "${SYSFS_CPLD3}/qsfpdd_p29_lp_mode"       #61
                         "${SYSFS_CPLD3}/qsfpdd_p30_lp_mode"       #62
                         "${SYSFS_CPLD3}/qsfpdd_p31_lp_mode"       #63
                         "${SYSFS_CPLD2}/qsfpdd_p0_rst"            #64
                         "${SYSFS_CPLD2}/qsfpdd_p1_rst"            #65
                         "${SYSFS_CPLD2}/qsfpdd_p2_rst"            #66
                         "${SYSFS_CPLD2}/qsfpdd_p3_rst"            #67
                         "${SYSFS_CPLD2}/qsfpdd_p4_rst"            #68
                         "${SYSFS_CPLD2}/qsfpdd_p5_rst"            #69
                         "${SYSFS_CPLD2}/qsfpdd_p6_rst"            #70
                         "${SYSFS_CPLD2}/qsfpdd_p7_rst"            #71
                         "${SYSFS_CPLD2}/qsfpdd_p8_rst"            #72
                         "${SYSFS_CPLD2}/qsfpdd_p9_rst"            #73
                         "${SYSFS_CPLD2}/qsfpdd_p10_rst"           #74
                         "${SYSFS_CPLD2}/qsfpdd_p11_rst"           #75
                         "${SYSFS_CPLD2}/qsfpdd_p12_rst"           #76
                         "${SYSFS_CPLD2}/qsfpdd_p13_rst"           #77
                         "${SYSFS_CPLD2}/qsfpdd_p14_rst"           #78
                         "${SYSFS_CPLD2}/qsfpdd_p15_rst"           #79
                         "${SYSFS_CPLD3}/qsfpdd_p16_rst"           #80
                         "${SYSFS_CPLD3}/qsfpdd_p17_rst"           #81
                         "${SYSFS_CPLD3}/qsfpdd_p18_rst"           #82
                         "${SYSFS_CPLD3}/qsfpdd_p19_rst"           #83
                         "${SYSFS_CPLD3}/qsfpdd_p20_rst"           #84
                         "${SYSFS_CPLD3}/qsfpdd_p21_rst"           #85
                         "${SYSFS_CPLD3}/qsfpdd_p22_rst"           #86
                         "${SYSFS_CPLD3}/qsfpdd_p23_rst"           #87
                         "${SYSFS_CPLD3}/qsfpdd_p24_rst"           #88
                         "${SYSFS_CPLD3}/qsfpdd_p25_rst"           #89
                         "${SYSFS_CPLD3}/qsfpdd_p26_rst"           #90
                         "${SYSFS_CPLD3}/qsfpdd_p27_rst"           #91
                         "${SYSFS_CPLD3}/qsfpdd_p28_rst"           #92
                         "${SYSFS_CPLD3}/qsfpdd_p29_rst"           #93
                         "${SYSFS_CPLD3}/qsfpdd_p30_rst"           #94
                         "${SYSFS_CPLD3}/qsfpdd_p31_rst"           #95
                         "${SYSFS_CPLD2}/qsfpdd_p0_intr"           #96
                         "${SYSFS_CPLD2}/qsfpdd_p1_intr"           #97
                         "${SYSFS_CPLD2}/qsfpdd_p2_intr"           #98
                         "${SYSFS_CPLD2}/qsfpdd_p3_intr"           #99
                         "${SYSFS_CPLD2}/qsfpdd_p4_intr"           #100
                         "${SYSFS_CPLD2}/qsfpdd_p5_intr"           #101
                         "${SYSFS_CPLD2}/qsfpdd_p6_intr"           #102
                         "${SYSFS_CPLD2}/qsfpdd_p7_intr"           #103
                         "${SYSFS_CPLD2}/qsfpdd_p8_intr"           #104
                         "${SYSFS_CPLD2}/qsfpdd_p9_intr"           #105
                         "${SYSFS_CPLD2}/qsfpdd_p10_intr"          #106
                         "${SYSFS_CPLD2}/qsfpdd_p11_intr"          #107
                         "${SYSFS_CPLD2}/qsfpdd_p12_intr"          #108
                         "${SYSFS_CPLD2}/qsfpdd_p13_intr"          #109
                         "${SYSFS_CPLD2}/qsfpdd_p14_intr"          #110
                         "${SYSFS_CPLD2}/qsfpdd_p15_intr"          #111
                         "${SYSFS_CPLD3}/qsfpdd_p16_intr"          #112
                         "${SYSFS_CPLD3}/qsfpdd_p17_intr"          #113
                         "${SYSFS_CPLD3}/qsfpdd_p18_intr"          #114
                         "${SYSFS_CPLD3}/qsfpdd_p19_intr"          #115
                         "${SYSFS_CPLD3}/qsfpdd_p20_intr"          #116
                         "${SYSFS_CPLD3}/qsfpdd_p21_intr"          #117
                         "${SYSFS_CPLD3}/qsfpdd_p22_intr"          #118
                         "${SYSFS_CPLD3}/qsfpdd_p23_intr"          #119
                         "${SYSFS_CPLD3}/qsfpdd_p24_intr"          #120
                         "${SYSFS_CPLD3}/qsfpdd_p25_intr"          #121
                         "${SYSFS_CPLD3}/qsfpdd_p26_intr"          #122
                         "${SYSFS_CPLD3}/qsfpdd_p27_intr"          #123
                         "${SYSFS_CPLD3}/qsfpdd_p28_intr"          #124
                         "${SYSFS_CPLD3}/qsfpdd_p29_intr"          #125
                         "${SYSFS_CPLD3}/qsfpdd_p30_intr"          #126
                         "${SYSFS_CPLD3}/qsfpdd_p31_intr"          #127
                         "${SYSFS_CPLD4}/sfp_p32_abs"              #128
                         "${SYSFS_CPLD4}/sfp_p33_abs"              #129
                         "${SYSFS_CPLD4}/sfp_p34_abs"              #130
                         "${SYSFS_CPLD4}/sfp_p35_abs"              #131
                         "${SYSFS_CPLD4}/mgmt_p0_abs"              #132
                         "${SYSFS_CPLD4}/mgmt_p1_abs"              #133
                         "${SYSFS_CPLD4}/sfp_p32_tx_flt"           #134
                         "${SYSFS_CPLD4}/sfp_p33_tx_flt"           #135
                         "${SYSFS_CPLD4}/sfp_p34_tx_flt"           #136
                         "${SYSFS_CPLD4}/sfp_p35_tx_flt"           #137
                         "${SYSFS_CPLD4}/mgmt_p0_tx_flt"           #138
                         "${SYSFS_CPLD4}/mgmt_p1_tx_flt"           #139
                         "${SYSFS_CPLD4}/sfp_p32_rx_los"           #140
                         "${SYSFS_CPLD4}/sfp_p33_rx_los"           #141
                         "${SYSFS_CPLD4}/sfp_p34_rx_los"           #142
                         "${SYSFS_CPLD4}/sfp_p35_rx_los"           #143
                         "${SYSFS_CPLD4}/mgmt_p0_rx_los"           #144
                         "${SYSFS_CPLD4}/mgmt_p1_rx_los"           #145
                         "${SYSFS_CPLD4}/sfp_p32_tx_dis"           #146
                         "${SYSFS_CPLD4}/sfp_p33_tx_dis"           #147
                         "${SYSFS_CPLD4}/sfp_p34_tx_dis"           #148
                         "${SYSFS_CPLD4}/sfp_p35_tx_dis"           #149
                         "${SYSFS_CPLD4}/mgmt_p0_tx_dis"           #150
                         "${SYSFS_CPLD4}/mgmt_p1_tx_dis"           #151
                         "${SYSFS_CPLD4}/sfp_p32_ts"               #152
                         "${SYSFS_CPLD4}/sfp_p33_ts"               #153
                         "${SYSFS_CPLD4}/sfp_p34_ts"               #154
                         "${SYSFS_CPLD4}/sfp_p35_ts"               #155
                         "${SYSFS_CPLD4}/mgmt_p0_ts"               #156
                         "${SYSFS_CPLD4}/mgmt_p1_ts"               #157
                         "${SYSFS_CPLD4}/sfp_p32_rs"               #158
                         "${SYSFS_CPLD4}/sfp_p33_rs"               #159
                         "${SYSFS_CPLD4}/sfp_p34_rs"               #160
                         "${SYSFS_CPLD4}/sfp_p35_rs"               #161
                         "${SYSFS_CPLD4}/mgmt_p0_rs"               #162
                         "${SYSFS_CPLD4}/mgmt_p1_rs"               #163
                        )

        port_name_array=(
            "0"   "1"   "2"   "3"   "4"   "5"   "6"   "7"
            "8"   "9"   "10"  "11"  "12"  "13"  "14"  "15"
            "16"  "17"  "18"  "19"  "20"  "21"  "22"  "23"
            "24"  "25"  "26"  "27"  "28"  "29"  "30"  "31"
            "32"  "33"  "34"  "35"  "36"  "37"
        )

        local QSFPDD=${PORT_T_QSFPDD}
        local QSFP=${PORT_T_QSFP}
        local SFP=${PORT_T_SFP}
        local MGMT=${PORT_T_MGMT}
        port_type_array=(
        #   0         1         2         3         4         5         6         7
            ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD}
        #   8         9         10        11        12        13        14        15
            ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD}
        #   16        17        18        19        20        21        22        23
            ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD}
        #   24        25        26        27        28        29        30        31
            ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD} ${QSFPDD}
        #   32        33        34        35        36        37
            ${SFP}    ${SFP}    ${SFP}    ${SFP}    ${MGMT}   ${MGMT}
        )

        #
        #  MGMT bit is bit stream
        #  Def: txdis txflt rxlos abs
        #   0b  xxx   xxx   xxx   xxx
        #
        #  Ex:  0x50 = 0b 000 001 010 000
        #       mgmt_abs  : bit 0
        #       mgmt_rxlos: bit 2
        #       mgmt_txflt: bit 1
        #       mgmt_txdis: bit 0
        #
        #  Ex:  0x974 = 0b 100 101 110 100
        #       mgmt_abs  : bit 4
        #       mgmt_rxlos: bit 6
        #       mgmt_txflt: bit 5
        #       mgmt_txdis: bit 4
        #
        port_bit_array=(
        #   0     1     2     3     4     5     6     7
            0     1     2     3     4     5     6     7
        #   8     9     10    11    12    13    14    15
            0     1     2     3     4     5     6     7
        #   16    17    18    19    20    21    22    23
            0     1     2     3     4     5     6     7
        #   24    25    26    27    28    29    30    31
            0     1     2     3     4     5     6     7
        #   32    33    34    35    36    37    38    39
            0     1     2     3     4     5     6     7
        #   40    41    42    43    44    45    46    47
            0     1     2     3     0     1     2     3
        #   48    49    50    51    52    53    54    55
            0     1     2     3     4     5     6     7
        #   56    57    58    59    60    61    62    63
            0     1     2     3     4     5     6     7
        #   64    65    66    67    68    69    70    71
            0     1     2     3     4     5     6     7
        #   72    73    74    75    76    77    78    79
            0     1     2     3     4     5     6     7
        #   80    81    82    83    84    85    86    87
            0     1     2     3     4     5     6     7
        #   88    89    90    91    92    93    94    95
            0     1     2     3     0     1     2     3
        #   96    97    98    99    100   101   102   103
            0     1     2     3     4     5     6     7
        #   104   105
            0x50  0x974
        )

        # ref sys_port_array
        port_absent_array=(
        #   0     1     2     3     4     5     6     7
            0     1     2     3     4     5     6     7
        #   8     9     10    11    12    13    14    15
            8     9     10    11    12    13    14    15
        #   16    17    18    19    20    21    22    23
            16    17    18    19    20    21    22    23
        #   24    25    26    27    28    29    30    31
            24    25    26    27    28    29    30    31
        #   32    33    34    35    36    37
            128   129   130   131   132   133
        )

        # ref sys_port_array
        port_lp_mode_array=(
        #    0     1     2     3     4     5     6     7
             32    33    34    35    36    37    38    39
        #    8     9     10    11    12    13    14    15
             40    41    42    43    44    45    46    47
        #    16    17    18    19    20    21    22    23
             48    49    50    51    52    53    54    55
        #    24    25    26    27    28    29    30    31
             56    57    58    59    60    61    62    63
        #    32    33    34    35    36    37
             -1    -1    -1    -1    -1    -1
        )

        # ref sys_port_array
        port_reset_array=(
        #    0     1     2     3     4     5     6     7
             64    65    66    67    68    69    70    71
        #    8     9     10    11    12    13    14    15
             72    73    74    75    76    77    78    79
        #    16    17    18    19    20    21    22    23
             80    81    82    83    84    85    86    87
        #    24    25    26    27    28    29    30    31
             88    89    90    91    92    93    94    95
        #    32    33    34    35    36    37
             -1    -1    -1    -1    -1    -1
        )

        # ref sys_port_array
        port_intr_array=(
        #    0     1     2     3     4     5     6     7
             96    97    98    99    100   101   102   103
        #    8     9     10    11    12    13    14    15
             104   105   106   107   108   109   110   111
        #    16    17    18    19    20    21    22    23
             112    113   114  115   116   117   118   119
        #    24    25    26    27    28    29    30    31
             120   121   122   123   124   125   126   127
        #    32    33    34    35    36    37
             -1    -1    -1    -1    -1    -1
        )

        # ref sys_port_array
        port_tx_fault_array=(
        #    0     1     2     3     4     5     6     7
             -1    -1    -1    -1    -1    -1    -1    -1
        #    8     9     10    11    12    13    14    15
             -1    -1    -1    -1    -1    -1    -1    -1
        #    16    17    18    19    20    21    22    23
             -1    -1    -1    -1    -1    -1    -1    -1
        #    24    25    26    27    28    29    30    31
             -1    -1    -1    -1    -1    -1    -1    -1
        #    32    33    34    35    36    37
             134   135   136   137   138   139
        )

        # ref sys_port_array
        port_rx_los_array=(
        #    0     1     2     3     4     5     6     7
             -1    -1    -1    -1    -1    -1    -1    -1
        #    8     9     10    11    12    13    14    15
             -1    -1    -1    -1    -1    -1    -1    -1
        #    16    17    18    19    20    21    22    23
             -1    -1    -1    -1    -1    -1    -1    -1
        #    24    25    26    27    28    29    30    31
             -1    -1    -1    -1    -1    -1    -1    -1
        #    32    33    34    35    36    37
             140   141   142   143   144   145
        )

        # ref sys_port_array
        port_tx_dis_array=(
        #    0     1     2     3     4     5     6     7
             -1    -1    -1    -1    -1    -1    -1    -1
        #    8     9     10    11    12    13    14    15
             -1    -1    -1    -1    -1    -1    -1    -1
        #    16    17    18    19    20    21    22    23
             -1    -1    -1    -1    -1    -1    -1    -1
        #    24    25    26    27    28    29    30    31
             -1    -1    -1    -1    -1    -1    -1    -1
        #    32    33    34    35    36    37
             146   147   148   149   150   151
        )

        # ref sys_port_array
        port_tx_rate_sel_array=(
        #    0     1     2     3     4     5     6     7
             -1    -1    -1    -1    -1    -1    -1    -1
        #    8     9     10    11    12    13    14    15
             -1    -1    -1    -1    -1    -1    -1    -1
        #    16    17    18    19    20    21    22    23
             -1    -1    -1    -1    -1    -1    -1    -1
        #    24    25    26    27    28    29    30    31
             -1    -1    -1    -1    -1    -1    -1    -1
        #    32    33    34    35    36    37
             152   153   154   155   156   157
        )

        # ref sys_port_array
        port_rx_rate_sel_array=(
        #    0     1     2     3     4     5     6     7
             -1    -1    -1    -1    -1    -1    -1    -1
        #    8     9     10    11    12    13    14    15
             -1    -1    -1    -1    -1    -1    -1    -1
        #    16    17    18    19    20    21    22    23
             -1    -1    -1    -1    -1    -1    -1    -1
        #    24    25    26    27    28    29    30    31
             -1    -1    -1    -1    -1    -1    -1    -1
        #    32    33    34    35    36    37
             158   159   160   161   162   163
        )

        port_eeprom_bus_array=(
        #   0     1     2     3     4     5     6     7
            18    19    20    21    22    23    24    25
        #   8     9     10    11    12    13    14    15
            26    27    28    29    30    31    32    33
        #   16    17    18    19    20    21    22    23
            34    35    36    37    38    39    40    41
        #   24    25    26    27    28    29    30    31
            42    43    44    45    46    47    48    49
        #   32    33    34    35    36    37
            50    51    52    53    54    52
        )

        for (( i=0; i<${#port_name_array[@]}; i++ ))
        do
            local reg=""
            local bit=0
            # Port Absent Status (0: Present, 1:Absence)
            local idx=${port_absent_array[i]}
            local sysfs_path="${sys_port_array[idx]}"
            local bit_stream=${port_bit_array[i]}
            if [ "${port_absent_array[${i}]}" != "-1" ] && _check_filepath ${sysfs_path}; then
                port_absent=$(eval "cat ${sysfs_path}")
                _echo "[Port${port_name_array[${i}]} Module Absent]: ${port_absent}"
                if [ "${show_sysfs}" == "${TRUE}" ]; then
                    _echo "${sysfs_path}"
                fi
            fi

            # Port Lower Power Mode Status (0: Normal Power Mode, 1:Low Power Mode)
            idx=${port_lp_mode_array[i]}
            sysfs_path="${sys_port_array[idx]}"
            if [ "${port_lp_mode_array[${i}]}" != "-1" ] && _check_filepath ${sysfs_path}; then
                port_lp_mode=$(eval "cat ${sysfs_path}")
                _echo "[Port${port_name_array[${i}]} Low Power Mode]: ${port_lp_mode}"
                if [ "${show_sysfs}" == "${TRUE}" ]; then
                    _echo "${sysfs_path}"
                fi
            fi

            # Port Reset Status (0:Reset, 1:Normal)
            idx=${port_reset_array[i]}
            sysfs_path="${sys_port_array[idx]}"
            if [ "${port_reset_array[${i}]}" != "-1" ] && _check_filepath ${sysfs_path}; then
                port_reset=$(eval "cat ${sysfs_path}")
                _echo "[Port${port_name_array[${i}]} Reset Status]: ${port_reset}"
                if [ "${show_sysfs}" == "${TRUE}" ]; then
                    _echo "${sysfs_path}"
                fi
            fi

            # Port Interrupt Status (0: Interrupted, 1:No Interrupt)
            idx=${port_intr_array[i]}
            sysfs_path="${sys_port_array[idx]}"
            if [ "${port_intr_array[${i}]}" != "-1" ] && _check_filepath ${sysfs_path}; then
                port_intr_l=$(eval "cat ${sysfs_path}")
                _echo "[Port${port_name_array[${i}]} Interrupt Status (L)]: ${port_intr_l}"
                if [ "${show_sysfs}" == "${TRUE}" ]; then
                    _echo "${sysfs_path}"
                fi
            fi

            # Port Tx Fault Status (0:normal, 1:tx fault)
            idx=${port_tx_fault_array[i]}
            sysfs_path="${sys_port_array[idx]}"
            if [ "${port_tx_fault_array[${i}]}" != "-1" ] && _check_filepath ${sysfs_path}; then
                port_tx_fault=$(eval "cat ${sysfs_path}")
                _echo "[Port${port_name_array[${i}]} Tx Fault Status]: ${port_tx_fault}"
                if [ "${show_sysfs}" == "${TRUE}" ]; then
                    _echo "${sysfs_path}"
                fi
            fi

            # Port Rx Loss Status (0:los undetected, 1: los detected)
            idx=${port_rx_los_array[i]}
            sysfs_path="${sys_port_array[idx]}"
            if [ "${port_rx_los_array[${i}]}" != "-1" ] && _check_filepath ${sysfs_path}; then
                port_rx_loss=$(eval "cat ${sysfs_path}")
                _echo "[Port${port_name_array[${i}]} Rx Loss Status]: ${port_rx_loss}"
                if [ "${show_sysfs}" == "${TRUE}" ]; then
                    _echo "${sysfs_path}"
                fi
            fi

            # Port Tx Disable Status (0:enable tx, 1: disable tx)
            idx=${port_tx_dis_array[i]}
            sysfs_path="${sys_port_array[idx]}"
            if [ "${port_tx_dis_array[${i}]}" != "-1" ] && _check_filepath ${sysfs_path}; then
                port_tx_dis=$(eval "cat ${sysfs_path}")
                _echo "[Port${port_name_array[${i}]} Tx Disable Status]: ${port_tx_dis}"
                if [ "${show_sysfs}" == "${TRUE}" ]; then
                    _echo "${sysfs_path}"
                fi
            fi

            # Port TX Rate Select (0: low rate, 1:full rate)
            idx=${port_tx_rate_sel_array[i]}
            sysfs_path="${sys_port_array[idx]}"
            if [ "${port_tx_rate_sel_array[${i}]}" != "-1" ] && _check_filepath ${sysfs_path}; then
                port_tx_rate_sel=$(eval "cat ${sysfs_path}")
                _echo "[Port${port_name_array[${i}]} Port TX Rate Select]: ${port_tx_rate_sel}"
                if [ "${show_sysfs}" == "${TRUE}" ]; then
                    _echo "${sysfs_path}"
                fi
            fi

            # Port RX Rate Select (0: low rate, 1:full rate)
            idx=${port_rx_rate_sel_array[i]}
            sysfs_path="${sys_port_array[idx]}"
            if [ "${port_rx_rate_sel_array[${i}]}" != "-1" ] && _check_filepath ${sysfs_path}; then
                port_rx_rate_sel=$(eval "cat ${sysfs_path}")
                _echo "[Port${port_name_array[${i}]} Port RX Rate Select]: ${port_rx_rate_sel}"
                if [ "${show_sysfs}" == "${TRUE}" ]; then
                    _echo "${sysfs_path}"
                fi
            fi

            # Port Dump EEPROM
            local eeprom_path="/sys/bus/i2c/devices/${port_eeprom_bus_array[${i}]}-0050/eeprom"

            if [ "${port_absent}" == "0" ] && _check_filepath "${eeprom_path}"; then
                port_eeprom=$(eval "dd if=${eeprom_path} bs=128 count=2 skip=0 status=none ${LOG_REDIRECT} | hexdump -C")
                if [ "${LOG_FILE_ENABLE}" == "1" ]; then
                    if [ "${port_type_array[${i}]}" == "${SFP}" ] || [ "${port_type_array[${i}]}" == "${MGMT}" ]; then
                        hexdump -C "${eeprom_path}" > ${LOG_FOLDER_PATH}/port${port_name_array[${i}]}_eeprom.log 2>&1
                    else
                        $(_eeprom_pages_dump ${port_name_array[${i}]} ${port_type_array[${i}]} ${eeprom_path})
                    fi
                fi
                if [ -z "$port_eeprom" ]; then
                    port_eeprom="ERROR!!! The result is empty. It should read failed (${eeprom_path})!!"
                fi
            else
                port_eeprom="N/A"
            fi

            _echo "[Port${port_name_array[${i}]} EEPROM Page0-1]:"
            _echo "${port_eeprom}"
            if [ "${show_sysfs}" == "${TRUE}" ]; then
                _echo "${eeprom_path}"
            fi
            _echo ""
        done
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _show_port_status {
    if [ "${BSP_INIT_FLAG}" == "1" ] && ([ "${GPIO_MAX_INIT_FLAG}" == "1" ] ||  [ "${GPIO_BASE_INIT_FLAG}" == "1" ]); then
        _show_port_status_sysfs
    fi
}

function _show_cpu_temperature_sysfs {
    _banner "show CPU Temperature"

    cpu_temp_array=("1")

    for (( i=0; i<${#cpu_temp_array[@]}; i++ ))
    do
        if [ -f "/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${cpu_temp_array[${i}]}_input" ]; then
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${cpu_temp_array[${i}]}_label"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${cpu_temp_array[${i}]}_input"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${cpu_temp_array[${i}]}_max"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${cpu_temp_array[${i}]}_crit"
            temp_label=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${cpu_temp_array[${i}]}_label ${LOG_REDIRECT}")
            temp_input=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${cpu_temp_array[${i}]}_input ${LOG_REDIRECT}")
            temp_max=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${cpu_temp_array[${i}]}_max ${LOG_REDIRECT}")
            temp_crit=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${cpu_temp_array[${i}]}_crit ${LOG_REDIRECT}")
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
    # Not Support
    return 0
    _banner "Show CPLD Interrupt"
}

function _show_cpld_interrupt {
    # Not Support
    return 0
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_cpld_interrupt_sysfs
    fi
}

function _show_system_led_sysfs {
    _banner "Show System LED"

    if [ "${MODEL_NAME}" == "${PLAT}" ]; then

        local sysfs_attr_status=(
            "psu_0_led_status"
            "psu_1_led_status"
            "system_led_status"
            "sync_led_status"
            "fan_led_status"
            "id_led_status"
        )

        local sysfs_attr_speed=(
            "psu_0_led_speed"
            "psu_1_led_speed"
            "system_led_speed"
            "sync_led_speed"
            "fan_led_speed"
            "id_led_speed"
        )

        local sysfs_attr_blink=(
            "psu_0_led_blink"
            "psu_1_led_blink"
            "system_led_blink"
            "sync_led_blink"
            "fan_led_blink"
            "id_led_blink"
        )

        local sysfs_attr_onoff=(
            "psu_0_led_onoff"
            "psu_1_led_onoff"
            "system_led_onoff"
            "sync_led_onoff"
            "fan_led_onoff"
            "id_led_onoff"
        )

        local desc_color=("Yellow" "Green" "Blue")
        local desc_speed=("0.5 Hz" "2 Hz")
        local desc_blink=("Solid" "Blink")
        local desc_onoff=("OFF" "ON")

        local led=(          "Sync " "Sys  " "Fan  " "PSU 0" "PSU 1" "ID   ")
        local led_sysfs_idx=(  3       2       4       0       1    5)
        local color=(          0       0       0       0       0    -1)
        #local blink=(          2       2       2       2       2    2)
        #local onoff=(          3       3       3       3       3    3)
        #local freq=(           1       1       1       1       1    1)
        local status="" speed="" blink="" onoff=""

        for (( i=0; i<${#led[@]}; i++ ))
        do
            idx=${led_sysfs_idx[i]}
            if _check_filepath "${SYSFS_CPLD1}/${sysfs_attr_status[idx]}"; then
                status=$(eval "cat ${SYSFS_CPLD1}/${sysfs_attr_status[idx]} ${LOG_REDIRECT}")
                speed=$(eval "cat ${SYSFS_CPLD1}/${sysfs_attr_speed[idx]} ${LOG_REDIRECT}") # (0: off,    1: on)
                blink=$(eval "cat ${SYSFS_CPLD1}/${sysfs_attr_blink[idx]} ${LOG_REDIRECT}") # (0: 0.5 Hz, 1: 2 Hz)
                onoff=$(eval "cat ${SYSFS_CPLD1}/${sysfs_attr_onoff[idx]} ${LOG_REDIRECT}") # (0: Solid,  1: Blink)

                if [ "${color[i]}" != "-1" ]; then
                    color=${status} # (0: yellow, 1: green)
                else
                    color=2 # (blue)
                fi

                #speed=$(((led_reg >> ${freq[i]})  & 2#1)) # (0: off,    1: on)
                #blink=$(((led_reg >> ${blink[i]}) & 2#1)) # (0: 0.5 Hz, 1: 2 Hz)
                #onoff=$(((led_reg >> ${onoff[i]}) & 2#1)) # (0: Solid,  1: Blink)

                _echo "[System LED ${led[i]}]: [${sysfs_attr_status[idx]} ${led_reg}] [${desc_color[color]}][${desc_speed[speed]}][${desc_blink[blink]}][${desc_onoff[onoff]}]"

            else
                _echo "${led[i]}: ${SYSFS_CPLD1}/${sysfs_attr_status[idx]} not exist!!!"
            fi
        done
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

    base=0xE00
    offset=0x0
    reg=$(( ${base} + ${offset} ))
    reg=`printf "0x%X\n" ${reg}`
    ret=""

    while [ "${reg}" != "0xF00" ]
    do
        ret=$(_dd_read_byte ${reg})
        _echo "[${reg}] ${ret}"

        offset=$(( ${offset} + 1 ))
        reg=$(( ${base} + ${offset} ))
        reg=`printf "0x%X\n" ${reg}`
    done

    base=0x600
    offset=0x0
    reg=$(( ${base} + ${offset} ))
    reg=`printf "0x%X\n" ${reg}`
    ret=""

    while [ "${reg}" != "0x700" ]
    do
        ret=$(_dd_read_byte ${reg})
        _echo "[${reg}] ${ret}"

        offset=$(( ${offset} + 1 ))
        reg=$(( ${base} + ${offset} ))
        reg=`printf "0x%X\n" ${reg}`
    done

    base=0x2300
    offset=0x0
    reg=$(( ${base} + ${offset} ))
    reg=`printf "0x%X\n" ${reg}`
    ret=""

    while [ "${reg}" != "0x2400" ]
    do
        ret=$(_dd_read_byte ${reg})
        _echo "[${reg}] ${ret}"

        offset=$(( ${offset} + 1 ))
        reg=$(( ${base} + ${offset} ))
        reg=`printf "0x%X\n" ${reg}`
    done
}

function _show_cpld_reg_sysfs {
    _banner "Show CPLD Register"

    case $PLAT in
        *$MODEL_NAME* )
            if _check_dirpath "$SYSFS_CPLD1"; then
                reg_dump=$(eval "i2cdump -f -y 2 0x30 ${LOG_REDIRECT}")
                _echo "[CPLD 1 Register]:"
                _echo "${reg_dump}"
            fi

            if _check_dirpath "$SYSFS_CPLD2"; then
                reg_dump=$(eval "i2cdump -f -y 2 0x31 ${LOG_REDIRECT}")
                _echo "[CPLD 2 Register]:"
                _echo "${reg_dump}"
            fi

            if _check_dirpath "$SYSFS_CPLD3"; then
                reg_dump=$(eval "i2cdump -f -y 2 0x32 ${LOG_REDIRECT}")
                _echo "[CPLD 3 Register]:"
                _echo "${reg_dump}"
            fi

            if _check_dirpath "$SYSFS_CPLD4"; then
                reg_dump=$(eval "i2cdump -f -y 16 0x33 ${LOG_REDIRECT}")
                _echo "[CPLD 4 Register]:"
                _echo "${reg_dump}"
            fi

            if _check_dirpath "$SYSFS_FPGA"; then
                reg_dump=$(eval "i2cdump -f -y 2 0x37 ${LOG_REDIRECT}")
                _echo "[FPGA Register]:"
                _echo "${reg_dump}"
            fi
            ;;
        *)
            _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
            exit 1
            ;;
    esac
}

function _show_cpld_reg {
    if [ "${BSP_INIT_FLAG}" == "1" ] ; then
        _show_cpld_reg_sysfs
    fi
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

    _echo "[Command]: grep 046b /sys/bus/usb/devices/*/idVendor"
    ret=$(eval "grep 046b /sys/bus/usb/devices/*/idVendor ${LOG_REDIRECT}")
    _echo "${ret}"
    _echo ""

    # check usb auth
    _echo "[USB Port Authentication]: "
    usb_auth_file_array=("/sys/bus/usb/devices/usb1/authorized" \
                         "/sys/bus/usb/devices/usb1/authorized_default" \
                         "/sys/bus/usb/devices/1-4/authorized" \
                         "/sys/bus/usb/devices/1-4.1/authorized" \
                         "/sys/bus/usb/devices/1-4:1.0/authorized" )

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

    cmd_array=("lsblk"
               "lsblk -O"
               "parted -s -l"
               "fdisk -l /dev/sda"
               "find /sys/fs/ -name errors_count -print -exec cat {} \;"
               "find /sys/fs/ -name first_error_time -print -exec cat {} \; -exec echo '' \;"
               "find /sys/fs/ -name last_error_time -print -exec cat {} \; -exec echo '' \;"
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

        echo "    The tarball is ready at ${LOG_FOLDER_ROOT}/${LOG_FOLDER_NAME}.tgz"
        _echo "    The tarball is ready at ${LOG_FOLDER_ROOT}/${LOG_FOLDER_NAME}.tgz"
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
    exit -1
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
    _show_gpio
    _show_psu_status_cpld
#   _show_rov # Not support
    _show_port_status
    _show_cpu_temperature
#   _show_cpld_interrupt # Not support
    _show_system_led
#   _show_beacon_led # Not support
    _show_ioport
    _show_cpld_reg
    _show_onlpdump
    _show_onlpd
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
    _show_bmc_sel_raw_data # Not support
    _show_bmc_sel_elist
    _show_bmc_sel_elist_detail
    _show_dmesg
    _additional_log_collection
    _show_time
    _compression

    echo "#   The tech-support collection is completed. Please share the tech support log file."
}

_getopts $@
_main
