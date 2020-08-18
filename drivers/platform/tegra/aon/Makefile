#
# Always On Sensor Processing Engine code.
#
GCOV_PROFILE := y

ccflags-y += -I$(srctree.nvidia-t23x)/drivers/platform/tegra/aon/include
ccflags-y += -I$(srctree)/include

ccflags-y += -Werror

obj-$(CONFIG_TEGRA_SPE) += tegra234-aon.o

tegra234-aon-y += \
	tegra-aon-reset.o \
	tegra-aon-hsp.o \
	tegra-aon-ast.o \
	tegra-aon-mail.o \
	tegra-aon-debug.o \
	tegra-aon-module.o
