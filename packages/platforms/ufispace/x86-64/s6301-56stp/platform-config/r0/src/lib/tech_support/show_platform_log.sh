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


# PLAT: This script is compatible with the platform.
PLAT="S6301-56STP"
# MODEL_NAME: set by function _board_info
MODEL_NAME=""
# HW_REV: set by function _board_info
HW_REV=""
# HW_EXT_ID: set by function _board_info
HW_EXT_ID=""
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

# GPIO_MAX: update by function _update_gpio_max
GPIO_MAX=0
GPIO_MAX_INIT_FLAG=0

# Execution Time
start_time=$(date +%s)
end_time=0
elapsed_time=0

# I2C Bus
i801_bus=""
ismt_bus=""

# Sysfs
SYSFS_DEV="/sys/bus/i2c/devices"
SYSFS_LPC="/sys/devices/platform/x86_64_ufispace_s6301_56stp_lpc"
SYSFS_LPC_CPLD="${SYSFS_LPC}/mb_cpld"
SYSFS_LPC_BSP="${SYSFS_LPC}/bsp"
SYSFS_POE="/sys/bus/i2c/devices/4-0020"
SYSFS_POE_SYS="${SYSFS_POE}/sys"

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
    _banner "Package Version = 1.0.0"
}

function _update_gpio_max {
    _banner "Update GPIO MAX"

    GPIO_MAX=$(cat ${SYSFS_LPC_BSP}/bsp_gpio_max)
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

    # As our bsp init status, we look at bsp_version.?
    if [ -f "${SYSFS_LPC_BSP}/bsp_version" ]; then
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
    model_id_array=($((2#11111101)))
    model_name_array=($PLAT)
    ext_id_array=(0 1 2)
    ext_id_name_array=("POE" "NPOE 0BASE" "NPOE 1BASE")

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

    # MODEL ID D[0:7]
    if [ $model_id -eq ${model_id_array[0]} ]; then
       model_name=${model_name_array[0]}
    else
       _echo "Invalid model_id: ${model_id}"
       exit 1
    fi

    hw_ext_id=`${IOGET} 0x706`
    ret=$?
    if [ $ret -eq 0 ]; then
        hw_ext_id=`echo ${hw_ext_id} | awk -F" " '{print $NF}'`
        hw_ext_id=$(((hw_ext_id & 2#00000111) >> 0))
    else
        _echo "Get extended id failed ($ret), Exit!!"
        exit $ret
    fi
    hw_ext_id_name=${ext_id_name_array[${hw_ext_id}]}

    MODEL_NAME=${model_name}
    HW_REV=${hw_rev}
    HW_EXT_ID=${hw_ext_id}
    _echo "[CPLD 0x0/0x1/0x6 Reg Raw ]: ${model_id} ${board_rev_id} ${hw_ext_id}"
    _echo "[Board Type               ]: ${model_name}"
    _echo "[Extended ID              ]: ${hw_ext_id_name}"
    _echo "[Design Phase             ]: ${deph_name}"
    _echo "[Hardware Revision        ]: ${hw_rev}"
    _echo "[BUILD_ID                 ]: ${build_rev}"
}

function _bios_version {
    _banner "Show BIOS Version"

    bios_ver=$(eval "cat /sys/class/dmi/id/bios_version ${LOG_REDIRECT}")
    bios_boot_sel=`${IOGET} 0xE30C`
    if [ $? -eq 0 ]; then
        bios_boot_sel=`echo ${bios_boot_sel} | awk -F" " '{print $NF}'`
    fi

    # EC BIOS BOOT SEL D[6]
    bios_boot_sel=$(((bios_boot_sel & 2#01000000) >> 6))

    _echo "[BIOS Vesion  ]: ${bios_ver}"
    _echo "[BIOS Boot ROM]: ${bios_boot_sel}"
}

function _cpld_version_lpc {
    _banner "Show CPLD Version (LPC)"

    if [ "${MODEL_NAME}" == $PLAT ]; then
        # MB CPLD S6301
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

        _echo "[MB CPLD Version]: $(( (mb_cpld_ver & 2#11000000) >> 6)).$(( mb_cpld_ver & 2#00111111 )).$(( mb_cpld_build & 2#11111111 ))"
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _cpld_version_sysfs {
    _banner "Show CPLD Version (Sysfs)"

    if [ "${MODEL_NAME}" == $PLAT ]; then
        sysfs="${SYSFS_LPC_CPLD}/mb_cpld_1_version_h"
        if _check_filepath $sysfs; then
            mb_cpld_ver=$(eval "cat ${sysfs} ${LOG_REDIRECT}")
            _echo "[MB CPLD Version]: $mb_cpld_ver"
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

    if [ "${MODEL_NAME}" != "${PLAT}" ]; then
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    #get ucd version via i2cdump
    ucd_ver_ascii=$(eval "i2cdump -f -y 5 0x34 s 0x9b | awk '{if (\$1 == \"00:\" ) print \$7 }' ${LOG_REDIRECT}")
    ret=$?

    #check return code
    if [ ! $ret -eq 0 ] ; then
        _echo "Require i2cdump to get UCD version"
        return $ret
    fi


    #get ucd date via BMC
    ucd_date_ascii=$(eval "i2cdump -f -y 5 0x34 s 0x9d | awk '{if (\$1 == \"00:\" ) print \$8 }' ${LOG_REDIRECT}")
    ret=$?

    #check return code
    if [ ! $ret -eq 0 ] ; then
        _echo "Require i2cdump to get UCD version"
        return $ret
    fi

    _echo "[${brd[i]} MFR_REVISION    ]: ${ucd_ver_ascii}"
    _echo "[${brd[i]} MFR_DATE        ]: ${ucd_date_ascii}"
}


function _show_version {
    _bios_version
    _cpld_version
    _ucd_version
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
        _echo "TCA954x Mux ${chip_dev_desc}"
        _echo "---------------------------------------------------"
        for (( i=0; i<${channel_num}; i++ ))
        do
            _echo "TCA954x Mux ${chip_dev_desc} - Channel ${i}"
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
    local bus=""

    if [ "${MODEL_NAME}" == "${PLAT}" ]; then
        # ismt_bus
        bus="${ismt_bus}"
        chip_addr1="0x72"
        ## 9546_MUX1-0x72
        _show_i2c_mux_devices "${bus}" "${chip_addr1}" "4" "9546_MUX1-0x76"

        ## 9546_MUX2-0x70
        chip_addr1="0x70"
        _show_i2c_mux_devices "${bus}" "${chip_addr1}" "4" "9546_MUX2-0x70"

        # i801_bus
        bus="${i801_bus}"
        ## 9548_MUX3-0x71
        chip_addr1="0x71"
        _show_i2c_mux_devices "${bus}" "${chip_addr1}" "8" "9548_MUX3-0x71"

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
        pca954x_device_id=("0x72" "0x70" "0x71")
        pca954x_device_bus=("${ismt_bus}" "${ismt_bus}" "${i801_bus}")

    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    for (( i=0; i<${#pca954x_device_id[@]}; i++ ))
    do
        for ((j=0;j<5;j++))
        do
            _echo "[DEV PCA954x (${j})]"
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

    eeprom_addr="0x56"
    bus=${ismt_bus}

    #read six times return empty content
    sys_eeprom=$(eval "i2cdump -y ${bus} ${eeprom_addr} c")
    sys_eeprom=$(eval "i2cdump -y ${bus} ${eeprom_addr} c")
    sys_eeprom=$(eval "i2cdump -y ${bus} ${eeprom_addr} c")
    sys_eeprom=$(eval "i2cdump -y ${bus} ${eeprom_addr} c")
    sys_eeprom=$(eval "i2cdump -y ${bus} ${eeprom_addr} c")
    sys_eeprom=$(eval "i2cdump -y ${bus} ${eeprom_addr} c")

    #seventh read return correct content
    sys_eeprom=$(eval "i2cdump -y ${ismt_bus} ${eeprom_addr} c")
    _echo "[System EEPROM]:"
    _echo "${sys_eeprom}"
}

function _show_sys_eeprom_sysfs {
    _banner "Show System EEPROM"

    addr="56"
    bus=${ismt_bus}
     
    sys_eeprom=$(eval "cat /sys/bus/i2c/devices/${bus}-00${addr}/eeprom ${LOG_REDIRECT} | hexdump -C")
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
    max_gpio=${max_gpio#gpio}
    min_gpio=${min_gpio#gpio}

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

function _show_psu_status_cpld_lpc {
    _banner "Show PSU Status (CPLD)"

    bus_id=""
    if [ "${MODEL_NAME}" == "${PLAT}" ]; then
        bus_id="1"
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    # Read PSU Status
    cpld_psu_status_reg=`${IOGET} 0x759`
    ret=$?
    if [ $ret -eq 0 ]; then
        cpld_psu_status_reg=`echo ${cpld_psu_status_reg} | awk -F" " '{print $NF}'`
        cpld_psu_status_reg=$((cpld_psu_status_reg))
    else
        _echo "Get psu status failed ($ret), Exit!!"
        exit $ret
    fi

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
        _show_psu_status_cpld_lpc
    fi
}

function _show_port_status_sysfs {
    _banner "Show Port Status / EEPROM"

    # check GPIO_MAX 
    if [ "$GPIO_MAX" == "0" ]; then
        _echo "Incorrect GPIO_MAX value (${GPIO_MAX}), exit!!!"
        return 0
    fi

    if [ "${MODEL_NAME}" == "${PLAT}" ]; then
        ## port index according to ext_id
        if [ "$HW_EXT_ID" == "2" ]; then
            start_port_index=49
        else
            start_port_index=48
        fi
        for (( i=0; i<8; i++ ))
        do
            port_name_array+=("$((${start_port_index}+${i}))")
        done

        port_absent_gpio_array=("8" "9" "10" "11" "12" "13" "14" "15")
        port_rx_los_gpio_array=("32" "33" "34" "35" "36" "37" "38" "39")
        port_tx_fault_gpio_array=("40" "41" "42" "43" "44" "45" "46" "47")
        port_tx_dis_gpio_array=("16" "17" "18" "19" "20" "21" "22" "23")
        port_eeprom_bus_array=("10" "11" "12" "13" "14" "15" "16" "17")

        for (( i=0; i<${#port_name_array[@]}; i++ ))
        do

            # Port Absent Status (0: Present, 1:Absence)
            gpio_num=$((${GPIO_MAX} - ${port_absent_gpio_array[${i}]}))
            gpio_path="/sys/class/gpio/gpio${gpio_num}/value"
            if [ "${port_absent_gpio_array[${i}]}" != "-1" ] && _check_filepath "$gpio_path"; then
                port_absent=$(eval "cat $gpio_path")
                _echo "[Port${port_name_array[${i}]} Module Absent]: ${port_absent}"
            fi

            # Port Tx Fault Status (0:normal, 1:tx fault)
            gpio_num=$((${GPIO_MAX} - ${port_tx_fault_gpio_array[${i}]}))
            gpio_path="/sys/class/gpio/gpio${gpio_num}/value"
            if [ "${port_tx_fault_gpio_array[${i}]}" != "-1" ] && _check_filepath "$gpio_path"; then
                port_tx_fault=$(eval "cat $gpio_path")
                _echo "[Port${port_name_array[${i}]} Tx Fault Status]: ${port_tx_fault}"
            fi

            # Port Rx Loss Status (0:los undetected, 1: los detected)
            gpio_num=$((${GPIO_MAX} - ${port_rx_los_gpio_array[${i}]}))
            gpio_path="/sys/class/gpio/gpio${gpio_num}/value"
            if [ "${port_tx_fault_gpio_array[${i}]}" != "-1" ] && _check_filepath "$gpio_path"; then
                port_rx_loss=$(eval "cat $gpio_path")
                _echo "[Port${port_name_array[${i}]} Rx Loss Status]: ${port_rx_loss}"
            fi

            # Port Tx Disable Status (0:enable tx, 1: disable tx)
            gpio_num=$((${GPIO_MAX} - ${port_tx_dis_gpio_array[${i}]}))
            gpio_path="/sys/class/gpio/gpio${gpio_num}/value"
            if [ "${port_tx_dis_gpio_array[${i}]}" != "-1" ] && _check_filepath "$gpio_path"; then
                port_tx_dis=$(eval "cat $gpio_path")
                _echo "[Port${port_name_array[${i}]} Tx Disable Status]: ${port_tx_dis}"
            fi

            # Port Dump EEPROM
            local eeprom_path="/sys/bus/i2c/devices/${port_eeprom_bus_array[${i}]}-0050/eeprom"
            if [ "${port_absent}" == "0" ] && _check_filepath "${eeprom_path}"; then
                port_eeprom=$(eval "dd if=${eeprom_path} bs=128 count=2 skip=0 status=none ${LOG_REDIRECT} | hexdump -C")
                if [ "${LOG_FILE_ENABLE}" == "1" ]; then
                    hexdump -C "${eeprom_path}" > ${LOG_FOLDER_PATH}/port${port_name_array[${i}]}_eeprom.log 2>&1
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
    if [ "${BSP_INIT_FLAG}" == "1" ] && [ "${GPIO_MAX_INIT_FLAG}" == "1" ]; then
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

function _show_system_led_sysfs {
    _banner "Show System LED"

    if [ "${MODEL_NAME}" == "${PLAT}" ]; then
        local sysfs_attr=("led_id"
                          "led_sys"
                          "led_fan"
                          "led_pwr1"
                          "led_pwr0")

        local desc_color=("Yellow" "Green" "Blue")
        local desc_blink=("Solid" "Blink")
        local desc_onoff=("OFF" "ON")

        local led=("ID   "
                   "Sys  "
                   "Fan  "
                   "PSU 1"
                   "PSU 0")

        local led_sysfs_idx=(0 1 2 3 4)
        local color=(-1 0 0 0 0)
        local blink=(2 2 2 2 2)
        local onoff=(3 3 3 3 3)

        for (( i=0; i<${#led[@]}; i++ ))
        do
            idx=${led_sysfs_idx[i]}
            if _check_filepath "${SYSFS_LPC_CPLD}/${sysfs_attr[idx]}"; then
                led_reg=$(eval "cat ${SYSFS_LPC_CPLD}/${sysfs_attr[idx]} ${LOG_REDIRECT}")


                if [ "${color[i]}" != "-1" ]; then
                    color=$(((led_reg >> ${color[i]}) & 2#1)) # (0: yellow, 1: green)
                else
                    color=2 # (blue)
                fi

                blink=$(((led_reg >> ${blink[i]}) & 2#1)) # (0: Solid,  1: Blink)
                onoff=$(((led_reg >> ${onoff[i]}) & 2#1)) # (0: off,    1: on)

                _echo "[System LED ${led[i]}]: [${sysfs_attr[idx]} ${led_reg}] [${desc_color[color]}][${desc_blink[blink]}][${desc_onoff[onoff]}]"

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

function _show_poe_status_sysfs {
    _banner "show POE SYS status"
    
    # show poe sys info
    sysfs="${SYSFS_POE_SYS}/poe_sys_info"
    if _check_filepath ${sysfs}; then
        sys_info=$(eval "cat ${sysfs} ${LOG_REDIRECT}")
    fi
    _echo "[POE sys info]:"
    _echo "${sys_info}"

    # show poe port info 
    for (( i=0; i<48; i++ ))
    do
        sysfs_port="${SYSFS_POE}/port-${i}"
        sysfs_port_status="${sysfs_port}/poe_port_${i}_status"
        sysfs_port_config="${sysfs_port}/poe_port_${i}_get_config"
        sysfs_port_power="${sysfs_port}/poe_port_${i}_get_power"
        sysfs_port_temp="${sysfs_port}/poe_port_${i}_get_temp"
        if _check_filepath ${sysfs_port_status}; then
            status=$(eval "cat ${sysfs_port_status} ${LOG_REDIRECT}")
        fi
        if _check_filepath ${sysfs_port_config}; then
            config=$(eval "cat ${sysfs_port_config} ${LOG_REDIRECT}")
        fi
        if _check_filepath ${sysfs_port_power}; then
            power=$(eval "cat ${sysfs_port_power} ${LOG_REDIRECT}")
        fi
        if _check_filepath ${sysfs_port_temp}; then
            temp=$(eval "cat ${sysfs_port_temp} ${LOG_REDIRECT}")
        fi
        _echo "[POE port ${i} info]:"
        _echo "[status]:"
        _echo "${status}"
        _echo "[config]:"
        _echo "${config}"
        _echo "[power]:"
        _echo "${power}"
        _echo "[temp]:"
        _echo "${temp}"        
    done
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
    _show_port_status
    _show_cpu_temperature
    _show_system_led
    _show_poe_status_sysfs
    _show_ioport
    _show_onlpdump
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

    echo "#   done..."
}

_main