#!/bin/bash

#Tech Support script version
TS_VERSION="1.1.1"

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

# MODEL_NAME: set by function _board_info
MODEL_NAME=""
# HW_REV: set by function _board_info
HW_REV=""
# BSP_INIT_FLAG: set by function _check_bsp_init
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

# GPIO_MAX: update by function _update_gpio_max
GPIO_MAX=0
GPIO_MAX_INIT_FLAG=0

# I2C Bus
i801_bus=""
ismt_bus=""
CPLD_BUS=""
SFP_BUS_BASE=""

# Sysfs
SYSFS_DEV="/sys/bus/i2c/devices"
SYSFS_CPLD1=""
SYSFS_CPLD2=""
FMT_SYSFS_GPIO_VAL="/sys/class/gpio/gpio%s/value"
SYSFS_LPC="/sys/devices/platform/x86_64_ufispace_s8901_54xc_lpc"

# Execution Time
start_time=$(date +%s)
end_time=0
elapsed_time=0

# Port
SFP_PORTS=48
QSFP_PORTS=6

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
    _banner "Update GPIO MAX"
    local sysfs="${SYSFS_LPC}/bsp/bsp_gpio_max"

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
        _update_gpio_max
    fi

    # init sysfs
    _init_sysfs
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
    if [ "${OPT_BYPASS_I2C_COMMAND}" == "${TRUE}" ]; then
        return
    fi

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

    CPLD_BUS=$((ismt_bus+1))
    # i801, ismt and 9548*3 = 26
    SFP_BUS_BASE=$((2+8*3))
}

