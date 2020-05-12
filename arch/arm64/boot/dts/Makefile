# SPDX-License-Identifier: GPL-2.0

DTC_CPP_FLAGS += -DLINUX_VERSION=504

DTC_INCLUDE += $(srctree)/arch/arm64/boot/dts/nvidia
DTC_INCLUDE += $(tegra-dtstree)/soc/tegra/kernel-include
DTC_INCLUDE += $(tegra-dtstree)/platform/tegra/common/kernel-dts

DTC_INCLUDE += $(tegra-dtstree)/soc/t210/kernel-dts
DTC_INCLUDE += $(tegra-dtstree)/platform/t210/common/kernel-dts
DTC_INCLUDE += $(tegra-dtstree)/platform/t210b01/common/kernel-dts

DTC_INCLUDE += $(tegra-dtstree)/soc/t18x/kernel-include
DTC_INCLUDE += $(tegra-dtstree)/soc/t18x/kernel-dts
DTC_INCLUDE += $(tegra-dtstree)/platform/t18x/common/kernel-dts

DTC_INCLUDE += $(tegra-dtstree)/soc/t19x/kernel-include
DTC_INCLUDE += $(tegra-dtstree)/soc/t19x/kernel-dts
DTC_INCLUDE += $(tegra-dtstree)/platform/t19x/common/kernel-dts

ifeq ($(CONFIG_ARCH_TEGRA_23x_SOC),y)
DTC_INCLUDE += $(tegra-dtstree)/soc/t23x/kernel-include
DTC_INCLUDE += $(tegra-dtstree)/soc/t23x/kernel-dts
DTC_INCLUDE += $(tegra-dtstree)/platform/t23x/common/kernel-dts
endif

ifeq ($(NV_BUILD_SYSTEM_TYPE),embedded-linux)
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b01-A.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b01-B.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b03-A.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b03-B.dtb
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-vcm31-p2382-010-a01-00-base.dtb
else ifeq ($(NV_BUILD_SYSTEM_TYPE),l4t)
# developer local build
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-p2771-0000.dtb
dtb-$(CONFIG_ARCH_TEGRA_23x_SOC) += nvidia/tegra234-sim-vdk.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-p2888-0001-p2822-0000.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-p2888-0001-p2822-0000-uefi-acpi.dtb
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-quill-p3310-1000-c03-00-base.dtb
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-quill-p3310-1000-a00-00-base.dtb
dtb-$(CONFIG_ARCH_TEGRA_210_SOC) += nvidia/tegra210-jetson-tx1-p2597-2180-a01-devkit.dtb
dtb-$(CONFIG_ARCH_TEGRA_210_SOC) += nvidia/tegra210-jetson-cv-base-p2597-2180-a00.dtb
else
# GVS case
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-p2771-0000.dtb
dtb-$(CONFIG_ARCH_TEGRA_23x_SOC) += nvidia/tegra234-sim-vdk.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-p2888-0001-p2822-0000.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-p2888-0001-p2822-0000-uefi-acpi.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b01-A.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b01-B.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b03-A.dtb
dtb-$(CONFIG_ARCH_TEGRA_194_SOC) += nvidia/tegra194-e3550-0001-b03-B.dtb
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-vcm31-p2382-010-a01-00-base.dtb
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-quill-p3310-1000-c03-00-base.dtb
dtb-$(CONFIG_ARCH_TEGRA_186_SOC) += nvidia/tegra186-quill-p3310-1000-a00-00-base.dtb
dtb-$(CONFIG_ARCH_TEGRA_210_SOC) += nvidia/tegra210-jetson-tx1-p2597-2180-a01-devkit.dtb
dtb-$(CONFIG_ARCH_TEGRA_210_SOC) += nvidia/tegra210-jetson-cv-base-p2597-2180-a00.dtb
endif

DTB_LIST := $(dtb-y)
DTBO_LIST := $(dtbo-y)
dtb-y :=
dts_makefile=$(foreach d,$(wildcard $1*), $(call dts_makefile,$(d)/,$(2)) $(if $(findstring Makefile,$(d)),$(d)))
dts_mfiles = $(call dts_makefile, $(tegra-dtstree)/platform/, Makefile)
ifneq ($(dts_mfiles),)
dts-include :=
include $(dts_mfiles)
dtb-y := $(addprefix $(tegra-rel-dtstree)/hardware/nvidia/,$(dtb-y))
dtbo-y := $(addprefix $(tegra-rel-dtstree)/hardware/nvidia/,$(dtbo-y))
ifneq ($(dts-include),)
DTC_INCLUDE += $(addprefix $(tegra-dtstree)/,$(dts-include))
endif
endif

