GCOV_PROFILE := y

ccflags-y += -I$(srctree.nvgpu)/drivers/gpu/nvgpu/include
ccflags-y += -I$(srctree.nvgpu)/drivers/gpu/nvgpu
ccflags-y += -I$(srctree.nvgpu)/include
ccflags-y += -I$(srctree.nvgpu)/include/uapi

ccflags-y += -I$(srctree)/drivers/devfreq

ccflags-y += -Wno-multichar
ccflags-y += -Werror
ccflags-y += -Wno-error=cpp
ifeq ($(VERSION),4)
ccflags-y += -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
endif

# Turn off when this is fixed upstream, if ever.
ccflags-y += -D__NVGPU_PREVENT_UNTRUSTED_SPECULATION

ifeq ($(CONFIG_NVGPU_DEBUGGER),y)
ccflags-y += -DNVGPU_DEBUGGER
endif

ifeq ($(CONFIG_NVGPU_LS_PMU),y)
ccflags-y += -DNVGPU_LS_PMU
endif

ccflags-y += -DNVGPU_ENGINE
ccflags-y += -DNVGPU_USERD

obj-$(CONFIG_GK20A) := nvgpu.o

# OS independent parts of nvgpu. The work to collect files here
# is in progress.

nvgpu-y += \
	common/ptimer/ptimer.o \
	common/ptimer/ptimer_gk20a.o \
	common/perf/perf_gm20b.o \
	common/perf/perf_gv11b.o \
	common/perf/perfbuf.o \
	common/xve/xve_gp106.o \
	common/therm/therm.o \
	common/top/top_gm20b.o \
	common/top/top_gp10b.o \
	common/top/top_gv100.o \
	common/mc/mc.o \
	common/sync/channel_sync.o \
	common/sync/channel_sync_semaphore.o \
	common/sync/sema_cmdbuf_gk20a.o \
	common/sync/sema_cmdbuf_gv11b.o \
	common/semaphore/semaphore_sea.o \
	common/semaphore/semaphore_pool.o \
	common/semaphore/semaphore_hw.o \
	common/semaphore/semaphore.o \
	common/boardobj/boardobj.o \
	common/boardobj/boardobjgrp.o \
	common/boardobj/boardobjgrpmask.o \
	common/boardobj/boardobjgrp_e255.o \
	common/boardobj/boardobjgrp_e32.o \
	common/regops/regops.o \
	common/regops/regops_gm20b.o \
	common/regops/regops_gp10b.o \
	common/regops/regops_gv100.o \
	common/regops/regops_gv11b.o \
	common/regops/regops_tu104.o \
	common/ltc/ltc.o \
	common/gr/gr.o \
	common/cbc/cbc.o \
	common/gr/global_ctx.o \
	common/gr/ctx.o \
	common/gr/gr_falcon.o \
	common/gr/subctx.o \
	common/gr/zcull.o \
	common/gr/gr_config.o \
	common/gr/fecs_trace.o \
	common/gr/zbc.o \
	common/gr/gr_setup.o \
	common/gr/hwpm_map.o \
	common/gr/obj_ctx.o \
	common/netlist/netlist.o \
	common/netlist/netlist_sim.o \
	common/netlist/netlist_gm20b.o \
	common/netlist/netlist_gp10b.o \
	common/netlist/netlist_gv100.o \
	common/netlist/netlist_gv11b.o \
	common/netlist/netlist_tu104.o \
	common/init/nvgpu_init.o \
	common/pmu/pmu.o \
	common/pmu/pmu_ipc.o \
	common/pmu/pmu_fw.o \
	common/pmu/pg/pmu_pg.o \
	common/pmu/pg/pmu_aelpg.o \
	common/pmu/pmu_perfmon.o \
	common/pmu/pmu_super_surface.o \
	common/pmu/pmu_debug.o \
	common/pmu/pmu_gk20a.o \
	common/pmu/pmu_gm20b.o \
	common/pmu/pmu_gp10b.o \
	common/pmu/pmu_gp106.o \
	common/pmu/pmu_gv11b.o \
	common/pmu/pg/pg_sw_gp10b.o \
	common/pmu/pg/pg_sw_gp106.o \
	common/pmu/pg/pg_sw_gv11b.o \
	common/pmu/pmu_gv100.o \
	common/pmu/pmu_tu104.o \
	common/acr/acr.o \
	common/acr/acr_wpr.o \
	common/acr/acr_blob_alloc.o \
	common/acr/acr_blob_construct_v0.o \
	common/acr/acr_blob_construct_v1.o \
	common/acr/acr_bootstrap.o \
	common/acr/acr_sw_gm20b.o \
	common/acr/acr_sw_gp10b.o \
	common/acr/acr_sw_gv100.o \
	common/acr/acr_sw_gv11b.o \
	common/acr/acr_sw_tu104.o \
	common/pmu/perf/vfe_var.o \
	common/pmu/perf/vfe_equ.o \
	common/pmu/perf/pmu_perf.o \
	common/pmu/perf/perf_gv100.o \
	common/pmu/perf/perf_ps35.o \
	common/pmu/perf/change_seq.o \
	common/pmu/pmgr/pwrdev.o \
	common/pmu/pmgr/pmgr.o \
	common/pmu/pmgr/pmgrpmu.o \
	common/pmu/pmgr/pwrmonitor.o \
	common/pmu/pmgr/pwrpolicy.o \
	common/pmu/volt/volt_rail.o \
	common/pmu/volt/volt_dev.o \
	common/pmu/volt/volt_policy.o \
	common/pmu/volt/volt_pmu.o \
	common/pmu/therm/thrm.o \
	common/pmu/therm/thrmdev.o \
	common/pmu/therm/thrmchannel.o \
	common/pmu/therm/thrmpmu.o \
	common/pmu/lpwr/rppg.o \
	common/pmu/lpwr/lpwr.o \
	common/pmu/pstate/pstate.o \
	common/pmu/clk/clk_vin.o \
	common/pmu/clk/clk_fll.o \
	common/pmu/clk/clk_domain.o \
	common/pmu/clk/clk_freq_domain.o \
	common/pmu/clk/clk_prog.o \
	common/pmu/clk/clk_vf_point.o \
	common/pmu/clk/clk_freq_controller.o \
	common/pmu/clk/clk.o \
	common/clk_arb/clk_arb.o \
	common/clk_arb/clk_arb_gp10b.o \
	common/clk_arb/clk_arb_gv100.o \
	common/nvlink/probe.o \
	common/nvlink/init/device_reginit_gv100.o \
	common/nvlink/init/device_reginit.o \
	common/nvlink/intr_and_err_handling_gv100.o \
	common/nvlink/minion.o \
	common/nvlink/link_mode_transitions.o \
	common/nvlink/nvlink.o \
	common/nvlink/nvlink_gv100.o \
	common/nvlink/nvlink_tu104.o \
	hal/mc/mc_gm20b.o \
	hal/mc/mc_gp10b.o  \
	hal/mc/mc_gv11b.o  \
	hal/mc/mc_gv100.o \
	hal/mc/mc_tu104.o \
	hal/bus/bus_gk20a.o \
	hal/bus/bus_gm20b.o \
	hal/bus/bus_gp10b.o \
	hal/bus/bus_gv100.o \
	hal/bus/bus_tu104.o \
	hal/gr/fecs_trace/fecs_trace_gm20b.o \
	hal/gr/fecs_trace/fecs_trace_gp10b.o \
	hal/gr/fecs_trace/fecs_trace_gv11b.o \
	hal/gr/zcull/zcull_gm20b.o \
	hal/gr/zcull/zcull_gv11b.o \
	hal/gr/ctxsw_prog/ctxsw_prog_gm20b.o \
	hal/gr/ctxsw_prog/ctxsw_prog_gp10b.o \
	hal/gr/ctxsw_prog/ctxsw_prog_gv11b.o \
	hal/gr/init/gr_init_gm20b.o \
	hal/gr/init/gr_init_gp10b.o \
	hal/gr/init/gr_init_gv100.o \
	hal/gr/init/gr_init_gv11b.o \
	hal/gr/init/gr_init_tu104.o \
	hal/gr/intr/gr_intr_gm20b.o \
	hal/gr/intr/gr_intr_gp10b.o \
	hal/gr/intr/gr_intr_gv11b.o \
	hal/gr/intr/gr_intr_tu104.o \
	hal/gr/hwpm_map/hwpm_map_gv100.o \
	hal/gr/zbc/zbc_gm20b.o \
	hal/gr/zbc/zbc_gp10b.o \
	hal/gr/zbc/zbc_gv11b.o \
	hal/priv_ring/priv_ring_gm20b.o \
	hal/priv_ring/priv_ring_gp10b.o \
	hal/power_features/cg/gm20b_gating_reglist.o \
	hal/power_features/cg/gp106_gating_reglist.o \
	hal/power_features/cg/gp10b_gating_reglist.o \
	hal/power_features/cg/gv100_gating_reglist.o \
	hal/power_features/cg/gv11b_gating_reglist.o \
	hal/power_features/cg/tu104_gating_reglist.o \
	hal/cbc/cbc_gm20b.o \
	hal/cbc/cbc_gp10b.o \
	hal/cbc/cbc_gv11b.o \
	hal/cbc/cbc_tu104.o \
	hal/therm/therm_gm20b.o \
	hal/therm/therm_gp10b.o \
	hal/therm/therm_gp106.o \
	hal/therm/therm_gv11b.o \
	hal/gr/falcon/gr_falcon_gm20b.o \
	hal/ltc/ltc_gm20b.o \
	hal/ltc/ltc_gp10b.o \
	hal/ltc/ltc_gv11b.o  \
	hal/ltc/ltc_tu104.o \
	hal/ltc/intr/ltc_intr_gm20b.o \
	hal/ltc/intr/ltc_intr_gp10b.o \
	hal/ltc/intr/ltc_intr_gv11b.o  \
	hal/fb/fb_gm20b.o \
	hal/fb/fb_gp10b.o \
	hal/fb/fb_gp106.o \
	hal/fb/fb_gv11b.o \
	hal/fb/fb_gv100.o \
	hal/fb/fb_tu104.o \
	hal/fuse/fuse_gm20b.o \
	hal/fuse/fuse_gp10b.o \
	hal/fuse/fuse_gp106.o \
	hal/fifo/engines_gm20b.o \
	hal/fifo/engines_gv11b.o \
	hal/fifo/pbdma_gm20b.o \
	hal/fifo/pbdma_gp10b.o \
	hal/fifo/pbdma_gv11b.o \
	hal/fifo/pbdma_tu104.o \
	hal/fifo/engine_status_gm20b.o \
	hal/fifo/engine_status_gv100.o \
	hal/fifo/pbdma_status_gm20b.o \
	hal/fifo/ramfc_gk20a.o \
	hal/fifo/ramfc_gp10b.o \
	hal/fifo/ramfc_gv11b.o \
	hal/fifo/ramfc_tu104.o \
	hal/fifo/ramin_gk20a.o \
	hal/fifo/ramin_gv11b.o \
	hal/fifo/tsg_gk20a.o \
	hal/fifo/tsg_gv11b.o \
	hal/fifo/userd_gk20a.o \
	hal/fifo/userd_gv11b.o \
	hal/fifo/fifo_intr_gk20a.o \
	hal/fifo/fifo_intr_gv11b.o \
	hal/fifo/ctxsw_timeout_gk20a.o \
	hal/fifo/ctxsw_timeout_gv11b.o \
	hal/falcon/falcon_gk20a.o \
	hal/nvdec/nvdec_gp106.o \
	hal/nvdec/nvdec_tu104.o \
	hal/nvlink/minion_gv100.o \
	hal/nvlink/minion_tu104.o \
	hal/nvlink/link_mode_transitions_gv100.o \
	hal/nvlink/link_mode_transitions_tu104.o \
	hal/gsp/gsp_gv100.o