function _init_sysfs {
    SYSFS_CPLD1="${SYSFS_DEV}/${CPLD_BUS}-0030"
    SYSFS_CPLD2="${SYSFS_DEV}/${CPLD_BUS}-0031"
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
    model_id_array=($((2#11111111)))
    model_name_array=("S8901-54XC")

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
    model_id=$(((model_id & 2#11111111) >> 0))
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

    bios_ver=$(eval "cat /sys/class/dmi/id/bios_version ${LOG_REDIRECT}")
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

function _cpld_version_i2c {
    if [ "${OPT_BYPASS_I2C_COMMAND}" == "${TRUE}" ]; then
        _banner "Show CPLD Version (I2C) (Bypass)"
        return
    fi

    _banner "Show CPLD Version (I2C)"

    if [[ $MODEL_NAME == *"S8901-54XC"* ]]; then

        local mux_i2c_bus=${ismt_bus}
        local mux_i2c_addr=0x70

        # MB CPLD
        mb_cpld1_ver=""
        mb_cpld2_ver=""
        mb_cpld1_build=""
        mb_cpld2_build=""

        # CPLD 1-2

        _check_i2c_device ${mux_i2c_bus} ${mux_i2c_addr}
        value=$(eval "i2cget -y -f ${ismt_bus} 0x75 ${LOG_REDIRECT}")
        ret=$?

        if [ ${ret} -eq 0 ]; then
            i2cset -y -f ${mux_i2c_bus} ${mux_i2c_addr} 0x1
            _check_i2c_device ${mux_i2c_bus} "0x30"
            _check_i2c_device ${mux_i2c_bus} "0x31"
            mb_cpld1_ver=$(eval "i2cget -y -f ${mux_i2c_bus} 0x30 0x2 ${LOG_REDIRECT}")
            mb_cpld2_ver=$(eval "i2cget -y -f ${mux_i2c_bus} 0x31 0x2 ${LOG_REDIRECT}")
            mb_cpld1_build=$(eval "i2cget -y -f ${mux_i2c_bus} 0x30 0x4 ${LOG_REDIRECT}")
            mb_cpld2_build=$(eval "i2cget -y -f ${mux_i2c_bus} 0x31 0x4 ${LOG_REDIRECT}")
            i2cset -y -f ${mux_i2c_bus} ${mux_i2c_addr} 0x0
        fi

        _printf "[MB CPLD1 Version]: %d.%02d.%03d\n" $(( (mb_cpld1_ver & 2#11000000) >> 6)) $(( mb_cpld1_ver & 2#00111111 )) $((mb_cpld1_build))
        _printf "[MB CPLD2 Version]: %d.%02d.%03d\n" $(( (mb_cpld2_ver & 2#11000000) >> 6)) $(( mb_cpld2_ver & 2#00111111 )) $((mb_cpld2_build))
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _cpld_version_sysfs {
    _banner "Show CPLD Version (Sysfs)"

    if [ "${MODEL_NAME}" == "S8901-54XC" ]; then
        # MB CPLD
        _check_filepath "/sys/bus/i2c/devices/${CPLD_BUS}-0030/cpld_version_h"
        _check_filepath "/sys/bus/i2c/devices/${CPLD_BUS}-0031/cpld_version_h"
        mb_cpld1_ver_h=$(eval "cat /sys/bus/i2c/devices/${CPLD_BUS}-0030/cpld_version_h ${LOG_REDIRECT}")
        mb_cpld2_ver_h=$(eval "cat /sys/bus/i2c/devices/${CPLD_BUS}-0031/cpld_version_h ${LOG_REDIRECT}")
        _echo "[MB CPLD1 Version]: ${mb_cpld1_ver_h}"
        _echo "[MB CPLD2 Version]: ${mb_cpld2_ver_h}"
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

    if [ "${MODEL_NAME}" != "S8901-54XC" ]; then
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
    if [ "${OPT_BYPASS_I2C_COMMAND}" == "${TRUE}" ]; then
        _banner "Show I2C Tree Bus (Bypass)"
        return
    fi

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
    if [ "${OPT_BYPASS_I2C_COMMAND}" == "${TRUE}" ]; then
        _banner "Show I2C Tree Bus MUX (I2C) (Bypass)"
        return
    fi

    _banner "Show I2C Tree Bus MUX (I2C)"

    local i=0
    local chip_addr1=""
    local chip_addr2=""
    local bus=""

    if [ "${MODEL_NAME}" == "S8901-54XC" ]; then
        # i801_bus
        bus="${i801_bus}"
        chip_addr1="0x72"
        chip_addr2="0x73"

        _check_i2c_device "${bus}" "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then

            ## SFP_ROOT-0x72
            _show_i2c_mux_devices ${bus} "0x72" "8" "ROOT-0x72"

            for (( chip_addr1_chann=0; chip_addr1_chann<=6; chip_addr1_chann++ ))
            do
                # open mux channel - 0x72 (chip_addr1)
                i2cset -y ${bus} ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
                _show_i2c_mux_devices ${bus} "${chip_addr2}" "8" "SFP_ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}"
                # close mux channel - 0x72 (chip_addr1)
                i2cset -y ${bus} ${chip_addr1} 0x0
            done
        fi

        # ismt_bus
        bus="${ismt_bus}"

        # 9548_CPLD
        chip_addr1="0x70"
        _check_i2c_device "${bus}" "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            _show_i2c_mux_devices "${bus}" "${chip_addr1}" "8" "(9548_CPLD)-${chip_addr1}"
        fi

        # 9548_FRU
        chip_addr1="0x71"
        _check_i2c_device "${bus}" "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            _show_i2c_mux_devices "${bus}" "${chip_addr1}" "8" "(9548_CPLD)-${chip_addr1}"
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
    if [ "${OPT_BYPASS_I2C_COMMAND}" == "${TRUE}" ]; then
        _banner "Show I2C Device Info (Bypass)"
        return
    fi

    _banner "Show I2C Device Info"

    local pca954x_device_id=("")
    local pca954x_device_bus=("")
    if [ "${MODEL_NAME}" == "S8901-54XC" ]; then
        # i801_bus
        pca954x_device_bus=${i801_bus}
        pca954x_device_id=("0x72")

        for ((i=0;i<5;i++))
        do
            _echo "[DEV PCA9548 (${i})]"
            for (( j=0; j<${#pca954x_device_id[@]}; j++ ))
            do
                ret=`i2cget -f -y ${pca954x_device_bus} ${pca954x_device_id[$j]}`
                _echo "[I2C Device ${pca954x_device_id[$j]}]: $ret"
            done
            sleep 0.4
        done

        # ismt_bus

        pca954x_device_bus=${ismt_bus}
        pca954x_device_id=("0x70" "0x71")

        for ((i=0;i<5;i++))
        do
            _echo "[DEV PCA9548 (${i})]"
            for (( j=0; j<${#pca954x_device_id[@]}; j++ ))
            do
                ret=`i2cget -f -y ${pca954x_device_bus} ${pca954x_device_id[$j]}`
                _echo "[I2C Device ${pca954x_device_id[$j]}]: $ret"
            done
            sleep 0.4
        done
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
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
    if [ "${OPT_BYPASS_I2C_COMMAND}" == "${TRUE}" ]; then
        _banner "Show System EEPROM (Bypass)"
        return
    fi

    _banner "Show System EEPROM"

    mux_addr="0x70"
    eeprom_addr="0x53"

    #open mux
    i2cset -y ${ismt_bus} ${mux_addr} 0x08

    #read six times return empty content
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")

    #seventh read return correct content
    cpu_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")

    #close mux
    i2cset -y ${ismt_bus} ${mux_addr} 0x00

    _echo "[CPU EEPROM]:"
    _echo "${cpu_eeprom}"
}

function _show_sys_eeprom_sysfs {
    _banner "Show System EEPROM"

    local bus="5"
    local addr="0053"
    sys_eeprom=$(eval "cat /sys/bus/i2c/devices/${bus}-${addr}/eeprom ${LOG_REDIRECT} | hexdump -C")
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

    max_gpio=`ls /sys/class/gpio/ | grep "gpio[[:digit:]]" | sed --expression='s/gpio//g' | sort -V | tail -n 1`
    min_gpio=`ls /sys/class/gpio/ | grep "gpio[[:digit:]]" | sed --expression='s/gpio//g' | sort -V | head -n 1`

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
    if [ "${MODEL_NAME}" != "S8901-54XC" ]; then
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    # Read PSU Status
    _check_filepath "/sys/bus/i2c/devices/${CPLD_BUS}-0030/cpld_psu_status"
    cpld_psu_status_reg=$(eval "cat /sys/bus/i2c/devices/${CPLD_BUS}-0030/cpld_psu_status ${LOG_REDIRECT}")

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

function _show_sfp_status_sysfs {
    _banner "Show SFP Status"

    local sysfs_idx_array=("0_7" "8_15" "16_23" "24_31" "32_39" "40_47")
    #local rx_rs_base=464
    #local tx_rs_base=416
    local rx_rs_base=$((GPIO_MAX - 47))
    local tx_rs_base=$((GPIO_MAX - 95))

    if [[ $MODEL_NAME != *"S8901-54XC"* ]]; then
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    for (( i=0; i<${SFP_PORTS}; i++ ))
    do
        local sysfs_idx=${sysfs_idx_array[$(($i / 8))]}
        local bit_shift=$(($i % 8))
        local sysfs_path=""

        # Port Present (0: Present, 1:Absence)
        sysfs_path="${SYSFS_CPLD2}/cpld_sfp_intr_present_${sysfs_idx}"
        _check_filepath ${sysfs_path}
        port_absent_reg=$(eval "cat ${sysfs_path} ${LOG_REDIRECT}")
        port_absent=$(( (port_absent_reg >> ${bit_shift}) & 2#00000001 ))

        # Port Tx Fault    (0:normal, 1:tx fault)
        sysfs_path="${SYSFS_CPLD2}/cpld_sfp_intr_tx_fault_${sysfs_idx}"
        _check_filepath ${sysfs_path}
        port_tx_fault_reg=$(eval "cat ${sysfs_path} ${LOG_REDIRECT}")
        port_tx_fault=$(( (port_tx_fault_reg >> ${bit_shift}) & 2#00000001 ))

        # Port Rx LOS    (0:los undetected, 1: los detected)
        sysfs_path="${SYSFS_CPLD2}/cpld_sfp_intr_rx_los_${sysfs_idx}"
        _check_filepath ${sysfs_path}
        port_rx_los_reg=$(eval "cat ${sysfs_path} ${LOG_REDIRECT}")
        port_rx_los=$(( (port_rx_los_reg >> ${bit_shift}) & 2#00000001 ))

        # Port Tx Disable (0:enable tx, 1: disable tx)
        sysfs_path="${SYSFS_CPLD2}/cpld_sfp_tx_disable_${sysfs_idx}"
        _check_filepath ${sysfs_path}
        port_tx_disable_reg=$(eval "cat ${sysfs_path} ${LOG_REDIRECT}")
        port_tx_disable=$(( (port_tx_disable_reg >> ${bit_shift}) & 2#00000001 ))

        # Port Rx Rate Select (0: low rate, 1:full rate)
        sysfs_path=$(printf ${FMT_SYSFS_GPIO_VAL} $(( rx_rs_base + i )))
        _check_filepath ${sysfs_path}
        port_rx_rate_sel=$(eval "cat ${sysfs_path}")

        # Port Tx Rate Select (0: low rate, 1:full rate)
        sysfs_path=$(printf ${FMT_SYSFS_GPIO_VAL} $(( tx_rs_base + i )))
        _check_filepath ${sysfs_path}
        port_tx_rate_sel=$(eval "cat ${sysfs_path}")

        # Show Port Status

        _echo "[Port${i} Present    ]: ${port_absent}"
        _echo "[Port${i} Tx Fault   ]: ${port_tx_fault}"
        _echo "[Port${i} Rx LOS     ]: ${port_rx_los}"
        _echo "[Port${i} Tx Disable ]: ${port_tx_disable}"
        _echo "[Port${i} Port Rx Rate Select]: ${port_rx_rate_sel}"
        _echo "[Port${i} Port Tx Rate Select]: ${port_tx_rate_sel}"
        _echo "[Port${i} Present Raw Reg   ]: ${port_absent_reg}"
        _echo "[Port${i} Tx Fault Raw Reg  ]: ${port_tx_fault_reg}"
        _echo "[Port${i} Rx LOS Raw Reg    ]: ${port_rx_los_reg}"
        _echo "[Port${i} Tx Disable Raw Reg]: ${port_tx_disable_reg}"

        # Port Dump EEPROM
        local eeprom_path="${SYSFS_DEV}/$((SFP_BUS_BASE + i))-0050/eeprom"
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
}

function _show_sfp_status {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_sfp_status_sysfs
    fi
}

function _show_qsfp_status_sysfs {
    _banner "Show QSFP Status"

    local sysfs_idx_array=("48_53")
    local port_start=0
    local port_end=${QSFP_PORTS}

    if [[ $MODEL_NAME != *"S8901-54XC"* ]]; then
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    for (( i=${port_start}; i<${port_end}; i++ ))
    do
        local sysfs_idx=${sysfs_idx_array[$(($i / 8))]}
        local bit_shift=$(($i % 8))
        local sysfs_path=""

        # Port Interrupt (0: Interrupted, 1:Normal)
        sysfs_path="${SYSFS_CPLD2}/cpld_qsfp_intr_port_${sysfs_idx}"
        _check_filepath ${sysfs_path}
        port_intr_l_reg=$(eval "cat ${sysfs_path} ${LOG_REDIRECT}")
        port_intr_l=$(( (port_intr_l_reg >> ${bit_shift}) & 2#00000001 ))

        # Port Absent Status (0: Present, 1:Absence)
        sysfs_path="${SYSFS_CPLD2}/cpld_qsfp_intr_present_${sysfs_idx}"
        _check_filepath ${sysfs_path}
        port_absent_l_reg=$(eval "cat ${sysfs_path} ${LOG_REDIRECT}")
        port_absent_l=$(( (port_absent_l_reg >> ${bit_shift}) & 2#00000001 ))

        # Low Power Mode Status (0: Normal Power Mode, 1:Low Power Mode)
        sysfs_path="${SYSFS_CPLD2}/cpld_qsfp_lpmode_${sysfs_idx}"
        _check_filepath ${sysfs_path}
        port_lp_mode_reg=$(eval "cat ${sysfs_path} ${LOG_REDIRECT}")
        port_lp_mode=$(( (port_lp_mode_reg >> ${bit_shift}) & 2#00000001 ))

        # Port Reset Status (0:Reset, 1:Normal)
        sysfs_path="${SYSFS_CPLD2}/cpld_qsfp_reset_${sysfs_idx}"
        _check_filepath ${sysfs_path}
        port_reset_l_reg=$(eval "cat ${sysfs_path} ${LOG_REDIRECT}")
        port_reset_l=$(( (port_reset_l_reg >> ${bit_shift}) & 2#00000001 ))

        # Show Port Status
        _echo "[Port$((i+SFP_PORTS)) Module INT (L)]: ${port_intr_l}"
        _echo "[Port$((i+SFP_PORTS)) Module Absent ]: ${port_absent_l}"
        _echo "[Port$((i+SFP_PORTS)) Low Power Mode]: ${port_lp_mode}"
        _echo "[Port$((i+SFP_PORTS)) Reset Status  ]: ${port_reset_l}"
        _echo "[Port$((i+SFP_PORTS)) Status Reg Raw       ]: ${port_intr_l_reg}"
        _echo "[Port$((i+SFP_PORTS)) Module Absent Reg Raw]: ${port_absent_l_reg}"
        _echo "[Port$((i+SFP_PORTS)) Config Reg Raw       ]: ${port_lp_mode_reg}"
        _echo "[Port$((i+SFP_PORTS)) Reset Reg Raw        ]: ${port_reset_l_reg}"

        # Port Dump EEPROM

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

        eeprom_path="/sys/bus/i2c/devices/$((SFP_BUS_BASE + SFP_PORTS + i))-0050/eeprom"
        _check_filepath ${eeprom_path}

        for (( page_i=0; page_i<${#eeprom_page_array[@]}; page_i++ ))
        do
            for (( repeate_i=0; repeate_i<${eeprom_repeat_array[page_i]}; repeate_i++ ))
            do
                if [ "${port_absent_l}" == "0" ]; then
                    eeprom_content=$(eval  "dd if=${eeprom_path} bs=128 count=1 skip=${eeprom_page_array[${page_i}]}  status=none ${LOG_REDIRECT} | hexdump -C")

                    if [ -z "$eeprom_content" ] && [ "${eeprom_repeat_array[page_i]}" == "0" ]; then
                        eeprom_content="ERROR!!! The result is empty. It should read failed ${eeprom_path}!!"
                    fi
                else
                    eeprom_content="N/A"
                fi
                _echo "[Port$((i+SFP_PORTS)) EEPROM $(_eeprom_page_desc ${eeprom_page_array[page_i]}) $(_eeprom_page_repeat_desc ${repeate_i} ${eeprom_repeat_array[page_i]})]:"
                _echo "${eeprom_content}"
            done
        done


    done
}

function _show_qsfp_status {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_qsfp_status_sysfs
    fi
}

function _show_cpu_temperature_sysfs {
    _banner "show CPU Temperature"

    for (( i=0; i<=16; i++ ))
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

function _show_cpld_interrupt_sysfs {
    _banner "Show CPLD Interrupt"

    if [[ $MODEL_NAME == *"S8901-54XC"* ]]; then
        cpld1_sysfs_array=("cpld_mac_intr" \
                           "cpld_hwm_intr" \
                           "cpld_cpld2_intr" \
                           "cpld_ntm_intr" \
                           "cpld_fan_psu_intr" \
                           "cpld_ioexp_sfp_rs_intr" \
                           "cpld_cpu_nmi_intr" \
                           "cpld_ptp_intr" \
                           "cpld_system_intr")
        cpld2_sysfs_array=("cpld_sfp_intr_present_0_7" \
                           "cpld_sfp_intr_present_8_15" \
                           "cpld_sfp_intr_present_16_23" \
                           "cpld_sfp_intr_present_24_31" \
                           "cpld_sfp_intr_present_32_39" \
                           "cpld_sfp_intr_present_40_47" \
                           "cpld_qsfp_intr_present_48_53" \
                           "cpld_qsfp_intr_port_48_53")
        cpld2_sysfs_desc=("SFP Present Interrupt 0-7" \
                           "SFP Present Interrupt 8-15" \
                           "SFP Present Interrupt 16-23" \
                           "SFP Present Interrupt 24-31" \
                           "SFP Present Interrupt 32-39" \
                           "SFP Present Interrupt 40-47" \
                           "QSFP Present Interrupt 48-53" \
                           "QSFP Port Interrupt 48-53")

        # MB CPLD Interrupt
        # CPLD 1 system interrupt register
        system_reg=$(eval "cat ${SYSFS_CPLD1}/cpld_system_intr ${LOG_REDIRECT}")
        intr_cpld_to_cpu=$(((system_reg >> 0) & 2#00000001))
        intr_cpld_nmi=$(((system_reg >> 1) & 2#00000001))
        intr_ptp_to_cpu=$(((system_reg >> 2) & 2#00000001))
        intr_eth_to_cpu=$(((system_reg >> 3) & 2#00000001))
        intr_thrm_to_cpu=$(((system_reg >> 4) & 2#00000001))
        intr_thrm_to_bmc=$(((system_reg >> 6) & 2#00000001))

        _echo "[CPLD to CPU Interrupt(L) ]: ${intr_cpld_to_cpu}"
        _echo "[CPLD NMI Interrupt(L)    ]: ${intr_cpld_nmi}"
        _echo "[PTP to CPU Interrupt(L)  ]: ${intr_ptp_to_cpu}"
        _echo "[Eth to CPU Interrupt(L)  ]: ${intr_eth_to_cpu}"
        _echo "[THRM to CPU Interrupt(L) ]: ${intr_thrm_to_cpu}"
        _echo "[THRM to BMC Interrupt(L) ]: ${intr_thrm_to_bmc}"

        # CPLD 1 cpld2 interrupt register
        cpld2_reg=$(eval "cat ${SYSFS_CPLD1}/cpld_cpld2_intr ${LOG_REDIRECT}")
        intr_cpld2=$(((cpld2_reg >> 0) & 2#00000001))

        _echo "[CPLD2 Interrupt(L) ]: ${intr_cpld_to_cpu}"

        # CPLD 1 fan_psu interrupt register
        fan_psu_reg=$(eval "cat ${SYSFS_CPLD1}/cpld_fan_psu_intr ${LOG_REDIRECT}")
        intr_fan_card=$(((fan_psu_reg >> 0) & 2#00000001))
        intr_fan_0=$(((fan_psu_reg >> 1) & 2#00000001))
        intr_fan_1=$(((fan_psu_reg >> 2) & 2#00000001))
        intr_fan_2=$(((fan_psu_reg >> 3) & 2#00000001))
        intr_fan_3=$(((fan_psu_reg >> 4) & 2#00000001))
        intr_fan_4=$(((fan_psu_reg >> 5) & 2#00000001))
        intr_psu_0=$(((fan_psu_reg >> 6) & 2#00000001))
        intr_psu_1=$(((fan_psu_reg >> 7) & 2#00000001))

        _echo "[FAN Card Interrupt(L) ]: ${intr_fan_card}"
        _echo "[FAN 0 Interrupt(L)    ]: ${intr_fan_0}"
        _echo "[FAN 1 Interrupt(L)    ]: ${intr_fan_1}"
        _echo "[FAN 2 Interrupt(L)    ]: ${intr_fan_2}"
        _echo "[FAN 3 Interrupt(L)    ]: ${intr_fan_3}"
        _echo "[FAN 4 Interrupt(L)    ]: ${intr_fan_4}"
        _echo "[PSU 0 Interrupt(L)    ]: ${intr_psu_0}"
        _echo "[PSU 1 Interrupt(L)    ]: ${intr_psu_1}"

        # CPLD 2 port interrupt
        for (( j=0; j<${#cpld2_sysfs_array[@]}; j++ ))
        do
            cpld2_sysfs_reg=$(eval "cat ${SYSFS_CPLD2}/${cpld2_sysfs_array[${j}]} ${LOG_REDIRECT}")
            _echo "[CPLD2 ${cpld2_sysfs_desc[${j}]} Interrupt(L) )]: ${cpld2_sysfs_reg}"
        done

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

    local sysfs_attr=("cpld_system_led_sync" \
                      "cpld_system_led_sys" \
                      "cpld_system_led_fan" \
                      "cpld_system_led_psu_0" \
                      "cpld_system_led_psu_1" \
                      "cpld_system_led_id")
    local desc=("Sync " \
                "Sys  " \
                "Fan  " \
                "PSU 0" \
                "PSU 1" \
                "ID   ")
    local desc_color=("Yellow" "Green" "Blue")
    local desc_speed=("0.5 Hz" "2 Hz")
    local desc_blink=("Solid" "Blink")
    local desc_onoff=("OFF" "ON")

    if [ "${MODEL_NAME}" == "S8901-54XC" ]; then
        for (( i=0; i<${#sysfs_attr[@]}; i++ ))
        do
            _check_filepath "${SYSFS_CPLD1}/${sysfs_attr[i]}"
            led_reg=$(eval "cat ${SYSFS_CPLD1}/${sysfs_attr[i]} ${LOG_REDIRECT}")
            color=$(((led_reg >> 0) & 2#00000001)) # (0: yellow, 1: green)
            speed=$(((led_reg >> 1) & 2#00000001)) # (0: 0.5 Hz, 1: 2 Hz)
            blink=$(((led_reg >> 2) & 2#00000001)) # (0: Solid,  1: Blink)
            onoff=$(((led_reg >> 3) & 2#00000001)) # (0: off,    1: on)

            # ID led only has blue color
            if [ "${sysfs_attr[i]}" == "cpld_system_led_id" ]; then
                _echo "[System LED ${desc[i]}]: ${led_reg} [${desc_color[2]}][${desc_speed[speed]}][${desc_blink[blink]}][${desc_onoff[onoff]}]"
            else
                _echo "[System LED ${desc[i]}]: ${led_reg} [${desc_color[color]}][${desc_speed[speed]}][${desc_blink[blink]}][${desc_onoff[onoff]}]"
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

    base=0xE00
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

    base=0xE300
    offset=0x0
    reg=$(( ${base} + ${offset} ))
    reg=`printf "0x%X\n" ${reg}`
    ret=""

    while [ "${reg}" != "0xE400" ]
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

function _show_cpld_reg_sysfs {
    _banner "Show CPLD Register"

    if [ "${MODEL_NAME}" == "S8901-54XC" ]; then
        if _check_dirpath "${SYSFS_CPLD1}" ;then
            reg_dump=$(eval "i2cdump -f -y ${CPLD_BUS} 0x30 ${LOG_REDIRECT}")
            _echo "[CPLD 1 Register]:"
            _echo "${reg_dump}"
        fi

        if _check_dirpath "${SYSFS_CPLD2}" ;then
            reg_dump=$(eval "i2cdump -f -y ${CPLD_BUS} 0x31 ${LOG_REDIRECT}")
            _echo "[CPLD 2 Register]:"
            _echo "${reg_dump}"
        fi
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
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

    _echo "[Command]: lsusb -v"
    ret=$(eval "lsusb -v ${LOG_REDIRECT}")
    _echo "${ret}"
    _echo ""

    _echo "[Command]: lsusb -t"
    ret=$(eval "lsusb -t ${LOG_REDIRECT}")
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

    cmd_array=("lsblk" \
               "lsblk -O" \
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

usage() {
    local f=$(basename "$0")
    echo ""
    echo "Usage:"
    echo "    $f [-b] [-d D_DIR] [-h] [-i identifier] [-v]"
    echo "Description:"
    echo "  -b                bypass i2c command (required when NOS vendor use their own platform bsp to control i2c devices)"
    echo "  -d                specify D_DIR as log destination instead of default path /tmp/log"
    echo "  -i                insert an identifier in the log file name"
    echo "  -v                show tech support script version"
    echo "Example:"
    echo "    $f -d /var/log"
    echo "    $f -i identifier"
    echo "    $f -v"
    exit -1
}

function _getopts {
    local OPTSTRING=":bd:fi:v"
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
    _show_sfp_status
    _show_qsfp_status
    _show_cpu_temperature
    _show_cpld_interrupt
    _show_system_led
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
    _show_bmc_device_status # Not support
    _show_bmc_sel_raw_data
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
