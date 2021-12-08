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

# GPIO_OFFSET: update by function _update_gpio_offset
GPIO_OFFSET=0



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
    _banner "Package Version = 1.0.1"
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
        echo "${LOG_FILE_NAME}" > "${LOG_FILE_PATH}"
    fi
    
    # check BSP init
    _check_bsp_init
    _update_gpio_offset
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
    i2c_addr=$1

    if [ -z "${i2c_addr}" ]; then
        _echo "ERROR, the ipnut string is empyt!!!"
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
    
    # CPLD1 0xE00 Register Definition
    build_rev_id_array=(0 1 2 3 4 5 6 7) 
    build_rev_array=(1 2 3 4 5 6 7 8)
    hw_rev_id_array=(0 1 2 3)
    deph_name_array=("NPI" "GA")
    hw_rev_array=("Proto" "Alpha" "Beta" "PVT")
    hw_rev_ga_array=("GA_1" "GA_2" "GA_3" "GA_4")
    model_id_array=($((2#00000000)) $((2#00010000)))
    model_name_array=("NCP3" "NCP3(w/o OP2)")
    model_name=""

    model_id=`${IOGET} 0xE00`
    ret=$?
    if [ $ret -eq 0 ]; then 
        model_id=`echo ${model_id} | awk -F" " '{print $NF}'`
        model_id=$((model_id))
    else
        _echo "Get board model id failed ($ret), Exit!!"
        exit $ret
    fi
    
    board_rev_id=`${IOGET} 0xE01`
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
   
    # Model Name    
    for (( i=0; i<${#model_id_array[@]}; i++ ))
    do
        if [ $model_id -eq ${model_id_array[$i]} ]; then    
           model_name=${model_name_array[$i]}
           break
        fi
    done
    
    if [ "$model_name" == "" ]; then    
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

    # CPU CPLD
    cpu_cpld_info=`${IOGET} 0x600`
    ret=$?
    if [ $ret -eq 0 ]; then
        cpu_cpld_info=`echo ${cpu_cpld_info} | awk -F" " '{print $NF}'`
    else
        _echo "Get CPU CPLD version info failed ($ret), Exit!!"
        exit $ret
    fi
    
    cpu_cpld_build=`${IOGET} 0x6e0`
    ret=$?
    if [ $ret -eq 0 ]; then
        cpu_cpld_build=`echo ${cpu_cpld_build} | awk -F" " '{print $NF}'`
    else
        _echo "Get CPU CPLD build info failed ($ret), Exit!!"
        exit $ret
    fi

    _echo "[CPU CPLD Reg Raw]: ${cpu_cpld_info} build ${cpu_cpld_build}" 
    _echo "[CPU CPLD Version]: $(( (cpu_cpld_info & 2#11000000) >> 6)).$(( cpu_cpld_info & 2#00111111 )) build $((cpu_cpld_build))"          

    if [[ $MODEL_NAME == *"NCP3"* ]]; then
        # MB CPLD NCP3
        mb_cpld1_ver=""
        mb_cpld2_ver=""
        mb_cpld3_ver=""
        mb_cpld4_ver=""
        mb_cpld5_ver=""
        mb_cpld1_build=""
        mb_cpld2_build=""
        mb_cpld3_build=""
        mb_cpld4_build=""
        mb_cpld5_build=""

        # CPLD 1-3
        
        _check_i2c_device "0x71"
        value=$(eval "i2cget -y -f 0 0x71 ${LOG_REDIRECT}")
        ret=$?
        
        if [ ${ret} -eq 0 ]; then
            i2cset -y 0 0x71 0x1
            _check_i2c_device "0x30"
            _check_i2c_device "0x31"
            _check_i2c_device "0x32"
            mb_cpld1_ver=$(eval "i2cget -y -f 0 0x30 0x2 ${LOG_REDIRECT}")
            mb_cpld2_ver=$(eval "i2cget -y -f 0 0x31 0x2 ${LOG_REDIRECT}")
            mb_cpld3_ver=$(eval "i2cget -y -f 0 0x32 0x2 ${LOG_REDIRECT}")            
            mb_cpld1_build=$(eval "i2cget -y -f 0 0x30 0x4 ${LOG_REDIRECT}")
            mb_cpld2_build=$(eval "i2cget -y -f 0 0x31 0x4 ${LOG_REDIRECT}")
            mb_cpld3_build=$(eval "i2cget -y -f 0 0x32 0x4 ${LOG_REDIRECT}")
            i2cset -y 0 0x71 0x0
        fi
        
        # CPLD 4-5
        
        _check_i2c_device "0x72"
        value=$(eval "i2cget -y -f 0 0x72 ${LOG_REDIRECT}")
        ret=$?
        
        if [ ${ret} -eq 0 ]; then
        
            i2cset -y 0 0x72 0x1
            
            _check_i2c_device "0x76"
            value=$(eval "i2cget -y -f 0 0x76 ${LOG_REDIRECT}")
            ret=$?
        
            if [ ${ret} -eq 0 ]; then 
                i2cset -y 0 0x76 0x20
                _check_i2c_device "0x33"
                _check_i2c_device "0x34"                
                mb_cpld4_ver=$(eval "i2cget -y -f 0 0x33 0x2 ${LOG_REDIRECT}")
                mb_cpld5_ver=$(eval "i2cget -y -f 0 0x34 0x2 ${LOG_REDIRECT}")
                mb_cpld4_build=$(eval "i2cget -y -f 0 0x33 0x4 ${LOG_REDIRECT}")
                mb_cpld5_build=$(eval "i2cget -y -f 0 0x34 0x4 ${LOG_REDIRECT}")                
                i2cset -y 0 0x76 0x0
            fi
            
            i2cset -y 0 0x72 0x0
        fi

        _echo "[MB CPLD1 Version]: $(( (mb_cpld1_ver & 2#11000000) >> 6)).$(( mb_cpld1_ver & 2#00111111 )) build $((mb_cpld1_build))"
        _echo "[MB CPLD2 Version]: $(( (mb_cpld2_ver & 2#11000000) >> 6)).$(( mb_cpld2_ver & 2#00111111 )) build $((mb_cpld2_build))"
        _echo "[MB CPLD3 Version]: $(( (mb_cpld3_ver & 2#11000000) >> 6)).$(( mb_cpld3_ver & 2#00111111 )) build $((mb_cpld3_build))"
        _echo "[MB CPLD4 Version]: $(( (mb_cpld4_ver & 2#11000000) >> 6)).$(( mb_cpld4_ver & 2#00111111 )) build $((mb_cpld4_build))"
        _echo "[MB CPLD5 Version]: $(( (mb_cpld5_ver & 2#11000000) >> 6)).$(( mb_cpld5_ver & 2#00111111 )) build $((mb_cpld4_build))"    
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

    cpu_cpld_build=`${IOGET} 0x6e0`
    ret=$?
    if [ $ret -eq 0 ]; then
        cpu_cpld_build=`echo ${cpu_cpld_build} | awk -F" " '{print $NF}'`
    else
        _echo "Get CPU CPLD build info failed ($ret), Exit!!"
        exit $ret
    fi

    _echo "[CPU CPLD Reg Raw]: ${cpu_cpld_info} build ${cpu_cpld_build}" 
    _echo "[CPU CPLD Version]: $(( (cpu_cpld_info & 2#11000000) >> 6)).$(( cpu_cpld_info & 2#00111111 )) build $((cpu_cpld_build))"

    if [[ $MODEL_NAME == *"NCP3"* ]]; then
        # MB CPLD NCP3
        _check_filepath "/sys/bus/i2c/devices/1-0030/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/1-0031/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/1-0032/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/30-0033/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/30-0034/cpld_version"
        _check_filepath "/sys/bus/i2c/devices/1-0030/cpld_build"
        _check_filepath "/sys/bus/i2c/devices/1-0031/cpld_build"
        _check_filepath "/sys/bus/i2c/devices/1-0032/cpld_build"
        _check_filepath "/sys/bus/i2c/devices/30-0033/cpld_build"
        _check_filepath "/sys/bus/i2c/devices/30-0034/cpld_build"        
        mb_cpld1_ver=$(eval "cat /sys/bus/i2c/devices/1-0030/cpld_version ${LOG_REDIRECT}")
        mb_cpld2_ver=$(eval "cat /sys/bus/i2c/devices/1-0031/cpld_version ${LOG_REDIRECT}")
        mb_cpld3_ver=$(eval "cat /sys/bus/i2c/devices/1-0032/cpld_version ${LOG_REDIRECT}")
        mb_cpld4_ver=$(eval "cat /sys/bus/i2c/devices/30-0033/cpld_version ${LOG_REDIRECT}")
        mb_cpld5_ver=$(eval "cat /sys/bus/i2c/devices/30-0034/cpld_version ${LOG_REDIRECT}")
        mb_cpld1_build=$(eval "cat /sys/bus/i2c/devices/1-0030/cpld_build ${LOG_REDIRECT}")
        mb_cpld2_build=$(eval "cat /sys/bus/i2c/devices/1-0031/cpld_build ${LOG_REDIRECT}")
        mb_cpld3_build=$(eval "cat /sys/bus/i2c/devices/1-0032/cpld_build ${LOG_REDIRECT}")
        mb_cpld4_build=$(eval "cat /sys/bus/i2c/devices/30-0033/cpld_build ${LOG_REDIRECT}")
        mb_cpld5_build=$(eval "cat /sys/bus/i2c/devices/30-0034/cpld_build ${LOG_REDIRECT}")

        _echo "[MB CPLD1 Version]: $(( (mb_cpld1_ver & 2#11000000) >> 6)).$(( mb_cpld1_ver & 2#00111111 )) build $((mb_cpld1_build))"
        _echo "[MB CPLD2 Version]: $(( (mb_cpld2_ver & 2#11000000) >> 6)).$(( mb_cpld2_ver & 2#00111111 )) build $((mb_cpld2_build))"
        _echo "[MB CPLD3 Version]: $(( (mb_cpld3_ver & 2#11000000) >> 6)).$(( mb_cpld3_ver & 2#00111111 )) build $((mb_cpld3_build))"
        _echo "[MB CPLD4 Version]: $(( (mb_cpld4_ver & 2#11000000) >> 6)).$(( mb_cpld4_ver & 2#00111111 )) build $((mb_cpld4_build))"
        _echo "[MB CPLD5 Version]: $(( (mb_cpld5_ver & 2#11000000) >> 6)).$(( mb_cpld5_ver & 2#00111111 )) build $((mb_cpld5_build))"    
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

    if [[ $MODEL_NAME == *"NCP3"* ]]; then 
        ## ROOT-0x71
        _show_i2c_mux_devices "0x71" "8" "ROOT-0x71"
        
        ## ROOT-0x72
        _show_i2c_mux_devices "0x72" "8" "ROOT-0x72"

        ## ROOT-0x72-Channel(0~5)-0x76-Channel(0~7)
        chip_addr1="0x72"
        chip_addr2="0x76"
        _check_i2c_device "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            for (( chip_addr1_chann=0; chip_addr1_chann<=5; chip_addr1_chann++ ))
            do
                # open mux channel - 0x72 (chip_addr1)
                i2cset -y 0 ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
                _show_i2c_mux_devices "${chip_addr2}" "8" "ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}"
                # close mux channel - 0x72 (chip_addr1)
                i2cset -y 0 ${chip_addr1} 0x0
            done
        fi
        
        ## ROOT-0x72-Channel(0)-0x76-Channel(0~4)-0x75-Channel(0~7)
        chip_addr1="0x72"
        chip_addr2="0x76"
        chip_addr3="0x75"
        _check_i2c_device "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            for (( chip_addr1_chann=0; chip_addr1_chann<1; chip_addr1_chann++ ))
            do
                # open mux channel - 0x72 (chip_addr1)
                i2cset -y 0 ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
                #_show_i2c_mux_devices "${chip_addr2}" "8" "ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}"
                
                _check_i2c_device "${chip_addr2}"
                ret=$?
                if [ "$ret" == "0" ]; then
                    for (( chip_addr2_chann=0; chip_addr2_chann<=4; chip_addr2_chann++ ))
                    do
                        # open mux channel - 0x76 (chip_addr1)
                        i2cset -y 0 ${chip_addr2} $(( 2 ** ${chip_addr2_chann} ))
                        
                        _show_i2c_mux_devices "${chip_addr3}" "8" "ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}-${chip_addr2_chann}"
                        
                        # close mux channel - 0x76 (chip_addr1)
                        i2cset -y 0 ${chip_addr2} 0x0 
                    done
                fi         
                
                # close mux channel - 0x72 (chip_addr1)
                i2cset -y 0 ${chip_addr1} 0x0
            done
        fi
        
        ## ROOT-0x73
        _show_i2c_mux_devices "0x73" "8" "ROOT-0x73"
        
        ## ROOT-0x73-Channel(1)-0x76-Channel(0~4)
        chip_addr1="0x73"
        chip_addr2="0x76"
        _check_i2c_device "${chip_addr1}"
        ret=$?
        if [ "$ret" == "0" ]; then
            for (( chip_addr1_chann=1; chip_addr1_chann<2; chip_addr1_chann++ ))
            do
                # open mux channel - 0x73 (chip_addr1)
                i2cset -y 0 ${chip_addr1} $(( 2 ** ${chip_addr1_chann} ))
                _check_i2c_device "${chip_addr2}"
                _show_i2c_mux_devices "${chip_addr2}" "8" "ROOT-${chip_addr1}-${chip_addr1_chann}-${chip_addr2}"
                # close mux channel - 0x73 (chip_addr1)
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

    ret=`i2cdump -y -f 0 0x77 b`
    _echo "[I2C Device 0x77]:"
    _echo "${ret}"
    _echo ""

    local pca954x_device_id=("")
    if [[ $MODEL_NAME == *"NCP3"* ]]; then
        pca954x_device_id=("0x71" "0x72" "0x73")    
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
    cpu_eeprom=$(eval "i2cdump -y 0 0x57 c")
    #second read return correct content
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
    if [[ $MODEL_NAME == *"NCP3"* ]]; then        
        bus_id="1"
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
    
    # Read PSU Status
    _check_filepath "/sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status"
    cpld_psu_status_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0030/cpld_psu_status ${LOG_REDIRECT}")
    
    # Read PSU0 Power Good Status (1: power good, 0: not providing power)
    psu0_power_ok=$(((cpld_psu_status_reg & 2#00000100) >> 2))

    # Read PSU0 Absent Status (0: psu present, 1: psu absent)
    psu0_absent_l=$(((cpld_psu_status_reg & 2#00000001) >> 0))
    
    # Read PSU1 Power Good Status (1: power good, 0: not providing power)
    psu1_power_ok=$(((cpld_psu_status_reg & 2#00001000) >> 3))

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
    _banner "Show ROV"

    bus_id=""
    if [[ $MODEL_NAME == *"NCP3"* ]]; then 
        bus_id="1"
        rov_i2c_bus="21"
        rov_i2c_addr=("0x60" "0x62")
        rov_mask=(2#00000111 2#00111000)
        rov_shift=(0 3)
        rov_config_reg="0x21"
        rov_output_reg="0x8b"

        # Read MAC ROV Status
        _check_filepath "/sys/bus/i2c/devices/${bus_id}-0030/cpld_mac_rov"
        cpld_mac_rov_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0030/cpld_mac_rov ${LOG_REDIRECT}")

        if [ -c "/dev/i2c-${rov_i2c_bus}" ]; then
        
            for (( i=0; i<${#rov_i2c_addr[@]}; i++ ))
            do
                mac_rov_stamp=$(((cpld_mac_rov_reg & rov_mask[i]) >> rov_shift[i]))
                rov_controller_config=`i2cget -y ${rov_i2c_bus} ${rov_i2c_addr[i]} ${rov_config_reg}`
                rov_controller_config_value=$((rov_controller_config + 0))
                #rov_controller_config_volt=$(( (rov_controller_config - 1) * 0.005 + 0.25))
                rov_controller_config_volt=$(echo "${rov_controller_config_value}" | awk '{printf "%.3f\n", (($1-1)*0.005+0.25)}')
                rov_controller_output=`i2cget -y ${rov_i2c_bus} ${rov_i2c_addr[i]} ${rov_output_reg}`
                rov_controller_output_value=$((rov_controller_output + 0))
                #rov_controller_output_volt=$(( (rov_controller_output - 1) * 0.005 + 0.25))
                rov_controller_output_volt=$(echo "${rov_controller_output_value}" | awk '{printf "%.3f\n", (($1-1)*0.005+0.25)}')
                
                _echo "[MAC ROV[${i}] Stamp      ]: ${mac_rov_stamp}"
                _echo "[MAC ROV[${i}] Stamp Array]: ROV Config  : [0x65 , 0x65 , 0x59 , 0x5D , 0x61 , 0x69 , 0x6D , 0x71 ]"
                _echo "                      VDDC Voltage: [0.75V, 0.75V, 0.69V, 0.71V, 0.73V, 0.77V, 0.79V, 0.81V]"
                _echo "[MAC ROV[${i}] Config     ]: ${rov_controller_config}"
                _echo "[MAC ROV[${i}] Config Volt]: ${rov_controller_config_volt}V"
                _echo "[MAC ROV[${i}] Output     ]: ${rov_controller_output}"
                _echo "[MAC ROV[${i}] Output Volt]: ${rov_controller_output_volt}V"
            done
        else
            _echo "device (/dev/i2c-${rov_i2c_bus}) not found!!!"
        fi

        

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

function _show_nif_port_status_sysfs {
    _banner "Show NIF Port Status / EEPROM"
    echo "    Show NIF Port Status / EEPROM, please wait..."
    
    bus_id=""
    port_status_cpld_addr_array=""
    port_status_sysfs_idx_array=""
    port_status_bit_idx_array=""

    if [[ $MODEL_NAME == *"NCP3"* ]]; then 
        bus_id="1"
        nif_port_eeprom_bus_id_base=73

        port_status_cpld_addr_array=("0031" "0031" "0031" "0031" "0031" \
                                     "0031" "0031" "0031" "0031" "0031" \
                                     "0031" "0031" "0031" "0031" "0031" \
                                     "0031" "0031" "0031" "0032" "0032" \
                                     "0032" "0032" "0032" "0032" "0032" \
                                     "0032" "0032" "0032" "0032" "0032" \
                                     "0032" "0032" "0032" "0032" "0032" \
                                     "0032" )
                                     
        port_status_sysfs_idx_array=("0" "0" "0" "0" "0" \
                                     "0" "0" "0" "1" "1" \
                                     "1" "1" "1" "1" "1" \
                                     "1" "2" "2" "0" "0" \
                                     "0" "0" "0" "0" "0" \
                                     "0" "1" "1" "1" "1" \
                                     "1" "1" "1" "1" "2" \
                                     "2" )                                     

        port_status_bit_idx_array=("0" "1" "2" "3" "4" \
                                   "5" "6" "7" "0" "1" \
                                   "2" "3" "4" "5" "6" \
                                   "7" "0" "1" "0" "1" \
                                   "2" "3" "4" "5" "6" \
                                   "7" "0" "1" "2" "3" \
                                   "4" "5" "6" "7" "0" \
                                   "1" )

        for (( i=0; i<${#port_status_cpld_addr_array[@]}; i++ ))
        do
            # Module NIF Port Interrupt Status (0: Interrupted, 1:No Interrupt)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_intr_port_${port_status_sysfs_idx_array[${i}]}"
            port_module_interrupt_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_intr_port_${port_status_sysfs_idx_array[${i}]} ${LOG_REDIRECT}")
            port_module_interrupt_l=$(( (port_module_interrupt_reg & 1 << ${port_status_bit_idx_array[i]})>> ${port_status_bit_idx_array[i]} ))
            
            # Module NIF Port Absent Status (0: Present, 1:Absence)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_intr_present_${port_status_sysfs_idx_array[${i}]}"
            port_module_absent_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_intr_present_${port_status_sysfs_idx_array[${i}]} ${LOG_REDIRECT}")
            port_module_absent_l=$(( (port_module_absent_reg & 1 << ${port_status_bit_idx_array[i]})>> ${port_status_bit_idx_array[i]} ))

            # Module NIF Port Get Low Power Mode Status (0: Normal Power Mode, 1:Low Power Mode)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_lpmode_${port_status_sysfs_idx_array[${i}]}"
            port_lp_mode_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_lpmode_${port_status_sysfs_idx_array[${i}]} ${LOG_REDIRECT}")
            port_lp_mode=$(( (port_lp_mode_reg & 1 << ${port_status_bit_idx_array[i]})>> ${port_status_bit_idx_array[i]} ))
            
            # Module FAB Port Reset Status (0:Reset, 1:Normal)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_reset_${port_status_sysfs_idx_array[${i}]}"
            port_reset_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_reset_${port_status_sysfs_idx_array[${i}]} ${LOG_REDIRECT}")
            port_reset=$(( (port_reset_reg & 1 << ${port_status_bit_idx_array[i]})>> ${port_status_bit_idx_array[i]} ))


            # Module NIF Port Dump EEPROM
            if [ "${port_module_absent_l}" == "0" ]; then
                _check_filepath "/sys/bus/i2c/devices/$((nif_port_eeprom_bus_id_base + i))-0050/eeprom"
                port_eeprom_p0_1st=$(eval  "dd if=/sys/bus/i2c/devices/$((nif_port_eeprom_bus_id_base + i))-0050/eeprom bs=128 count=2 skip=0  status=none ${LOG_REDIRECT} | hexdump -C")
                port_eeprom_p0_2nd=$(eval  "dd if=/sys/bus/i2c/devices/$((nif_port_eeprom_bus_id_base + i))-0050/eeprom bs=128 count=2 skip=0  status=none ${LOG_REDIRECT} | hexdump -C")
                port_eeprom_p17_1st=$(eval "dd if=/sys/bus/i2c/devices/$((nif_port_eeprom_bus_id_base + i))-0050/eeprom bs=128 count=1 skip=18 status=none ${LOG_REDIRECT} | hexdump -C")
                port_eeprom_p17_2nd=$(eval "dd if=/sys/bus/i2c/devices/$((nif_port_eeprom_bus_id_base + i))-0050/eeprom bs=128 count=1 skip=18 status=none ${LOG_REDIRECT} | hexdump -C")
                port_eeprom_p18=$(eval     "dd if=/sys/bus/i2c/devices/$((nif_port_eeprom_bus_id_base + i))-0050/eeprom bs=128 count=1 skip=19 status=none ${LOG_REDIRECT} | hexdump -C")
                if [ -z "$port_eeprom_p0_1st" ]; then
                    port_eeprom_p0_1st="ERROR!!! The result is empty. It should read failed (/sys/bus/i2c/devices/$((nif_port_eeprom_bus_id_base + i))-0050/eeprom)!!"
                fi
                
                # Full EEPROM Log
                if [ "${LOG_FILE_ENABLE}" == "1" ]; then
                    hexdump -C "/sys/bus/i2c/devices/$((nif_port_eeprom_bus_id_base + i))-0050/eeprom" > ${LOG_FOLDER_PATH}/qsfpdd_nif_port${i}_eeprom.log 2>&1
                fi
            else
                port_eeprom_p0_1st="N/A"
                port_eeprom_p0_2nd="N/A"
                port_eeprom_p17_1st="N/A"
                port_eeprom_p17_2nd="N/A"
                port_eeprom_p18="N/A"
            fi

            _echo "[Port${i} Status Reg Raw]: ${port_module_interrupt_reg}"
            _echo "[Port${i} Module INT (L)]: ${port_module_interrupt_l}"
            _echo "[Port${i} Module Absent Reg Raq]: ${port_module_absent_reg}"
            _echo "[Port${i} Module Absent ]: ${port_module_absent_l}"
            _echo "[Port${i} Config Reg Raw]: ${port_lp_mode_reg}"
            _echo "[Port${i} Low Power Mode]: ${port_lp_mode}"
            _echo "[Port${i} Reset Reg Raw]: ${port_reset_reg}"
            _echo "[Port${i} Reset Status]: ${port_reset}"
            _echo "[Port${i} EEPROM Page0-0(1st)]:"
            _echo "${port_eeprom_p0_1st}"
            _echo "[Port${i} EEPROM Page0-0(2nd)]:"
            _echo "${port_eeprom_p0_2nd}"
            _echo "[Port${i} EEPROM Page17 (1st)]:"
            _echo "${port_eeprom_p17_1st}"
            _echo "[Port${i} EEPROM Page17 (2nd)]:"
            _echo "${port_eeprom_p17_2nd}"
            _echo "[Port${i} EEPROM Page18      ]:"
            _echo "${port_eeprom_p18}"
            _echo ""
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
    port_status_sysfs_idx_array=""
    port_status_bit_idx_array=""
    port_led_cpld_addr_array=""
    port_led_cpld_index_array=""

    if [[ $MODEL_NAME == *"NCP3"* ]]; then
        bus_id="30"
        fab_port_eeprom_bus_id_base=33
        port_status_cpld_addr_array=("0033" "0033" "0033" "0033" "0033" \
                                     "0033" "0033" "0033" "0033" "0033" \
                                     "0033" "0033" "0033" "0033" "0033" \
                                     "0033" "0033" "0033" "0033" "0033" \
                                     "0034" "0034" "0034" "0034" "0034" \
                                     "0034" "0034" "0034" "0034" "0034" \
                                     "0034" "0034" "0034" "0034" "0034" \
                                     "0034" "0034" "0034" "0034" "0034" )
                                     
        port_status_sysfs_idx_array=("0" "0" "0" "0" "0" \
                                     "0" "0" "0" "1" "1" \
                                     "1" "1" "1" "1" "1" \
                                     "1" "2" "2" "2" "2" \
                                     "0" "0" "0" "0" "0" \
                                     "0" "0" "0" "1" "1" \
                                     "1" "1" "1" "1" "1" \
                                     "1" "2" "2" "2" "2" )                                     

        port_status_bit_idx_array=("0" "1" "2" "3" "4" \
                                   "5" "6" "7" "0" "1" \
                                   "2" "3" "4" "5" "6" \
                                   "7" "0" "1" "2" "3" \
                                   "0" "1" "2" "3" "4" \
                                   "5" "6" "7" "0" "1" \
                                   "2" "3" "4" "5" "6" \
                                   "7" "0" "1" "2" "3" )

        port_led_sysfs_idx_array=("0" "0" "1" "1" "2" \
                                  "2" "3" "3" "4" "4" \
                                  "5" "5" "6" "6" "7" \
                                  "7" "8" "8" "9" "9" \
                                  "0" "0" "1" "1" "2" \
                                  "2" "3" "3" "4" "4" \
                                  "5" "5" "6" "6" "7" \
                                  "7" "8" "8" "9" "9" )

        for (( i=0; i<${#port_status_cpld_addr_array[@]}; i++ ))
        do
            # Module FAB Port Interrupt Status (0: Interrupted, 1:No Interrupt)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_intr_port_${port_status_sysfs_idx_array[${i}]}"
            port_module_interrupt_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_intr_port_${port_status_sysfs_idx_array[${i}]} ${LOG_REDIRECT}")
            port_module_interrupt_l=$(( (port_module_interrupt_reg & 1 << ${port_status_bit_idx_array[i]})>> ${port_status_bit_idx_array[i]} ))
            
            # Module FAB Port Absent Status (0: Present, 1:Absence)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_intr_present_${port_status_sysfs_idx_array[${i}]}"
            port_module_absent_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_intr_present_${port_status_sysfs_idx_array[${i}]} ${LOG_REDIRECT}")
            port_module_absent_l=$(( (port_module_absent_reg & 1 << ${port_status_bit_idx_array[i]})>> ${port_status_bit_idx_array[i]} ))

            # Module FAB Port Get Low Power Mode Status (0: Normal Power Mode, 1:Low Power Mode)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_lpmode_${port_status_sysfs_idx_array[${i}]}"
            port_lp_mode_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_lpmode_${port_status_sysfs_idx_array[${i}]} ${LOG_REDIRECT}")
            port_lp_mode=$(( (port_lp_mode_reg & 1 << ${port_status_bit_idx_array[i]})>> ${port_status_bit_idx_array[i]} ))
            
            # Module FAB Port Reset Status (0:Reset, 1:Normal)
            _check_filepath "/sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_reset_${port_status_sysfs_idx_array[${i}]}"
            port_reset_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_reset_${port_status_sysfs_idx_array[${i}]} ${LOG_REDIRECT}")
            port_reset=$(( (port_reset_reg & 1 << ${port_status_bit_idx_array[i]})>> ${port_status_bit_idx_array[i]} ))

            # Module FAB Port Dump EEPROM
            if [ "${port_module_absent_l}" == "0" ]; then
                _check_filepath "/sys/bus/i2c/devices/$((fab_port_eeprom_bus_id_base + i))-0050/eeprom"
                port_eeprom_p0_1st=$(eval  "dd if=/sys/bus/i2c/devices/$((fab_port_eeprom_bus_id_base + i))-0050/eeprom bs=128 count=2 skip=0  status=none ${LOG_REDIRECT} | hexdump -C")
                port_eeprom_p0_2nd=$(eval  "dd if=/sys/bus/i2c/devices/$((fab_port_eeprom_bus_id_base + i))-0050/eeprom bs=128 count=2 skip=0  status=none ${LOG_REDIRECT} | hexdump -C")
                port_eeprom_p17_1st=$(eval "dd if=/sys/bus/i2c/devices/$((fab_port_eeprom_bus_id_base + i))-0050/eeprom bs=128 count=1 skip=18 status=none ${LOG_REDIRECT} | hexdump -C")
                port_eeprom_p17_2nd=$(eval "dd if=/sys/bus/i2c/devices/$((fab_port_eeprom_bus_id_base + i))-0050/eeprom bs=128 count=1 skip=18 status=none ${LOG_REDIRECT} | hexdump -C")
                port_eeprom_p18=$(eval     "dd if=/sys/bus/i2c/devices/$((fab_port_eeprom_bus_id_base + i))-0050/eeprom bs=128 count=1 skip=19 status=none ${LOG_REDIRECT} | hexdump -C")
                if [ -z "$port_eeprom_p0_1st" ]; then
                    port_eeprom_p0_1st="ERROR!!! The result is empty. It should read failed (/sys/bus/i2c/devices/$((fab_port_eeprom_bus_id_base + i))-0050/eeprom)!!"
                fi
                
                # Full EEPROM Log
                if [ "${LOG_FILE_ENABLE}" == "1" ]; then
                    hexdump -C "/sys/bus/i2c/devices/$((fab_port_eeprom_bus_id_base + i))-0050/eeprom" > ${LOG_FOLDER_PATH}/qsfpdd_fab_port${i}_eeprom.log 2>&1
                fi
            else
                port_eeprom_p0_1st="N/A"
                port_eeprom_p0_2nd="N/A"
                port_eeprom_p17_1st="N/A"
                port_eeprom_p17_2nd="N/A"
                port_eeprom_p18="N/A"
            fi

            # QSFPDD FAB Port LED
            cpld_qsfpdd_led_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-${port_status_cpld_addr_array[${i}]}/cpld_qsfpdd_fab_led_${port_led_sysfs_idx_array[${i}]} ${LOG_REDIRECT}")
            if [ $(($i % 2)) -eq 0 ]; then
                cpld_qsfpdd_led_reg=$((cpld_qsfpdd_led_reg & 2#00001111))           # Clear higher 4 bits on reg value for even ports
            else
                cpld_qsfpdd_led_reg=$(((cpld_qsfpdd_led_reg & 2#11110000) >> 4))    # Clear and shift lower 4 bits on reg value for odd ports
            fi
            led_red=$((cpld_qsfpdd_led_reg & 2#00000001))           # (0: red off, 1:red on)
            led_green=$(((cpld_qsfpdd_led_reg & 2#00000010) >> 1))  # (0: green off, 1:green on)
            led_blue=$(((cpld_qsfpdd_led_reg & 2#00000100) >> 2))   # (0: blue off, 1:blue on)
            led_blink=$(((cpld_qsfpdd_led_reg & 2#00001000) >> 3))  # (0: solid, 1:blinking)

            _echo "[Port${i} Status Reg Raw]: ${port_module_interrupt_reg}"
            _echo "[Port${i} Module INT (L)]: ${port_module_interrupt_l}"
            _echo "[Port${i} Module Absent Reg Raq]: ${port_module_absent_reg}"
            _echo "[Port${i} Module Absent ]: ${port_module_absent_l}"
            _echo "[Port${i} Config Reg Raw]: ${port_lp_mode_reg}"
            _echo "[Port${i} Low Power Mode]: ${port_lp_mode}"
            _echo "[Port${i} Reset Reg Raw]: ${port_reset_reg}"
            _echo "[Port${i} Reset Status]: ${port_reset}"            
            _echo "[Port${i} LED Reg Raw   ]: ${cpld_qsfpdd_led_reg}"
            _echo "[Port${i} LED Status    ]: Red: ${led_red}, Green: ${led_green}, "\
                                            "Blue: ${led_blue}, Blinking: ${led_blink}"           
            _echo "[Port${i} EEPROM Page0-0(1st)]:"
            _echo "${port_eeprom_p0_1st}"
            _echo "[Port${i} EEPROM Page0-0(2nd)]:"
            _echo "${port_eeprom_p0_2nd}"
            _echo "[Port${i} EEPROM Page17 (1st)]:"
            _echo "${port_eeprom_p17_1st}"
            _echo "[Port${i} EEPROM Page17 (2nd)]:"
            _echo "${port_eeprom_p17_2nd}"
            _echo "[Port${i} EEPROM Page18      ]:"
            _echo "${port_eeprom_p18}"
            _echo ""
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

    if [[ $MODEL_NAME == *"NCP3"* ]]; then
        bus_id="1"
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi

    _check_filepath "/sys/bus/i2c/devices/${bus_id}-0031/cpld_sfp_status"
    cpld_sfp_port_status_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0031/cpld_sfp_status ${LOG_REDIRECT}")
    
    port0_tx_fault=$(((cpld_sfp_port_status_reg & 2#00000010) >> 1))
    port0_rx_los=$(((cpld_sfp_port_status_reg & 2#00000100) >> 2))

    _check_filepath "/sys/bus/i2c/devices/${bus_id}-0031/cpld_sfp_config"
    cpld_sfp_port_config_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0031/cpld_sfp_config ${LOG_REDIRECT}")
    port0_tx_disable=$(((cpld_sfp_port_config_reg & 2#00000001) >> 0))    

    _echo "[Port0 Status Reg Raw]: ${cpld_sfp_port_status_reg}"
    _echo "[Port0 Tx Fault      ]: ${port0_tx_fault}"    
    _echo "[Port0 Rx LOS        ]: ${port0_rx_los}"    
    _echo "[Port0 Config Reg Raw]: ${cpld_sfp_port_config_reg}"
    _echo "[Port0 Tx Disable    ]: ${port0_tx_disable}"

    _check_filepath "/sys/bus/i2c/devices/${bus_id}-0032/cpld_sfp_status"
    cpld_sfp_port_status_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0032/cpld_sfp_status ${LOG_REDIRECT}")
    
    port1_tx_fault=$(((cpld_sfp_port_status_reg & 2#00000010) >> 1))
    port1_rx_los=$(((cpld_sfp_port_status_reg & 2#00000100) >> 2))

    _check_filepath "/sys/bus/i2c/devices/${bus_id}-0032/cpld_sfp_config"
    cpld_sfp_port_config_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id}-0032/cpld_sfp_config ${LOG_REDIRECT}")
    port1_tx_disable=$(((cpld_sfp_port_config_reg & 2#00000001) >> 0))    

    _echo "[Port1 Status Reg Raw]: ${cpld_sfp_port_status_reg}"
    _echo "[Port1 Tx Fault      ]: ${port1_tx_fault}"    
    _echo "[Port1 Rx LOS        ]: ${port1_rx_los}"    
    _echo "[Port1 Config Reg Raw]: ${cpld_sfp_port_config_reg}"
    _echo "[Port1 Tx Disable    ]: ${port1_tx_disable}"    
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
}

function _show_cpu_temperature {
    if [ "${BSP_INIT_FLAG}" == "1" ]; then
        _show_cpu_temperature_sysfs
    fi
}

function _show_cpld_interrupt_sysfs {
    _banner "Show CPLD Interrupt"

    if [[ $MODEL_NAME == *"NCP3"* ]]; then
        bus_id=("1" "1" "1" "30" "30")
        cpld_addr_array=("0030" "0031" "0032" "0033" "0034")
        sysfs_array=("cpld_qsfpdd_intr_port" "cpld_qsfpdd_intr_present" "cpld_qsfpdd_intr_fuse")
        sysfs_desc_array=("QSFPDD Port" "QSFPDD Present" "QSFPDD Fuse")

        # MB CPLD Interrupt
        # CPLD 1
        cpld_interrupt_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id[0]}-${cpld_addr_array[0]}/cpld_cpldx_intr ${LOG_REDIRECT}")
        cpld2_interrupt_l=$(((cpld_interrupt_reg & 2#00000001) >> 0))
        cpld3_interrupt_l=$(((cpld_interrupt_reg & 2#00000010) >> 1))
        cpld4_interrupt_l=$(((cpld_interrupt_reg & 2#00000100) >> 2))
        cpld5_interrupt_l=$(((cpld_interrupt_reg & 2#00001000) >> 3))
        psu0_interrupt_l=$(((cpld_interrupt_reg & 2#00100000) >> 5))
        psu1_interrupt_l=$(((cpld_interrupt_reg & 2#01000000) >> 6))
        
        _echo "[CPLD2 Interrupt(L)  ]: ${cpld2_interrupt_l}"
        _echo "[CPLD3 Interrupt(L)  ]: ${cpld3_interrupt_l}"
        _echo "[CPLD4 Interrupt(L)  ]: ${cpld4_interrupt_l}"
        _echo "[CPLD5 Interrupt(L)  ]: ${cpld5_interrupt_l}"
        _echo "[PSU0 Interrupt(L)   ]: ${psu0_interrupt_l}"
        _echo "[PSU1 Interrupt(L)   ]: ${psu1_interrupt_l}"
        
        # CPLD 2-5
        for (( i=1; i<${#cpld_addr_array[@]}; i++ ))
        do
            for (( j=0; j<${#sysfs_array[@]}; j++ ))
            do
                for (( k=0; k<3; k++ ))
                do
                    cpld_interrupt_reg=$(eval "cat /sys/bus/i2c/devices/${bus_id[${i}]}-${cpld_addr_array[${i}]}/${sysfs_array[${j}]}_${k} ${LOG_REDIRECT}")
                    _echo "[CPLD$((i+1)) ${sysfs_desc_array[${j}]} Interrupt(L) Reg (${k})]: ${cpld_interrupt_reg}"
                done                
            done
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

    if [[ $MODEL_NAME == *"NCP3"* ]]; then
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

    if [[ $MODEL_NAME == *"NCP3"* ]]; then
        for ((i=496;i<=502;i++))
        do
            _check_filepath "/sys/class/gpio/gpio$((${i}-${GPIO_OFFSET}))/value"
            beacon_lled=$(eval "cat /sys/class/gpio/gpio$((${i}-${GPIO_OFFSET}))/value ${LOG_REDIRECT}")
            _echo "[Left Beacon LED${i} ]: ${beacon_lled}"
        done

        # Right LED
        for ((i=504;i<=510;i++))
        do
            _check_filepath "/sys/class/gpio/gpio$((${i}-${GPIO_OFFSET}))/value"
            beacon_rled=$(eval "cat /sys/class/gpio/gpio$((${i}-${GPIO_OFFSET}))/value ${LOG_REDIRECT}")
            _echo "[Right Beacon LED${i}]: ${beacon_rled}"
        done
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
}

function _show_beacon_led {
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
   
    cmd_array=("lsblk" \
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
    _banner "Show BMC Device Status"
    
    if [[ $MODEL_NAME == *"NCP3"* ]]; then 
        # Step 1: Get PSU 0 Status Registers
        _echo "[PSU 0 Device Status (BMC) ]"        
        status_word_psu0=$(eval     "ipmitool i2c bus=4 0x80 0x2 0x79 ${LOG_REDIRECT}" | head -n 1)
        status_vout_psu0=$(eval     "ipmitool i2c bus=4 0x80 0x1 0x7a ${LOG_REDIRECT}" | head -n 1)
        status_iout_psu0=$(eval     "ipmitool i2c bus=4 0x80 0x1 0x7b ${LOG_REDIRECT}" | head -n 1)
        status_temp_psu0=$(eval     "ipmitool i2c bus=4 0x80 0x1 0x7d ${LOG_REDIRECT}" | head -n 1)
        status_cml_psu0=$(eval      "ipmitool i2c bus=4 0x80 0x1 0x7e ${LOG_REDIRECT}" | head -n 1)
        status_other_psu0=$(eval    "ipmitool i2c bus=4 0x80 0x1 0x7f ${LOG_REDIRECT}" | head -n 1)
        status_mfr_psu0=$(eval      "ipmitool i2c bus=4 0x80 0x1 0x80 ${LOG_REDIRECT}" | head -n 1)
        status_fan_psu0=$(eval      "ipmitool i2c bus=4 0x80 0x1 0x81 ${LOG_REDIRECT}" | head -n 1)
        status_oem_0xf0_psu0=$(eval "ipmitool i2c bus=4 0x80 0x3 0xf0 ${LOG_REDIRECT}" | head -n 1)
        status_oem_0xf1_psu0=$(eval "ipmitool i2c bus=4 0x80 0x1 0xf1 ${LOG_REDIRECT}" | head -n 1)        
        _echo "[PSU0 Status Word ][0x79]: ${status_word_psu0}"
        _echo "[PSU0 Status VOUT ][0x7A]: ${status_vout_psu0}"
        _echo "[PSU0 Status IOUT ][0x7B]: ${status_iout_psu0}"
        _echo "[PSU0 Status Temp ][0x7D]: ${status_temp_psu0}"
        _echo "[PSU0 Status CML  ][0x7E]: ${status_cml_psu0}"
        _echo "[PSU0 Status Other][0x7F]: ${status_other_psu0}"
        _echo "[PSU0 Status MFR  ][0x80]: ${status_mfr_psu0}"
        _echo "[PSU0 Status FAN  ][0x81]: ${status_fan_psu0}"
        _echo "[PSU0 Status OEM  ][0xF0]: ${status_oem_0xf0_psu0}"
        _echo "[PSU0 Status OEM  ][0xF1]: ${status_oem_0xf1_psu0}"

        # Step 2: Get PSU 1 Status Registers
        _echo "[PSU 1 Device Status (BMC) ]"
        status_word_psu1=$(eval     "ipmitool i2c bus=5 0x80 0x2 0x79 ${LOG_REDIRECT}" | head -n 1)
        status_vout_psu1=$(eval     "ipmitool i2c bus=5 0x80 0x1 0x7a ${LOG_REDIRECT}" | head -n 1)
        status_iout_psu1=$(eval     "ipmitool i2c bus=5 0x80 0x1 0x7b ${LOG_REDIRECT}" | head -n 1)
        status_temp_psu1=$(eval     "ipmitool i2c bus=5 0x80 0x1 0x7d ${LOG_REDIRECT}" | head -n 1)
        status_cml_psu1=$(eval      "ipmitool i2c bus=5 0x80 0x1 0x7e ${LOG_REDIRECT}" | head -n 1)
        status_other_psu1=$(eval    "ipmitool i2c bus=5 0x80 0x1 0x7f ${LOG_REDIRECT}" | head -n 1)
        status_mfr_psu1=$(eval      "ipmitool i2c bus=5 0x80 0x1 0x80 ${LOG_REDIRECT}" | head -n 1)
        status_fan_psu1=$(eval      "ipmitool i2c bus=5 0x80 0x1 0x81 ${LOG_REDIRECT}" | head -n 1)
        status_oem_0xf0_psu1=$(eval "ipmitool i2c bus=5 0x80 0x3 0xf0 ${LOG_REDIRECT}" | head -n 1)
        status_oem_0xf1_psu1=$(eval "ipmitool i2c bus=5 0x80 0x1 0xf1 ${LOG_REDIRECT}" | head -n 1)
        _echo "[PSU1 Status Word ][0x79]: ${status_word_psu1}"
        _echo "[PSU1 Status VOUT ][0x7A]: ${status_vout_psu1}"
        _echo "[PSU1 Status IOUT ][0x7B]: ${status_iout_psu1}"
        _echo "[PSU1 Status Temp ][0x7D]: ${status_temp_psu1}"
        _echo "[PSU1 Status CML  ][0x7E]: ${status_cml_psu1}"
        _echo "[PSU1 Status Other][0x7F]: ${status_other_psu1}"
        _echo "[PSU1 Status MFR  ][0x80]: ${status_mfr_psu1}"
        _echo "[PSU1 Status FAN  ][0x81]: ${status_fan_psu1}"
        _echo "[PSU1 Status OEM  ][0xF0]: ${status_oem_0xf0_psu1}"
        _echo "[PSU1 Status OEM  ][0xF1]: ${status_oem_0xf1_psu1}"
        _echo ""
    else
        _echo "Unknown MODEL_NAME (${MODEL_NAME}), exit!!!"
        exit 1
    fi
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
    #_show_memory_correctable_error_count
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
    _compression

    echo "#   done..."
}

_main

