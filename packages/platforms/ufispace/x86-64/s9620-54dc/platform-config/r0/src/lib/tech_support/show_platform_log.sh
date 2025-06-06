#!/bin/bash

#Tech Support script version
TS_VERSION="1.0.0"

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
PLAT="S9620-54DC"
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
SYSFS_FPGA="${SYSFS_DEV}/2-0037"
SYSFS_LPC="/sys/devices/platform/x86_64_ufispace_s9620_54dc_lpc"

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

    #GPIO_BASE=$(cat ${sysfs_gpio_base})
    #if [ $? -eq 1 ] || [ "$GPIO_BASE" == "-1" ]; then
    #    GPIO_BASE_INIT_FLAG=0
    #else
    #    GPIO_BASE_INIT_FLAG=1
    #fi

    _echo "[GPIO_MAX_INIT_FLAG]: ${GPIO_MAX_INIT_FLAG}"
    _echo "[GPIO_MAX]: ${GPIO_MAX}"

    # _echo "[GPIO_BASE_INIT_FLAG]: ${GPIO_BASE_INIT_FLAG}"
    # _echo "[GPIO_BASE]: ${GPIO_BASE}"
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
    model_id_array=($((2#00101001)))
    model_name_array=("S9620-54DC")
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
    hw_rev_id=$(((board_rev_id & 2#00000011) >> 0))
    hw_rev=${hw_rev_array[${hw_rev_id}]}
    if [ $deph_id -eq 0 ]; then
        hw_rev=${hw_rev_array[${hw_rev_id}]}
    else
        hw_rev=${hw_rev_ga_array[${hw_rev_id}]}
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
    #     # MB CPLD S9620-54DC
    #     mb_cpld_ver=$(_dd_read_byte 0x702)
    #     ret=$?
    #     if [ ${ret} -eq 0 ]; then
    #         mb_cpld_ver=`echo ${mb_cpld_ver} | awk -F" " '{print $NF}'`
    #     fi

    #     mb_cpld_build=$(_dd_read_byte 0x704)
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
        local mux_i2c_addr_1=0x71
        local mux_i2c_addr_2=0x72

        # MB CPLD
        mb_cpld1_ver=""
        mb_cpld2_ver=""
        mb_cpld3_ver=""
        mb_fpga_ver=""
        
        mb_cpld1_build=""
        mb_cpld2_build=""
        mb_cpld3_build=""
        mb_fpga_build=""

        # CPLD 1-3

        _check_i2c_device ${mux_i2c_bus} ${mux_i2c_addr_1}
        ret=$?

        if [ ${ret} -eq ${TRUE} ]; then
            i2cset -y -f ${mux_i2c_bus} ${mux_i2c_addr_1} 0x1
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
                _printf "[MB FPGA  Version]: %d.%02d.%03d\n" $(( (mb_fpga_ver & 2#11000000) >> 6)) $(( mb_fpga_ver & 2#00111111 )) $((mb_fpga_build))
            fi

            

            i2cset -y -f ${mux_i2c_bus} ${mux_i2c_addr} 0x0
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

        if _check_filepath "$SYSFS_FPGA/fpga_version_h"; then
            mb_fpga_ver=$(eval "cat $SYSFS_FPGA/fpga_version_h ${LOG_REDIRECT}")
            _echo "[MB FPGA  Version]: ${mb_fpga_ver}"
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

    ret=$(eval "i2cdetect -y "${i801_bus}" ${LOG_REDIRECT}")

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
            ret=$(eval "i2cdetect -y ${bus} ${LOG_REDIRECT}")
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

            ## (9548_ROOT_FPGA_CPLD)-0x72
            _show_i2c_mux_devices "${bus}" "${chip_addr1}" "8" "9548_ROOT_FPGA_CPLD-${chip_addr1}"
            local chip_addr2_array=("0x30" "0x31" " 0x32" "0x37" "" "" "" "")
            local mux_name_array=("CPLD_1" "CPLD_2" "CPLD_3""FPGA" "" "" "" "")

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

   
        chip_addr1="0x73"
        _check_i2c_device "${bus}" "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then

            ## (9546_ROOT_CLK)-0x73
            _show_i2c_mux_devices "${bus}" "${chip_addr1}" "4" "9546_ROOT_CLK-${chip_addr1}"
            local chip_addr2_array=("" "" "" "" "" "" "" "")
            local mux_name_array=("" "" "" "" "" "" "" "")

            for (( chip_addr1_chann=0; chip_addr1_chann<${#chip_addr2_array[@]}; chip_addr1_chann++ ))
            do
                if [ -z "${chip_addr2_array[${chip_addr1_chann}]}" ]; then
                    continue
                fi

                local chip_addr2=${chip_addr2_array[${chip_addr1_chann}]}
                local mux_name=${mux_name_array[${chip_addr1_chann}]}
                # open mux channel - 0x72 (chip_addr1)
                i2cset -y ${bus} ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
                _show_i2c_mux_devices ${bus} "${chip_addr2}" "4" "${mux_name}-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}"
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
        pca954x_device_id=("0x72" "0x73")
        pca954x_device_bus=("${i801_bus}" "${i801_bus}")
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

    if [ "${eeprom_mux}" != "" ] && [ "${eeprom_ch}" != "" ]; then
        under_mux=true
    fi

    if $under_mux; then
        i2cset -y ${ismt_bus} ${eeprom_mux} $(( 2 ** ${eeprom_ch} ))
    fi

    #first read return empty content
    sys_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")
    #second read return correct content
    sys_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")

    if $under_mux; then
        i2cset -y -f ${ismt_bus} ${eeprom_mux} 0x0
    fi
    _echo "[System EEPROM]:"
    _echo "${sys_eeprom}"
}

function _show_sys_eeprom_sysfs {
    _banner "Show System EEPROM"

    sys_eeprom=$(eval "cat /sys/bus/i2c/devices/${ismt_bus}-0057/eeprom ${LOG_REDIRECT} | hexdump -C")
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
    if [ "${MODEL_NAME}" != "${PLAT}" ]; then
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    # Check file paths
    for filepath in "${SYSFS_CPLD1}/psu_0_prsnt" "${SYSFS_CPLD1}/psu_1_prsnt" "${SYSFS_CPLD1}/psu_0_pg" "${SYSFS_CPLD1}/psu_1_pg"; do
        if ! _check_filepath "${filepath}"; then
            _echo "File not found or inaccessible: ${filepath}"
            exit 1
        fi
    done

    # Read PSU Status
    psu_0_prsnt_reg=$(eval "cat ${SYSFS_CPLD1}/psu_0_prsnt ${LOG_REDIRECT}")
    psu_1_prsnt_reg=$(eval "cat ${SYSFS_CPLD1}/psu_1_prsnt ${LOG_REDIRECT}")
    psu_0_pg_reg=$(eval "cat ${SYSFS_CPLD1}/psu_0_pg ${LOG_REDIRECT}")
    psu_1_pg_reg=$(eval "cat ${SYSFS_CPLD1}/psu_1_pg ${LOG_REDIRECT}")

    # Extract status flags
    psu0_power_ok=$(( (psu_0_pg_reg & 1) ))
    psu0_absent=$(( (psu_0_prsnt_reg & 1) ))
    psu1_power_ok=$(( (psu_1_pg_reg & 1) ))
    psu1_absent=$(( (psu_1_prsnt_reg & 1) ))

    # Output PSU status
    _echo "[PSU0 Power Good Status]: ${psu0_power_ok}"
    _echo "[PSU0 Absent Status (L)]: ${psu0_absent}"
    _echo "[PSU1 Power Good Status]: ${psu1_power_ok}"
    _echo "[PSU1 Absent Status (L)]: ${psu1_absent}"
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

    if [ "${MODEL_NAME}" == "${PLAT}" ]; then      

        port_name_array=(
            "0"   "1"   "2"   "3"   "4"   "5"   "6"   "7"
            "8"   "9"   "10"  "11"  "12"  "13"  "14"  "15"
            "16"  "17"  "18"  "19"  "20"  "21"  "22"  "23"
            "24"  "25"  "26"  "27"  "28"  "29"  "30"  "31"
            "32"  "33"  "34"  "35"  "36"  "37"  "38"  "39"
            "40"  "41"  "42"  "43"  "44"  "45"  "46"  "47"
            "48"  "49"  "50"  "51"  "52"  "53"  
        )


        local QSFPDD=${PORT_T_QSFPDD}
        local QSFP=${PORT_T_QSFP}
        local SFP=${PORT_T_SFP}
        local MGMT=${PORT_T_MGMT}
        port_type_array=(
        #      0         1        2         3         4         5        6          7
            ${SFP}    ${SFP}    ${SFP}    ${SFP}   ${SFP}    ${SFP}    ${SFP}    ${SFP}
        #      8         9        10        11        12        13       14         15
            ${SFP}    ${SFP}    ${SFP}    ${SFP}   ${SFP}    ${SFP}    ${SFP}    ${SFP}
        #      16        17       18        19        20        21       22         23
            ${SFP}    ${SFP}    ${SFP}    ${SFP}   ${SFP}    ${SFP}    ${SFP}    ${SFP}  
        #      24        25       26        27        28        29       30         31
            ${SFP}    ${SFP}    ${SFP}    ${SFP}   ${SFP}    ${SFP}    ${SFP}    ${SFP}
	    #      32        33       34        35        36        37       38         39
            ${SFP}    ${SFP}    ${SFP}    ${SFP}   ${SFP}    ${SFP}    ${SFP}    ${SFP}  
	    #      40        41       42        43        45        46       47         48
            ${SFP}    ${SFP}    ${SFP}    ${SFP}   ${SFP}    ${SFP}    ${SFP}    ${SFP}  
	    #      48        49       50        51        52        53          
            ${QSFP}   ${QSFP}   ${QSFP}   ${QSFP}  ${QSFPDD} ${QSFPDD}

        )

        port_absent_array=(
            "${SYSFS_CPLD2}/sfp28_p0_abs"             #0 
            "${SYSFS_CPLD2}/sfp28_p1_abs"             #1 
            "${SYSFS_CPLD2}/sfp28_p2_abs"             #2 
            "${SYSFS_CPLD2}/sfp28_p3_abs"             #3 
            "${SYSFS_CPLD2}/sfp28_p4_abs"             #4 
            "${SYSFS_CPLD2}/sfp28_p5_abs"             #5 
            "${SYSFS_CPLD2}/sfp28_p6_abs"             #6 
            "${SYSFS_CPLD2}/sfp28_p7_abs"             #7 
            "${SYSFS_CPLD2}/sfp28_p8_abs"             #8 
            "${SYSFS_CPLD2}/sfp28_p9_abs"             #9 
            "${SYSFS_CPLD2}/sfp28_p10_abs"            #10
            "${SYSFS_CPLD2}/sfp28_p11_abs"            #11
            "${SYSFS_CPLD2}/sfp28_p12_abs"            #12
            "${SYSFS_CPLD2}/sfp28_p13_abs"            #13
            "${SYSFS_CPLD2}/sfp28_p14_abs"            #14
            "${SYSFS_CPLD2}/sfp28_p15_abs"            #15
            "${SYSFS_CPLD2}/sfp28_p16_abs"            #16
            "${SYSFS_CPLD2}/sfp28_p17_abs"            #17
            "${SYSFS_CPLD2}/sfp28_p18_abs"            #18
            "${SYSFS_CPLD2}/sfp28_p19_abs"            #19
            "${SYSFS_CPLD2}/sfp28_p20_abs"            #20
            "${SYSFS_CPLD2}/sfp28_p21_abs"            #21
            "${SYSFS_CPLD2}/sfp28_p22_abs"            #22
            "${SYSFS_CPLD2}/sfp28_p23_abs"            #23
            "${SYSFS_CPLD2}/sfp28_p24_abs"            #24
            "${SYSFS_CPLD2}/sfp28_p25_abs"            #25
            "${SYSFS_CPLD2}/sfp28_p26_abs"            #26
            "${SYSFS_CPLD2}/sfp28_p27_abs"            #27
            "${SYSFS_CPLD3}/sfp28_p28_abs"            #28
            "${SYSFS_CPLD3}/sfp28_p29_abs"            #29
            "${SYSFS_CPLD3}/sfp28_p30_abs"            #30
            "${SYSFS_CPLD3}/sfp28_p31_abs"            #31
            "${SYSFS_CPLD3}/sfp28_p32_abs"            #32
            "${SYSFS_CPLD3}/sfp28_p33_abs"            #33
            "${SYSFS_CPLD3}/sfp28_p34_abs"            #34
            "${SYSFS_CPLD3}/sfp28_p35_abs"            #35
            "${SYSFS_CPLD3}/sfp28_p36_abs"            #36
            "${SYSFS_CPLD3}/sfp28_p37_abs"            #37
            "${SYSFS_CPLD3}/sfp28_p38_abs"            #38
            "${SYSFS_CPLD3}/sfp28_p39_abs"            #39
            "${SYSFS_CPLD3}/sfp56_p40_abs"            #40
            "${SYSFS_CPLD3}/sfp56_p41_abs"            #41
            "${SYSFS_CPLD3}/sfp56_p42_abs"            #42
            "${SYSFS_CPLD3}/sfp56_p43_abs"            #43
            "${SYSFS_CPLD3}/sfp56_p44_abs"            #44
            "${SYSFS_CPLD3}/sfp56_p45_abs"            #45
            "${SYSFS_CPLD3}/sfp56_p46_abs"            #46
            "${SYSFS_CPLD3}/sfp56_p47_abs"            #47
            "${SYSFS_CPLD3}/qsfp28_p48_abs"           #48
            "${SYSFS_CPLD3}/qsfp28_p49_abs"           #49
            "${SYSFS_CPLD3}/qsfp28_p50_abs"           #50
            "${SYSFS_CPLD3}/qsfp28_p51_abs"           #51
            "${SYSFS_CPLD3}/qsfpdd_p52_abs"           #52
            "${SYSFS_CPLD3}/qsfpdd_p53_abs"           #53
        )

         port_tx_fault_array=(
            "${SYSFS_CPLD2}/sfp28_p0_tx_flt"            #0
            "${SYSFS_CPLD2}/sfp28_p1_tx_flt"            #1
            "${SYSFS_CPLD2}/sfp28_p2_tx_flt"            #2
            "${SYSFS_CPLD2}/sfp28_p3_tx_flt"            #3
            "${SYSFS_CPLD2}/sfp28_p4_tx_flt"            #4
            "${SYSFS_CPLD2}/sfp28_p5_tx_flt"            #5
            "${SYSFS_CPLD2}/sfp28_p6_tx_flt"            #6
            "${SYSFS_CPLD2}/sfp28_p7_tx_flt"            #7
            "${SYSFS_CPLD2}/sfp28_p8_tx_flt"            #8
            "${SYSFS_CPLD2}/sfp28_p9_tx_flt"            #9
            "${SYSFS_CPLD2}/sfp28_p10_tx_flt"           #10
            "${SYSFS_CPLD2}/sfp28_p11_tx_flt"           #11
            "${SYSFS_CPLD2}/sfp28_p12_tx_flt"           #12
            "${SYSFS_CPLD2}/sfp28_p13_tx_flt"           #13
            "${SYSFS_CPLD2}/sfp28_p14_tx_flt"           #14
            "${SYSFS_CPLD2}/sfp28_p15_tx_flt"           #15
            "${SYSFS_CPLD2}/sfp28_p16_tx_flt"           #16
            "${SYSFS_CPLD2}/sfp28_p17_tx_flt"           #17
            "${SYSFS_CPLD2}/sfp28_p18_tx_flt"           #18
            "${SYSFS_CPLD2}/sfp28_p19_tx_flt"           #19
            "${SYSFS_CPLD2}/sfp28_p20_tx_flt"           #20
            "${SYSFS_CPLD2}/sfp28_p21_tx_flt"           #21
            "${SYSFS_CPLD2}/sfp28_p22_tx_flt"           #22
            "${SYSFS_CPLD2}/sfp28_p23_tx_flt"           #23
            "${SYSFS_CPLD2}/sfp28_p24_tx_flt"           #24
            "${SYSFS_CPLD2}/sfp28_p25_tx_flt"           #25
            "${SYSFS_CPLD2}/sfp28_p26_tx_flt"           #26
            "${SYSFS_CPLD2}/sfp28_p27_tx_flt"           #27
            "${SYSFS_CPLD3}/sfp28_p28_tx_flt"           #28
            "${SYSFS_CPLD3}/sfp28_p29_tx_flt"           #29
            "${SYSFS_CPLD3}/sfp28_p30_tx_flt"           #30
            "${SYSFS_CPLD3}/sfp28_p31_tx_flt"           #31
            "${SYSFS_CPLD3}/sfp28_p32_tx_flt"           #32
            "${SYSFS_CPLD3}/sfp28_p33_tx_flt"           #33
            "${SYSFS_CPLD3}/sfp28_p34_tx_flt"           #34
            "${SYSFS_CPLD3}/sfp28_p35_tx_flt"           #35
            "${SYSFS_CPLD3}/sfp28_p36_tx_flt"           #36
            "${SYSFS_CPLD3}/sfp28_p37_tx_flt"           #37
            "${SYSFS_CPLD3}/sfp28_p38_tx_flt"           #38
            "${SYSFS_CPLD3}/sfp28_p39_tx_flt"           #39
            "${SYSFS_CPLD3}/sfp56_p40_tx_flt"           #40
            "${SYSFS_CPLD3}/sfp56_p41_tx_flt"           #41
            "${SYSFS_CPLD3}/sfp56_p42_tx_flt"           #42
            "${SYSFS_CPLD3}/sfp56_p43_tx_flt"           #43
            "${SYSFS_CPLD3}/sfp56_p44_tx_flt"           #44
            "${SYSFS_CPLD3}/sfp56_p45_tx_flt"           #45
            "${SYSFS_CPLD3}/sfp56_p46_tx_flt"           #46
            "${SYSFS_CPLD3}/sfp56_p47_tx_flt"           #47
            ""                                          #48
            ""                                          #49
            ""                                          #50
            ""                                          #51
            ""                                          #52
            ""                                          #53
        )

        port_rx_los_array=(
            "${SYSFS_CPLD2}/sfp28_p0_rx_los"            #0
            "${SYSFS_CPLD2}/sfp28_p1_rx_los"            #1
            "${SYSFS_CPLD2}/sfp28_p2_rx_los"            #2
            "${SYSFS_CPLD2}/sfp28_p3_rx_los"            #3
            "${SYSFS_CPLD2}/sfp28_p4_rx_los"            #4
            "${SYSFS_CPLD2}/sfp28_p5_rx_los"            #5
            "${SYSFS_CPLD2}/sfp28_p6_rx_los"            #6
            "${SYSFS_CPLD2}/sfp28_p7_rx_los"            #7
            "${SYSFS_CPLD2}/sfp28_p8_rx_los"            #8
            "${SYSFS_CPLD2}/sfp28_p9_rx_los"            #9
            "${SYSFS_CPLD2}/sfp28_p10_rx_los"           #10
            "${SYSFS_CPLD2}/sfp28_p11_rx_los"           #11
            "${SYSFS_CPLD2}/sfp28_p12_rx_los"           #12
            "${SYSFS_CPLD2}/sfp28_p13_rx_los"           #13
            "${SYSFS_CPLD2}/sfp28_p14_rx_los"           #14
            "${SYSFS_CPLD2}/sfp28_p15_rx_los"           #15
            "${SYSFS_CPLD2}/sfp28_p16_rx_los"           #16
            "${SYSFS_CPLD2}/sfp28_p17_rx_los"           #17
            "${SYSFS_CPLD2}/sfp28_p18_rx_los"           #18
            "${SYSFS_CPLD2}/sfp28_p19_rx_los"           #19
            "${SYSFS_CPLD2}/sfp28_p20_rx_los"           #20
            "${SYSFS_CPLD2}/sfp28_p21_rx_los"           #21
            "${SYSFS_CPLD2}/sfp28_p22_rx_los"           #22
            "${SYSFS_CPLD2}/sfp28_p23_rx_los"           #23
            "${SYSFS_CPLD2}/sfp28_p24_rx_los"           #24
            "${SYSFS_CPLD2}/sfp28_p25_rx_los"           #25
            "${SYSFS_CPLD2}/sfp28_p26_rx_los"           #26
            "${SYSFS_CPLD2}/sfp28_p27_rx_los"           #27
            "${SYSFS_CPLD3}/sfp28_p28_rx_los"           #28
            "${SYSFS_CPLD3}/sfp28_p29_rx_los"           #29
            "${SYSFS_CPLD3}/sfp28_p30_rx_los"           #30
            "${SYSFS_CPLD3}/sfp28_p31_rx_los"           #31
            "${SYSFS_CPLD3}/sfp28_p32_rx_los"           #32
            "${SYSFS_CPLD3}/sfp28_p33_rx_los"           #33
            "${SYSFS_CPLD3}/sfp28_p34_rx_los"           #34
            "${SYSFS_CPLD3}/sfp28_p35_rx_los"           #35
            "${SYSFS_CPLD3}/sfp28_p36_rx_los"           #36
            "${SYSFS_CPLD3}/sfp28_p37_rx_los"           #37
            "${SYSFS_CPLD3}/sfp28_p38_rx_los"           #38
            "${SYSFS_CPLD3}/sfp28_p39_rx_los"           #39
            "${SYSFS_CPLD3}/sfp56_p40_rx_los"           #40
            "${SYSFS_CPLD3}/sfp56_p41_rx_los"           #41
            "${SYSFS_CPLD3}/sfp56_p42_rx_los"           #42
            "${SYSFS_CPLD3}/sfp56_p43_rx_los"           #43
            "${SYSFS_CPLD3}/sfp56_p44_rx_los"           #44
            "${SYSFS_CPLD3}/sfp56_p45_rx_los"           #45
            "${SYSFS_CPLD3}/sfp56_p46_rx_los"           #46
            "${SYSFS_CPLD3}/sfp56_p47_rx_los"           #47
            ""                                          #48
            ""                                          #49
            ""                                          #50
            ""                                          #51
            ""                                          #52
            ""                                          #53
        )

        port_tx_dis_array=(
            "${SYSFS_CPLD2}/sfp28_p0_tx_dis"            #0
            "${SYSFS_CPLD2}/sfp28_p1_tx_dis"            #1
            "${SYSFS_CPLD2}/sfp28_p2_tx_dis"            #2
            "${SYSFS_CPLD2}/sfp28_p3_tx_dis"            #3
            "${SYSFS_CPLD2}/sfp28_p4_tx_dis"            #4
            "${SYSFS_CPLD2}/sfp28_p5_tx_dis"            #5
            "${SYSFS_CPLD2}/sfp28_p6_tx_dis"            #6
            "${SYSFS_CPLD2}/sfp28_p7_tx_dis"            #7
            "${SYSFS_CPLD2}/sfp28_p8_tx_dis"            #8
            "${SYSFS_CPLD2}/sfp28_p9_tx_dis"            #9
            "${SYSFS_CPLD2}/sfp28_p10_tx_dis"           #10
            "${SYSFS_CPLD2}/sfp28_p11_tx_dis"           #11
            "${SYSFS_CPLD2}/sfp28_p12_tx_dis"           #12
            "${SYSFS_CPLD2}/sfp28_p13_tx_dis"           #13
            "${SYSFS_CPLD2}/sfp28_p14_tx_dis"           #14
            "${SYSFS_CPLD2}/sfp28_p15_tx_dis"           #15
            "${SYSFS_CPLD2}/sfp28_p16_tx_dis"           #16
            "${SYSFS_CPLD2}/sfp28_p17_tx_dis"           #17
            "${SYSFS_CPLD2}/sfp28_p18_tx_dis"           #18
            "${SYSFS_CPLD2}/sfp28_p19_tx_dis"           #19
            "${SYSFS_CPLD2}/sfp28_p20_tx_dis"           #20
            "${SYSFS_CPLD2}/sfp28_p21_tx_dis"           #21
            "${SYSFS_CPLD2}/sfp28_p22_tx_dis"           #22
            "${SYSFS_CPLD2}/sfp28_p23_tx_dis"           #23
            "${SYSFS_CPLD2}/sfp28_p24_tx_dis"           #24
            "${SYSFS_CPLD2}/sfp28_p25_tx_dis"           #25
            "${SYSFS_CPLD2}/sfp28_p26_tx_dis"           #26
            "${SYSFS_CPLD2}/sfp28_p27_tx_dis"           #27
            "${SYSFS_CPLD3}/sfp28_p28_tx_dis"           #28
            "${SYSFS_CPLD3}/sfp28_p29_tx_dis"           #29
            "${SYSFS_CPLD3}/sfp28_p30_tx_dis"           #30
            "${SYSFS_CPLD3}/sfp28_p31_tx_dis"           #31
            "${SYSFS_CPLD3}/sfp28_p32_tx_dis"           #32
            "${SYSFS_CPLD3}/sfp28_p33_tx_dis"           #33
            "${SYSFS_CPLD3}/sfp28_p34_tx_dis"           #34
            "${SYSFS_CPLD3}/sfp28_p35_tx_dis"           #35
            "${SYSFS_CPLD3}/sfp28_p36_tx_dis"           #36
            "${SYSFS_CPLD3}/sfp28_p37_tx_dis"           #37
            "${SYSFS_CPLD3}/sfp28_p38_tx_dis"           #38
            "${SYSFS_CPLD3}/sfp28_p39_tx_dis"           #39
            "${SYSFS_CPLD3}/sfp56_p40_tx_dis"           #40
            "${SYSFS_CPLD3}/sfp56_p41_tx_dis"           #41
            "${SYSFS_CPLD3}/sfp56_p42_tx_dis"           #42
            "${SYSFS_CPLD3}/sfp56_p43_tx_dis"           #43
            "${SYSFS_CPLD3}/sfp56_p44_tx_dis"           #44
            "${SYSFS_CPLD3}/sfp56_p45_tx_dis"           #45
            "${SYSFS_CPLD3}/sfp56_p46_tx_dis"           #46
            "${SYSFS_CPLD3}/sfp56_p47_tx_dis"           #47
            ""                                          #48
            ""                                          #49
            ""                                          #50
            ""                                          #51
            ""                                          #52
            ""                                          #53       
        )

        port_lp_mode_array=(
            ""                                           #0
            ""                                           #1
            ""                                           #2
            ""                                           #3
            ""                                           #4
            ""                                           #5
            ""                                           #6
            ""                                           #7
            ""                                           #8
            ""                                           #9
            ""                                          #10
            ""                                          #11
            ""                                          #12
            ""                                          #13
            ""                                          #14
            ""                                          #15
            ""                                          #16
            ""                                          #17
            ""                                          #18
            ""                                          #19
            ""                                          #20
            ""                                          #21
            ""                                          #22
            ""                                          #23
            ""                                          #24
            ""                                          #25
            ""                                          #26
            ""                                          #27
            ""                                          #28
            ""                                          #29
            ""                                          #30
            ""                                          #31
            ""                                          #32
            ""                                          #33
            ""                                          #34
            ""                                          #35
            ""                                          #36
            ""                                          #37
            ""                                          #38
            ""                                          #39
            ""                                          #40
            ""                                          #41
            ""                                          #42
            ""                                          #43
            ""                                          #44
            ""                                          #45
            ""                                          #46
            ""                                          #47
            "${SYSFS_CPLD3}/qsfp28_p48_lp_mode"         #48
            "${SYSFS_CPLD3}/qsfp28_p49_lp_mode"         #49
            "${SYSFS_CPLD3}/qsfp28_p50_lp_mode"         #50
            "${SYSFS_CPLD3}/qsfp28_p51_lp_mode"         #51
            "${SYSFS_CPLD3}/qsfpdd_p52_lp_mode"         #52
            "${SYSFS_CPLD3}/qsfpdd_p53_lp_mode"         #53
        )

        port_reset_array=(
            ""                                           #0
            ""                                           #1
            ""                                           #2
            ""                                           #3
            ""                                           #4
            ""                                           #5
            ""                                           #6
            ""                                           #7
            ""                                           #8
            ""                                           #9
            ""                                          #10
            ""                                          #11
            ""                                          #12
            ""                                          #13
            ""                                          #14
            ""                                          #15
            ""                                          #16
            ""                                          #17
            ""                                          #18
            ""                                          #19
            ""                                          #20
            ""                                          #21
            ""                                          #22
            ""                                          #23
            ""                                          #24
            ""                                          #25
            ""                                          #26
            ""                                          #27
            ""                                          #28
            ""                                          #29
            ""                                          #30
            ""                                          #31
            ""                                          #32
            ""                                          #33
            ""                                          #34
            ""                                          #35
            ""                                          #36
            ""                                          #37
            ""                                          #38
            ""                                          #39
            ""                                          #40
            ""                                          #41
            ""                                          #42
            ""                                          #43
            ""                                          #44
            ""                                          #45
            ""                                          #46
            ""                                          #47
            "${SYSFS_CPLD3}/qsfp28_p48_rst"             #48
            "${SYSFS_CPLD3}/qsfp28_p49_rst"             #49
            "${SYSFS_CPLD3}/qsfp28_p50_rst"             #50
            "${SYSFS_CPLD3}/qsfp28_p51_rst"             #51
            "${SYSFS_CPLD3}/qsfpdd_p52_rst"             #52
            "${SYSFS_CPLD3}/qsfpdd_p53_rst"             #53
        )

        port_intr_array=(
            ""                                           #0
            ""                                           #1
            ""                                           #2
            ""                                           #3
            ""                                           #4
            ""                                           #5
            ""                                           #6
            ""                                           #7
            ""                                           #8
            ""                                           #9
            ""                                          #10
            ""                                          #11
            ""                                          #12
            ""                                          #13
            ""                                          #14
            ""                                          #15
            ""                                          #16
            ""                                          #17
            ""                                          #18
            ""                                          #19
            ""                                          #20
            ""                                          #21
            ""                                          #22
            ""                                          #23
            ""                                          #24
            ""                                          #25
            ""                                          #26
            ""                                          #27
            ""                                          #28
            ""                                          #29
            ""                                          #30
            ""                                          #31
            ""                                          #32
            ""                                          #33
            ""                                          #34
            ""                                          #35
            ""                                          #36
            ""                                          #37
            ""                                          #38
            ""                                          #39
            ""                                          #40
            ""                                          #41
            ""                                          #42
            ""                                          #43
            ""                                          #44
            ""                                          #45
            ""                                          #46
            ""                                          #47
            "${SYSFS_CPLD3}/qsfp28_p48_intr"            #48
            "${SYSFS_CPLD3}/qsfp28_p49_intr"            #49
            "${SYSFS_CPLD3}/qsfp28_p50_intr"            #50
            "${SYSFS_CPLD3}/qsfp28_p51_intr"            #51
            "${SYSFS_CPLD3}/qsfpdd_p52_intr"            #52 
            "${SYSFS_CPLD3}/qsfpdd_p53_intr"            #53  
        )

        port_eeprom_bus_array=(
        #    0     1     2     3     4     5     6     7
             18    19    20    21    22    23    24    25
        #    8     9     10    11    12    13    14    15
             26    27    28    29    30    31    32    33
        #    16    17    18    19    20    21    22    23
             34    35    36    37    38    39    40    41
        #    24    25    26    27    28    29    30    31
             42    43    44    45    46    47    48    49
        #    32    33    34    35    36    37    38    39
             50    51    52    53    54    55    56    57
        #    40    41    42    43    44    45    46    47
             58    59    60    61    62    63    64    65
        #    48    49    50    51    52    53    
             66    67    68    69    70    71   
        )

        # original port_abs
        for (( i=0; i<${#port_name_array[@]}; i++ ))
        do
            local reg=""
            # Port Absent Status (0: Present, 1:Absence)
            local sysfs_path=${port_absent_array[i]}
            
            if [ "${port_absent_array[${i}]}" != "-1" ] && _check_filepath ${sysfs_path}; then
                reg=$(cat "${sysfs_path}")
                port_absent="${reg}"
                _echo "[Port${port_name_array[${i}]} Module Absent]: ${port_absent}"
            fi

            # Port Tx Fault Status (0:normal, 1:tx fault)
            
            sysfs_path=${port_tx_fault_array[i]}

            if [ "${port_type_array[${i}]}" == "${SFP}" ] && _check_filepath ${sysfs_path}; then
                reg=$(cat "${sysfs_path}")
                port_tx_fault="${reg}"
                _echo "[Port${port_name_array[${i}]} Tx Fault Status]: ${port_tx_fault}"
            fi

            # Port Rx Loss Status (0:los undetected, 1: los detected)
            sysfs_path=${port_rx_los_array[i]}

            if [ "${port_type_array[${i}]}" == "${SFP}" ] && _check_filepath ${sysfs_path}; then
                reg=$(cat "${sysfs_path}")
                port_rx_loss="${reg}"
                _echo "[Port${port_name_array[${i}]} Rx Loss Status]: ${port_rx_loss}"
            fi

            # Port Tx Disable Status (0:enable tx, 1: disable tx)
            sysfs_path=${port_tx_dis_array[i]}

            if [ "${port_type_array[${i}]}" == "${SFP}" ] && _check_filepath ${sysfs_path}; then
                reg=$(eval "cat ${sysfs_path}")
                port_tx_dis="${reg}"
                _echo "[Port${port_name_array[${i}]} Tx Disable Status]: ${port_tx_dis}"
            fi

            # Port Lower Power Mode Status (0: Normal Power Mode, 1:Low Power Mode)
            sysfs_path=${port_lp_mode_array[i]}

            if [[ "${port_type_array[${i}]}" == "${QSFPDD}" || "${port_type_array[${i}]}" == "${QSFP}" ]] && _check_filepath ${sysfs_path}; then
                reg=$(cat "${sysfs_path}")
                port_lp_mode="${reg}"
                _echo "[Port${port_name_array[${i}]} Low Power Mode]: ${port_lp_mode}"
            fi

            # Port Reset Status (0:Reset, 1:Normal)
            sysfs_path=${port_reset_array[i]}

            if [[ "${port_type_array[${i}]}" == "${QSFPDD}" || "${port_type_array[${i}]}" == "${QSFP}" ]] && _check_filepath ${sysfs_path}; then
                reg=$(cat "${sysfs_path}")
                port_reset="${reg}"
                _echo "[Port${port_name_array[${i}]} Reset Status]: ${port_reset}"
            fi

            # Port Interrupt Status (0: Interrupted, 1:No Interrupt)
            sysfs_path=${port_intr_array[i]}

            if [[ "${port_type_array[${i}]}" == "${QSFPDD}" || "${port_type_array[${i}]}" == "${QSFP}" ]] && _check_filepath ${sysfs_path}; then
                reg=$(cat "${sysfs_path}")
                port_intr_l="${reg}"
                _echo "[Port${port_name_array[${i}]} Interrupt Status (L)]: ${port_intr_l}"
            fi

            # Port Dump EEPROM
            local eeprom_path="/sys/bus/i2c/devices/${port_eeprom_bus_array[${i}]}-0050/eeprom"

            if [ "${port_absent}" == "0" ] && _check_filepath "${eeprom_path}"; then
                port_eeprom=$(eval "dd if=${eeprom_path} bs=128 count=2 skip=0 status=none ${LOG_REDIRECT} | hexdump -C")
                if [ "${LOG_FILE_ENABLE}" == "1" ]; then

                    if [ "${port_type_array[${i}]}" == "${QSFPDD}" ]; then
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

        local sysfs_attr=(
            "psu_0_led_status"
            "psu_1_led_status"
            "system_led_status"
            "sync_led_status"
            "fan_led_status"
            "id_led_status"      
        )

        local desc_color=("Yellow" "Green" "Blue")
        local desc_speed=("0.5 Hz" "2 Hz")
        local desc_blink=("Solid" "Blink")
        local desc_onoff=("OFF" "ON")

        local led=(          "Sync " "Sys  " "Fan  " "PSU 0" "PSU 1" "ID")
        local led_sysfs_idx=(  3       2       4       0       1    5)
        local color=(          0       0       0       0       0    -1)
        local blink=(          2       2       2       2       2    1)
        local onoff=(          3       3       3       3       3    2)
        local freq=(           1       1       1       1       1    0)

        for (( i=0; i<${#led[@]}; i++ ))
        do
            idx=${led_sysfs_idx[i]}
            if _check_filepath "${SYSFS_CPLD1}/${sysfs_attr[idx]}"; then
                led_reg=$(eval "cat ${SYSFS_CPLD1}/${sysfs_attr[idx]} ${LOG_REDIRECT}")


                if [ "${color[i]}" != "-1" ]; then
                    color=$(((led_reg >> ${color[i]}) & 2#1)) # (0: yellow, 1: green)
                else
                    color=2 # (blue)
                fi

                blink=$(((led_reg >> ${blink[i]}) & 2#1)) # (0: 0.5 Hz, 1: 2 Hz)
                onoff=$(((led_reg >> ${onoff[i]}) & 2#1)) # (0: Solid,  1: Blink)
                speed=$(((led_reg >> ${freq[i]}) & 2#1)) # (0: off,    1: on)

                _echo "[System LED ${led[i]}]: [${sysfs_attr[idx]} ${led_reg}] [${desc_color[color]}][${desc_speed[speed]}][${desc_blink[blink]}][${desc_onoff[onoff]}]"

            else
                _echo "${led[i]}: ${SYSFS_CPLD1}/${sysfs_attr[idx]} not exist!!!"
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
    local sgg7=(           "A"   "B"   "C"   "D"   "E"   "F"   "G" )
    #                      502   501   496   498   497   500   499
    local sgg7_left_off=(  9     10    15    13    14    11    12 )
    #                      504   507   509   510   508   505   506
    local sgg7_right_off=( 7     4     2     1     3     6     5  )
    local sgg7_mum=( "0" "1" "2" "3" "4" "5" "6" "7" "8" "9" "A" "B" "C" "D" "E" "F")
    local sgg7_value=( 
            "0000001"  # 0
            "1001111"  # 1
            "0010010"  # 2
            "0000110"  # 3
            "1001100"  # 4
            "0100100"  # 5
            "0100000"  # 6
            "0001101"  # 7
            "0000000"  # 8
            "0001100"  # 9
            "0001000"  # A
            "1100000"  # B
            "0110001"  # C
            "1000010"  # D
            "0110000"  # E
            "0111000"  # F
        )

    local sgg7_left=""
    local sgg7_right=""
    local sgg7_left_num=""
    local sgg7_right_num=""

    for (( i=0; i<${#sgg7[@]}; i++))
    do
        num=$(cat /sys/class/gpio/gpio$(( GPIO_MAX - sgg7_left_off[i] ))/value)
        sgg7_left=$sgg7_left$num
        num=$(cat /sys/class/gpio/gpio$(( GPIO_MAX - sgg7_right_off[i] ))/value)
        sgg7_right=$sgg7_right$num
    done

    for (( i=0; i<${#sgg7_value[@]}; i++))
    do
        if [ "$sgg7_left" == "${sgg7_value[i]}" ]; then
            sgg7_left_num=${sgg7_mum[i]}
        fi

        if [ "$sgg7_right" == "${sgg7_value[i]}" ]; then
            sgg7_right_num=${sgg7_mum[i]}
        fi

        if [ "${sgg7_left_num}" != "" ] && [ "${sgg7_right_num}" != "" ]; then
            break
        fi
    done

    _echo "[Left Beacon LED pin A:B:C:D:E:F:G] : ${sgg7_left}"
    _echo "[Left Beacon LED number(0-F)]       : ${sgg7_left_num}"
    _echo "[Right Beacon LED pin A:B:C:D:E:F:G]: ${sgg7_right}"
    _echo "[Right Beacon LED number(0-F)]      : ${sgg7_right_num}"
    return 0
}

function _show_beacon_led {
    if [ "${BSP_INIT_FLAG}" == "1" ] && [ "${GPIO_MAX_INIT_FLAG}" == "1" ] ; then
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
        offset=$(( ${offset} + 1 ))
        reg=$(( ${base} + ${offset} ))
        reg=`printf "0x%X\n" ${reg}`
        _echo "${ret}"
    done

    
}

function _show_cpld_reg_sysfs {
    _banner "Show CPLD/FPGA Register"

    if [ "${MODEL_NAME}" != "${PLAT}" ]; then
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

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
    
    if _check_dirpath "$SYSFS_FPGA"; then
        reg_dump=$(eval "i2cdump -f -y 2 0x37 ${LOG_REDIRECT}")
        _echo "[FPGA   Register]:"
        _echo "${reg_dump}"
    fi


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

    _echo "[Command]: lsusb -v"
    ret=$(eval "lsusb -v ${LOG_REDIRECT}")
    _echo "${ret}"
    _echo ""


    _echo ""
    _echo "[Command]: find /sys/devices -name authorized* -exec tail -n +1 {} +"
    ret=$(eval "find /sys/devices -name authorized* -exec tail -n +1 {} + ${LOG_REDIRECT}")
    _echo "${ret}"
    _echo ""

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

    cmd_array=("lsblk"
                "lsblk -O"
               "parted -l"
               "fdisk -l /dev/sda"
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
    echo "    $f [-d D_DIR] [-i identifier] [-v]"
    echo "Description:"
    echo "  -d                specify D_DIR as log destination instead of default path /tmp/log"
    echo "  -v                show tech support script version"
    echo "Example:"
    echo "    $f -d /var/log"
    echo "    $f -i identifier"
    echo "    $f -v"
    exit -1
}

function _getopts {
    local OPTSTRING=":d:fi:v"
    # default log dir
    local log_folder_root="/tmp/log"

    while getopts ${OPTSTRING} opt; do
        case ${opt} in
            d)
              log_folder_root=${OPTARG}
              ;;
            f)
              LOG_FAST=${TRUE}
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
#    _show_beacon_led 
    _show_ioport
    _show_cpld_reg
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
    _show_time
    _compression

    echo "#   The tech-support collection is completed. Please share the tech support log file."
}

_getopts $@
_main
