/*
 * Copyright (c) 2020 NVIDIA Corporation.  All rights reserved.
 *
 * NVIDIA Corporation and its licensors retain all intellectual property
 * and proprietary rights in and to this software and related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA Corporation is strictly prohibited.
 */
#ifndef __LDPC_IOCTL_H
#define __LDPC_IOCTL_H

struct ldpc_kmd_buf
{
    char kmd_version[10];
};

#define LDPC_IOCTL_MAGIC  'l'
#define LDPC_IOCTL_KMD_VER _IOR(LDPC_IOCTL_MAGIC, 1, struct ldpc_kmd_buf)
#define LDPC_IOC_MAXNR 1

#endif
