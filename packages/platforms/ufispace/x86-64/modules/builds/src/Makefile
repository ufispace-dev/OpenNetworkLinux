obj-m += x86-64-ufispace-eeprom-mb.o
obj-m += x86-64-ufispace-sys-eeprom.o
obj-m += x86-64-ufispace-optoe.o

KVERSION := $(shell echo $(KERNELVERSION) | cut -d "." -f 1,2)
KVERSION_MAJOR := $(shell echo $(KERNELVERSION) | cut -d "." -f 1)
KVERSION_MINOR := $(shell echo $(KERNELVERSION) | cut -d "." -f 2)
PATCH_MAJOR := 6
PATCH_MINOR := 0
KV_GE_6_0 := $(shell [ $(KVERSION_MAJOR) -gt $(PATCH_MAJOR) -o \( $(KVERSION_MAJOR) -eq $(PATCH_MAJOR) -a $(KVERSION_MINOR) -ge $(PATCH_MINOR) \) ] && echo true)

ifneq ($(KV_GE_6_0),true)
    #$(info KERNELVERSION=$(KERNELVERSION) < 6.0)
    #i40e ccflags
    ccflags-y += -I$(src)/i40e
    #ice ccflags
    ccflags-y += -I$(src)/ice -DUSE_INTEL_AUX_BUS
    subdir-ccflags-y += -I$(src)/i40e -I$(src)/ice
    obj-m += i40e.o
    i40e-objs := i40e/i40e_main.o i40e/i40e_ethtool.o i40e/i40e_adminq.o i40e/i40e_common.o i40e/i40e_hmc.o \
                 i40e/i40e_lan_hmc.o i40e/i40e_nvm.o i40e/i40e_debugfs.o i40e/i40e_diag.o i40e/i40e_txrx.o \
                 i40e/i40e_ptp.o i40e/i40e_filters.o i40e/i40e_ddp.o i40e/i40e_client.o i40e/i40e_virtchnl_pf.o \
                 i40e/i40e_dcb.o i40e/i40e_dcb_nl.o i40e/kcompat.o i40e/kcompat_vfd.o
    obj-m += ice.o
    ice-objs := ice/ice_main.o ice/ice_controlq.o ice/ice_common.o ice/ice_nvm.o ice/ice_switch.o \
                ice/ice_sched.o ice/ice_base.o ice/ice_lib.o ice/ice_txrx_lib.o ice/ice_txrx.o \
                ice/ice_fltr.o ice/ice_irq.o ice/ice_pf_vsi_vlan_ops.o ice/ice_vsi_vlan_ops.o \
                ice/ice_vsi_vlan_lib.o ice/ice_tc_lib.o ice/ice_fdir.o ice/ice_ethtool_fdir.o \
                ice/ice_acl_main.o ice/ice_acl.o ice/ice_acl_ctrl.o ice/ice_vlan_mode.o \
                ice/ice_ddp.o ice/ice_flex_pipe.o ice/ice_flow.o ice/ice_parser.o ice/ice_imem.o \
                ice/ice_pg_cam.o ice/ice_metainit.o ice/ice_bst_tcam.o ice/ice_ptype_mk.o \
                ice/ice_mk_grp.o ice/ice_proto_grp.o ice/ice_flg_rd.o ice/ice_xlt_kb.o \
                ice/ice_parser_rt.o ice/ice_lag.o ice/ice_fwlog.o ice/ice_ieps.o ice/ice_gnss.o \
                ice/ice_ethtool.o ice/ice_idc.o ice/ice_debugfs.o \
                ice/ice_dcf.o ice/ice_sriov.o ice/ice_vf_mbx.o ice/ice_vf_vsi_vlan_ops.o ice/ice_virtchnl_allowlist.o \
                ice/ice_vf_adq.o ice/ice_virtchnl.o ice/ice_virtchnl_fdir.o ice/ice_virtchnl_fsub.o ice/ice_vf_lib.o \
                ice/ice_ptp.o ice/ice_ptp_hw.o \
                ice/ice_arfs.o \
                ice/kcompat.o ice/kcompat_pldmfw.o ice/kcompat_dim.o ice/kcompat_net_dim.o ice/kcompat_gnss.o \

    obj-m += intel_auxiliary.o
    intel_auxiliary-objs := ice/auxiliary.o

else ifeq ($(KVERSION), 6.8)
    #$(info KERNELVERSION=$(KERNELVERSION) = 6.8, please set the following configurations in your kernel config )
    #$(info CONFIG_I40E=m)
    #$(info CONFIG_ICE=m
    #$(info CONFIG_I2C_I801=m)
    #$(info CONFIG_I2C_SMBUS=m)
    obj-m += i2c-smbus.o

else
    $(warning KERNELVERSION=$(KERNELVERSION), not supported kernel version)

endif
