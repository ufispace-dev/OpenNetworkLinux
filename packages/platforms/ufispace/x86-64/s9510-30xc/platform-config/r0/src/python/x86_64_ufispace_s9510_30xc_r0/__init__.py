from onl.platform.base import *
from onl.platform.ufispace import *
from struct import *
from ctypes import c_int, sizeof
import os
import sys
import subprocess
import time
import fcntl
import yaml

def msg(s, fatal=False):
    sys.stderr.write(s)
    sys.stderr.flush()
    if fatal:
        sys.exit(1)

class IPMI_Ioctl(object):
    _IONONE = 0
    _IOWRITE = 1
    _IOREAD = 2

    IPMI_MAINTENANCE_MODE_AUTO = 0
    IPMI_MAINTENANCE_MODE_OFF  = 1
    IPMI_MAINTENANCE_MODE_ON   = 2

    IPMICTL_GET_MAINTENANCE_MODE_CMD = _IOREAD << 30 | sizeof(c_int) << 16 | \
        ord('i') << 8 | 30  # from ipmi.h
    IPMICTL_SET_MAINTENANCE_MODE_CMD = _IOWRITE << 30 | sizeof(c_int) << 16 | \
        ord('i') << 8 | 31  # from ipmi.h

    def __init__(self):
        self.ipmidev = None
        devnodes=["/dev/ipmi0", "/dev/ipmi/0", "/dev/ipmidev/0"]
        for dev in devnodes:
            try:
                self.ipmidev = open(dev, 'r+')
                break
            except Exception as e:
                print("open file {} failed, error: {}".format(dev, e))

    def __del__(self):
        if self.ipmidev is not None:
            self.ipmidev.close()

    def get_ipmi_maintenance_mode(self):
        input_buffer=pack('i',0)
        out_buffer=fcntl.ioctl(self.ipmidev, self.IPMICTL_GET_MAINTENANCE_MODE_CMD, input_buffer)
        maintanence_mode=unpack('i',out_buffer)[0]

        return maintanence_mode

    def set_ipmi_maintenance_mode(self, mode):
        fcntl.ioctl(self.ipmidev, self.IPMICTL_SET_MAINTENANCE_MODE_CMD, c_int(mode))