# Linux specific parts of nvgpu.
nvgpu-y += \
	os/linux/os_ops.o \
	os/linux/os_ops_gm20b.o \
	os/linux/os_ops_gp10b.o \
	os/linux/os_ops_gv100.o \
	os/linux/os_ops_gv11b.o \
	os/linux/kmem.o \
	os/linux/timers.o \
	os/linux/ioctl.o \
	os/linux/ioctl_ctrl.o \
	os/linux/ioctl_as.o \
	os/linux/ioctl_channel.o \
	os/linux/ioctl_tsg.o \
	os/linux/ioctl_dbg.o \
	os/linux/ioctl_clk_arb.o \
	os/linux/log.o \
	os/linux/cond.o \
	os/linux/nvgpu_mem.o \
	os/linux/linux-dma.o \
	os/linux/driver_common.o \
	os/linux/firmware.o \
	os/linux/thread.o \
	os/linux/vm.o \
	os/linux/intr.o \
	os/linux/sysfs.o \
	os/linux/linux-io.o \
	os/linux/io_usermode.o \
	os/linux/rwsem.o \
	os/linux/comptags.o \
	os/linux/dmabuf.o \
	os/linux/sched.o \
	os/linux/linux-channel.o \
	os/linux/sim.o \
	os/linux/sim_pci.o \
	os/linux/os_sched.o \
	os/linux/nvlink_probe.o \
	os/linux/nvlink.o \
	os/linux/dt.o \
	os/linux/ecc_sysfs.o \
	os/linux/linux-cbc.o \
	os/linux/os_ops_tu104.o \
	os/linux/bsearch.o

