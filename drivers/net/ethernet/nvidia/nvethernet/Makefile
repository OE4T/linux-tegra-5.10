# Copyright (c) 2018-2019, NVIDIA CORPORATION.  All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

OSI := ../../../../../../nvethernetrm/osi

obj-$(CONFIG_NVETHERNET) += nvethernet.o

nvethernet-objs:= ether_linux.o \
		  osd.o \
		  ethtool.o \
		  sysfs.o \
		  ioctl.o \
		  $(OSI)/osi_core.o \
		  $(OSI)/osi_common.o \
		  $(OSI)/osi_dma.o \
		  $(OSI)/osi_dma_txrx.o \
		  $(OSI)/eqos_core.o \
		  $(OSI)/eqos_dma.o
