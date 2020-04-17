/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * Copyright (c) 2020, NVIDIA Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _UAPI_LDPC_IOCTL_H_
#define _UAPI_LDPC_IOCTL_H_

struct ldpc_kmd_buf
{
    char kmd_version[10];
};

#define LDPC_IOCTL_MAGIC  'l'
#define LDPC_IOCTL_KMD_VER _IOR(LDPC_IOCTL_MAGIC, 1, struct ldpc_kmd_buf)
#define LDPC_IOC_MAXNR 1

#endif