nvgpu-$(CONFIG_GK20A_VIDMEM) += \
	os/linux/dmabuf_vidmem.o

nvgpu-$(CONFIG_DEBUG_FS) += \
	os/linux/debug.o \
	os/linux/debug_gr.o \
	os/linux/debug_fifo.o \
	os/linux/debug_ce.o \
	os/linux/debug_pmu.o \
	os/linux/debug_pmgr.o \
	os/linux/debug_sched.o \
	os/linux/debug_allocator.o \
	os/linux/debug_hal.o \
	os/linux/debug_clk_gm20b.o \
	os/linux/debug_clk_gp106.o \
	os/linux/debug_therm_gp106.o \
	os/linux/debug_bios.o \
	os/linux/debug_ltc.o \
	os/linux/debug_xve.o \
	os/linux/debug_clk_gv100.o \
	os/linux/debug_volt.o \
	os/linux/debug_s_param.o

ifeq ($(CONFIG_NVGPU_TRACK_MEM_USAGE),y)
nvgpu-$(CONFIG_DEBUG_FS) += \
	os/linux/debug_kmem.o
endif

nvgpu-$(CONFIG_GK20A_CTXSW_TRACE) += \
	os/linux/fecs_trace_linux.o

ifeq ($(CONFIG_GK20A_CTXSW_TRACE),y)
nvgpu-$(CONFIG_DEBUG_FS) += \
	os/linux/debug_fecs_trace.o
