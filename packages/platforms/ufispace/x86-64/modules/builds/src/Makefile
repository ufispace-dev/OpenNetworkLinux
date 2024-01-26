obj-m += x86-64-ufispace-eeprom-mb.o
obj-m += x86-64-ufispace-sys-eeprom.o

#i40e ccflags
ccflags-y += -I$(src)/i40e -DUSE_INTEL_AUX_BUS

#ice ccflags
ccflags-y += -I$(src)/ice -DUSE_INTEL_AUX_BUS
subdir-ccflags-y += -I$(src)/i40e -I$(src)/ice

obj-m += i40e.o
i40e-objs := i40e/i40e_main.o i40e/i40e_ethtool.o i40e/i40e_xsk.o i40e/i40e_adminq.o i40e/i40e_common.o \
             i40e/i40e_hmc.o i40e/i40e_lan_hmc.o i40e/i40e_nvm.o i40e/i40e_debugfs.o i40e/i40e_diag.o \
             i40e/i40e_txrx.o i40e/i40e_ptp.o i40e/i40e_filters.o i40e/i40e_ddp.o i40e/i40e_client.o \
             i40e/i40e_virtchnl_pf.o i40e/kcompat.o i40e/kcompat_vfd.o

ifdef CONFIG_DCB
i40e-objs += i40e/i40e_dcb.o i40e/i40e_dcb_nl.o
endif

obj-m += intel_auxiliary.o
intel_auxiliary-objs := i40e/auxiliary.o

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
            ice/ice_ethtool.o ice/ice_idc.o ice/kcompat.o

ifdef CONFIG_NET_DEVLINK
ice-objs += ice/ice_devlink.o ice/ice_fw_update.o ice/ice_eswitch.o ice/ice_repr.o
endif

ifdef CONFIG_DEBUG_FS
ice-objs += ice/ice_debugfs.o
endif

ifdef CONFIG_PCI_IOV
ice-objs += ice/ice_dcf.o ice/ice_sriov.o ice/ice_vf_mbx.o ice/ice_vf_vsi_vlan_ops.o \
            ice/ice_virtchnl_allowlist.o ice/ice_vf_adq.o ice/ice_virtchnl.o ice/ice_virtchnl_fdir.o \
            ice/ice_virtchnl_fsub.o ice/ice_vf_lib.o
endif

ifdef CONFIG_PTP_1588_CLOCK
ice-objs += ice/ice_ptp.o ice/ice_ptp_hw.o
endif

ifdef CONFIG_DCB
ice-objs += ice/ice_dcb.o ice/ice_dcb_nl.o ice/ice_dcb_lib.o
endif

ifdef CONFIG_RFS_ACCEL
ice-objs += ice/ice_arfs.o
endif

ifdef CONFIG_XDP_SOCKETS
ice-objs += ice/ice_xsk.o
endif

ifndef CONFIG_PLDMFW
ice-objs += ice/kcompat_pldmfw.o
endif

ifndef CONFIG_DIMLIB
ice-objs += ice/kcompat_dim.o ice/kcompat_net_dim.o
endif

ifndef CONFIG_GNSS
ice-objs += ice/kcompat_gnss.o
endif