DTB_LIST += $(dtb-y)
DTB_NEW_RULE_LIST := $(dtb-y)
DTBO_LIST += $(dtbo-y)
DTBO_NEW_RULE_LIST := $(dtbo-y)

# Now save DTB_LIST to dtb-y
dtb-y := $(DTB_LIST)
dtbo-y := $(DTBO_LIST)

DTB_OBJS := $(addprefix $(obj)/,$(DTB_LIST))
DTBO_OBJS := $(addprefix $(obj)/,$(DTBO_LIST))

define _define_dtb_rule
$(obj)/$(call replace_ddot,$(1)): $(src)/$(patsubst %.dtb,%.dts,$(1)) FORCE
endef

$(foreach _dtb, $(DTB_NEW_RULE_LIST), $(eval $(call _define_dtb_rule,$(_dtb))))

DTB_OBJS := $(call replace_ddot,$(DTB_OBJS))
DTB_NEW_RULE_LIST := $(addprefix $(obj)/,$(DTB_NEW_RULE_LIST))
DTB_NEW_RULE_LIST := $(call replace_ddot,$(DTB_NEW_RULE_LIST))

$(DTB_NEW_RULE_LIST):
	$(call if_changed_dep,dtc,dtb)

define _define_dtbo_rule
$(obj)/$(call replace_ddot,$(1)): $(src)/$(patsubst %.dtbo,%.dts,$(1)) FORCE
endef

$(foreach _dtbo, $(DTBO_NEW_RULE_LIST), $(eval $(call _define_dtbo_rule,$(_dtbo))))

DTBO_OBJS := $(call replace_ddot,$(DTBO_OBJS))
DTBO_NEW_RULE_LIST := $(addprefix $(obj)/,$(DTBO_NEW_RULE_LIST))
DTBO_NEW_RULE_LIST := $(call replace_ddot,$(DTBO_NEW_RULE_LIST))

DTB_DTBO_OBJS := $(filter-out arch/arm64/boot/dts/nvidia/%.dtb, arch/arm64/boot/dts/nvidia/tegra210-jetson-cv-base-p2597-2180-a00.dtb $(DTB_OBJS) $(DTBO_OBJS))

$(DTBO_NEW_RULE_LIST):
	$(call if_changed_dep,dtc,dtb)

# FIXME: after kstable packaging disable for knext
# kstable packages the upstream file tegra186-p2771-0000.dts hence need special
# gcc argument with only one -I include-prefixes argument. Additional
# -I arguments passed for downstream DT build is resulting in
# errors from dtc command next.
%/tegra186-p2771-0000.dtb: %/tegra186-p2771-0000.dts
	mkdir -p arch/arm64/boot/dts/nvidia/ ; gcc -E -Wp,-MD,arch/arm64/boot/dts/nvidia/.tegra186-p2771-0000.dtb.d.pre.tmp -nostdinc -I$(srctree)/scripts/dtc/include-prefixes  -undef -D__DTS__ -x assembler-with-cpp -o arch/arm64/boot/dts/nvidia/.tegra186-p2771-0000.dtb.dts.tmp $< ; ./scripts/dtc/dtc -O dtb -o $@ -b 0 -i$(srctree)/arch/arm64/boot/dts/nvidia/ -i$(srctree)/scripts/dtc/include-prefixes -Wno-unit_address_vs_reg -Wno-unit_address_format -Wno-avoid_unnecessary_addr_size -Wno-alias_paths -Wno-graph_child_address -Wno-simple_bus_reg -Wno-unique_unit_address -Wno-pci_device_reg  -d arch/arm64/boot/dts/nvidia/.tegra186-p2771-0000.dtb.d.dtc.tmp arch/arm64/boot/dts/nvidia/.tegra186-p2771-0000.dtb.dts.tmp ; cat arch/arm64/boot/dts/nvidia/.tegra186-p2771-0000.dtb.d.pre.tmp arch/arm64/boot/dts/nvidia/.tegra186-p2771-0000.dtb.d.dtc.tmp > arch/arm64/boot/dts/nvidia/.tegra186-p2771-0000.dtb.d

dtbs: $(DTB_OBJS) $(DTBO_OBJS) FORCE
	if [ ! -d arch/arm64/boot/dts/nvidia/ ] ; then \
		mkdir arch/arm64/boot/dts/nvidia/ ; \
	fi
	cp -u $(DTB_DTBO_OBJS) arch/arm64/boot/dts/nvidia/
	if [ -d arch/arm64/boot/dts/_ddot_/ ] ; then \
		rm -rf arch/arm64/boot/dts/_ddot_/ ; \
	fi

clean-files := *.dtb *.dtbo *.tmp