endif


nvgpu-$(CONFIG_TEGRA_GK20A) += \
	os/linux/module.o \
	os/linux/module_usermode.o \
	os/linux/soc.o \
	os/linux/fuse.o \
	os/linux/platform_gk20a_tegra.o \
	os/linux/platform_gp10b_tegra.o \
	os/linux/platform_gv11b_tegra.o

nvgpu-$(CONFIG_SYNC) += \
	os/linux/sync_sema_android.o \
	os/linux/os_fence_android.o \
	os/linux/os_fence_android_sema.o

ifeq ($(CONFIG_TEGRA_GK20A_NVHOST), y)
nvgpu-$(CONFIG_SYNC) += \
	os/linux/os_fence_android_syncpt.o
nvgpu-y += common/sync/channel_sync_syncpt.o
endif

nvgpu-$(CONFIG_GK20A_PCI) += \
	os/linux/pci.o \
	os/linux/pci_power.o \
	os/linux/pci_usermode.o

nvgpu-$(CONFIG_TEGRA_GK20A_NVHOST) += \
	os/linux/nvhost.o \
	common/sync/syncpt_cmdbuf_gk20a.o \
	common/sync/syncpt_cmdbuf_gv11b.o

nvgpu-$(CONFIG_TEGRA_GR_VIRTUALIZATION) += \
	os/linux/vgpu/platform_vgpu_tegra.o \
	os/linux/vgpu/sysfs_vgpu.o \
	os/linux/vgpu/vgpu_ivc.o \
	os/linux/vgpu/vgpu_ivm.o \
	os/linux/vgpu/vgpu_linux.o \
	os/linux/vgpu/fecs_trace_vgpu_linux.o \
	os/linux/vgpu/gv11b/platform_gv11b_vgpu_tegra.o

