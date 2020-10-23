# SPDX-License-Identifier: GPL-2.0
CONFIG_DRM_TEGRA_NEXT := m
ccflags-$(CONFIG_DRM_TEGRA_DEBUG) += -DDEBUG
ccflags-y += -I$(srctree.tegra-drm)/include

tegra-drm-next-y := \
	drm.o \
	uapi/uapi.o \
	uapi/submit.o \
	uapi/firewall.o \
	uapi/gather_bo.o \
	gem.o \
	fb.o \
	dp.o \
	hub.o \
	plane.o \
	dc.o \
	output.o \
	rgb.o \
	hda.o \
	hdmi.o \
	mipi-phy.o \
	dsi.o \
	sor.o \
	dpaux.o \
	gr2d.o \
	gr3d.o \
	falcon.o \
	vic.o

obj-$(CONFIG_DRM_TEGRA_NEXT) += tegra-drm-next.o
