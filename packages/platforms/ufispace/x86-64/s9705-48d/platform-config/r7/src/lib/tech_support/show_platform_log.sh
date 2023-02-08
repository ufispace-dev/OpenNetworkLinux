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
LOG_REDIRECT="2> /dev/null"

# GPIO_MAX: update by function _update_gpio_max
GPIO_MAX=0
GPIO_MAX_INIT_FLAG=0

# Execution Time
start_time=$(date +%s)
end_time=0
elapsed_time=0

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
    _banner "Package Version = 1.0.10"
}

function _update_gpio_max {
    _banner "Update GPIO MAX"
    local sysfs="/sys/devices/platform/x86_64_ufispace_s9705_48d_lpc/bsp/bsp_gpio_max"

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
        #_echo "File Path: ${filepath}"
        return ${TRUE}
    fi
}

function _check_i2c_device {
    i2c_addr=$1

    if [ -z "${i2c_addr}" ]; then
        _echo "ERROR, the ipnut string is empty!!!"
        return ${FALSE}
    fi

    value=$(eval "i2cget -y -f 0 ${i2c_addr} ${LOG_REDIRECT}")
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

    i2c_bus_0=$(eval "i2cdetect -y 0 ${LOG_REDIRECT} | grep UU")
    ret=$?
    if [ $ret -eq 0 ] && [ ! -z "${i2c_bus_0}" ] ; then
        BSP_INIT_FLAG=1
    else
        BSP_INIT_FLAG=0
    fi

    _echo "[BSP_INIT_FLAG]: ${BSP_INIT_FLAG}"
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
               "cat /proc/sys/kernel/printk" )

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
    build_rev_id_array=(0 1 2 3 4 5 6 7)
    build_rev_array=(1 2 3 4 5 6 7 8)
    hw_rev_id_array=(0 1 2 3)
    hw_rev_array=("Proto" "Alpha" "Beta" "PVT")
    model_id_array=($((2#010)) $((2#100)) $((2#111)) $((2#110)))
    model_name_array=("NCP1-1" "NCP2-1" "NCF" "NCF-AI")

    #board_info=`${IOGET} 0x700 | awk -F" " '{print $NF}'`
    board_info=`${IOGET} 0x700`
    ret=$?
    if [ $ret -eq 0 ]; then
        board_info=`echo ${board_info} | awk -F" " '{print $NF}'`
    else
        _echo "Get board info failed ($ret), Exit!!"
        exit $ret
    fi

    # Build Rev D[7],D[0:1]
    build_rev_id=$(((board_info & 2#10000000) >> 5 | (board_info & 2#00000011) >> 0))
    build_rev=${build_rev_array[${build_rev_id}]}

    # HW Rev D[2:3]
    hw_rev_id=$(((board_info & 2#00001100) >> 2))
    hw_rev=${hw_rev_array[${hw_rev_id}]}

    # Model ID D[6:4]
    model_id=$(((board_info & 2#01110000) >> 4))
    if [ $model_id -eq ${model_id_array[0]} ]; then
       model_name=${model_name_array[0]}
    elif [ $model_id -eq ${model_id_array[1]} ]; then
       model_name=${model_name_array[1]}
    elif [ $model_id -eq ${model_id_array[2]} ]; then
       model_name=${model_name_array[2]}
    else
       _echo "Invalid model_id: ${model_id}"
       exit 1
    fi

    MODEL_NAME=${model_name}
    HW_REV=${hw_rev}
    _echo "[Board Type/Rev Reg Raw ]: ${board_info}"
    _echo "[Board Type and Revision]: ${model_name} ${hw_rev} ${build_rev}"

    if [ "${model_name}" == "NCF" ]; then
        _check_filepath "/sys/bus/i2c/devices/2-0032/cpld_board_type"
        board_info_top=$(eval "cat /sys/bus/i2c/devices/2-0032/cpld_board_type ${LOG_REDIRECT}")

        # Build Rev D[7],D[0:1]
        build_rev_id_top=$(((board_info_top & 2#10000000) >> 5 | (board_info_top & 2#00000011) >> 0))
        build_rev_top=${build_rev_array[${build_rev_id_top}]}

        # HW Rev D[2:3]
        hw_rev_id_top=$(((board_info_top & 2#00001100) >> 2))
        hw_rev_top=${hw_rev_array[${hw_rev_id_top}]}

        # Model ID D[6:4]
        model_id_top=$(((board_info_top & 2#01110000) >> 4))
        if [ $model_id_top -eq ${model_id_array[2]} ]; then
           model_name_top=${model_name_array[2]}
        elif [ $model_id_top -eq ${model_id_array[3]} ]; then
           model_name_top=${model_name_array[3]}
        else
           _echo "Invalid model_id_top: ${model_id_top}"
           exit 1
        fi

        MODEL_NAME_TOP=${model_name_top}
        HW_REV_TOP=${hw_rev_top}
        _echo "[Board Type/Rev Reg Raw  (TOP)]: ${board_info_top}"
        _echo "[Board Type and Revision (TOP)]: ${model_name_top} ${hw_rev_top} ${build_rev_top}"
    fi
}

function _bios_version {
    _banner "Show BIOS Version"

    bios_ver=$(eval "dmidecode -s bios-version ${LOG_REDIRECT}")
    me_ver=$(eval "ipmitool -t 0x2c -b 0x6 raw 0x2e 0xde 0x57 0x01 0x00 0x00 0x0 0x0 0x0 0x2 0x0 0x0 0xff ${LOG_REDIRECT}")
    bios_boot_rom=`${IOGET} 0x602`
    if [ $? -eq 0 ]; then
        bios_boot_rom=`echo ${bios_boot_rom} | awk -F" " '{print $NF}'`
    fi

    _echo "[BIOS Vesion  ]: ${bios_ver}"
    _echo "[BIOS Boot ROM]: ${bios_boot_rom}"
    _echo "[ME Vesion    ]: ${me_ver}"
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

    # CPU CPLD
    cpu_cpld_info=`${IOGET} 0x600`
    ret=$?
    if [ $ret -eq 0 ]; then
        cpu_cpld_info=`echo ${cpu_cpld_info} | awk -F" " '{print $NF}'`
    else
        _echo "Get CPU CPLD version info failed ($ret), Exit!!"
        exit $ret
    fi

    _echo "[CPU CPLD Reg Raw]: ${cpu_cpld_info} "
    _echo "[CPU CPLD Version]: $(( (cpu_cpld_info & 2#01000000) >> 6)).$(( cpu_cpld_info & 2#00111111 ))"

    if [ "${MODEL_NAME}" == "NCF" ]; then
        # MB CPLD NCF
        mb_cpld1_ver=""
        mb_cpld2_ver=""
        mb_cpld3_ver=""
        mb_cpld4_ver=""

        _check_i2c_device "0x75"
        value=$(eval "i2cget -y -f 0 0x75 ${LOG_REDIRECT}")
        ret=$?

        if [ ${ret} -eq 0 ]; then
            i2cset -y 0 0x75 0x2
            _check_i2c_device "0x30"
            _check_i2c_device "0x31"
            _check_i2c_device "0x32"
            _check_i2c_device "0x33"
            mb_cpld1_ver=$(eval "i2cget -y -f 0 0x30 0x2 ${LOG_REDIRECT}")
            mb_cpld2_ver=$(eval "i2cget -y -f 0 0x31 0x2 ${LOG_REDIRECT}")
            mb_cpld3_ver=$(eval "i2cget -y -f 0 0x32 0x2 ${LOG_REDIRECT}")
            mb_cpld4_ver=$(eval "i2cget -y -f 0 0x33 0x2 ${LOG_REDIRECT}")
            i2cset -y 0 0x75 0x0
        fi

        _echo "[MB CPLD1 Version]: $(( (mb_cpld1_ver & 2#11000000) >> 6)).$(( mb_cpld1_ver & 2#00111111 ))"
        _echo "[MB CPLD2 Version]: $(( (mb_cpld2_ver & 2#11000000) >> 6)).$(( mb_cpld2_ver & 2#00111111 ))"
        _echo "[MB CPLD3 Version]: $(( (mb_cpld3_ver & 2#11000000) >> 6)).$(( mb_cpld3_ver & 2#00111111 ))"
        _echo "[MB CPLD4 Version]: $(( (mb_cpld4_ver & 2#11000000) >> 6)).$(( mb_cpld4_ver & 2#00111111 ))"

    elif [ "${MODEL_NAME}" == "NCP1-1" ]; then
        # MB CPLD NCP1-1
        mb_cpld1_ver=""
        mb_cpld2_ver=""
        mb_cpld3_ver=""
        mb_cpld4_ver=""
        mb_cpld5_ver=""

        _check_i2c_device "0x75"
        value=$(eval "i2cget -y -f 0 0x75 ${LOG_REDIRECT}")
        ret=$?

        if [ ${ret} -eq 0 ]; then
            i2cset -y 0 0x75 0x1
            _check_i2c_device "0x30"
            _check_i2c_device "0x39"
            _check_i2c_device "0x3a"
            _check_i2c_device "0x3b"
            _check_i2c_device "0x3c"
            mb_cpld1_ver=$(eval "i2cget -y -f 0 0x30 0x2 ${LOG_REDIRECT}")
            mb_cpld2_ver=$(eval "i2cget -y -f 0 0x39 0x2 ${LOG_REDIRECT}")
            mb_cpld3_ver=$(eval "i2cget -y -f 0 0x3a 0x2 ${LOG_REDIRECT}")
            mb_cpld4_ver=$(eval "i2cget -y -f 0 0x3b 0x2 ${LOG_REDIRECT}")
            mb_cpld5_ver=$(eval "i2cget -y -f 0 0x3c 0x2 ${LOG_REDIRECT}")
            i2cset -y 0 0x75 0x0
        fi

        _echo "[MB CPLD1 Version]: $(( (mb_cpld1_ver & 2#11000000) >> 6)).$(( mb_cpld1_ver & 2#00111111 ))"
        _echo "[MB CPLD2 Version]: $(( (mb_cpld2_ver & 2#11000000) >> 6)).$(( mb_cpld2_ver & 2#00111111 ))"
        _echo "[MB CPLD3 Version]: $(( (mb_cpld3_ver & 2#11000000) >> 6)).$(( mb_cpld3_ver & 2#00111111 ))"
        _echo "[MB CPLD4 Version]: $(( (mb_cpld4_ver & 2#11000000) >> 6)).$(( mb_cpld4_ver & 2#00111111 ))"
        _echo "[MB CPLD5 Version]: $(( (mb_cpld5_ver & 2#11000000) >> 6)).$(( mb_cpld5_ver & 2#00111111 ))"

    elif [ "${MODEL_NAME}" == "NCP2-1" ]; then
        # MB CPLD NCP2-1
        mb_cpld1_ver=""
        mb_cpld2_ver=""
        mb_cpld3_ver=""

        _check_i2c_device "0x75"
        value=$(eval "i2cget -y -f 0 0x75 ${LOG_REDIRECT}")
        ret=$?

        if [ ${ret} -eq 0 ]; then
            i2cset -y 0 0x75 0x1
            _check_i2c_device "0x30"
            _check_i2c_device "0x31"
            _check_i2c_device "0x32"
            mb_cpld1_ver=$(eval "i2cget -y -f 0 0x30 0x2 ${LOG_REDIRECT}")
            mb_cpld2_ver=$(eval "i2cget -y -f 0 0x31 0x2 ${LOG_REDIRECT}")
            mb_cpld3_ver=$(eval "i2cget -y -f 0 0x32 0x2 ${LOG_REDIRECT}")
            i2cset -y 0 0x75 0x0
        fi

        _echo "[MB CPLD1 Version]: $(( (mb_cpld1_ver & 2#11000000) >> 6)).$(( mb_cpld1_ver & 2#00111111 ))"
        _echo "[MB CPLD2 Version]: $(( (mb_cpld2_ver & 2#11000000) >> 6)).$(( mb_cpld2_ver & 2#00111111 ))"
        _echo "[MB CPLD3 Version]: $(( (mb_cpld3_ver & 2#11000000) >> 6)).$(( mb_cpld3_ver & 2#00111111 ))"

    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _cpld_version_sysfs {
    _banner "Show CPLD Version (Sysfs)"

    # CPU CPLD
    cpu_cpld_info=`${IOGET} 0x600`
    ret=$?
    if [ $ret -eq 0 ]; then
        cpu_cpld_info=`echo ${cpu_cpld_info} | awk -F" " '{print $NF}'`
    else
        _echo "Get CPU CPLD version info failed ($ret), Exit!!"
        exit $ret
    fi

    _echo "[CPU CPLD Reg Raw]: ${cpu_cpld_info} "
    _echo "[CPU CPLD Version]: $(( (cpu_cpld_info & 2#01000000) >> 6)).$(( cpu_cpld_info & 2#00111111 ))"

    if [ "${MODEL_NAME}" == "NCF" ]; then
        # MB CPLD NCF
        _check_filepath "/sys/bus/i2c/devices/2-0030/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/2-0031/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/2-0032/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/2-0033/cpld_version"
        mb_cpld1_ver=$(eval "cat /sys/bus/i2c/devices/2-0030/cpld_version ${LOG_REDIRECT}")
        mb_cpld2_ver=$(eval "cat /sys/bus/i2c/devices/2-0031/cpld_version ${LOG_REDIRECT}")
        mb_cpld3_ver=$(eval "cat /sys/bus/i2c/devices/2-0032/cpld_version ${LOG_REDIRECT}")
        mb_cpld4_ver=$(eval "cat /sys/bus/i2c/devices/2-0033/cpld_version ${LOG_REDIRECT}")

        _echo "[MB CPLD1 Version]: $(( (mb_cpld1_ver & 2#11000000) >> 6)).$(( mb_cpld1_ver & 2#00111111 ))"
        _echo "[MB CPLD2 Version]: $(( (mb_cpld2_ver & 2#11000000) >> 6)).$(( mb_cpld2_ver & 2#00111111 ))"
        _echo "[MB CPLD3 Version]: $(( (mb_cpld3_ver & 2#11000000) >> 6)).$(( mb_cpld3_ver & 2#00111111 ))"
        _echo "[MB CPLD4 Version]: $(( (mb_cpld4_ver & 2#11000000) >> 6)).$(( mb_cpld4_ver & 2#00111111 ))"

    elif [ "${MODEL_NAME}" == "NCP1-1" ]; then
        # MB CPLD NCP1-1
        _check_filepath "/sys/bus/i2c/devices/1-0030/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/1-0039/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/1-003a/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/1-003b/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/1-003c/cpld_version"
        mb_cpld1_ver=$(eval "cat /sys/bus/i2c/devices/1-0030/cpld_version ${LOG_REDIRECT}")
        mb_cpld2_ver=$(eval "cat /sys/bus/i2c/devices/1-0039/cpld_version ${LOG_REDIRECT}")
        mb_cpld3_ver=$(eval "cat /sys/bus/i2c/devices/1-003a/cpld_version ${LOG_REDIRECT}")
        mb_cpld4_ver=$(eval "cat /sys/bus/i2c/devices/1-003b/cpld_version ${LOG_REDIRECT}")
        mb_cpld5_ver=$(eval "cat /sys/bus/i2c/devices/1-003c/cpld_version ${LOG_REDIRECT}")

        _echo "[MB CPLD1 Version]: $(( (mb_cpld1_ver & 2#11000000) >> 6)).$(( mb_cpld1_ver & 2#00111111 ))"
        _echo "[MB CPLD2 Version]: $(( (mb_cpld2_ver & 2#11000000) >> 6)).$(( mb_cpld2_ver & 2#00111111 ))"
        _echo "[MB CPLD3 Version]: $(( (mb_cpld3_ver & 2#11000000) >> 6)).$(( mb_cpld3_ver & 2#00111111 ))"
        _echo "[MB CPLD4 Version]: $(( (mb_cpld4_ver & 2#11000000) >> 6)).$(( mb_cpld4_ver & 2#00111111 ))"
        _echo "[MB CPLD5 Version]: $(( (mb_cpld5_ver & 2#11000000) >> 6)).$(( mb_cpld5_ver & 2#00111111 ))"

    elif [ "${MODEL_NAME}" == "NCP2-1" ]; then
        # MB CPLD NCP2-1
        _check_filepath "/sys/bus/i2c/devices/1-0030/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/1-0031/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/1-0032/cpld_version"
        mb_cpld1_ver=$(eval "cat /sys/bus/i2c/devices/1-0030/cpld_version ${LOG_REDIRECT}")
        mb_cpld2_ver=$(eval "cat /sys/bus/i2c/devices/1-0031/cpld_version ${LOG_REDIRECT}")
        mb_cpld3_ver=$(eval "cat /sys/bus/i2c/devices/1-0032/cpld_version ${LOG_REDIRECT}")

        _echo "[MB CPLD1 Version]: $(( (mb_cpld1_ver & 2#11000000) >> 6)).$(( mb_cpld1_ver & 2#00111111 ))"
        _echo "[MB CPLD2 Version]: $(( (mb_cpld2_ver & 2#11000000) >> 6)).$(( mb_cpld2_ver & 2#00111111 ))"
        _echo "[MB CPLD3 Version]: $(( (mb_cpld3_ver & 2#11000000) >> 6)).$(( mb_cpld3_ver & 2#00111111 ))"

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


    if [ "${MODEL_NAME}" == "NCF" ]; then
        brd=("TOP" "BOT")
    elif [ "${MODEL_NAME}" == "NCP1-1" ]; then
        brd=("MB")
    elif [ "${MODEL_NAME}" == "NCP2-1" ]; then
        brd=("MB")
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    brd=("TOP" "BOT")


    for (( i=0; i<${#brd[@]}; i++ ))
    do
        #get ucd version via BMC
        if [ "${MODEL_NAME}" == "NCF" ]; then
            ucd_ver_raw=$(eval "ipmitool raw 0x3c 0x08 $(( 1-i )) ${LOG_REDIRECT}")
        else
            ucd_ver_raw=$(eval "ipmitool raw 0x3c 0x08 ${LOG_REDIRECT}")
        fi

        ret=$?

        #check return code
        if [ ! $ret -eq 0 ] ; then
            _echo "Require BMC v2.10.1 or later to get UCD version"
            return $ret
        fi

        #add prefix 0x for each byte
        ucd_ver_hex=${ucd_ver_raw// /0x}

        #convert hex to ascii
        ucd_ver_ascii=`echo $ucd_ver_hex | xxd -r`

        #ascii len
        ucd_ver_ascii_len=${#ucd_ver_ascii}

        #last 6 char are for date
        ucd_ver_date=${ucd_ver_ascii:len-6:ucd_ver_ascii_len}

        #first len-6 char are for revision
        ucd_ver_revision=${ucd_ver_ascii:0:ucd_ver_ascii_len-6}

        _echo "[${brd[i]} UCD Info Raw]: ${ucd_ver_raw}"
        _echo "[${brd[i]} MFR_REVISION]: ${ucd_ver_revision}"
        _echo "[${brd[i]} MFR_DATE    ]: ${ucd_ver_date}"
    done
}

function _show_version {
    _bios_version
    _bmc_version
    _cpld_version
    _ucd_version
}

function _show_i2c_tree_bus_0 {
    _banner "Show I2C Tree Bus 0"

    ret=$(eval "i2cdetect -y 0 ${LOG_REDIRECT}")

    _echo "[I2C Tree]:"
    _echo "${ret}"
}

function _show_i2c_mux_devices {
    local chip_addr=$1
    local channel_num=$2
    local chip_dev_desc=$3
    local i=0;

    if [ -z "${chip_addr}" ] || [ -z "${channel_num}" ] || [ -z "${chip_dev_desc}" ]; then
        _echo "ERROR: parameter cannot be empty!!!"
        exit 99
    fi

    _check_i2c_device "$chip_addr"
    ret=$?
    if [ "$ret" == "0" ]; then
        _echo "TCA9548 Mux ${chip_dev_desc}"
        _echo "---------------------------------------------------"
        for (( i=0; i<${channel_num}; i++ ))
        do
            _echo "TCA9548 Mux ${chip_dev_desc} - Channel ${i}"
            # open mux channel - 0x75
            i2cset -y 0 ${chip_addr} $(( 2 ** ${i} ))
            # dump i2c tree
            ret=$(eval "i2cdetect -y 0 ${LOG_REDIRECT}")
            _echo "${ret}"
            # close mux channel
            i2cset -y 0 ${chip_addr} 0x0
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

    if [ "${MODEL_NAME}" == "NCF" ]; then
        ## ROOT-0x75
        _show_i2c_mux_devices "0x75" "8" "ROOT-0x75"

        ## ROOT-0x75-Channel(5)-0x72
        chip_addr1="0x75"
        chip_addr2="0x72"
        chip_addr1_chann="5"
        _check_i2c_device "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            # open mux channel - 0x75 (chip_addr1) channel 5
            i2cset -y 0 ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
            _show_i2c_mux_devices "${chip_addr2}" "4" "ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}"
            # close mux channel - 0x75 (chip_addr1) channel 5
            i2cset -y 0 ${chip_addr1} 0x0
        fi

        ## ROOT-0x75-Channel(5)-0x72-Channel(1~3)-0x70
        chip_addr1="0x75"
        chip_addr2="0x72"
        chip_addr3="0x70"
        chip_addr1_chann="5"
        _check_i2c_device "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            # open mux channel - 0x75 (chip_addr1) channel 5
            i2cset -y 0 ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
            _check_i2c_device "${chip_addr2}"
            ret=$?
            if [ "$ret" == "0" ]; then
                for (( chip_addr2_chann=1; chip_addr2_chann<4; chip_addr2_chann++ ))
                do
                    # open mux channel - 0x72 (chip_addr2) channel n
                    i2cset -y 0 ${chip_addr2} $(( 2 ** ${chip_addr2_chann} ))
                    _show_i2c_mux_devices "${chip_addr3}" "8" "ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}-${chip_addr2_chann}-${chip_addr3}"
                    # close mux channel - 0x72 (chip_addr2) channel n
                    i2cset -y 0 ${chip_addr2} 0x0
                done
            fi
            # close mux channel - 0x75 (chip_addr1) channel 5
            i2cset -y 0 ${chip_addr1} 0x0
        fi

        ## ROOT-0x75-Channel(5)-0x74
        chip_addr1="0x75"
        chip_addr2="0x74"
        chip_addr1_chann="5"
        _check_i2c_device "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            # open mux channel - 0x75 (chip_addr1) channel 5
            i2cset -y 0 ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
            _show_i2c_mux_devices "${chip_addr2}" "4" "ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}"
            # close mux channel - 0x75 (chip_addr1) channel 5
            i2cset -y 0 ${chip_addr1} 0x0
        fi

        ## ROOT-0x75-Channel(5)-0x74-Channel(1~3)-0x70
        chip_addr1="0x75"
        chip_addr2="0x74"
        chip_addr3="0x70"
        chip_addr1_chann="5"
        _check_i2c_device "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            # open mux channel - 0x75 (chip_addr1) channel 5
            i2cset -y 0 ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
            _check_i2c_device "${chip_addr2}"
            ret=$?
            if [ "$ret" == "0" ]; then
                for (( chip_addr2_chann=1; chip_addr2_chann<4; chip_addr2_chann++ ))
                do
                    # open mux channel - 0x74 (chip_addr2) channel n
                    i2cset -y 0 ${chip_addr2} $(( 2 ** ${chip_addr2_chann} ))
                    _show_i2c_mux_devices "${chip_addr3}" "8" "ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}-${chip_addr2_chann}-${chip_addr3}"
                    # close mux channel - 0x74 (chip_addr2) channel n
                    i2cset -y 0 ${chip_addr2} 0x0
                done
            fi
            # close mux channel - 0x75 (chip_addr1) channel 5
            i2cset -y 0 ${chip_addr1} 0x0
        fi

        ## ROOT-0x71
        _show_i2c_mux_devices "0x71" "4" "ROOT-0x71"

    elif [ "${MODEL_NAME}" == "NCP1-1" ]; then
        ## ROOT-0x75
        _show_i2c_mux_devices "0x75" "8" "ROOT-0x75"

        ## ROOT-0x72
        _show_i2c_mux_devices "0x72" "8" "ROOT-0x72"

        ## ROOT-0x72-Channel(0~7)-0x76-Channel(0~7)
        chip_addr1="0x72"
        chip_addr2="0x76"
        _check_i2c_device "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            for (( chip_addr1_chann=0; chip_addr1_chann<8; chip_addr1_chann++ ))
            do
                # open mux channel - 0x72 (chip_addr1)
                i2cset -y 0 ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
                _show_i2c_mux_devices "${chip_addr2}" "8" "ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}"
                # close mux channel - 0x72 (chip_addr1)
                i2cset -y 0 ${chip_addr1} 0x0
            done
        fi

        ## ROOT-0x73
        _show_i2c_mux_devices "0x73" "4" "ROOT-0x73"

    elif [ "${MODEL_NAME}" == "NCP2-1" ]; then
        ## ROOT-0x75
        _show_i2c_mux_devices "0x75" "8" "ROOT-0x75"

        ## ROOT-0x72
        _show_i2c_mux_devices "0x72" "8" "ROOT-0x72"

        ## ROOT-0x72-Channel(0,1,6,7)-0x76-Channel(0~7)
        chip_addr1="0x72"
        chip_addr2="0x76"
        _check_i2c_device "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            for chip_addr1_chann in {0,1,6,7}
            do
                # open mux channel - 0x72 (chip_addr1)
                i2cset -y 0 ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
                _show_i2c_mux_devices "${chip_addr2}" "8" "ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}"
                # close mux channel - 0x72 (chip_addr1)
                i2cset -y 0 ${chip_addr1} 0x0
            done
        fi

        ## ROOT-0x73
        if [ "${HW_REV}" == "PVT" ]; then
            _show_i2c_mux_devices "0x73" "8" "ROOT-0x73"
        else
            # Hardware Limitation
            _show_i2c_mux_devices "0x73" "4" "ROOT-0x73"
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

    ret=`i2cdump -y -f 0 0x77 b`
    _echo "[I2C Device 0x77]:"
    _echo "${ret}"
    _echo ""

    local pca954x_device_id=("")
    if [ "${MODEL_NAME}" == "NCF" ]; then
        pca954x_device_id=("0x71" "0x75")
    elif [ "${MODEL_NAME}" == "NCP1-1" ] || [ "${MODEL_NAME}" == "NCP2-1" ]; then
        pca954x_device_id=("0x72" "0x73" "0x75")
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    for ((i=0;i<5;i++))
    do
        _echo "[DEV PCA9548 (${i})]"
        for (( j=0; j<${#pca954x_device_id[@]}; j++ ))
        do
            ret=`i2cget -f -y 0 ${pca954x_device_id[$j]}`
            _echo "[I2C Device ${pca954x_device_id[$j]}]: $ret"
        done
        sleep 0.4
    done
}

function _show_sys_devices {
    _banner "Show System Devices"

    _echo "[Command]: ls /sys/class/gpio/"
    ret=($(ls /sys/class/gpio/))
    _echo "#${ret[*]}"

    local file_path="/sys/kernel/debug/gpio"
    if [ -f "${file_path}" ]; then
        _echo ""
        _echo "[Command]: cat ${file_path}"
        _echo "$(cat ${file_path})"
    fi

    _echo ""
    _echo "[Command]: ls /sys/bus/i2c/devices/"
    ret=($(ls /sys/bus/i2c/devices/))
    _echo "#${ret[*]}"

    _echo ""
    _echo "[Command]: ls /dev/"
    ret=($(ls /dev/))
    _echo "#${ret[*]}"
}

function _show_cpu_eeprom_i2c {
    _banner "Show CPU EEPROM"

    cpu_eeprom=$(eval "i2cdump -y 0 0x57 c")
    cpu_eeprom=$(eval "i2cdump -y 0 0x57 c")
    _echo "[CPU EEPROM]:"
    _echo "${cpu_eeprom}"
}

function _show_cpu_eeprom_sysfs {
    _banner "Show CPU EEPROM"

    cpu_eeprom=$(eval "cat /sys/bus/i2c/devices/0-0057/eeprom ${LOG_REDIRECT} | hexdump -C")
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
    if [ "${MODEL_NAME}" == "NCF" ]; then
        bus_id="2"
    elif [ "${MODEL_NAME}" == "NCP1-1" ] || [ "${MODEL_NAME}" == "NCP2-1" ]; then
        bus_id="1"
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    # Read PSU0 Power Good Status (1: power good, 0: not providing power)
    _check_filepath "/sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status_0"
    cpld_psu_status_0_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status_0 ${LOG_REDIRECT}")
    psu0_power_ok=$(((cpld_psu_status_0_reg & 2#00010000) >> 4))

    # Read PSU0 Absent Status (0: psu present, 1: psu absent)
    _check_filepath "/sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status_0"
    cpld_psu_status_0_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status_0 ${LOG_REDIRECT}")
    psu0_absent_l=$(((cpld_psu_status_0_reg & 2#01000000) >> 6))

    # Read PSU1 Power Good Status (1: power good, 0: not providing power)
    _check_filepath "/sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status_1"
    cpld_psu_status_1_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status_1 ${LOG_REDIRECT}")
    psu1_power_ok=$(((cpld_psu_status_1_reg & 2#00100000) >> 5))

    # Read PSU1 Absent Status (0: psu present, 1: psu absent)
    _check_filepath "/sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status_1"
    cpld_psu_status_1_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status_1 ${LOG_REDIRECT}")
    psu1_absent_l=$(((cpld_psu_status_1_reg & 2#10000000) >> 7))


    _echo "[PSU0 Status Reg Raw   ]: ${cpld_psu_status_0_reg}"
    _echo "[PSU0 Power Good Status]: ${psu0_power_ok}"
    _echo "[PSU0 Absent Status (L)]: ${psu0_absent_l}"
    _echo "[PSU1 Status Reg Raw   ]: ${cpld_psu_status_1_reg}"
    _echo "[PSU1 Power Good Status]: ${psu1_power_ok}"
    _echo "[PSU1 Absent Status (L)]: ${psu1_absent_l}"
}

function _show_psu_status_cpld {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_psu_status_cpld_sysfs
    fi
}

function _show_rov_sysfs {
    _banner "Show ROV"

    bus_id=""
    if [ "${MODEL_NAME}" == "NCF" ]; then
        bus_id="2"
        top_rov_i2c_bus="5"
        bot_rov_i2c_bus="4"
        rov_i2c_addr="0x70"
        rov_config_reg="0x21"
        rov_output_reg="0x8b"

        # Read Top RAMON ROV Status
        _check_filepath "/sys/bus/i2c/devices/${bus_id}-0030/cpld_10gmux_config"
        cpld_10gmux_config_reg1=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0030/cpld_10gmux_config ${LOG_REDIRECT}")

        if [ -c "/dev/i2c-${top_rov_i2c_bus}" ]; then
            # TOP ROV
            top_ramon_rov_stamp=$(((cpld_10gmux_config_reg1 & 2#00000111) >> 0))
            top_rov_controller_config=`i2cget -y ${top_rov_i2c_bus} ${rov_i2c_addr} ${rov_config_reg}`
            top_rov_controller_config_value=$((top_rov_controller_config + 0))
            #top_rov_controller_config_volt=$(( (top_rov_controller_config - 1) * 0.005 + 0.25))
            top_rov_controller_config_volt=$(echo "${top_rov_controller_config_value}" | awk '{printf "%.3f\n", (($1-1)*0.005+0.25)}')
            top_rov_controller_output=`i2cget -y ${top_rov_i2c_bus} ${rov_i2c_addr} ${rov_output_reg}`
            top_rov_controller_output_value=$((top_rov_controller_output + 0))
            #top_rov_controller_output_volt=$(( (top_rov_controller_output - 1) * 0.005 + 0.25))
            top_rov_controller_output_volt=$(echo "${top_rov_controller_output_value}" | awk '{printf "%.3f\n", (($1-1)*0.005+0.25)}')
        else
            _echo "device (/dev/i2c-${top_rov_i2c_bus}) not found!!!"
        fi

        if [ -c "/dev/i2c-${bot_rov_i2c_bus}" ]; then
            # Read Bottom RAMON ROV Status
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-0032/cpld_10gmux_config"
            cpld_10gmux_config_reg2=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0032/cpld_10gmux_config ${LOG_REDIRECT}")

            bot_ramon_rov_stamp=$(((cpld_10gmux_config_reg2 & 2#00000111) >> 0))
            bot_rov_controller_config=`i2cget -y ${bot_rov_i2c_bus} ${rov_i2c_addr} ${rov_config_reg}`
            bot_rov_controller_config_value=$((bot_rov_controller_config + 0))
            #bot_rov_controller_config_volt=$(( (bot_rov_controller_config - 1) * 0.005 + 0.25))
            bot_rov_controller_config_volt=$(echo "${bot_rov_controller_config_value}" | awk '{printf "%.3f\n", (($1-1)*0.005+0.25)}')
            bot_rov_controller_output=`i2cget -y ${bot_rov_i2c_bus} ${rov_i2c_addr} ${rov_output_reg}`
            bot_rov_controller_output_value=$((bot_rov_controller_output + 0))
            #bot_rov_controller_output_volt=$(( (bot_rov_controller_output - 1) * 0.005 + 0.25))
            bot_rov_controller_output_volt=$(echo "${bot_rov_controller_output_value}" | awk '{printf "%.3f\n", (($1-1)*0.005+0.25)}')
        else
            _echo "device (/dev/i2c-${bot_rov_i2c_bus}) not found!!!"
        fi

        _echo "[TOP RAMON ROV Stamp      ]: ${top_ramon_rov_stamp}"
        _echo "[TOP RAMON ROV Stamp Array]: ROV Config  : [0x73 , 0x73 , 0x6F , 0x73 , 0x77 , 0x7B , 0x73 , 0x6B ]"
        _echo "                             VDDC Voltage: [0.82V  0.82V, 0.80V, 0.82V, 0.84V, 0.86V, 0.82V, 0.78V]"
        _echo "[TOP RAMON ROV Config     ]: ${top_rov_controller_config}"
        _echo "[TOP RAMON ROV Config Volt]: ${top_rov_controller_config_volt}V"
        _echo "[TOP RAMON ROV Output     ]: ${top_rov_controller_output}"
        _echo "[TOP RAMON ROV Output Volt]: ${top_rov_controller_output_volt}V"
        _echo "============================"
        _echo "[BOT RAMON ROV Stamp      ]: ${bot_ramon_rov_stamp}"
        _echo "[TOP RAMON ROV Stamp Array]: ROV Config  : [0x73 , 0x73 , 0x6F , 0x73 , 0x77 , 0x7B , 0x73 , 0x6B ]"
        _echo "                             VDDC Voltage: [0.82V  0.82V, 0.80V, 0.82V, 0.84V, 0.86V, 0.82V, 0.78V]"
        _echo "[BOT RAMON ROV Config     ]: ${bot_rov_controller_config}"
        _echo "[BOT RAMON ROV Config Volt]: ${bot_rov_controller_config_volt}V"
        _echo "[BOT RAMON ROV Output     ]: ${bot_rov_controller_output}"
        _echo "[BOT RAMON ROV Output Volt]: ${bot_rov_controller_output_volt}V"

    elif [ "${MODEL_NAME}" == "NCP1-1" ] || [ "${MODEL_NAME}" == "NCP2-1" ]; then
        bus_id="1"
        rov_i2c_bus="4"
        rov_i2c_addr="0x76"
        rov_config_reg="0x21"
        rov_output_reg="0x8b"

        # Read J2 ROV Status
        _check_filepath "/sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status_0"
        cpld_psu_status_0_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status_0 ${LOG_REDIRECT}")

        if [ -c "/dev/i2c-${rov_i2c_bus}" ]; then
            j2_rov_stamp=$(((cpld_psu_status_0_reg & 2#00001110) >> 1))
            rov_controller_config=`i2cget -y ${rov_i2c_bus} ${rov_i2c_addr} ${rov_config_reg}`
            rov_controller_config_value=$((rov_controller_config + 0))
            #rov_controller_config_volt=$(( (rov_controller_config - 1) * 0.005 + 0.25))
            rov_controller_config_volt=$(echo "${rov_controller_config_value}" | awk '{printf "%.3f\n", (($1-1)*0.005+0.25)}')
            rov_controller_output=`i2cget -y ${rov_i2c_bus} ${rov_i2c_addr} ${rov_output_reg}`
            rov_controller_output_value=$((rov_controller_output + 0))
            #rov_controller_output_volt=$(( (rov_controller_output - 1) * 0.005 + 0.25))
            rov_controller_output_volt=$(echo "${rov_controller_output_value}" | awk '{printf "%.3f\n", (($1-1)*0.005+0.25)}')
        else
            _echo "device (/dev/i2c-${rov_i2c_bus}) not found!!!"
        fi

        _echo "[J2 ROV Stamp      ]: ${j2_rov_stamp}"
        _echo "[J2 ROV Stamp Array]: ROV Config  : [0x73 , 0x73 , 0x67 , 0x6B , 0x6F , 0x77 , 0x7B , 0x7F ]"
        _echo "                      VDDC Voltage: [0.82V, 0.82V, 0.76V, 0.78V, 0.80V, 0.84V, 0.86V, 0.88V]"
        _echo "[J2 ROV Config     ]: ${rov_controller_config}"
        _echo "[J2 ROV Config Volt]: ${rov_controller_config_volt}V"
        _echo "[J2 ROV Output     ]: ${rov_controller_output}"
        _echo "[J2 ROV Output Volt]: ${rov_controller_output_volt}V"

    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _show_rov {
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

function _show_nif_port_status_sysfs {
    _banner "Show NIF Port Status / EEPROM"
    echo "    Show NIF Port Status / EEPROM, please wait..."

    bus_id=""
    port_status_cpld_addr_array=""
    port_status_cpld_index_array=""
    port_led_cpld_addr_array=""
    port_led_cpld_index_array=""

    if [ "${MODEL_NAME}" == "NCF" ]; then
        _echo "[No NIF Port on NCF]"

    elif [ "${MODEL_NAME}" == "NCP1-1" ]; then
        bus_id="1"

        nif_port_eeprom_bus_id_array=( "25" "26" "27" "28" "29" \
                                       "30" "31" "32" "33" "34" \
                                       "35" "36" "37" "38" "39" \
                                       "40" "41" "42" "43" "44" \
                                       "46" "45" "48" "47" "50" \
                                       "49" "52" "51" "54" "53" \
                                       "56" "55" "58" "57" "60" \
                                       "59" "62" "61" "64" "63" )

        port_status_cpld_addr_array=("0030" "0030" "0030" "0030" "0030" \
                                     "0030" "0030" "0030" "0030" "0030" \
                                     "0030" "0030" "0039" "0039" "0039" \
                                     "0039" "0039" "0039" "0039" "0039" \
                                     "0039" "0039" "0039" "0039" "003a" \
                                     "0039" "003a" "003a" "003a" "003a" \
                                     "003a" "003a" "003a" "003a" "003a" \
                                     "003a" "003a" "003a" "003b" "003b" )

        port_status_cpld_index_array=( "0"  "1"  "2"  "3"  "4" \
                                       "5"  "6"  "7"  "8"  "9" \
                                      "10" "11"  "0"  "1"  "2" \
                                       "3"  "4"  "5"  "6"  "7" \
                                       "9"  "8" "11" "10"  "0" \
                                      "12"  "2"  "1"  "4"  "3" \
                                       "6"  "5"  "8"  "7" "10" \
                                       "9" "12" "11"  "1"  "0" )

        for (( i=0; i<${#port_status_cpld_addr_array[@]}; i++ ))
        do
            # Module NIF Port Interrupt Event (0: interrupt exist, 1: non-interrupt) ,Absent Event (0: Present, 1:Absence)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfp_port_status_${port_status_cpld_index_array[${i}]}"
            cpld_qsfp_port_status_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfp_port_status_${port_status_cpld_index_array[${i}]} ${LOG_REDIRECT}")
            port_module_interrupt_l=$(((cpld_qsfp_port_status_reg & 2#00000001) >> 0))
            port_module_absent=$(((cpld_qsfp_port_status_reg & 2#00000010) >> 1))

            # Module NIF Port Get Low Power Mode Status (0: Normal Power Mode, 1:Low Power Mode)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfp_port_config_${port_status_cpld_index_array[${i}]}"
            cpld_qsfp_port_config_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfp_port_config_${port_status_cpld_index_array[${i}]} ${LOG_REDIRECT}")
            port_lp_mode=$(((cpld_qsfp_port_config_reg & 2#00000100) >> 2))

            _echo "[Port${i} Status Reg Raw]: ${cpld_qsfp_port_status_reg}"
            _echo "[Port${i} Module INT (L)]: ${port_module_interrupt_l}"
            _echo "[Port${i} Module Absent ]: ${port_module_absent}"
            _echo "[Port${i} Config Reg Raw]: ${cpld_qsfp_port_config_reg}"
            _echo "[Port${i} Low Power Mode]: ${port_lp_mode}"

            # Module NIF Port Dump EEPROM

            # 0:  eeprom lower page 0
            # 1:  eeprom upper page 0
            # 17: eeprom upper page 16 (10h)
            # 33: eeprom upper page 32 (20h)

            eeprom_page_array=(0 1 2 3 4 5 \
                               17 18 19 \
                               33 34 35 36 37 38 39 40 \
                               41 42 43 44 45 46 47 48 )
            eeprom_repeat_array=(2 2 2 1 1 1 \
                                 1 1 1 \
                                 1 1 1 1 1 1 1 1 \
                                 1 1 1 1 1 1 1 1 )

            eeprom_path="/sys/bus/i2c/devices/${nif_port_eeprom_bus_id_array[${i}]}-0050/eeprom"
            _check_filepath ${eeprom_path}

            for (( page_i=0; page_i<${#eeprom_page_array[@]}; page_i++ ))
            do
                for (( repeate_i=0; repeate_i<${eeprom_repeat_array[page_i]}; repeate_i++ ))
                do
                    if [ "${port_module_absent}" == "0" ]; then
                        eeprom_content=$(eval  "dd if=${eeprom_path} bs=128 count=1 skip=${eeprom_page_array[${page_i}]}  status=none ${LOG_REDIRECT} | hexdump -C")

                        if [ -z "$eeprom_content" ] && [ "${eeprom_repeat_array[page_i]}" == "0" ]; then
                            eeprom_content="ERROR!!! The result is empty. It should read failed ${eeprom_path}!!"
                        fi
                    else
                        eeprom_content="N/A"
                    fi
                    _echo "[Port${i} EEPROM $(_eeprom_page_desc ${eeprom_page_array[page_i]}) $(_eeprom_page_repeat_desc ${repeate_i} ${eeprom_repeat_array[page_i]})]:"
                    _echo "${eeprom_content}"
                done
            done

        done

    elif [ "${MODEL_NAME}" == "NCP2-1" ]; then
        bus_id="1"
        nif_port_eeprom_bus_id_base=25
        port_status_cpld_addr_array=("0030" "0030" "0030" "0030" "0030" \
                                     "0030" "0030" "0030" "0030" "0030" )
        port_status_cpld_index_array=( "0"  "1"  "2"  "3"  "4" \
                                       "5"  "6"  "7"  "8"  "9" )

        for (( i=0; i<${#port_status_cpld_addr_array[@]}; i++ ))
        do
            # Module NIF Port Interrupt Event (0: interrupt exist, 1: non-interrupt) ,Absent Event (0: Present, 1:Absence)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_nif_port_status_${port_status_cpld_index_array[${i}]}"
            cpld_qsfpdd_nif_port_status_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_nif_port_status_${port_status_cpld_index_array[${i}]} ${LOG_REDIRECT}")


            port_module_interrupt_l=$(((cpld_qsfpdd_nif_port_status_reg & 2#00000001) >> 0))
            port_module_absent=$(((cpld_qsfpdd_nif_port_status_reg & 2#00000010) >> 1))

            # Module NIF Port Get Low Power Mode Status (0: Normal Power Mode, 1:Low Power Mode)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_nif_port_config_${port_status_cpld_index_array[${i}]}"
            cpld_qsfpdd_nif_port_config_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_nif_port_config_${port_status_cpld_index_array[${i}]} ${LOG_REDIRECT}")

            port_lp_mode=$(((cpld_qsfpdd_nif_port_config_reg & 2#00000100) >> 2))

            _echo "[Port${i} Status Reg Raw]: ${cpld_qsfpdd_nif_port_status_reg}"
            _echo "[Port${i} Module INT (L)]: ${port_module_interrupt_l}"
            _echo "[Port${i} Module Absent ]: ${port_module_absent}"
            _echo "[Port${i} Config Reg Raw]: ${cpld_qsfpdd_nif_port_config_reg}"
            _echo "[Port${i} Low Power Mode]: ${port_lp_mode}"

            # Module NIF Port Dump EEPROM

            # 0:  eeprom lower page 0
            # 1:  eeprom upper page 0
            # 17: eeprom upper page 16 (10h)
            # 33: eeprom upper page 32 (20h)

            eeprom_page_array=(0 1 2 3 4 5 \
                               17 18 19 \
                               33 34 35 36 37 38 39 40 \
                               41 42 43 44 45 46 47 48 )
            eeprom_repeat_array=(2 2 2 1 1 1 \
                                 1 1 1 \
                                 1 1 1 1 1 1 1 1 \
                                 1 1 1 1 1 1 1 1 )

            eeprom_path="/sys/bus/i2c/devices/$(($nif_port_eeprom_bus_id_base+$i))-0050/eeprom"
            _check_filepath ${eeprom_path}

            for (( page_i=0; page_i<${#eeprom_page_array[@]}; page_i++ ))
            do
                for (( repeate_i=0; repeate_i<${eeprom_repeat_array[page_i]}; repeate_i++ ))
                do
                    if [ "${port_module_absent}" == "0" ]; then
                        eeprom_content=$(eval  "dd if=${eeprom_path} bs=128 count=1 skip=${eeprom_page_array[${page_i}]}  status=none ${LOG_REDIRECT} | hexdump -C")

                        if [ -z "$eeprom_content" ] && [ "${eeprom_repeat_array[page_i]}" == "0" ]; then
                            eeprom_content="ERROR!!! The result is empty. It should read failed ${eeprom_path}!!"
                        fi
                    else
                        eeprom_content="N/A"
                    fi
                    _echo "[Port${i} EEPROM $(_eeprom_page_desc ${eeprom_page_array[page_i]}) $(_eeprom_page_repeat_desc ${repeate_i} ${eeprom_repeat_array[page_i]})]:"
                    _echo "${eeprom_content}"
                done
            done

        done
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _show_nif_port_status {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_nif_port_status_sysfs
    fi
}

function _show_fab_port_status_sysfs {
    _banner "Show FAB Port Status / EEPROM"
    echo "    Show FAB Port Status / EEPROM, please wait..."

    bus_id=""
    port_status_cpld_addr_array=""
    port_status_cpld_index_array=""
    port_led_cpld_addr_array=""
    port_led_cpld_index_array=""

    if [ "${MODEL_NAME}" == "NCF" ]; then
        bus_id="2"
        fab_port_eeprom_bus_id_base=21
        port_status_cpld_addr_array=("0032" "0032" "0032" "0032" "0032" "0032" \
                                     "0032" "0032" "0032" "0032" "0032" "0032" \
                                     "0033" "0033" "0033" "0033" "0033" "0033" \
                                     "0033" "0033" "0033" "0033" "0033" "0033" \
                                     "0030" "0030" "0030" "0030" "0030" "0030" \
                                     "0030" "0030" "0030" "0030" "0030" "0030" \
                                     "0031" "0031" "0031" "0031" "0031" "0031" \
                                     "0031" "0031" "0031" "0031" "0031" "0031" )

        port_status_cpld_index_array=( "0"  "1"  "2"  "3"  "4"  "5" \
                                       "6"  "7"  "8"  "9" "10" "11" \
                                       "0"  "1"  "2"  "3"  "4"  "5" \
                                       "6"  "7"  "8"  "9" "10" "11" \
                                       "0"  "1"  "2"  "3"  "4"  "5" \
                                       "6"  "7"  "8"  "9" "10" "11" \
                                       "0"  "1"  "2"  "3"  "4"  "5" \
                                       "6"  "7"  "8"  "9" "10" "11")

        port_led_cpld_addr_array=("0033" "0033" "0033" "0033" "0033" "0033" \
                                  "0033" "0033" "0033" "0033" "0033" "0033" \
                                  "0033" "0033" "0033" "0033" "0033" "0033" \
                                  "0033" "0033" "0033" "0033" "0033" "0033" \
                                  "0031" "0031" "0031" "0031" "0031" "0031" \
                                  "0031" "0031" "0031" "0031" "0031" "0031" \
                                  "0031" "0031" "0031" "0031" "0031" "0031" \
                                  "0031" "0031" "0031" "0031" "0031" "0031" )

        port_led_cpld_index_array=( "0"  "1"  "2"  "3"  "4"  "5" \
                                    "6"  "7"  "8"  "9" "10" "11" \
                                   "12" "13" "14" "15" "16" "17" \
                                   "18" "19" "20" "21" "22" "23" \
                                    "0"  "1"  "2"  "3"  "4"  "5" \
                                    "6"  "7"  "8"  "9" "10" "11" \
                                   "12" "13" "14" "15" "16" "17" \
                                   "18" "19" "20" "21" "22" "23")

        for (( i=0; i<${#port_status_cpld_addr_array[@]}; i++ ))
        do
            # QSFPDD FAB Port Interrupt Event (0: interrupt exist, 1: non-interrupt) ,Absent Event (0: Present, 1:Absence)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_port_status_${port_status_cpld_index_array[${i}]}"
            cpld_qsfpdd_port_status_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_port_status_${port_status_cpld_index_array[${i}]} ${LOG_REDIRECT}")
            port_module_interrupt_l=$(((cpld_qsfpdd_port_status_reg & 2#00000001) >> 0))
            port_module_absent=$(((cpld_qsfpdd_port_status_reg & 2#00000010) >> 1))

            # QSFPDD FAB Port Get Low Power Mode Status (0: Normal Power Mode, 1:Low Power Mode)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_port_config_${port_status_cpld_index_array[${i}]}"
            cpld_qsfpdd_port_config_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_port_config_${port_status_cpld_index_array[${i}]} ${LOG_REDIRECT}")
            port_lp_mode=$(((cpld_qsfpdd_port_config_reg & 2#00000100) >> 2))

            # QSFPDD FAB Port LED
            cpld_qsfpdd_led_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_led_cpld_addr_array[${i}]}/cpld_qsfpdd_led_${port_led_cpld_index_array[${i}]} ${LOG_REDIRECT}")
            if [ $(($i % 2)) -eq 0 ]; then
                cpld_qsfpdd_led_reg=$((cpld_qsfpdd_led_reg & 2#00001111))           # Clear higher 4 bits on reg value for even ports
            else
                cpld_qsfpdd_led_reg=$(((cpld_qsfpdd_led_reg & 2#11110000) >> 4))    # Clear and shift lower 4 bits on reg value for odd ports
            fi
            led_red=$((cpld_qsfpdd_led_reg & 2#00000001))           # (0: red off, 1:red on)
            led_green=$(((cpld_qsfpdd_led_reg & 2#00000010) >> 1))  # (0: green off, 1:green on)
            led_blue=$(((cpld_qsfpdd_led_reg & 2#00000100) >> 2))   # (0: blue off, 1:blue on)
            led_blink=$(((cpld_qsfpdd_led_reg & 2#00001000) >> 3))  # (0: blinking, 1:solid)


            _echo "[Port${i} Status Reg Raw]: ${cpld_qsfpdd_port_status_reg}"
            _echo "[Port${i} Module INT (L)]: ${port_module_interrupt_l}"
            _echo "[Port${i} Module Absent ]: ${port_module_absent}"
            _echo "[Port${i} Config Reg Raw]: ${cpld_qsfpdd_port_config_reg}"
            _echo "[Port${i} Low Power Mode]: ${port_lp_mode}"
            _echo "[Port${i} LED Reg Raw   ]: ${cpld_qsfpdd_led_reg}"
            _echo "[Port${i} LED Status    ]: Red: ${led_red}, Green: ${led_green}, "\
                                            "Blue: ${led_blue}, Blinking: ${led_blink}"

            # Module FAB Port Dump EEPROM

            # 0:  eeprom lower page 0
            # 1:  eeprom upper page 0
            # 17: eeprom upper page 16 (10h)
            # 33: eeprom upper page 32 (20h)

            eeprom_page_array=(0 1 2 3 4 5 \
                               17 18 19 \
                               33 34 35 36 37 38 39 40 \
                               41 42 43 44 45 46 47 48 )
            eeprom_repeat_array=(2 2 2 1 1 1 \
                                 1 1 1 \
                                 1 1 1 1 1 1 1 1 \
                                 1 1 1 1 1 1 1 1 )

            eeprom_path="/sys/bus/i2c/devices/$(($fab_port_eeprom_bus_id_base+$i))-0050/eeprom"
            _check_filepath ${eeprom_path}

            for (( page_i=0; page_i<${#eeprom_page_array[@]}; page_i++ ))
            do
                for (( repeate_i=0; repeate_i<${eeprom_repeat_array[page_i]}; repeate_i++ ))
                do
                    if [ "${port_module_absent}" == "0" ]; then
                        eeprom_content=$(eval  "dd if=${eeprom_path} bs=128 count=1 skip=${eeprom_page_array[${page_i}]}  status=none ${LOG_REDIRECT} | hexdump -C")

                        if [ -z "$eeprom_content" ] && [ "${eeprom_repeat_array[page_i]}" == "0" ]; then
                            eeprom_content="ERROR!!! The result is empty. It should read failed ${eeprom_path}!!"
                        fi
                    else
                        eeprom_content="N/A"
                    fi
                    _echo "[Port${i} EEPROM $(_eeprom_page_desc ${eeprom_page_array[page_i]}) $(_eeprom_page_repeat_desc ${repeate_i} ${eeprom_repeat_array[page_i]})]:"
                    _echo "${eeprom_content}"
                done
            done

        done

    elif [ "${MODEL_NAME}" == "NCP1-1" ]; then
        bus_id="1"
        fab_port_eeprom_bus_id_base=65
        port_status_cpld_addr_array=("003b" "003b" "003b" "003c" "003c" "003c" \
                                     "003c" "003c" "003c" "003c" "003c" "003c" \
                                     "003c" )

        port_status_cpld_index_array=( "0"  "1"  "2"  "0"  "1"  "2" \
                                       "3"  "4"  "5"  "6"  "7"  "8" \
                                       "9" )

        port_led_cpld_addr_array=("003c" "003c" "003c" "003c" "003c" "003c" \
                                  "003c" "003c" "003c" "003c" "003c" "003c" \
                                  "003c" )

        port_led_cpld_index_array=( "0"  "1"  "2"  "3"  "4"  "5" \
                                    "6"  "7"  "8"  "9" "10" "11" \
                                   "12" )

        for (( i=0; i<${#port_status_cpld_addr_array[@]}; i++ ))
        do
            # QSFPDD FAB Port Interrupt Event (0: interrupt exist, 1: non-interrupt) ,Absent Event (0: Present, 1:Absence)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_port_status_${port_status_cpld_index_array[${i}]}"
            cpld_qsfpdd_port_status_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_port_status_${port_status_cpld_index_array[${i}]} ${LOG_REDIRECT}")
            port_module_interrupt_l=$(((cpld_qsfpdd_port_status_reg & 2#00000001) >> 0))
            port_module_absent=$(((cpld_qsfpdd_port_status_reg & 2#00000010) >> 1))

            # QSFPDD FAB Port Get Low Power Mode Status (0: Normal Power Mode, 1:Low Power Mode)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_port_config_${port_status_cpld_index_array[${i}]}"
            cpld_qsfpdd_port_config_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_port_config_${port_status_cpld_index_array[${i}]} ${LOG_REDIRECT}")
            port_lp_mode=$(((cpld_qsfpdd_port_config_reg & 2#00000100) >> 2))

            # QSFPDD FAB Port LED
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_led_cpld_addr_array[${i}]}/cpld_qsfpdd_led_${port_led_cpld_index_array[${i}]}"
            cpld_qsfpdd_led_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_led_cpld_addr_array[${i}]}/cpld_qsfpdd_led_${port_led_cpld_index_array[${i}]} ${LOG_REDIRECT}")
            if [ $(($i % 2)) -eq 0 ]; then
                cpld_qsfpdd_led_reg=$((cpld_qsfpdd_led_reg & 2#00001111))           # Clear higher 4 bits on reg value for even ports
            else
                cpld_qsfpdd_led_reg=$(((cpld_qsfpdd_led_reg & 2#11110000) >> 4))    # Clear and shift lower 4 bits on reg value for odd ports
            fi
            led_red=$((cpld_qsfpdd_led_reg & 2#00000001))           # (0: red off, 1:red on)
            led_green=$(((cpld_qsfpdd_led_reg & 2#00000010) >> 1))  # (0: green off, 1:green on)
            led_blue=$(((cpld_qsfpdd_led_reg & 2#00000100) >> 2))   # (0: blue off, 1:blue on)
            led_blink=$(((cpld_qsfpdd_led_reg & 2#00001000) >> 3))  # (0: blinking, 1:solid)


            _echo "[Port${i} Statuc Reg Raw]: ${cpld_qsfpdd_port_status_reg}"
            _echo "[Port${i} Module INT (L)]: ${port_module_interrupt_l}"
            _echo "[Port${i} Module Absent ]: ${port_module_absent}"
            _echo "[Port${i} Config Reg Raw]: ${cpld_qsfpdd_port_config_reg}"
            _echo "[Port${i} Low Power Mode]: ${port_lp_mode}"
            _echo "[Port${i} LED Reg Raw   ]: ${cpld_qsfpdd_led_reg}"
            _echo "[Port${i} LED Status    ]: Red: ${led_red}, Green: ${led_green}, "\
                                            "Blue: ${led_blue}, Blinking: ${led_blink}"

            # Module FAB Port Dump EEPROM

            # 0:  eeprom lower page 0
            # 1:  eeprom upper page 0
            # 17: eeprom upper page 16 (10h)
            # 33: eeprom upper page 32 (20h)

            eeprom_page_array=(0 1 2 3 4 5 \
                               17 18 19 \
                               33 34 35 36 37 38 39 40 \
                               41 42 43 44 45 46 47 48 )
            eeprom_repeat_array=(2 2 2 1 1 1 \
                                 1 1 1 \
                                 1 1 1 1 1 1 1 1 \
                                 1 1 1 1 1 1 1 1 )

            eeprom_path="/sys/bus/i2c/devices/$(($fab_port_eeprom_bus_id_base+$i))-0050/eeprom"
            _check_filepath ${eeprom_path}

            for (( page_i=0; page_i<${#eeprom_page_array[@]}; page_i++ ))
            do
                for (( repeate_i=0; repeate_i<${eeprom_repeat_array[page_i]}; repeate_i++ ))
                do
                    if [ "${port_module_absent}" == "0" ]; then
                        eeprom_content=$(eval  "dd if=${eeprom_path} bs=128 count=1 skip=${eeprom_page_array[${page_i}]}  status=none ${LOG_REDIRECT} | hexdump -C")

                        if [ -z "$eeprom_content" ] && [ "${eeprom_repeat_array[page_i]}" == "0" ]; then
                            eeprom_content="ERROR!!! The result is empty. It should read failed ${eeprom_path}!!"
                        fi
                    else
                        eeprom_content="N/A"
                    fi
                    _echo "[Port${i} EEPROM $(_eeprom_page_desc ${eeprom_page_array[page_i]}) $(_eeprom_page_repeat_desc ${repeate_i} ${eeprom_repeat_array[page_i]})]:"
                    _echo "${eeprom_content}"
                done
            done
        done

    elif [ "${MODEL_NAME}" == "NCP2-1" ]; then
        bus_id="1"
        fab_port_eeprom_bus_id_base=41
        port_status_cpld_addr_array=("0031" "0031" "0031" "0032" "0032" "0032" \
                                     "0032" "0032" "0032" "0032" "0032" "0032" \
                                     "0032" )

        port_status_cpld_index_array=( "0"  "1"  "2"  "0"  "1"  "2" \
                                       "3"  "4"  "5"  "6"  "7"  "8" \
                                       "9" )

        port_led_cpld_addr_array=("0032" "0032" "0032" "0032" "0032" "0032" \
                                  "0032" "0032" "0032" "0032" "0032" "0032" \
                                  "0032" )

        port_led_cpld_index_array=( "0"  "1"  "2"  "3"  "4"  "5" \
                                    "6"  "7"  "8"  "9" "10" "11" \
                                   "12" )

        for (( i=0; i<${#port_status_cpld_addr_array[@]}; i++ ))
        do
            # QSFPDD FAB Port Interrupt Event (0: interrupt exist, 1: non-interrupt) ,Absent Event (0: Present, 1:Absence)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_fab_port_status_${port_status_cpld_index_array[${i}]}"
            cpld_qsfpdd_fab_port_status_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_fab_port_status_${port_status_cpld_index_array[${i}]} ${LOG_REDIRECT}")

            port_module_interrupt_l=$(((cpld_qsfpdd_fab_port_status_reg & 2#00000001) >> 0))
            port_module_absent=$(((cpld_qsfpdd_fab_port_status_reg & 2#00000010) >> 1))

            # QSFPDD FAB Port Get Low Power Mode Status (0: Normal Power Mode, 1:Low Power Mode)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_fab_port_config_${port_status_cpld_index_array[${i}]}"
            cpld_qsfpdd_fab_port_config_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_fab_port_config_${port_status_cpld_index_array[${i}]} ${LOG_REDIRECT}")
            port_lp_mode=$(((cpld_qsfpdd_fab_port_config_reg & 2#00000100) >> 2))

            # QSFPDD FAB Port LED
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_led_cpld_addr_array[${i}]}/cpld_qsfpdd_fab_led_${port_led_cpld_index_array[${i}]}"
            cpld_qsfpdd_led_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_led_cpld_addr_array[${i}]}/cpld_qsfpdd_fab_led_${port_led_cpld_index_array[${i}]} ${LOG_REDIRECT}")
            if [ $(($i % 2)) -eq 0 ]; then
                reg=$((cpld_qsfpdd_led_reg & 2#00001111))           # Clear higher 4 bits on reg value for even ports
            else
                reg=$(((cpld_qsfpdd_led_reg & 2#11110000) >> 4))    # Clear and shift lower 4 bits on reg value for odd ports
            fi
            led_red=$((cpld_qsfpdd_led_reg & 2#00000001))           # (0: red off, 1:red on)
            led_green=$(((cpld_qsfpdd_led_reg & 2#00000010) >> 1))  # (0: green off, 1:green on)
            led_blue=$(((cpld_qsfpdd_led_reg & 2#00000100) >> 2))   # (0: blue off, 1:blue on)
            led_blink=$(((cpld_qsfpdd_led_reg & 2#00001000) >> 3))  # (0: blinking, 1:solid)


            _echo "[Port${i} Status Reg Raw]: ${cpld_qsfpdd_fab_port_status_reg}"
            _echo "[Port${i} Module INT (L)]: ${port_module_interrupt_l}"
            _echo "[Port${i} Module Absent ]: ${port_module_absent}"
            _echo "[Port${i} Config Reg Raw]: ${cpld_qsfpdd_fab_port_config_reg}"
            _echo "[Port${i} Low Power Mode]: ${port_lp_mode}"
            _echo "[Port${i} LED Reg Raw   ]: ${cpld_qsfpdd_led_reg}"
            _echo "[Port${i} LED Status    ]: Red: ${led_red}, Green: ${led_green}, "\
                                             "Blue: ${led_blue}, Blinking: ${led_blink}"

            # Module FAB Port Dump EEPROM

            # 0:  eeprom lower page 0
            # 1:  eeprom upper page 0
            # 17: eeprom upper page 16 (10h)
            # 33: eeprom upper page 32 (20h)

            eeprom_page_array=(0 1 2 3 4 5 \
                               17 18 19 \
                               33 34 35 36 37 38 39 40 \
                               41 42 43 44 45 46 47 48 )
            eeprom_repeat_array=(2 2 2 1 1 1 \
                                 1 1 1 \
                                 1 1 1 1 1 1 1 1 \
                                 1 1 1 1 1 1 1 1 )

            eeprom_path="/sys/bus/i2c/devices/$(($fab_port_eeprom_bus_id_base+$i))-0050/eeprom"
            _check_filepath ${eeprom_path}

            for (( page_i=0; page_i<${#eeprom_page_array[@]}; page_i++ ))
            do
                for (( repeate_i=0; repeate_i<${eeprom_repeat_array[page_i]}; repeate_i++ ))
                do
                    if [ "${port_module_absent}" == "0" ]; then
                        eeprom_content=$(eval  "dd if=${eeprom_path} bs=128 count=1 skip=${eeprom_page_array[${page_i}]}  status=none ${LOG_REDIRECT} | hexdump -C")

                        if [ -z "$eeprom_content" ] && [ "${eeprom_repeat_array[page_i]}" == "0" ]; then
                            eeprom_content="ERROR!!! The result is empty. It should read failed ${eeprom_path}!!"
                        fi
                    else
                        eeprom_content="N/A"
                    fi
                    _echo "[Port${i} EEPROM $(_eeprom_page_desc ${eeprom_page_array[page_i]}) $(_eeprom_page_repeat_desc ${repeate_i} ${eeprom_repeat_array[page_i]})]:"
                    _echo "${eeprom_content}"
                done
            done

        done
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _show_fab_port_status {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_fab_port_status_sysfs
    fi
}

function _show_sfp_status_sysfs {
    _banner "Show SFP Status"

    bus_id=""

    if [ "${MODEL_NAME}" == "NCF" ]; then
        bus_id="2"
    elif [ "${MODEL_NAME}" == "NCP1-1" ]; then
        bus_id="1"
    elif [ "${MODEL_NAME}" == "NCP2-1" ]; then
        bus_id="1"
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    _check_filepath "/sys/bus/i2c/devices/${bus_id}-0030/cpld_sfp_port_status"
    cpld_sfp_port_status_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0030/cpld_sfp_port_status ${LOG_REDIRECT}")

    port0_tx_fault=$(((cpld_sfp_port_status_reg & 2#00000010) >> 1))
    port1_tx_fault=$(((cpld_sfp_port_status_reg & 2#00100000) >> 5))

    port_0_rx_los=$(((cpld_sfp_port_status_reg & 2#00000100) >> 2))
    port_1_rx_los=$(((cpld_sfp_port_status_reg & 2#01000000) >> 6))

    _check_filepath "/sys/bus/i2c/devices/${bus_id}-0030/cpld_sfp_port_config"
    cpld_sfp_port_config_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0030/cpld_sfp_port_config ${LOG_REDIRECT}")
    port0_tx_disable=$(((cpld_sfp_port_config_reg & 2#00000001) >> 0))
    port1_tx_disable=$(((cpld_sfp_port_config_reg & 2#00010000) >> 4))


    _echo "[Port Status Reg Raw]: ${cpld_sfp_port_status_reg}"
    _echo "[Port0 Tx Fault     ]: ${port0_tx_fault}"
    _echo "[Port1 Tx Fault     ]: ${port1_tx_fault}"
    _echo "[Port0 Rx LOS       ]: ${port_0_rx_los}"
    _echo "[Port1 Rx LOS       ]: ${port_1_rx_los}"
    _echo "[Port Config Reg Raw]: ${cpld_sfp_port_config_reg}"
    _echo "[Port0 Tx Disable   ]: ${port0_tx_disable}"
    _echo "[Port1 Tx Disable   ]: ${port1_tx_disable}"
}

function _show_sfp_status {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_sfp_status_sysfs
    fi
}

function _show_cpu_temperature_sysfs {
    _banner "show CPU Temperature"

    for ((i=1;i<=9;i++))
    do
        if [ -f "/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${i}_input" ]; then
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${i}_input"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${i}_max"
            _check_filepath "/sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${i}_crit"
            temp_input=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${i}_input ${LOG_REDIRECT}")
            temp_max=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${i}_max ${LOG_REDIRECT}")
            temp_crit=$(eval "cat /sys/devices/platform/coretemp.0/hwmon/hwmon0/temp${i}_crit ${LOG_REDIRECT}")
        elif [ -f "/sys/devices/platform/coretemp.0/temp${i}_input" ]; then
            _check_filepath "/sys/devices/platform/coretemp.0/temp${i}_input"
            _check_filepath "/sys/devices/platform/coretemp.0/temp${i}_max"
            _check_filepath "/sys/devices/platform/coretemp.0/temp${i}_crit"
            temp_input=$(eval "cat /sys/devices/platform/coretemp.0/temp${i}_input ${LOG_REDIRECT}")
            temp_max=$(eval "cat /sys/devices/platform/coretemp.0/temp${i}_max ${LOG_REDIRECT}")
            temp_crit=$(eval "cat /sys/devices/platform/coretemp.0/temp${i}_crit ${LOG_REDIRECT}")
        else
            _echo "sysfs of CPU core temperature not found!!!"
        fi

        _echo "[CPU Core Temp${i} Input   ]: ${temp_input}"
        _echo "[CPU Core Temp${i} Max     ]: ${temp_max}"
        _echo "[CPU Core Temp${i} Crit    ]: ${temp_crit}"
        _echo ""
    done

    if [ -f "/sys/bus/i2c/devices/0-004f/hwmon/hwmon1/temp1_input" ]; then
        _check_filepath "/sys/bus/i2c/devices/0-004f/hwmon/hwmon1/temp1_input"
        _check_filepath "/sys/bus/i2c/devices/0-004f/hwmon/hwmon1/temp1_max"
        _check_filepath "/sys/bus/i2c/devices/0-004f/hwmon/hwmon1/temp1_max_hyst"
        temp_input=$(eval "cat /sys/bus/i2c/devices/0-004f/hwmon/hwmon1/temp1_input ${LOG_REDIRECT}")
        temp_max=$(eval "cat /sys/bus/i2c/devices/0-004f/hwmon/hwmon1/temp1_max ${LOG_REDIRECT}")
        temp_max_hyst=$(eval "cat /sys/bus/i2c/devices/0-004f/hwmon/hwmon1/temp1_max_hyst ${LOG_REDIRECT}")
    elif [ -f "/sys/bus/i2c/devices/0-004f/hwmon/hwmon1/device/temp1_input" ]; then
        _check_filepath "/sys/bus/i2c/devices/0-004f/hwmon/hwmon1/device/temp1_input"
        _check_filepath "/sys/bus/i2c/devices/0-004f/hwmon/hwmon1/device/temp1_max"
        _check_filepath "/sys/bus/i2c/devices/0-004f/hwmon/hwmon1/device/temp1_max_hyst"
        temp_input=$(eval "cat /sys/bus/i2c/devices/0-004f/hwmon/hwmon1/device/temp1_input ${LOG_REDIRECT}")
        temp_max=$(eval "cat /sys/bus/i2c/devices/0-004f/hwmon/hwmon1/device/temp1_max ${LOG_REDIRECT}")
        temp_max_hyst=$(eval "cat /sys/bus/i2c/devices/0-004f/hwmon/hwmon1/device/temp1_max_hyst ${LOG_REDIRECT}")
    else
        _echo "sysfs of CPU board temperature not found!!!"
    fi

    _echo "[CPU Board Temp Input   ]: ${temp_input}"
    _echo "[CPU Board Temp Max     ]: ${temp_max}"
    _echo "[CPU Board Temp Max Hyst]: ${temp_max_hyst}"
}

function _show_cpu_temperature {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_cpu_temperature_sysfs
    fi
}

function _show_cpld_interrupt_sysfs {
    _banner "Show CPLD Interrupt"

    if [ "${MODEL_NAME}" == "NCF" ]; then
        bus_id="2"
        cpld_addr_array=("0030" "0031" "0032" "0033")

        # CPLD to CPU Interrupt
        _check_filepath "/sys/class/gpio/gpio$((GPIO_MAX-3))/value"
        _check_filepath "/sys/class/gpio/gpio$((GPIO_MAX-8))/value"
        cpld12_to_cpu_interrupt_l=$(eval "cat /sys/class/gpio/gpio$((GPIO_MAX-3))/value ${LOG_REDIRECT}")
        cpld34_to_cpu_interrupt_l=$(eval "cat /sys/class/gpio/gpio$((GPIO_MAX-8))/value ${LOG_REDIRECT}")
        _echo "[CPLD12 to CPU INT (L)]: ${cpld12_to_cpu_interrupt_l}"
        _echo "[CPLD34 to CPU INT (L)]: ${cpld12_to_cpu_interrupt_l}"
        _echo ""

        # MB CPLD Interrupt
        for (( i=0; i<${#cpld_addr_array[@]}; i++ ))
        do
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${cpld_addr_array[${i}]}/cpld_interrupt"
            cpld_interrupt_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${cpld_addr_array[${i}]}/cpld_interrupt ${LOG_REDIRECT}")
            port_interrupt_l=$(((cpld_interrupt_reg & 2#00000001) >> 0))
            reserved1_interrupt_l=$(((cpld_interrupt_reg & 2#00000010) >> 1))
            usb_interrupt_l=$(((cpld_interrupt_reg & 2#00000100) >> 2))
            port_presence_l=$(((cpld_interrupt_reg & 2#00001000) >> 3))
            psu0_interrupt_l=$(((cpld_interrupt_reg & 2#00010000) >> 4))
            psu1_interrupt_l=$(((cpld_interrupt_reg & 2#00100000) >> 5))
            reserved2_interrupt_l=$(((cpld_interrupt_reg & 2#01000000) >> 6))
            cs4227_interrupt_l=$(((cpld_interrupt_reg & 2#10000000) >> 7))

            if [ ${i} -eq 0 ]; then
                _echo "[MB CPLD$((${i}+1)) INT Reg Raw]: ${cpld_interrupt_reg}"
                _echo "[Port Interrupt(L)   ]: ${port_interrupt_l}"
                _echo "[Reserved            ]: ${reserved1_interrupt_l}"
                _echo "[USB Interrupt(L)    ]: ${usb_interrupt_l}"
                _echo "[Port Presence(L)    ]: ${port_presence_l}"
                _echo "[PSU0 Interrupt(L)   ]: ${psu0_interrupt_l}"
                _echo "[PSU1 Interrupt(L)   ]: ${psu1_interrupt_l}"
                _echo "[Reserved            ]: ${reserved1_interrupt_l}"
                _echo "[CS4227 Interrupt(L) ]: ${cs4227_interrupt_l}"
                _echo ""
            else
                _echo "[MB CPLD$((${i}+1)) INT Reg Raw]: ${cpld_interrupt_reg}"
                _echo "[Port Interrupt(L)   ]: ${port_interrupt_l}"
                _echo "[Reserved            ]: ${reserved1_interrupt_l}"
                _echo "[Reserved            ]: ${usb_interrupt_l}"
                _echo "[Port Presence(L)    ]: ${port_presence_l}"
                _echo "[Reserved            ]: ${psu0_interrupt_l}"
                _echo "[Reserved            ]: ${psu1_interrupt_l}"
                _echo "[Reserved            ]: ${reserved1_interrupt_l}"
                _echo "[Reserved            ]: ${cs4227_interrupt_l}"
                _echo ""
            fi
        done

    elif [ "${MODEL_NAME}" == "NCP1-1" ]; then
        bus_id="1"
        cpld_addr_array=("0030" "0039" "003a" "003b" "003c")

        # CPLD to CPU Interrupt
        _check_filepath "/sys/class/gpio/gpio$((GPIO_MAX-3))/value"
        _check_filepath "/sys/class/gpio/gpio$((GPIO_MAX-4))/value"
        _check_filepath "/sys/class/gpio/gpio$((GPIO_MAX-5))/value"
        _check_filepath "/sys/class/gpio/gpio$((GPIO_MAX-6))/value"
        cpld12_to_cpu_interrupt_l=$(eval "cat /sys/class/gpio/gpio$((GPIO_MAX-3))/value ${LOG_REDIRECT}")
        cpld3_to_cpu_interrupt_l=$(eval "cat /sys/class/gpio/gpio$((GPIO_MAX-4))/value ${LOG_REDIRECT}")
        cpld4_to_cpu_interrupt_l=$(eval "cat /sys/class/gpio/gpio$((GPIO_MAX-5))/value ${LOG_REDIRECT}")
        cpld5_to_cpu_interrupt_l=$(eval "cat /sys/class/gpio/gpio$((GPIO_MAX-6))/value ${LOG_REDIRECT}")
        _echo "[CPLD12 to CPU INT (L)]: ${cpld12_to_cpu_interrupt_l}"
        _echo "[CPLD3  to CPU INT (L)]: ${cpld3_to_cpu_interrupt_l}"
        _echo "[CPLD4  to CPU INT (L)]: ${cpld4_to_cpu_interrupt_l}"
        _echo "[CPLD5  to CPU INT (L)]: ${cpld5_to_cpu_interrupt_l}"
        _echo ""

        # MB CPLD Interrupt
        for (( i=0; i<${#cpld_addr_array[@]}; i++ ))
        do
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${cpld_addr_array[${i}]}/cpld_interrupt"
            cpld_interrupt_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${cpld_addr_array[${i}]}/cpld_interrupt ${LOG_REDIRECT}")
            port_interrupt_l=$(((cpld_interrupt_reg & 2#00000001) >> 0))
            gearbox_interrupt_l=$(((cpld_interrupt_reg & 2#00000010) >> 1))
            usb_interrupt_l=$(((cpld_interrupt_reg & 2#00000100) >> 2))
            port_presence_l=$(((cpld_interrupt_reg & 2#00001000) >> 3))
            psu0_interrupt_l=$(((cpld_interrupt_reg & 2#00010000) >> 4))
            psu1_interrupt_l=$(((cpld_interrupt_reg & 2#00100000) >> 5))
            pex8724_interrupt_l=$(((cpld_interrupt_reg & 2#01000000) >> 6))
            cs4227_interrupt_l=$(((cpld_interrupt_reg & 2#10000000) >> 7))

            if [ ${i} -eq 0 ]; then
                cpld_interrupt2_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${cpld_addr_array[${i}]}/cpld_interrupt_2 ${LOG_REDIRECT}")
                retimer_interrupt_l=$(((cpld_interrupt2_reg & 2#00000001) >> 0))

                _echo "[MB CPLD$((${i}+1)) INT Reg Raw]: ${cpld_interrupt_reg}"
                _echo "[Port Interrupt(L)   ]: ${port_interrupt_l}"
                _echo "[Gearbox Interrupt(L)]: ${gearbox_interrupt_l}"
                _echo "[USB Interrupt(L)    ]: ${usb_interrupt_l}"
                _echo "[Port Presence(L)    ]: ${port_presence_l}"
                _echo "[PSU0 Interrupt(L)   ]: ${psu0_interrupt_l}"
                _echo "[PSU1 Interrupt(L)   ]: ${psu1_interrupt_l}"
                _echo "[PEX8724 Interrupt(L)]: ${pex8724_interrupt_l}"
                _echo "[CS4227 Interrupt(L) ]: ${cs4227_interrupt_l}"
                _echo "[MB CPLD INT2 Reg Raw]: ${cpld_interrupt2_reg}"
                _echo "[Retimer Interrupt(L)]: ${retimer_interrupt_l}"
                _echo ""
            else
                _echo "[MB CPLD$((${i}+1)) INT Reg Raw]: ${cpld_interrupt_reg}"
                _echo "[Port Interrupt(L)   ]: ${port_interrupt_l}"
                _echo "[Reserved            ]: ${gearbox_interrupt_l}"
                _echo "[Reserved            ]: ${usb_interrupt_l}"
                _echo "[Port Presence(L)    ]: ${port_presence_l}"
                _echo "[Reserved            ]: ${psu0_interrupt_l}"
                _echo "[Reserved            ]: ${psu1_interrupt_l}"
                _echo "[Reserved            ]: ${pex8724_interrupt_l}"
                _echo "[Reserved            ]: ${cs4227_interrupt_l}"
                _echo ""
            fi
        done

    elif [ "${MODEL_NAME}" == "NCP2-1" ]; then
        bus_id="1"
        cpld_addr_array=("0030" "0031" "0032")

        # CPLD to CPU Interrupt
        cpld12_to_cpu_interrupt_l=$(eval "cat /sys/class/gpio/gpio$((GPIO_MAX-3))/value ${LOG_REDIRECT}")
        cpld3_to_cpu_interrupt_l=$(eval "cat /sys/class/gpio/gpio$((GPIO_MAX-4))/value ${LOG_REDIRECT}")
        _echo "[CPLD12 to CPU INT (L)]: ${cpld12_to_cpu_interrupt_l}"
        _echo "[CPLD3  to CPU INT (L)]: ${cpld3_to_cpu_interrupt_l}"
        _echo ""

        # MB CPLD Interrupt
        for (( i=0; i<${#cpld_addr_array[@]}; i++ ))
        do
            cpld_interrupt_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${cpld_addr_array[${i}]}/cpld_interrupt ${LOG_REDIRECT}")
            port_interrupt_l=$(((cpld_interrupt_reg & 2#00000001) >> 0))
            reserved1_interrupt_l=$(((cpld_interrupt_reg & 2#00000010) >> 1))
            usb_interrupt_l=$(((cpld_interrupt_reg & 2#00000100) >> 2))
            port_presence_l=$(((cpld_interrupt_reg & 2#00001000) >> 3))
            psu0_interrupt_l=$(((cpld_interrupt_reg & 2#00010000) >> 4))
            psu1_interrupt_l=$(((cpld_interrupt_reg & 2#00100000) >> 5))
            pex8724_interrupt_l=$(((cpld_interrupt_reg & 2#01000000) >> 6))
            cs4227_interrupt_l=$(((cpld_interrupt_reg & 2#10000000) >> 7))

            _echo "[MB CPLD$((${i}+1)) Interrupt Register]"
            if [ ${i} -eq 0 ]; then
                _echo "[MB CPLD$((${i}+1)) INT Reg Raw]: ${cpld_interrupt_reg}"
                _echo "[Port Interrupt(L)   ]: ${port_interrupt_l}"
                _echo "[Reserved            ]: ${reserved1_interrupt_l}"
                _echo "[USB Interrupt(L)    ]: ${usb_interrupt_l}"
                _echo "[Port Presence(L)    ]: ${port_presence_l}"
                _echo "[PSU0 Interrupt(L)   ]: ${psu0_interrupt_l}"
                _echo "[PSU1 Interrupt(L)   ]: ${psu1_interrupt_l}"
                _echo "[PEX8724 Interrupt(L)]: ${pex8724_interrupt_l}"
                _echo "[CS4227 Interrupt(L) ]: ${cs4227_interrupt_l}"
                _echo ""
            else
                _echo "[MB CPLD$((${i}+1)) INT Reg Raw]: ${cpld_interrupt_reg}"
                _echo "[Port Interrupt(L)   ]: ${port_interrupt_l}"
                _echo "[Reserved            ]: ${reserved1_interrupt_l}"
                _echo "[Reserved            ]: ${usb_interrupt_l}"
                _echo "[Port Presence(L)    ]: ${port_presence_l}"
                _echo "[Reserved            ]: ${psu0_interrupt_l}"
                _echo "[Reserved            ]: ${psu1_interrupt_l}"
                _echo "[Reserved            ]: ${pex8724_interrupt_l}"
                _echo "[Reserved            ]: ${cs4227_interrupt_l}"
                _echo ""
            fi
        done
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

}

function _show_cpld_interrupt {
    if [ "${BSP_INIT_FLAG}" == "1" ] && [ "${GPIO_MAX_INIT_FLAG}" == "1" ] ; then
        _show_cpld_interrupt_sysfs
    fi
}

function _show_system_led_sysfs {
    _banner "Show System LED"

    if [ "${MODEL_NAME}" == "NCF" ]; then
        _check_filepath "/sys/bus/i2c/devices/2-0030/cpld_system_led_0"
        _check_filepath "/sys/bus/i2c/devices/2-0030/cpld_system_led_1"
        system_led0=$(eval "cat /sys/bus/i2c/devices/2-0030/cpld_system_led_0 ${LOG_REDIRECT}")
        system_led1=$(eval "cat /sys/bus/i2c/devices/2-0030/cpld_system_led_1 ${LOG_REDIRECT}")
        _echo "[System LED0]: ${system_led0}"
        _echo "[System LED1]: ${system_led1}"
    elif [ "${MODEL_NAME}" == "NCP1-1" ] || [ "${MODEL_NAME}" == "NCP2-1" ]; then
        _check_filepath "/sys/bus/i2c/devices/1-0030/cpld_system_led_0"
        _check_filepath "/sys/bus/i2c/devices/1-0030/cpld_system_led_1"
        system_led0=$(eval "cat /sys/bus/i2c/devices/1-0030/cpld_system_led_0 ${LOG_REDIRECT}")
        system_led1=$(eval "cat /sys/bus/i2c/devices/1-0030/cpld_system_led_1 ${LOG_REDIRECT}")
        _echo "[System LED0]: ${system_led0}"
        _echo "[System LED1]: ${system_led1}"
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
    _banner "Show Beacon LED"

    if [ "${MODEL_NAME}" == "NCF" ]; then
        _check_filepath "/sys/bus/i2c/devices/2-0033/cpld_beacon"
        beacon_led=$(eval "cat /sys/bus/i2c/devices/2-0033/cpld_beacon ${LOG_REDIRECT}")
        _echo "[Beacon LED]: ${beacon_led}"
    elif [ "${MODEL_NAME}" == "NCP1-1" ] || [ "${MODEL_NAME}" == "NCP2-1" ]; then
        for ((i=31;i>=25;i--))
        do
            _check_filepath "/sys/class/gpio/gpio$((GPIO_MAX-i))/value"
            beacon_lled=$(eval "cat /sys/class/gpio/gpio$((GPIO_MAX-i))/value ${LOG_REDIRECT}")
            _echo "[Left Beacon LED$((GPIO_MAX-i))]: ${beacon_lled}"
        done

        # Right LED
        for ((i=23;i>=17;i--))
        do
            _check_filepath "/sys/class/gpio/gpio$((GPIO_MAX-i))/value"
            beacon_rled=$(eval "cat /sys/class/gpio/gpio$((GPIO_MAX-i))/value ${LOG_REDIRECT}")
            _echo "[Right Beacon LED$((GPIO_MAX-i))]: ${beacon_rled}"
        done
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _show_beacon_led {
    if [ "${BSP_INIT_FLAG}" == "1" ] && [ "${GPIO_MAX_INIT_FLAG}" == "1" ] ; then
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

    while [ "${reg}" != "0x800" ]
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

function _show_cpld_error_log {
    _banner "Show CPLD Error Log"

    _echo "Register: 0xB7 0xB6 0xB5 0xB4"
    _echo "============================="
    for ((i=0;i<256;i++))
    do
        i2cset -y 0 0x66 0xbf 0x00
        i2cset -y 0 0x66 0xb9 0x00
        i2cset -y 0 0x66 0xb8 ${i}
        i2cset -y 0 0x66 0xba 0x01
        i2cset -y 0 0x66 0xbb 0x01

        reg_0xb7=`i2cget -y 0 0x66 0xb7`
        reg_0xb6=`i2cget -y 0 0x66 0xb6`
        reg_0xb5=`i2cget -y 0 0x66 0xb5`
        ret_0xb4=`i2cget -y 0 0x66 0xb4`

        _echo "$(printf "Addr_%03d: %s %s %s %s\n" $i ${reg_0xb7} ${reg_0xb6} ${reg_0xb5} ${ret_0xb4})"
    done
}

# Note: In order to prevent affecting MCE mechanism,
#       the function will not clear the 0x425 and 0x429 registers at step 1.1/1.2,
#       and only use to show the current correctable error count.
function _show_memory_correctable_error_count {
    _banner "Show Memory Correctable Error Count"

    which rdmsr > /dev/null 2>&1
    ret_rdmsr=$?
    which wrmsr > /dev/null 2>&1
    ret_wrmsr=$?

    if [ ${ret_rdmsr} -eq 0 ] && [ ${ret_wrmsr} -eq 0 ]; then
        ERROR_COUNT_THREASHOLD=12438
        modprobe msr

        # Step 0.1: Before clear the register, dump the correctable error count in channel 0 bank 9
        reg_c0_str=`rdmsr -p0 0x425 2> /dev/null`
        if [ "${reg_c0_str}" == "" ]; then
            reg_c0_str="0"
        fi
        reg_c0_value=`printf "%u\n" 0x${reg_c0_str}`
        # CORRECTED_ERR_COUNT bit[52:38]
        error_count_c0=$(((reg_c0_value >> 38) & 0x7FFF))
        _echo "[Ori_C0_Error_Count]: ${error_count_c0}"

        # Step 0.2: Before clear the register, dump the correctable error count in channel 1 bank 10
        reg_c1_str=`rdmsr -p0 0x429 2> /dev/null`
        if [ "${reg_c1_str}" == "" ]; then
            reg_c1_str="0"
        fi
        reg_c1_value=`printf "%u\n" 0x${reg_c1_str}`
        # CORRECTED_ERR_COUNT bit[52:38]
        error_count_c1=$(((reg_c1_value >> 38) & 0x7FFF))
        _echo "[Ori_C1_Error_Count]: ${error_count_c1}"

        # Step 1.1: clear correctable error count in channel 0 bank 9
        #wrmsr -p0 0x425 0x0

        # Step 1.2: clear correctable error count in channel 1 bank 10
        #wrmsr -p0 0x429 0x0

        # Step 2: wait 2 seconds
        sleep 2

        # Step 3.1: Read correctable error count in channel 0 bank 9
        reg_c0_str=`rdmsr -p0 0x425 2> /dev/null`
        if [ "${reg_c0_str}" == "" ]; then
            reg_c0_str="0"
        fi
        reg_c0_value=`printf "%u\n" 0x${reg_c0_str}`
        # CORRECTED_ERR_COUNT bit[52:38]
        error_count_c0=$(((reg_c0_value >> 38) & 0x7FFF))
        if [ ${error_count_c0} -gt ${ERROR_COUNT_THREASHOLD} ]; then
            _echo "[ERROR] Channel 0 Bank  9 Register Value: 0x${reg_c0_str}, Error Count: ${error_count_c0}"
        else
            _echo "[Info] Channel 0 Bank  9 Register Value: 0x${reg_c0_str}, Error Count: ${error_count_c0}"
        fi

        # Step 3.2: Read correctable error count in channel 1 bank 10
        reg_c1_str=`rdmsr -p0 0x429 2> /dev/null`
        if [ "${reg_c1_str}" == "" ]; then
            reg_c1_str="0"
        fi
        reg_c1_value=`printf "%u\n" 0x${reg_c1_str}`
        # CORRECTED_ERR_COUNT bit[52:38]
        error_count_c1=$(((reg_c1_value >> 38) & 0x7FFF))
        if [ ${error_count_c1} -gt ${ERROR_COUNT_THREASHOLD} ]; then
            _echo "[ERROR] Channel 1 Bank 10 Register Value: 0x${reg_c1_str}, Error Count: ${error_count_c1}"
        else
            _echo "[Info] Channel 1 Bank 10 Register Value: 0x${reg_c1_str}, Error Count: ${error_count_c1}"
        fi
    else
        _echo "Not support! Please install msr-tools to enble this function."
    fi
}

function _show_usb_info {
    _banner "Show USB Info"

    _echo "[Command]: lsusb -v"
    ret=$(eval "lsusb -v ${LOG_REDIRECT}")
    _echo "${ret}"
    _echo ""

    _echo "[Command]: lsusb -t"
    ret=$(eval "lsusb -t ${LOG_REDIRECT}")
    _echo "${ret}"
    _echo ""

    # check usb auth
    _echo "[USB Port Authentication]: "
    usb_auth_file_array=("/sys/bus/usb/devices/usb2/authorized" \
                         "/sys/bus/usb/devices/usb2/authorized_default" \
                         "/sys/bus/usb/devices/usb2/2-4/authorized" \
                         "/sys/bus/usb/devices/usb2/2-4/2-4.1/authorized" \
                         "/sys/bus/usb/devices/usb2/2-4/2-4:1.0/authorized" )

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

    cmd_array=("lsblk" "lsblk -O" "parted -l /dev/sda" "fdisk -l /dev/sda" "cat /sys/fs/*/*/errors_count")

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
               "ipmitool sol info 0x1" "ipmitool i2c bus=2 0x80 0x1 0x45" \
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
    _banner "Show BMC Device Status"

    # Step1: Stop IPMI Polling
    ret=$(eval "ipmitool raw 0x3c 0x4 0x9 0x1 0x0 ${LOG_REDIRECT}")
    _echo "[Stop IPMI Polling ]: ${ret}"
    _echo ""
    sleep 3

    # UCD Device
    # Step1: Stop IPMI Polling
    # Step2: Switch I2C MUX to UCD related channel
    # Step3: Check status register of UCD
    # Step4: Re-start IPMI polling
    _echo "[UCD Device Status (BMC) ]"
    if [ "${MODEL_NAME}" == "NCF" ]; then
        ## Step1: Stop IPMI Polling
        #ret=$(eval "ipmitool raw 0x3c 0x4 0x9 0x1 0x0 ${LOG_REDIRECT}")
        #_echo "[Stop IPMI Polling ]: ${ret}"
        #sleep 3

        # Step2: Switch I2C MUX to UCD related channel for NCF
        ret=$(eval "ipmitool i2c bus=2 0xec 0x0 0x10 ${LOG_REDIRECT}")

        # Step3: Check status register of UCD
        # UCD0 (BOT Board)
        status_word_bot=$(eval "ipmitool i2c bus=2 0x68 0x2 0x79 ${LOG_REDIRECT}" | head -n 1)
        logged_faults_bot=$(eval "ipmitool i2c bus=2 0x68 0xf 0xea ${LOG_REDIRECT}")
        mfr_status_bot=$(eval "ipmitool i2c bus=2 0x68 0x5 0xf3 ${LOG_REDIRECT}")
        status_vout_bot=$(eval "ipmitool i2c bus=2 0x68 0x1 0x7a ${LOG_REDIRECT}" | head -n 1)
        status_iout_bot=$(eval "ipmitool i2c bus=2 0x68 0x1 0x7b ${LOG_REDIRECT}" | head -n 1)
        status_temperature_bot=$(eval "ipmitool i2c bus=2 0x68 0x1 0x7d ${LOG_REDIRECT}" | head -n 1)
        status_cml_bot=$(eval "ipmitool i2c bus=2 0x68 0x1 0x7e ${LOG_REDIRECT}" | head -n 1)
        _echo "[UCD0 Status Word  ][0x79]: ${status_word_bot}"
        _echo "[UCD0 Logged Faults][0xEA]: ${logged_faults_bot}"
        _echo "[UCD0 MFR Status   ][0xF3]: ${mfr_status_bot}"
        _echo "[UCD0 Status VOUT  ][0x7A]: ${status_vout_bot}"
        _echo "[UCD0 Status IOUT  ][0x7B]: ${status_iout_bot}"
        _echo "[UCD0 Status Tempe ][0x7D]: ${status_temperature_bot}"
        _echo "[UCD0 Status CML   ][0x7E]: ${status_cml_bot}"

        # Check every gpio status (gpio#0~25)
        # 0xfa - GPIO_SELECT
        # 0xfb - GPIO_CONFIG
        #      - bit[7:4] = reserved
        #      - bit3 = Status
        #      - bit2 = Out_Value
        #      - bit1 = Out_Enable
        #      - bit0 = Enable
        for (( i=0; i<=25; i++ ))
        do
            i_hex=`printf "0x%X\n" ${i}`
            ret=$(eval "ipmitool i2c bus=2 0x68 0x0 0xfa ${i_hex} ${LOG_REDIRECT}")
            gpio_status=$(eval "ipmitool i2c bus=2 0x68 0x1 0xfb ${LOG_REDIRECT}")
            _echo "[UCD0 GPIO${i} Status ]: ${gpio_status}"
        done

        # UCD1 (TOP Board)
        status_word_top=$(eval "ipmitool i2c bus=2 0x6a 0x2 0x79 ${LOG_REDIRECT}" | head -n 1)
        logged_faults_top=$(eval "ipmitool i2c bus=2 0x6a 0xf 0xea ${LOG_REDIRECT}")
        mfr_status_top=$(eval "ipmitool i2c bus=2 0x6a 0x5 0xf3 ${LOG_REDIRECT}")
        status_vout_top=$(eval " ipmitool i2c bus=2 0x6a 0x1 0x7a ${LOG_REDIRECT}" | head -n 1)
        status_iout_top=$(eval "ipmitool i2c bus=2 0x6a 0x1 0x7b ${LOG_REDIRECT}" | head -n 1)
        status_temperature_top=$(eval "ipmitool i2c bus=2 0x6a 0x1 0x7d ${LOG_REDIRECT}" | head -n 1)
        status_cml_top=$(eval "ipmitool i2c bus=2 0x6a 0x1 0x7e ${LOG_REDIRECT}" | head -n 1)
        _echo "[UCD1 Status Word  ][0x79]: ${status_word_top}"
        _echo "[UCD1 Logged Faults][0xEA]: ${logged_faults_top}"
        _echo "[UCD1 MFR Status   ][0xF3]: ${mfr_status_top}"
        _echo "[UCD1 Status VOUT  ][0x7A]: ${status_vout_top}"
        _echo "[UCD1 Status IOUT  ][0x7B]: ${status_iout_top}"
        _echo "[UCD1 Status Tempe ][0x7D]: ${status_temperature_top}"
        _echo "[UCD1 Status CML   ][0x7E]: ${status_cml_top}"

        # Check every gpio status (gpio#0~25)
        # 0xfa - GPIO_SELECT
        # 0xfb - GPIO_CONFIG
        #      - bit[7:4] = reserved
        #      - bit3 = Status
        #      - bit2 = Out_Value
        #      - bit1 = Out_Enable
        #      - bit0 = Enable
        for (( i=0; i<=25; i++ ))
        do
            i_hex=`printf "0x%X\n" ${i}`
            ret=$(eval "ipmitool i2c bus=2 0x6a 0x0 0xfa ${i_hex} ${LOG_REDIRECT}")
            gpio_status=$(eval "ipmitool i2c bus=2 0x6a 0x1 0xfb ${LOG_REDIRECT}" | head -n 1)
            _echo "[UCD1 GPIO${i} Status ]: ${gpio_status}"
        done
        _echo ""

        _show_ucd 2 0x68
        _show_ucd 2 0x6a

        ## Step4: Re-start IPMI polling
        #ret=$(eval "ipmitool raw 0x3c 0x4 0x9 0x1 0x1 ${LOG_REDIRECT}")
        #_echo "[Start IPMI Polling]: ${ret}"
        #_echo ""

    elif [ "${MODEL_NAME}" == "NCP1-1" ] || [ "${MODEL_NAME}" == "NCP2-1" ]; then
        ## Step1: Stop IPMI Polling
        #ret=$(eval "ipmitool raw 0x3c 0x4 0x9 0x1 0x0 ${LOG_REDIRECT}")
        #_echo "[Stop IPMI Polling ]: ${ret}"
        #sleep 3

        # Step2: Switch I2C MUX to UCD related channel for NCP
        ret=$(eval "ipmitool i2c bus=2 0xe0 0x0 0x08 ${LOG_REDIRECT}")

        # Step3: Check Status Register of UCD
        status_word=$(eval "ipmitool i2c bus=2 0x68 0x2 0x79 ${LOG_REDIRECT}" | head -n 1)
        logged_faults=$(eval "ipmitool i2c bus=2 0x68 0xf 0xea ${LOG_REDIRECT}")
        mfr_status=$(eval "ipmitool i2c bus=2 0x68 0x5 0xf3 ${LOG_REDIRECT}")
        status_vout=$(eval "ipmitool i2c bus=2 0x68 0x1 0x7a ${LOG_REDIRECT}" | head -n 1)
        status_iout=$(eval "ipmitool i2c bus=2 0x68 0x1 0x7b ${LOG_REDIRECT}" | head -n 1)
        status_temperature=$(eval "ipmitool i2c bus=2 0x68 0x1 0x7d ${LOG_REDIRECT}" | head -n 1)
        status_cml=$(eval "ipmitool i2c bus=2 0x68 0x1 0x7e ${LOG_REDIRECT}" | head -n 1)
        _echo "[UCD Status Word   ][0x79]: ${status_word}"
        _echo "[UCD Logged Faults ][0xEA]: ${logged_faults}"
        _echo "[UCD MFR Status    ][0xF3]: ${mfr_status}"
        _echo "[UCD Status VOUT   ][0x7A]: ${status_vout}"
        _echo "[UCD Status IOUT   ][0x7B]: ${status_iout}"
        _echo "[UCD Status Tempe  ][0x7D]: ${status_temperature}"
        _echo "[UCD Status CML    ][0x7E]: ${status_cml}"

        # Check every gpio status (gpio#0~25)
        # 0xfa - GPIO_SELECT
        # 0xfb - GPIO_CONFIG
        #      - bit[7:4] = reserved
        #      - bit3 = Status
        #      - bit2 = Out_Value
        #      - bit1 = Out_Enable
        #      - bit0 = Enable
        for (( i=0; i<=25; i++ ))
        do
            i_hex=`printf "0x%X\n" ${i}`
            ret=$(eval "ipmitool i2c bus=2 0x68 0x0 0xfa ${i_hex} ${LOG_REDIRECT}")
            gpio_status=$(eval "ipmitool i2c bus=2 0x68 0x1 0xfb ${LOG_REDIRECT}" | head -n 1)
            _echo "[UCD GPIO${i} Status  ]: ${gpio_status}"
        done
        _echo ""

        _show_ucd 2 0x68

        ## Step4: Re-start IPMI polling
        #ret=$(eval "ipmitool raw 0x3c 0x4 0x9 0x1 0x1 ${LOG_REDIRECT}")
        #_echo "[Start IPMI Polling]: ${ret}"
        #_echo ""
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    # PSU Device
    # Step1: Stop IPMI Polling
    # Step2: Switch I2C MUX to PSU0 Channel and Check Status Registers
    # Step3: Switch I2C MUX to PSU1 Channel and Check Status Registers
    # Step4: Re-start IPMI polling
    _echo "[PSU Device Status (BMC) ]"
    if [ "${MODEL_NAME}" == "NCF" ]; then
        ## Step1: Stop IPMI Polling
        #ret=$(eval "ipmitool raw 0x3c 0x4 0x9 0x1 0x0 ${LOG_REDIRECT}")
        #_echo "[Stop IPMI Polling ]: ${ret}"
        #sleep 3

        # Step2: Switch I2C MUX to PSU0 Channel and Check Status Registers
        ret=$(eval "ipmitool i2c bus=2 0xec 0x0 0x04 ${LOG_REDIRECT}")
        status_word_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x2 0x79 ${LOG_REDIRECT}" | head -n 1)
        status_vout_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7a ${LOG_REDIRECT}" | head -n 1)
        status_iout_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7b ${LOG_REDIRECT}" | head -n 1)
        status_temp_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7d ${LOG_REDIRECT}" | head -n 1)
        status_cml_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7e ${LOG_REDIRECT}" | head -n 1)
        status_other_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7f ${LOG_REDIRECT}" | head -n 1)
        status_msr_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x80 ${LOG_REDIRECT}" | head -n 1)
        status_fan_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x81 ${LOG_REDIRECT}" | head -n 1)
        status_oem_0xf0_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x3 0xf0 ${LOG_REDIRECT}" | head -n 1)
        status_oem_0xf1_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0xf1 ${LOG_REDIRECT}" | head -n 1)
        _echo "[PSU0 Status Word ][0x79]: ${status_word_psu0}"
        _echo "[PSU0 Status VOUT ][0x7A]: ${status_vout_psu0}"
        _echo "[PSU0 Status IOUT ][0x7B]: ${status_iout_psu0}"
        _echo "[PSU0 Status Temp ][0x7D]: ${status_temp_psu0}"
        _echo "[PSU0 Status CML  ][0x7E]: ${status_cml_psu0}"
        _echo "[PSU0 Status Other][0x7F]: ${status_other_psu0}"
        _echo "[PSU0 Status MFR  ][0x80]: ${status_msr_psu0}"
        _echo "[PSU0 Status FAN  ][0x81]: ${status_fan_psu0}"
        _echo "[PSU0 Status OEM  ][0xF0]: ${status_oem_0xf0_psu0}"
        _echo "[PSU0 Status OEM  ][0xF1]: ${status_oem_0xf1_psu0}"


        # Step3: Switch I2C MUX to PSU1 Channel and Check Status Registers
        ret=$(eval "ipmitool i2c bus=2 0xec 0x0 0x08 ${LOG_REDIRECT}")
        status_word_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x2 0x79 ${LOG_REDIRECT}" | head -n 1)
        status_vout_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7a ${LOG_REDIRECT}" | head -n 1)
        status_iout_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7b ${LOG_REDIRECT}" | head -n 1)
        status_temp_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7d ${LOG_REDIRECT}" | head -n 1)
        status_cml_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7e ${LOG_REDIRECT}" | head -n 1)
        status_other_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7f ${LOG_REDIRECT}" | head -n 1)
        status_msr_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x80 ${LOG_REDIRECT}" | head -n 1)
        status_fan_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x81 ${LOG_REDIRECT}" | head -n 1)
        status_oem_0xf0_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x3 0xf0 ${LOG_REDIRECT}" | head -n 1)
        status_oem_0xf1_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0xf1 ${LOG_REDIRECT}" | head -n 1)
        _echo "[PSU1 Status Word ][0x79]: ${status_word_psu1}"
        _echo "[PSU1 Status VOUT ][0x7A]: ${status_vout_psu1}"
        _echo "[PSU1 Status IOUT ][0x7B]: ${status_iout_psu1}"
        _echo "[PSU1 Status Temp ][0x7D]: ${status_temp_psu1}"
        _echo "[PSU1 Status CML  ][0x7E]: ${status_cml_psu1}"
        _echo "[PSU1 Status Other][0x7F]: ${status_other_psu1}"
        _echo "[PSU1 Status MFR  ][0x80]: ${status_msr_psu1}"
        _echo "[PSU1 Status FAN  ][0x81]: ${status_fan_psu1}"
        _echo "[PSU1 Status OEM  ][0xF0]: ${status_oem_0xf0_psu1}"
        _echo "[PSU1 Status OEM  ][0xF1]: ${status_oem_0xf1_psu1}"
        _echo ""

        ## Step4: Re-start IPMI polling
        #ret=$(eval "ipmitool raw 0x3c 0x4 0x9 0x1 0x1 ${LOG_REDIRECT}")
        #_echo "[Start IPMI Polling]: ${ret}"
        #_echo ""
    elif [ "${MODEL_NAME}" == "NCP1-1" ] || [ "${MODEL_NAME}" == "NCP2-1" ]; then
        ## Step1: Stop IPMI Polling
        #ret=$(eval "ipmitool raw 0x3c 0x4 0x9 0x1 0x0 ${LOG_REDIRECT}")
        #_echo "[Stop IPMI Polling ]: ${ret}"
        #sleep 3

        # Step2: Switch I2C MUX to PSU0 Channel and Check Status Registers
        ret=$(eval "ipmitool i2c bus=2 0xe0 0x0 0x01 ${LOG_REDIRECT}")
        status_word_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x2 0x79 ${LOG_REDIRECT}" | head -n 1)
        status_vout_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7a ${LOG_REDIRECT}" | head -n 1)
        status_iout_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7b ${LOG_REDIRECT}" | head -n 1)
        status_temp_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7d ${LOG_REDIRECT}" | head -n 1)
        status_cml_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7e ${LOG_REDIRECT}" | head -n 1)
        status_other_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7f ${LOG_REDIRECT}" | head -n 1)
        status_msr_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x80 ${LOG_REDIRECT}" | head -n 1)
        status_fan_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x81 ${LOG_REDIRECT}" | head -n 1)
        status_oem_0xf0_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x3 0xf0 ${LOG_REDIRECT}" | head -n 1)
        status_oem_0xf1_psu0=$(eval "ipmitool i2c bus=2 0xb0 0x1 0xf1 ${LOG_REDIRECT}" | head -n 1)
        _echo "[PSU0 Status Word ][0x79]: ${status_word_psu0}"
        _echo "[PSU0 Status VOUT ][0x7A]: ${status_vout_psu0}"
        _echo "[PSU0 Status IOUT ][0x7B]: ${status_iout_psu0}"
        _echo "[PSU0 Status Temp ][0x7D]: ${status_temp_psu0}"
        _echo "[PSU0 Status CML  ][0x7E]: ${status_cml_psu0}"
        _echo "[PSU0 Status Other][0x7F]: ${status_other_psu0}"
        _echo "[PSU0 Status MFR  ][0x80]: ${status_msr_psu0}"
        _echo "[PSU0 Status FAN  ][0x81]: ${status_fan_psu0}"
        _echo "[PSU0 Status OEM  ][0xF0]: ${status_oem_0xf0_psu0}"
        _echo "[PSU0 Status OEM  ][0xF1]: ${status_oem_0xf1_psu0}"

        # Step3: Switch I2C MUX to PSU1 Channel and Check Status Registers
        ret=$(eval "ipmitool i2c bus=2 0xe0 0x0 0x02 ${LOG_REDIRECT}")
        status_word_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x2 0x79 ${LOG_REDIRECT}" | head -n 1)
        status_vout_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7a ${LOG_REDIRECT}" | head -n 1)
        status_iout_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7b ${LOG_REDIRECT}" | head -n 1)
        status_temp_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7d ${LOG_REDIRECT}" | head -n 1)
        status_cml_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7e ${LOG_REDIRECT}" | head -n 1)
        status_other_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x7f ${LOG_REDIRECT}" | head -n 1)
        status_msr_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x80 ${LOG_REDIRECT}" | head -n 1)
        status_fan_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0x81 ${LOG_REDIRECT}" | head -n 1)
        status_oem_0xf0_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x3 0xf0 ${LOG_REDIRECT}" | head -n 1)
        status_oem_0xf1_psu1=$(eval "ipmitool i2c bus=2 0xb0 0x1 0xf1 ${LOG_REDIRECT}" | head -n 1)
        _echo "[PSU1 Status Word ][0x79]: ${status_word_psu1}"
        _echo "[PSU1 Status VOUT ][0x7A]: ${status_vout_psu1}"
        _echo "[PSU1 Status IOUT ][0x7B]: ${status_iout_psu1}"
        _echo "[PSU1 Status Temp ][0x7D]: ${status_temp_psu1}"
        _echo "[PSU1 Status CML  ][0x7E]: ${status_cml_psu1}"
        _echo "[PSU1 Status Other][0x7F]: ${status_other_psu1}"
        _echo "[PSU1 Status MFR  ][0x80]: ${status_msr_psu1}"
        _echo "[PSU1 Status FAN  ][0x81]: ${status_fan_psu1}"
        _echo "[PSU1 Status OEM  ][0xF0]: ${status_oem_0xf0_psu1}"
        _echo "[PSU1 Status OEM  ][0xF1]: ${status_oem_0xf1_psu1}"
        _echo ""

        ## Step4: Re-start IPMI polling
        #ret=$(eval "ipmitool raw 0x3c 0x4 0x9 0x1 0x1 ${LOG_REDIRECT}")
        #_echo "[Start IPMI Polling]: ${ret}"
        #_echo ""
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    # Step4: Re-start IPMI polling
    ret=$(eval "ipmitool raw 0x3c 0x4 0x9 0x1 0x1 ${LOG_REDIRECT}")
    _echo "[Start IPMI Polling]: ${ret}"
    _echo ""

    sleep 3
}

function _show_ucd {

    pmbus_cmd=(\
        "FAULT_RESPONSE 0xe9    0x0a" \
        "LOGGED_FAULTS  0xea    0x0f" \
        "STATUS_WORD    0x79    0x02" \
        "STATUS_VOUT    0x7a    0x01" \
        "STATUS_IOUT    0x7b    0x01" \
        "STATUS_TEMP    0x7d    0x01" \
        "STATUS_CML     0x7e    0x01" \
        "MFR_STATUS     0xf3    0x05" \
        "VOUT_MODE      0x20    0x01" \
        "UV_WARN_LIMIT  0x43    0x02" \
        "UV_FAULT_LIMIT 0x44    0x02" \
    )

    # Check STATUS register of UCD
    for i in $(seq 0 1 11);
    do
        # Set PAGE to each power rail
        ipmitool i2c bus=${1} ${2} 0x0 0x0 ${i} >/dev/null 2>&1
        _printf "===== PAGE %2sh (${2}) =====\n" ${i}
        for entry in "${pmbus_cmd[@]}"
        do
            cmd=(${entry})
            cmd_result=`ipmitool i2c bus=${1} ${2} ${cmd[2]} ${cmd[1]} | sed -n '1p'`
            _printf "%15s [%2s] =%-15s\n" ${cmd[0]} ${cmd[1]} "${cmd_result}"
        done
    done

    # Check every GPIO status (gpio#0~25)
    _printf "\n===== GPIO Status (${2}) =====\n"
    for i in $(seq 0 1 25);
    do
        ipmitool i2c bus=${1} ${2} 0x0 0xfa ${i} >/dev/null 2>&1
        gpio_sts=`ipmitool i2c bus=${1} ${2} 0x1 0xfb | sed -n '2p' | cut -c 5-8`
        case "${gpio_sts}" in
            0000) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Default  input  low" ;;
            0001) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Enable   input  low" ;;
            0010) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Default  output low" ;;
            0011) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Enable   output low" ;;
            0100) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Default  input  low" ;;
            0101) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Enable   input  low" ;;
            0110) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Default  output low" ;;
            0111) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Enable   output low" ;;
            1000) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Default  input  HIGH" ;;
            1001) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Enable   input  HIGH" ;;
            1010) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Default  output HIGH" ;;
            1011) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Enable   output HIGH" ;;
            1100) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Default  input  HIGH" ;;
            1101) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Enable   input  HIGH" ;;
            1110) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Default  output HIGH" ;;
            1111) _printf "%-7s = %2s,  %-15s\n" GPIO_${i} ${gpio_sts} "Enable   output HIGH" ;;
            *) _printf "%-15s raw data = %-15s\n" GPIO_${i} ${gpio_sts} ;;
        esac
    done

    _ucd_detail_info ${1} ${2}

}

function _ucd_detail_info {
    fault_type_list=(\
        "Reserved" \
        "System Watchdog Timeout" \
        "Resequence Error" \
        "Watchdog Timeout" \
        "Reserved" \
        "Reserved" \
        "Reserved" \
        "Reserved" \
        "Fan Fault" \
        "GPI Fault" \
    )

    fault_type_list_page=(\
        "VOUT_OV" \
        "VOUT_UV" \
        "TON_MAX" \
        "IOUT_OC" \
        "IOUT_UC" \
        "Temperature_OT" \
        "SequenceOn Timeout" \
        "SequenceOff Timeout" \
    )

     _printf "\n===== FAULT DETAIL in UCD (${2}) =====\n"

    # Get size of Fault Buffer
    detail_count=`ipmitool i2c bus=${1} ${2} 0x2 0xeb | head -n 1 | awk '{print $2}'`
    detail_count=`echo $((16#${detail_count}))`
    _printf "Total %2s fault details is recorded\n" ${detail_count}
    detail_count=`expr ${detail_count} - 1`

    # Get raw data of each FAULT DETAIL
    for i in $(seq 0 1 ${detail_count});
    do
        ipmitool i2c bus=${1} ${2} 0x0 0xeb ${i} 0x0 >/dev/null 2>&1
        detail_raw=`ipmitool i2c bus=${1} ${2} 0xb 0xec`

        _printf "%-15s =%-30s\n" FAULT_DETAIL_${i} "${detail_raw}"
    done
    _printf "\n"

    # Decode FAULT DETAIL
    for i in $(seq 0 1 ${detail_count});
    do
        ipmitool i2c bus=${1} ${2} 0x0 0xeb ${i} 0x0 >/dev/null 2>&1
        detail_raw="`ipmitool i2c bus=${1} ${2} 0xb 0xec`"
        detail_sts=(${detail_raw})

        time_ms="${detail_sts[1]}${detail_sts[2]}${detail_sts[3]}${detail_sts[4]}"
        time_ms=`echo $((16#${time_ms}))`
        day="${detail_sts[6]}${detail_sts[7]}${detail_sts[8]}"
        day=`echo $((16#${day}))`
        ((day &= 4194303))

        detail_is_page=`echo $((16#${detail_sts[5]}))`
        ((detail_is_page >>= 7))
        detail_fault_type=`echo $((16#${detail_sts[5]}))`
        ((detail_fault_type &= 127))
        ((detail_fault_type >>= 3))
        detail_page=${detail_sts[5]}${detail_sts[6]}
        detail_page=`echo $((16#${detail_page}))`
        ((detail_page &= 1920))
        ((detail_page >>= 7))
        detail_fault_data=0x${detail_sts[10]}${detail_sts[9]}

        if [ "${detail_is_page}" = '1' ]; then
            _printf "%-15s - time = %4sd+%-10s page = %-3s type = %-10s data = %-6s\n" \
                FAULT_DETAIL_${i} \
                ${day} ${time_ms}ms, \
                ${detail_page}, \
                "${fault_type_list_page[${detail_fault_type}]}," \
                ${detail_fault_data}
        else
            _printf "%-15s - page = %s type = %-10s data = %-6x \n" \
                FAULT_DETAIL_${i} \
                '-,' \
                "${fault_type_list[${detail_fault_type}]}," \
                ${detail_fault_data}
        fi
    done
    _printf "\n\n"
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
        ret=$(eval "ipmitool sel save ${LOG_FOLDER_PATH}/sel_raw_data.log ${LOG_REDIRECT}")
        _echo "The file is located at ${LOG_FOLDER_NAME}/sel_raw_data.log"
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

    echo "    Show BMC SEL details, please wait..."
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


    #sel_rid_array=`ipmitool sel elist | grep -P -i '(Undetermined|Bus|CATERR|OEM)' | awk '{printf("0x%s ",$1)}'`
    #sel_detail=`ipmitool sel get ${sel_rid_array}`

    #_echo "[SEL Record ID]: ${sel_rid_array}"
    #_echo ""
    #_echo "[SEL Detail   ]:"
    #_echo "${sel_detail}"

    #sel_elist_number=10
    #bmc_sel_id_array=($(ipmitool sel list | awk -F" " '{print $1}' | tail -n ${sel_elist_number}))
    #
    #for (( i=0; i<${#bmc_sel_id_array[@]}; i++ ))
    #do
    #    _echo ""
    #    _echo "[Command]: ipmitool sel get 0x${bmc_sel_id_array[${i}]} "
    #    ret=`ipmitool sel get 0x${bmc_sel_id_array[${i}]}`
    #    _echo "${ret}"
    #done
}

function _show_dmesg {
    _banner "Show Dmesg"

    ret=$(eval "dmesg ${LOG_REDIRECT}")
    _echo "${ret}"
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

function _compression {
    _banner "Compression"

    if [ ! -z "${LOG_FOLDER_PATH}" ] && [ -d "${LOG_FOLDER_PATH}" ]; then
        cd "${LOG_FOLDER_ROOT}"
        tar -zcf "${LOG_FOLDER_NAME}".tgz "${LOG_FOLDER_NAME}"

        echo "    The tarball is ready at ${LOG_FOLDER_ROOT}/${LOG_FOLDER_NAME}.tgz"
        _echo "    The tarball is ready at ${LOG_FOLDER_ROOT}/${LOG_FOLDER_NAME}.tgz"
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
    _show_nif_port_status
    _show_fab_port_status
    _show_sfp_status
    _show_cpu_temperature
    _show_cpld_interrupt
    _show_system_led
    _show_beacon_led
    _show_ioport
    _show_onlpdump
    _show_onlps
    _show_system_info
    _show_cpld_error_log
    _show_memory_correctable_error_count
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