nvgpu-$(CONFIG_COMMON_CLK) += \
	os/linux/clk.o

nvgpu-$(CONFIG_GK20A_DEVFREQ) += \
	os/linux/scale.o

nvgpu-$(CONFIG_NVGPU_SUPPORT_CDE) += \
	os/linux/cde.o \
	os/linux/cde_gm20b.o \
	os/linux/cde_gp10b.o

nvgpu-$(CONFIG_GK20A_RDMA) += \
        os/linux/nvidia_p2p.o

ifeq ($(CONFIG_DEBUG_FS),y)
nvgpu-$(CONFIG_NVGPU_SUPPORT_CDE) += \
	os/linux/debug_cde.o
endif

nvgpu-y += \
	common/mm/allocators/nvgpu_allocator.o \
	common/mm/allocators/bitmap_allocator.o \
	common/mm/allocators/buddy_allocator.o \
	common/mm/allocators/page_allocator.o \
	common/mm/allocators/lockless_allocator.o \
	common/mm/gmmu/page_table.o \
	common/mm/gmmu/pd_cache.o \
	common/mm/as.o \
	common/mm/vm.o \
	common/mm/vm_area.o \
	common/mm/nvgpu_mem.o \
	common/mm/nvgpu_sgt.o \
	common/mm/comptags.o \
	common/mm/mm.o \
	common/mm/dma.o \
	common/enabled.o \
	common/string.o \
	common/pramin.o \
	common/rbtree.o \
	common/vbios/bios.o \
	common/falcon/falcon.o \
	common/falcon/falcon_sw_gk20a.o \
	common/falcon/falcon_sw_gp106.o \
	common/falcon/falcon_sw_gv100.o \
	common/falcon/falcon_sw_tu104.o \
	common/engine_queues/engine_mem_queue.o \
	common/engine_queues/engine_dmem_queue.o \
	common/engine_queues/engine_emem_queue.o \
	common/engine_queues/engine_fb_queue.o \
	common/init/hal_init.o \
	common/sec2/sec2.o \
	common/sec2/sec2_ipc.o \
	common/io/io.o \
	common/power_features/power_features.o \
	common/power_features/cg/cg.o \
	common/power_features/pg/pg.o \
	common/sim.o \
	common/sim_pci.o \
	common/rc/rc.o \
	common/fifo/fifo.o \
	common/fifo/channel.o \
	common/fifo/submit.o \
	common/fifo/tsg.o \
	common/fifo/runlist.o \
	common/fifo/runlist_gk20a.o \
	common/fifo/runlist_gv11b.o \
	common/fifo/runlist_gv100.o \
	common/fifo/runlist_tu104.o \
	common/fifo/channel_gk20a.o \
	common/fifo/channel_gm20b.o \
	common/fifo/channel_gv11b.o \
	common/fifo/channel_gv100.o \
	common/fifo/engine_status.o \
	common/fifo/engines.o \
	common/fifo/pbdma_status.o \
	common/fifo/userd.o \
	common/ecc.o \
	common/ce2.o \
	common/debugger.o \
	gk20a/ce2_gk20a.o \
	gk20a/fifo_gk20a.o \
	gk20a/gr_gk20a.o \
	gk20a/mm_gk20a.o \
	gk20a/fence_gk20a.o \
	gm20b/hal_gm20b.o \
	gm20b/gr_gm20b.o \
	gm20b/clk_gm20b.o \
	gm20b/fifo_gm20b.o \
	gm20b/mm_gm20b.o