class OnlPlatform_x86_64_ufispace_s9510_30xc_r0(OnlPlatformUfiSpace):
    PLATFORM='x86-64-ufispace-s9510-30xc-r0'
    MODEL="S9510-30XC"
    SYS_OBJECT_ID=".9510.30"
    PORT_COUNT=30
    PORT_CONFIG="28x25 + 2x100"
    LEVEL_INFO=1
    LEVEL_ERR=2
    PATH_SYS_I2C_DEV_ATTR="/sys/bus/i2c/devices/{}-{:0>4x}/{}"
    PATH_SYS_GPIO = "/sys/class/gpio"
    PATH_LPC="/sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc"
    PATH_LPC_GRP_BSP=PATH_LPC+"/bsp"
    PATH_LPC_GRP_MB_CPLD=PATH_LPC+"/mb_cpld"
    PATH_LPC_GRP_EC=PATH_LPC+"/ec"
    PATH_PORT_CONFIG="/lib/platform-config/"+PLATFORM+"/onl/port_config.yml"

    port_type_dict = {
        0x03: [2, 'SFP/SFP+/SFP28'],  # [dev_class, type_str]
        0x0B: [2, 'DWDM-SFP/SFP+'],
        0x0C: [1, 'QSFP'],
        0x0D: [1, 'QSFP+'],
        0x11: [1, 'QSFP28'],
        0x18: [3, 'QSFP-DD Double Density 8x (INF-8628)'],
        0x19: [3, 'OSFP 8x Pluggable Transceiver'],
        0x1E: [3, 'QSFP+ or later with CMIS spec'],
        0x1F: [3, 'SFP-DD Double Density 2X Pluggable Transceiver with CMIS spec'],
    }

    port_conf = {
        0: {"type": "QSFP"  , "bus": 11, "driver": "optoe1", "abs": {'type': 'gpio', 'data': 485}},
        1: {"type": "QSFP"  , "bus": 10, "driver": "optoe1", "abs": {'type': 'gpio', 'data': 484}},
        2: {"type": "SFP"   , "bus": 14, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 375}},
        3: {"type": "SFP"   , "bus": 15, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 374}},
        4: {"type": "SFP"   , "bus": 16, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 373}},
        5: {"type": "SFP"   , "bus": 17, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 372}},
        6: {"type": "SFP"   , "bus": 18, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 371}},
        7: {"type": "SFP"   , "bus": 19, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 370}},
        8: {"type": "SFP"   , "bus": 20, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 369}},
        9: {"type": "SFP"   , "bus": 21, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 368}},
        10:{"type": "SFP"   , "bus": 22, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 383}},
        11:{"type": "SFP"   , "bus": 23, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 382}},
        12:{"type": "SFP"   , "bus": 24, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 381}},
        13:{"type": "SFP"   , "bus": 25, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 380}},
        14:{"type": "SFP"   , "bus": 26, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 379}},
        15:{"type": "SFP"   , "bus": 27, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 378}},
        16:{"type": "SFP"   , "bus": 28, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 377}},
        17:{"type": "SFP"   , "bus": 29, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 376}},
        18:{"type": "SFP"   , "bus": 30, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 359}},
        19:{"type": "SFP"   , "bus": 31, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 358}},
        20:{"type": "SFP"   , "bus": 32, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 357}},
        21:{"type": "SFP"   , "bus": 33, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 356}},
        22:{"type": "SFP"   , "bus": 34, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 355}},
        23:{"type": "SFP"   , "bus": 35, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 354}},
        24:{"type": "SFP"   , "bus": 36, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 353}},
        25:{"type": "SFP"   , "bus": 37, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 352}},
        26:{"type": "SFP"   , "bus": 38, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 367}},
        27:{"type": "SFP"   , "bus": 39, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 366}},
        28:{"type": "SFP"   , "bus": 40, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 365}},
        29:{"type": "SFP"   , "bus": 41, "driver": "optoe2", "abs": {'type': 'gpio', 'data': 364}},
    }

    gpio_map = {
        511:{'offset': {"max": 0  , "base": 15 }, 'dir': 'in'   , 'desc': "SKU_ID3"},
        510:{'offset': {"max": 1  , "base": 14 }, 'dir': 'in'   , 'desc': "SKU_ID2"},
        509:{'offset': {"max": 2  , "base": 13 }, 'dir': 'in'   , 'desc': "SKU_ID1"},
        508:{'offset': {"max": 3  , "base": 12 }, 'dir': 'in'   , 'desc': "SKU_ID0"},
        507:{'offset': {"max": 4  , "base": 11 }, 'dir': 'in'   , 'desc': "HW_REV_ID0"},
        506:{'offset': {"max": 5  , "base": 10 }, 'dir': 'in'   , 'desc': "HW_REV_ID1"},
        505:{'offset': {"max": 6  , "base": 9  }, 'dir': 'in'   , 'desc': "BUILD_ID0"},
        504:{'offset': {"max": 7  , "base": 8  }, 'dir': 'in'   , 'desc': "BUILD_ID1"},
        503:{'offset': {"max": 8  , "base": 7  }, 'dir': 'in'   , 'desc': "RSVD_ID"},
        502:{'offset': {"max": 9  , "base": 6  }, 'dir': 'in'   , 'desc': "REV_ID2"},
        501:{'offset': {"max": 10 , "base": 5  }, 'dir': 'in'   , 'desc': "REV_ID1"},
        500:{'offset': {"max": 11 , "base": 4  }, 'dir': 'in'   , 'desc': "REV_ID0"},
        499:{'offset': {"max": 12 , "base": 3  }, 'dir': 'in'   , 'desc': "DEPH_ID"},
        498:{'offset': {"max": 13 , "base": 2  }, 'dir': 'in'   , 'desc': "MAC_VCORE_ROV2"},
        497:{'offset': {"max": 14 , "base": 1  }, 'dir': 'in'   , 'desc': "MAC_VCORE_ROV1"},
        496:{'offset': {"max": 15 , "base": 0  }, 'dir': 'in'   , 'desc': "MAC_VCORE_ROV0"},
        495:{'offset': {"max": 16 , "base": 31 }, 'dir': 'in'   , 'desc': "NI"},
        494:{'offset': {"max": 17 , "base": 30 }, 'dir': 'in'   , 'desc': "NI"},
        493:{'offset': {"max": 18 , "base": 29 }, 'dir': 'high' , 'desc': "QSFP28_P01_RST_N"},
        492:{'offset': {"max": 19 , "base": 28 }, 'dir': 'high' , 'desc': "QSFP28_P02_RST_N"},
        491:{'offset': {"max": 20 , "base": 27 }, 'dir': 'in'   , 'desc': "NI"},
        490:{'offset': {"max": 21 , "base": 26 }, 'dir': 'in'   , 'desc': "NI"},
        489:{'offset': {"max": 22 , "base": 25 }, 'dir': 'low'  , 'desc': "QSFP28_P01_LPMODE"},
        488:{'offset': {"max": 23 , "base": 24 }, 'dir': 'low'  , 'desc': "QSFP28_P02_LPMODE"},
        487:{'offset': {"max": 24 , "base": 23 }, 'dir': 'in'   , 'desc': "NI"},
        486:{'offset': {"max": 25 , "base": 22 }, 'dir': 'in'   , 'desc': "NI"},
        485:{'offset': {"max": 26 , "base": 21 }, 'dir': 'in'   , 'desc': "QSFP28_P01_PRSNT_N"},
        484:{'offset': {"max": 27 , "base": 20 }, 'dir': 'in'   , 'desc': "QSFP28_P02_PRSNT_N"},
        483:{'offset': {"max": 28 , "base": 19 }, 'dir': 'in'   , 'desc': "NI"},
        482:{'offset': {"max": 29 , "base": 18 }, 'dir': 'in'   , 'desc': "NI"},
        481:{'offset': {"max": 30 , "base": 17 }, 'dir': 'in'   , 'desc': "QSFP28_P01_INT_N"},
        480:{'offset': {"max": 31 , "base": 16 }, 'dir': 'in'   , 'desc': "QSFP28_P02_INT_N"},
        479:{'offset': {"max": 32 , "base": 47 }, 'dir': 'low'  , 'desc': "SFP28_P11_TX_DIS"},
        478:{'offset': {"max": 33 , "base": 46 }, 'dir': 'low'  , 'desc': "SFP28_P12_TX_DIS"},
        477:{'offset': {"max": 34 , "base": 45 }, 'dir': 'low'  , 'desc': "SFP28_P13_TX_DIS"},
        476:{'offset': {"max": 35 , "base": 44 }, 'dir': 'low'  , 'desc': "SFP28_P14_TX_DIS"},
        475:{'offset': {"max": 36 , "base": 43 }, 'dir': 'low'  , 'desc': "SFP28_P15_TX_DIS"},
        474:{'offset': {"max": 37 , "base": 42 }, 'dir': 'low'  , 'desc': "SFP28_P16_TX_DIS"},
        473:{'offset': {"max": 38 , "base": 41 }, 'dir': 'low'  , 'desc': "SFP28_P17_TX_DIS"},
        472:{'offset': {"max": 39 , "base": 40 }, 'dir': 'low'  , 'desc': "SFP28_P18_TX_DIS"},
        471:{'offset': {"max": 40 , "base": 39 }, 'dir': 'low'  , 'desc': "SFP28_P03_TX_DIS"},
        470:{'offset': {"max": 41 , "base": 38 }, 'dir': 'low'  , 'desc': "SFP28_P04_TX_DIS"},
        469:{'offset': {"max": 42 , "base": 37 }, 'dir': 'low'  , 'desc': "SFP28_P05_TX_DIS"},
        468:{'offset': {"max": 43 , "base": 36 }, 'dir': 'low'  , 'desc': "SFP28_P06_TX_DIS"},
        467:{'offset': {"max": 44 , "base": 35 }, 'dir': 'low'  , 'desc': "SFP28_P07_TX_DIS"},
        466:{'offset': {"max": 45 , "base": 34 }, 'dir': 'low'  , 'desc': "SFP28_P08_TX_DIS"},
        465:{'offset': {"max": 46 , "base": 33 }, 'dir': 'low'  , 'desc': "SFP28_P09_TX_DIS"},
        464:{'offset': {"max": 47 , "base": 32 }, 'dir': 'low'  , 'desc': "SFP28_P10_TX_DIS"},
        463:{'offset': {"max": 48 , "base": 63 }, 'dir': 'low'  , 'desc': "SFP28_P27_TX_DIS"},
        462:{'offset': {"max": 49 , "base": 62 }, 'dir': 'low'  , 'desc': "SFP28_P28_TX_DIS"},
        461:{'offset': {"max": 50 , "base": 61 }, 'dir': 'low'  , 'desc': "SFP28_P29_TX_DIS"},
        460:{'offset': {"max": 51 , "base": 60 }, 'dir': 'low'  , 'desc': "SFP28_P30_TX_DIS"},
        459:{'offset': {"max": 52 , "base": 59 }, 'dir': 'in'   , 'desc': "NI"},
        458:{'offset': {"max": 53 , "base": 58 }, 'dir': 'in'   , 'desc': "NI"},
        457:{'offset': {"max": 54 , "base": 57 }, 'dir': 'in'   , 'desc': "NI"},
        456:{'offset': {"max": 55 , "base": 56 }, 'dir': 'in'   , 'desc': "NI"},
        455:{'offset': {"max": 56 , "base": 55 }, 'dir': 'low'  , 'desc': "SFP28_P19_TX_DIS"},
        454:{'offset': {"max": 57 , "base": 54 }, 'dir': 'low'  , 'desc': "SFP28_P20_TX_DIS"},
        453:{'offset': {"max": 58 , "base": 53 }, 'dir': 'low'  , 'desc': "SFP28_P21_TX_DIS"},
        452:{'offset': {"max": 59 , "base": 52 }, 'dir': 'low'  , 'desc': "SFP28_P22_TX_DIS"},
        451:{'offset': {"max": 60 , "base": 51 }, 'dir': 'low'  , 'desc': "SFP28_P23_TX_DIS"},
        450:{'offset': {"max": 61 , "base": 50 }, 'dir': 'low'  , 'desc': "SFP28_P24_TX_DIS"},
        449:{'offset': {"max": 62 , "base": 49 }, 'dir': 'low'  , 'desc': "SFP28_P25_TX_DIS"},
        448:{'offset': {"max": 63 , "base": 48 }, 'dir': 'low'  , 'desc': "SFP28_P26_TX_DIS"},
        447:{'offset': {"max": 64 , "base": 79 }, 'dir': 'in'   , 'desc': "SFP28_P11_TX_FLT"},
        446:{'offset': {"max": 65 , "base": 78 }, 'dir': 'in'   , 'desc': "SFP28_P12_TX_FLT"},
        445:{'offset': {"max": 66 , "base": 77 }, 'dir': 'in'   , 'desc': "SFP28_P13_TX_FLT"},
        444:{'offset': {"max": 67 , "base": 76 }, 'dir': 'in'   , 'desc': "SFP28_P14_TX_FLT"},
        443:{'offset': {"max": 68 , "base": 75 }, 'dir': 'in'   , 'desc': "SFP28_P15_TX_FLT"},
        442:{'offset': {"max": 69 , "base": 74 }, 'dir': 'in'   , 'desc': "SFP28_P16_TX_FLT"},
        441:{'offset': {"max": 70 , "base": 73 }, 'dir': 'in'   , 'desc': "SFP28_P17_TX_FLT"},
        440:{'offset': {"max": 71 , "base": 72 }, 'dir': 'in'   , 'desc': "SFP28_P18_TX_FLT"},
        439:{'offset': {"max": 72 , "base": 71 }, 'dir': 'in'   , 'desc': "SFP28_P03_TX_FLT"},
        438:{'offset': {"max": 73 , "base": 70 }, 'dir': 'in'   , 'desc': "SFP28_P04_TX_FLT"},
        437:{'offset': {"max": 74 , "base": 69 }, 'dir': 'in'   , 'desc': "SFP28_P05_TX_FLT"},
        436:{'offset': {"max": 75 , "base": 68 }, 'dir': 'in'   , 'desc': "SFP28_P06_TX_FLT"},
        435:{'offset': {"max": 76 , "base": 67 }, 'dir': 'in'   , 'desc': "SFP28_P07_TX_FLT"},
        434:{'offset': {"max": 77 , "base": 66 }, 'dir': 'in'   , 'desc': "SFP28_P08_TX_FLT"},
        433:{'offset': {"max": 78 , "base": 65 }, 'dir': 'in'   , 'desc': "SFP28_P09_TX_FLT"},
        432:{'offset': {"max": 79 , "base": 64 }, 'dir': 'in'   , 'desc': "SFP28_P10_TX_FLT"},
        431:{'offset': {"max": 80 , "base": 95 }, 'dir': 'in'   , 'desc': "SFP28_P27_TX_FLT"},
        430:{'offset': {"max": 81 , "base": 94 }, 'dir': 'in'   , 'desc': "SFP28_P28_TX_FLT"},
        429:{'offset': {"max": 82 , "base": 93 }, 'dir': 'in'   , 'desc': "SFP28_P29_TX_FLT"},
        428:{'offset': {"max": 83 , "base": 92 }, 'dir': 'in'   , 'desc': "SFP28_P30_TX_FLT"},
        427:{'offset': {"max": 84 , "base": 91 }, 'dir': 'in'   , 'desc': "NI"},
        426:{'offset': {"max": 85 , "base": 90 }, 'dir': 'in'   , 'desc': "NI"},
        425:{'offset': {"max": 86 , "base": 89 }, 'dir': 'in'   , 'desc': "NI"},
        424:{'offset': {"max": 87 , "base": 88 }, 'dir': 'in'   , 'desc': "NI"},
        423:{'offset': {"max": 88 , "base": 87 }, 'dir': 'in'   , 'desc': "SFP28_P19_TX_FLT"},
        422:{'offset': {"max": 89 , "base": 86 }, 'dir': 'in'   , 'desc': "SFP28_P20_TX_FLT"},
        421:{'offset': {"max": 90 , "base": 85 }, 'dir': 'in'   , 'desc': "SFP28_P21_TX_FLT"},
        420:{'offset': {"max": 91 , "base": 84 }, 'dir': 'in'   , 'desc': "SFP28_P22_TX_FLT"},
        419:{'offset': {"max": 92 , "base": 83 }, 'dir': 'in'   , 'desc': "SFP28_P23_TX_FLT"},
        418:{'offset': {"max": 93 , "base": 82 }, 'dir': 'in'   , 'desc': "SFP28_P24_TX_FLT"},
        417:{'offset': {"max": 94 , "base": 81 }, 'dir': 'in'   , 'desc': "SFP28_P25_TX_FLT"},
        416:{'offset': {"max": 95 , "base": 80 }, 'dir': 'in'   , 'desc': "SFP28_P26_TX_FLT"},
        415:{'offset': {"max": 96 , "base": 111}, 'dir': 'high' , 'desc': "SFP28_P11_RATE_SEL"},
        414:{'offset': {"max": 97 , "base": 110}, 'dir': 'high' , 'desc': "SFP28_P12_RATE_SEL"},
        413:{'offset': {"max": 98 , "base": 109}, 'dir': 'high' , 'desc': "SFP28_P13_RATE_SEL"},
        412:{'offset': {"max": 99 , "base": 108}, 'dir': 'high' , 'desc': "SFP28_P14_RATE_SEL"},
        411:{'offset': {"max": 100, "base": 107}, 'dir': 'high' , 'desc': "SFP28_P15_RATE_SEL"},
        410:{'offset': {"max": 101, "base": 106}, 'dir': 'high' , 'desc': "SFP28_P16_RATE_SEL"},
        409:{'offset': {"max": 102, "base": 105}, 'dir': 'high' , 'desc': "SFP28_P17_RATE_SEL"},
        408:{'offset': {"max": 103, "base": 104}, 'dir': 'high' , 'desc': "SFP28_P18_RATE_SEL"},
        407:{'offset': {"max": 104, "base": 103}, 'dir': 'high' , 'desc': "SFP28_P03_RATE_SEL"},
        406:{'offset': {"max": 105, "base": 102}, 'dir': 'high' , 'desc': "SFP28_P04_RATE_SEL"},
        405:{'offset': {"max": 106, "base": 101}, 'dir': 'high' , 'desc': "SFP28_P05_RATE_SEL"},
        404:{'offset': {"max": 107, "base": 100}, 'dir': 'high' , 'desc': "SFP28_P06_RATE_SEL"},
        403:{'offset': {"max": 108, "base": 99 }, 'dir': 'high' , 'desc': "SFP28_P07_RATE_SEL"},
        402:{'offset': {"max": 109, "base": 98 }, 'dir': 'high' , 'desc': "SFP28_P08_RATE_SEL"},
        401:{'offset': {"max": 110, "base": 97 }, 'dir': 'high' , 'desc': "SFP28_P09_RATE_SEL"},
        400:{'offset': {"max": 111, "base": 96 }, 'dir': 'high' , 'desc': "SFP28_P10_RATE_SEL"},
        399:{'offset': {"max": 112, "base": 127}, 'dir': 'high' , 'desc': "SFP28_P27_RATE_SEL"},
        398:{'offset': {"max": 113, "base": 126}, 'dir': 'high' , 'desc': "SFP28_P28_RATE_SEL"},
        397:{'offset': {"max": 114, "base": 125}, 'dir': 'high' , 'desc': "SFP28_P29_RATE_SEL"},
        396:{'offset': {"max": 115, "base": 124}, 'dir': 'high' , 'desc': "SFP28_P30_RATE_SEL"},
        395:{'offset': {"max": 116, "base": 123}, 'dir': 'in'   , 'desc': "RST_I2C_MUX8_N"},
        394:{'offset': {"max": 117, "base": 122}, 'dir': 'in'   , 'desc': "RST_I2C_MUX7_N"},
        393:{'offset': {"max": 118, "base": 121}, 'dir': 'in'   , 'desc': "UART_MUX_SEL"},
        392:{'offset': {"max": 119, "base": 120}, 'dir': 'in'   , 'desc': "NI"},
        391:{'offset': {"max": 120, "base": 119}, 'dir': 'high' , 'desc': "SFP28_P19_RATE_SEL"},
        390:{'offset': {"max": 121, "base": 118}, 'dir': 'high' , 'desc': "SFP28_P20_RATE_SEL"},
        389:{'offset': {"max": 122, "base": 117}, 'dir': 'high' , 'desc': "SFP28_P21_RATE_SEL"},
        388:{'offset': {"max": 123, "base": 116}, 'dir': 'high' , 'desc': "SFP28_P22_RATE_SEL"},
        387:{'offset': {"max": 124, "base": 115}, 'dir': 'high' , 'desc': "SFP28_P23_RATE_SEL"},
        386:{'offset': {"max": 125, "base": 114}, 'dir': 'high' , 'desc': "SFP28_P24_RATE_SEL"},
        385:{'offset': {"max": 126, "base": 113}, 'dir': 'high' , 'desc': "SFP28_P25_RATE_SEL"},
        384:{'offset': {"max": 127, "base": 112}, 'dir': 'high' , 'desc': "SFP28_P26_RATE_SEL"},
        383:{'offset': {"max": 128, "base": 143}, 'dir': 'in'   , 'desc': "SFP28_P11_MOD_ABS"},
        382:{'offset': {"max": 129, "base": 142}, 'dir': 'in'   , 'desc': "SFP28_P12_MOD_ABS"},
        381:{'offset': {"max": 130, "base": 141}, 'dir': 'in'   , 'desc': "SFP28_P13_MOD_ABS"},
        380:{'offset': {"max": 131, "base": 140}, 'dir': 'in'   , 'desc': "SFP28_P14_MOD_ABS"},
        379:{'offset': {"max": 132, "base": 139}, 'dir': 'in'   , 'desc': "SFP28_P15_MOD_ABS"},
        378:{'offset': {"max": 133, "base": 138}, 'dir': 'in'   , 'desc': "SFP28_P16_MOD_ABS"},
        377:{'offset': {"max": 134, "base": 137}, 'dir': 'in'   , 'desc': "SFP28_P17_MOD_ABS"},
        376:{'offset': {"max": 135, "base": 136}, 'dir': 'in'   , 'desc': "SFP28_P18_MOD_ABS"},
        375:{'offset': {"max": 136, "base": 135}, 'dir': 'in'   , 'desc': "SFP28_P03_MOD_ABS"},
        374:{'offset': {"max": 137, "base": 134}, 'dir': 'in'   , 'desc': "SFP28_P04_MOD_ABS"},
        373:{'offset': {"max": 138, "base": 133}, 'dir': 'in'   , 'desc': "SFP28_P05_MOD_ABS"},
        372:{'offset': {"max": 139, "base": 132}, 'dir': 'in'   , 'desc': "SFP28_P06_MOD_ABS"},
        371:{'offset': {"max": 140, "base": 131}, 'dir': 'in'   , 'desc': "SFP28_P07_MOD_ABS"},
        370:{'offset': {"max": 141, "base": 130}, 'dir': 'in'   , 'desc': "SFP28_P08_MOD_ABS"},
        369:{'offset': {"max": 142, "base": 129}, 'dir': 'in'   , 'desc': "SFP28_P09_MOD_ABS"},
        368:{'offset': {"max": 143, "base": 128}, 'dir': 'in'   , 'desc': "SFP28_P10_MOD_ABS"},
        367:{'offset': {"max": 144, "base": 159}, 'dir': 'in'   , 'desc': "SFP28_P27_MOD_ABS"},
        366:{'offset': {"max": 145, "base": 158}, 'dir': 'in'   , 'desc': "SFP28_P28_MOD_ABS"},
        365:{'offset': {"max": 146, "base": 157}, 'dir': 'in'   , 'desc': "SFP28_P29_MOD_ABS"},
        364:{'offset': {"max": 147, "base": 156}, 'dir': 'in'   , 'desc': "SFP28_P30_MOD_ABS"},
        363:{'offset': {"max": 148, "base": 155}, 'dir': 'in'   , 'desc': "NI"},
        362:{'offset': {"max": 149, "base": 154}, 'dir': 'in'   , 'desc': "NI"},
        361:{'offset': {"max": 150, "base": 153}, 'dir': 'in'   , 'desc': "NI"},
        360:{'offset': {"max": 151, "base": 152}, 'dir': 'in'   , 'desc': "NI"},
        359:{'offset': {"max": 152, "base": 151}, 'dir': 'in'   , 'desc': "SFP28_P19_MOD_ABS"},
        358:{'offset': {"max": 153, "base": 150}, 'dir': 'in'   , 'desc': "SFP28_P20_MOD_ABS"},
        357:{'offset': {"max": 154, "base": 149}, 'dir': 'in'   , 'desc': "SFP28_P21_MOD_ABS"},
        356:{'offset': {"max": 155, "base": 148}, 'dir': 'in'   , 'desc': "SFP28_P22_MOD_ABS"},
        355:{'offset': {"max": 156, "base": 147}, 'dir': 'in'   , 'desc': "SFP28_P23_MOD_ABS"},
        354:{'offset': {"max": 157, "base": 146}, 'dir': 'in'   , 'desc': "SFP28_P24_MOD_ABS"},
        353:{'offset': {"max": 158, "base": 145}, 'dir': 'in'   , 'desc': "SFP28_P25_MOD_ABS"},
        352:{'offset': {"max": 159, "base": 144}, 'dir': 'in'   , 'desc': "SFP28_P26_MOD_ABS"},
        351:{'offset': {"max": 160, "base": 175}, 'dir': 'in'   , 'desc': "SFP28_P11_RX_LOS"},
        350:{'offset': {"max": 161, "base": 174}, 'dir': 'in'   , 'desc': "SFP28_P12_RX_LOS"},
        349:{'offset': {"max": 162, "base": 173}, 'dir': 'in'   , 'desc': "SFP28_P13_RX_LOS"},
        348:{'offset': {"max": 163, "base": 172}, 'dir': 'in'   , 'desc': "SFP28_P14_RX_LOS"},
        347:{'offset': {"max": 164, "base": 171}, 'dir': 'in'   , 'desc': "SFP28_P15_RX_LOS"},
        346:{'offset': {"max": 165, "base": 170}, 'dir': 'in'   , 'desc': "SFP28_P16_RX_LOS"},
        345:{'offset': {"max": 166, "base": 169}, 'dir': 'in'   , 'desc': "SFP28_P17_RX_LOS"},
        344:{'offset': {"max": 167, "base": 168}, 'dir': 'in'   , 'desc': "SFP28_P18_RX_LOS"},
        343:{'offset': {"max": 168, "base": 167}, 'dir': 'in'   , 'desc': "SFP28_P03_RX_LOS"},
        342:{'offset': {"max": 169, "base": 166}, 'dir': 'in'   , 'desc': "SFP28_P04_RX_LOS"},
        341:{'offset': {"max": 170, "base": 165}, 'dir': 'in'   , 'desc': "SFP28_P05_RX_LOS"},
        340:{'offset': {"max": 171, "base": 164}, 'dir': 'in'   , 'desc': "SFP28_P06_RX_LOS"},
        339:{'offset': {"max": 172, "base": 163}, 'dir': 'in'   , 'desc': "SFP28_P07_RX_LOS"},
        338:{'offset': {"max": 173, "base": 162}, 'dir': 'in'   , 'desc': "SFP28_P08_RX_LOS"},
        337:{'offset': {"max": 174, "base": 161}, 'dir': 'in'   , 'desc': "SFP28_P09_RX_LOS"},
        336:{'offset': {"max": 175, "base": 160}, 'dir': 'in'   , 'desc': "SFP28_P10_RX_LOS"},
        335:{'offset': {"max": 176, "base": 191}, 'dir': 'in'   , 'desc': "SFP28_P27_RX_LOS"},
        334:{'offset': {"max": 177, "base": 190}, 'dir': 'in'   , 'desc': "SFP28_P28_RX_LOS"},
        333:{'offset': {"max": 178, "base": 189}, 'dir': 'in'   , 'desc': "SFP28_P29_RX_LOS"},
        332:{'offset': {"max": 179, "base": 188}, 'dir': 'in'   , 'desc': "SFP28_P30_RX_LOS"},
        331:{'offset': {"max": 180, "base": 187}, 'dir': 'in'   , 'desc': "NI"},
        330:{'offset': {"max": 181, "base": 186}, 'dir': 'in'   , 'desc': "NI"},
        329:{'offset': {"max": 182, "base": 185}, 'dir': 'in'   , 'desc': "NI"},
        328:{'offset': {"max": 183, "base": 184}, 'dir': 'in'   , 'desc': "NI"},
        327:{'offset': {"max": 184, "base": 183}, 'dir': 'in'   , 'desc': "SFP28_P19_RX_LOS"},
        326:{'offset': {"max": 185, "base": 182}, 'dir': 'in'   , 'desc': "SFP28_P20_RX_LOS"},
        325:{'offset': {"max": 186, "base": 181}, 'dir': 'in'   , 'desc': "SFP28_P21_RX_LOS"},
        324:{'offset': {"max": 187, "base": 180}, 'dir': 'in'   , 'desc': "SFP28_P22_RX_LOS"},
        323:{'offset': {"max": 188, "base": 179}, 'dir': 'in'   , 'desc': "SFP28_P23_RX_LOS"},
        322:{'offset': {"max": 189, "base": 178}, 'dir': 'in'   , 'desc': "SFP28_P24_RX_LOS"},
        321:{'offset': {"max": 190, "base": 177}, 'dir': 'in'   , 'desc': "SFP28_P25_RX_LOS"},
        320:{'offset': {"max": 191, "base": 176}, 'dir': 'in'   , 'desc': "SFP28_P26_RX_LOS"},
    }

    def get_conf(self, board=None):
        if board is None:
            gpio_map = self.gpio_map
            port_conf = self.port_conf
        elif board['hw_rev'] == 0:
            gpio_map = self.gpio_map_proto if hasattr(self, 'gpio_map_proto') else self.gpio_map
            port_conf = self.port_conf_proto if hasattr(self, 'port_conf_proto') else self.port_conf
        elif board['hw_rev'] == 1:
            gpio_map = self.gpio_map_alpha if hasattr(self, 'gpio_map_alpha') else self.gpio_map
            port_conf = self.port_conf_alpha if hasattr(self, 'port_conf_alpha') else self.port_conf
        elif board['hw_rev'] == 2:
            gpio_map = self.gpio_map_beta if hasattr(self, 'gpio_map_beta') else self.gpio_map
            port_conf = self.port_conf_beta if hasattr(self, 'port_conf_beta') else self.port_conf
        elif board['hw_rev'] == 3:
            gpio_map = self.gpio_map_pvt if hasattr(self, 'gpio_map_pvt') else self.gpio_map
            port_conf = self.port_conf_pvt if hasattr(self, 'port_conf_pvt') else self.port_conf
        else:
            gpio_map = self.gpio_map
            port_conf = self.port_conf
        return (port_conf, gpio_map)

    def check_bmc_enable(self):
        return 1

    def check_i2c_status(self, bus_i801):
        sysfs_mux_reset = self.PATH_LPC_GRP_MB_CPLD + "/mux_reset_all"

        bus=bus_i801

        # Check I2C status,assume
        retcode = os.system("i2cget -f -y {} 0x76 > /dev/null 2>&1".format(bus))
        if retcode != 0:

            #read mux failed, i2c bus may be stuck
            msg("Warning: Read I2C Mux Failed!! (ret=%d)\n" % (retcode) )

            #Recovery I2C
            if os.path.exists(sysfs_mux_reset):
                with open(sysfs_mux_reset, "w") as f:
                    #write 0 to sysfs
                    f.write("{}".format(0))
                    msg("I2C bus recovery done.\n")
            else:
                msg("Warning: I2C recovery sysfs does not exist!! (path=%s)\n" % (sysfs_mux_reset) )

    def bsp_pr(self, pr_msg, level = LEVEL_INFO):
        if level == self.LEVEL_INFO:
            sysfs_bsp_logging = self.PATH_LPC_GRP_BSP+"/bsp_pr_info"
        elif level == self.LEVEL_ERR:
            sysfs_bsp_logging = self.PATH_LPC_GRP_BSP+"/bsp_pr_err"
        else:
            msg("Warning: BSP pr level is unknown, using LEVEL_INFO.\n")
            sysfs_bsp_logging = self.PATH_LPC_GRP_BSP+"/bsp_pr_info"

        if os.path.exists(sysfs_bsp_logging):
            with open(sysfs_bsp_logging, "w") as f:
                f.write(pr_msg)
        else:
            msg("Warning: bsp logging sys is not exist\n")

    def get_gpio_max(self):
        cmd = "cat {}/bsp_gpio_max".format(self.PATH_LPC_GRP_BSP)
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("Get gpio max failed, exception={}".format(e), self.LEVEL_ERR)
            output="511"

        gpio_max = int(output, 10)

        return gpio_max

    def get_gpio_base(self):
        cmd = "cat {}/bsp_gpio_base".format(self.PATH_LPC_GRP_BSP)
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("Get gpio base failed, exception={}".format(e), self.LEVEL_ERR)
            output="512"

        gpio_base = int(output, 10)

        return gpio_base

    def init_i2c_mux_idle_state(self, muxs):
        IDLE_STATE_DISCONNECT = -2

        for mux in muxs:
            i2c_addr = mux[1]
            i2c_bus = mux[2]
            sysfs_idle_state = self.PATH_SYS_I2C_DEV_ATTR.format(i2c_bus, i2c_addr, "idle_state")
            if os.path.exists(sysfs_idle_state):
                with open(sysfs_idle_state, 'w') as f:
                    f.write(str(IDLE_STATE_DISCONNECT))

    def get_port_presence(self, port, gpio_max = 511, gpio_base = 0, board=None):
        try:
            port_conf, gpio_map = self.get_conf(board)
            if port not in port_conf:
                return False

            abs_type = port_conf[port]['abs'].get('type')
            abs_data = port_conf[port]['abs'].get('data')
            abs_bit = port_conf[port]['abs'].get('bit', 0)
            if abs_type == 'gpio':
                if gpio_max < 0:
                    gpio_num = gpio_base + gpio_map[abs_data]['offset'].get("base")
                else:
                    gpio_num = gpio_max - gpio_map[abs_data]['offset'].get("max")
                    sysfs = "{}/gpio{}/value".format(self.PATH_SYS_GPIO, gpio_num)
            elif abs_type == 'sysfs':
                sysfs = abs_data
            else:
                return False

            with open(sysfs, "r") as f:
                present_raw = f.read().strip()

            reg_val = (int(present_raw, 0) & (1 << abs_bit))
            pres_status = True if reg_val == 0 else False

            return pres_status

        except:
            return False

    def update_dev_class(self, gpio_max = 511, gpio_base = 0, board=None):
        port_conf, _ = self.get_conf(board)

        for port, config in port_conf.items():  # QSFPX ports

            if config.get('type') not in ['QSFPDD', 'QSFP']:
                continue

            # check module presence
            if not self.get_port_presence(port, gpio_max, gpio_base, board):
                continue

            bus = config.get('bus')
            # get dev_class
            sysfs = "/sys/bus/i2c/devices/{}-0050/dev_class".format(bus)
            cmd = ["cat", sysfs]
            dev_class_str = subprocess.check_output(cmd)
            dev_class = int(dev_class_str, 10)

            # get port type
            cmd = ["dd", "if=/sys/bus/i2c/devices/{}-0050/eeprom".format(bus), "bs=1", "count=1", "skip=0", "status=none"]
            output = subprocess.check_output(cmd)
            hex_str = unpack('B', output)[0]
            type_str = "{:02x}".format(hex_str)
            if type_str == "": #i2c maybe stuck
                self.check_i2c_status()
                continue
            port_type = int(type_str, 16)


            # check if port_type is in port_type_dict
            if port_type not in self.port_type_dict:
                self.bsp_pr("Port[{}] Type: {} is Unknown.".format(port, hex(port_type)))
                continue

            # check if dev_class matches port_type_dev_class
            port_type_dev_class = self.port_type_dict.get(port_type)[0]
            if dev_class != port_type_dev_class:
                with open(sysfs, "w") as f:
                    f.write("{}".format(port_type_dev_class))
                self.bsp_pr("Port[{}] dev_class is changed from {} to {}".format(port, dev_class, port_type_dev_class))

        self.bsp_pr("Please run ONLP API onlp_sfpi_dev_class_update() after inserting QSFP/QSFPDD modules at runtime")

    def init_mux(self, bus_i801, bus_ismt, hw_rev_id):
        # Alpha and later
        if hw_rev_id >=1:
            i2c_muxs = [
                ('pca9546', 0x75, bus_ismt), #9546_ROOT_TIMING
                ('pca9546', 0x76, bus_i801), #9546_ROOT_SFP
                ('pca9546', 0x70, 9),        #9546_CHILD_QSFP_0_1
                ('pca9548', 0x71, 9),        #9548_CHILD_SFP_2_9
                ('pca9548', 0x72, 9),        #9548_CHILD_SFP_10_17
                ('pca9548', 0x73, 9),        #9548_CHILD_SFP_18_25
                ('pca9548', 0x74, 9),        #9548_CHILD_SFP_26_29
            ]

            self.new_i2c_devices(i2c_muxs)

            #init idle state on mux
            self.init_i2c_mux_idle_state(i2c_muxs)

    def init_eeprom(self):
        data = None

        with open(self.PATH_PORT_CONFIG, 'r') as yaml_file:
            data = yaml.safe_load(yaml_file)

        # config eeprom
        port_conf, _ = self.get_conf()
        for port, config in port_conf.items():
            addr=0x50
            self.new_i2c_device(config["driver"], addr, config["bus"])
            port_name = data[config["type"]][port]["port_name"]
            sysfs=self.PATH_SYS_I2C_DEV_ATTR.format( config["bus"], addr, "port_name")
            subprocess.call("echo {} > {}".format(port_name, sysfs), shell=True)

    def init_gpio(self, gpio_max, gpio_base, board=None):
        self.new_i2c_devices(
            [
                ('pca9535', 0x20, 4), #9535_IO_EXP_01 (9535_BOARD_ID)
                ('pca9535', 0x21, 6), #9535_IO_EXP_02 (9535_QSFPX_0_1)
                ('pca9535', 0x22, 6), #9535_IO_EXP_03 (9535_TX_DIS_2_17)
                ('pca9535', 0x24, 6), #9535_IO_EXP_04 (9535_TX_DIS_18_29)
                ('pca9535', 0x26, 7), #9535_IO_EXP_05 (9535_TX_FLT_2_17)
                ('pca9535', 0x27, 7), #9535_IO_EXP_06 (9535_TX_FLT_18_29)
                ('pca9535', 0x25, 7), #9535_IO_EXP_07 (9535_RATE_SELECT_2_17)
                ('pca9535', 0x23, 7), #9535_IO_EXP_08 (9535_RATE_SELECT_18_29)
                ('pca9535', 0x20, 8), #9535_IO_EXP_11 (9535_MOD_ABS_2_17)
                ('pca9535', 0x22, 8), #9535_IO_EXP_09 (9535_MOD_ABS_18_29)
                ('pca9535', 0x21, 8), #9535_IO_EXP_12 (9535_RX_LOS_2_17)
                ('pca9535', 0x24, 8)  #9535_IO_EXP_10 (9535_RX_LOS_18_29)
            ]
        )

        _, gpio_map = self.get_conf()
        for _, conf in gpio_map.items():
            if gpio_max < 0:
                gpio_num = gpio_base + conf['offset'].get("base")
            else:
                gpio_num = gpio_max - conf['offset'].get("max")
            gpio_dir = conf['dir']
            os.system("echo {} > {}/export".format(gpio_num, self.PATH_SYS_GPIO))
            os.system("echo {}   > {}/gpio{}/direction".format(gpio_dir, self.PATH_SYS_GPIO, gpio_num))

        # Certain signals (e.g., qsfp_reset) need time to settle after being set.
        # A delay is added here to prevent failures in subsequent operations and ensure
        # the configuration is applied correctly.
        time.sleep(0.5)

    def enable_ipmi_maintenance_mode(self):
        ipmi_ioctl = IPMI_Ioctl()

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("Current IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

        ipmi_ioctl.set_ipmi_maintenance_mode(IPMI_Ioctl.IPMI_MAINTENANCE_MODE_ON)

        mode=ipmi_ioctl.get_ipmi_maintenance_mode()
        msg("After IPMI_IOCTL IPMI_MAINTENANCE_MODE=%d\n" % (mode) )

    def disable_bmc_watchdog(self):
        os.system("ipmitool mc watchdog off")

    def update_pci_device(self, driver, device, action):
        driver_path = os.path.join("/sys/bus/pci/drivers", driver, action)

        if os.path.exists(driver_path):
            try:
                with open(driver_path, "w") as file:
                    file.write(device)
            except Exception as e:
                print("Open file failed, error: {}".format(e))

    def init_i2c_bus_order(self):
        device_actions = [
            #driver_name   bus_address     action
            ("i801_smbus", "0000:00:1f.4", "unbind"),
            ("ismt_smbus", "0000:00:12.0", "unbind"),
            ("i801_smbus", "0000:00:1f.4", "bind"),
            ("ismt_smbus", "0000:00:12.0", "bind"),
        ]

        # Iterate over the list and call modify_device for each tuple
        for driver_name, bus_address, action in device_actions:
            self.update_pci_device(driver_name, bus_address, action)

    def baseconfig(self):

        # load default kernel driver
        os.system("modprobe -rq i2c_i801")
        self.insmod("i2c-smbus", False)
        os.system("modprobe i2c_i801")
        os.system("modprobe i2c_ismt")
        os.system("modprobe i2c_dev")
        os.system("modprobe gpio_pca953x")
        os.system("modprobe i2c_mux_pca954x")
        os.system("modprobe coretemp")
        os.system("modprobe ipmi_devintf")
        os.system("modprobe ipmi_si")
        self.init_i2c_bus_order()

        #lpc driver
        self.insmod("x86-64-ufispace-s9510-30xc-lpc")

        # get hardware revision
        cmd = "cat /sys/devices/platform/x86_64_ufispace_s9510_30xc_lpc/mb_cpld/board_hw_id"
        try:
            output = subprocess.check_output(cmd.split())
        except Exception as e:
            self.bsp_pr("Get hw rev id from LPC failed, exception={}".format(e), self.LEVEL_ERR)
            output="1"

        hw_rev_id = int(output, 16)

        gpio_max = self.get_gpio_max()
        self.bsp_pr("GPIO MAX: {}".format(gpio_max))

        gpio_base = self.get_gpio_base()
        self.bsp_pr("GPIO BASE: {}".format(gpio_base))

        ########### initialize I2C bus 0 ###########
        # i2c_i801 is built-in
        # add i2c_ismt
        #self.insmod("i2c-ismt") #module not found
        bus_i801=0
        bus_ismt=1
        os.system("modprobe i2c-ismt")

        # check i2c bus status
        self.check_i2c_status(bus_i801)

        bmc_enable = self.check_bmc_enable()
        msg("bmc enable : %r\n" % (True if bmc_enable else False))

        # record the result for onlp
        os.system("echo %d > /etc/onl/bmc_en" % bmc_enable)

        # init MUX sysfs
        self.bsp_pr("Init i2c");
        self.init_mux(bus_i801, bus_ismt, hw_rev_id)

        self.insmod("x86-64-ufispace-eeprom-mb")
        self.insmod("optoe")

        # init SYS EEPROM devices
        self.bsp_pr("Init mb eeprom");
        self.new_i2c_devices(
            [
                #  on cpu board
                ('mb_eeprom', 0x57, bus_ismt),
            ]
        )

        # init EEPROM
        self.bsp_pr("Init port eeprom");
        self.init_eeprom()

        # init GPIO sysfs
        self.bsp_pr("Init gpio");
        self.init_gpio(gpio_max, gpio_base)

        # init dev_class for CMIS/non-CMIS modules
        self.update_dev_class(gpio_max, gpio_base, None)

        #enable ipmi maintenance mode
        self.enable_ipmi_maintenance_mode()

        # disable bmc watchdog
        self.disable_bmc_watchdog()

        # sets the System Event Log (SEL) timestamp to the current system time
        os.system ("timeout 5 ipmitool sel time set now > /dev/null 2>&1")

        self.bsp_pr("Init done");
        return True

