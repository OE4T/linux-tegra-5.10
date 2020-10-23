# SPDX-License-Identifier: GPL-2.0
CONFIG_TEGRA_HOST1X_NEXT := m
ccflags-y := -I$(srctree.host1x)/include

host1x-next-y = \
	bus.o \
	syncpt.o \
	dev.o \
	intr.o \
	cdma.o \
	channel.o \
	job.o \
	debug.o \
	mipi.o \
	uapi.o \
	fence.o \
	hw/host1x01.o \
	hw/host1x02.o \
	hw/host1x04.o \
	hw/host1x05.o \
	hw/host1x06.o \
	hw/host1x07.o

obj-$(CONFIG_TEGRA_HOST1X_NEXT) += host1x-next.o