nvgpu-$(CONFIG_GK20A_VIDMEM) += \
	common/mm/vidmem.o

nvgpu-y += \
	hal/gr/config/gr_config_gm20b.o \
	hal/gr/config/gr_config_gv100.o

nvgpu-$(CONFIG_TEGRA_GR_VIRTUALIZATION) += \
	common/vgpu/ltc/ltc_vgpu.o \
	common/vgpu/cbc/cbc_vgpu.o \
	common/vgpu/gr/gr_vgpu.o \
	common/vgpu/gr/ctx_vgpu.o \
	common/vgpu/gr/subctx_vgpu.o \
	common/vgpu/fifo/fifo_vgpu.o \
	common/vgpu/fifo/runlist_vgpu.o \
	common/vgpu/fifo/vgpu_fifo_gv11b.o \
	common/vgpu/fifo/ramfc_vgpu.o \
	common/vgpu/fifo/userd_vgpu.o \
	common/vgpu/ce_vgpu.o \
	common/vgpu/mm/mm_vgpu.o \
	common/vgpu/mm/vm_vgpu.o \
	common/vgpu/vgpu.o \
	common/vgpu/debugger_vgpu.o \
	common/vgpu/tsg_vgpu.o \
	common/vgpu/perf/cyclestats_snapshot_vgpu.o \
	common/vgpu/perf/perf_vgpu.o \
	common/vgpu/ecc_vgpu.o \
	common/vgpu/clk_vgpu.o \
	common/vgpu/gr/fecs_trace_vgpu.o \
	common/vgpu/gp10b/vgpu_hal_gp10b.o  \
	common/vgpu/gv11b/vgpu_gv11b.o \
	common/vgpu/gv11b/vgpu_hal_gv11b.o \
	common/vgpu/gv11b/vgpu_tsg_gv11b.o \

nvgpu-$(CONFIG_GK20A_CYCLE_STATS) += \
	common/perf/cyclestats_snapshot.o

nvgpu-y += \
	gp10b/gr_gp10b.o  \
	gp10b/ce_gp10b.o \
	gp10b/fifo_gp10b.o  \
	gp10b/mm_gp10b.o \
	gp10b/hal_gp10b.o \
	gp10b/gp10b.o \
	gp10b/ecc_gp10b.o \
	gp106/gr_gp106.o \
	gp106/sec2_gp106.o \
	gp106/bios_gp106.o \
	gv11b/gv11b.o  \
	gv11b/hal_gv11b.o \
	gv11b/gr_gv11b.o \
	gv11b/fifo_gv11b.o \
	gv11b/mm_gv11b.o \
	gv11b/ce_gv11b.o \
	gv11b/subctx_gv11b.o \
	gv11b/ecc_gv11b.o \
	gv100/mm_gv100.o \
	gv100/bios_gv100.o \
	gv100/fifo_gv100.o \
	gv100/gr_gv100.o \
	gv100/hal_gv100.o \
	gv100/clk_gv100.o \
	tu104/mm_tu104.o \
	tu104/hal_tu104.o \
	tu104/fifo_tu104.o \
	tu104/gr_tu104.o \
	tu104/func_tu104.o \
	tu104/bios_tu104.o \
	tu104/fbpa_tu104.o \
	tu104/sec2_tu104.o \
	tu104/ecc_tu104.o \
	gp106/clk_gp106.o
