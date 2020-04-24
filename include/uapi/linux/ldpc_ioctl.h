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

#include <linux/types.h>

enum ldpc_cmd
{
    /* Command to check if LDPC KMD, RISC-V and Engine are initialized */
    LDPC_CMD_IF_ALIVE = 1U,
    /* Command to put RISC-V and Engine to low power/idle wait state */
    LDPC_CMD_EN_SLEEP,
    /* Command RISC-V to start polling of semaphore for new task */
    LDPC_CMD_EN_POLL,
    /* Command RISC-V to stop polling semaphore for new task */
    LDPC_CMD_DIS_POLL,
    /* Command to disable/enable sub-engine features of LDPC */
    LDPC_CMD_CFG_ENG
};

enum buffer_operation
{
    /* Buffer map operation */
    LDPC_BUFFER_MAP = 1U,
    /* Buffer unmap operation */
    LDPC_BUFFER_UNMAP
};

enum error_code
{
    /* ENG: Invalid CMD Or DESC */
    LDPC_EC_DESC_INVAL = 1U,
    /* ENG: HARQ buffer full */
    LDPC_EC_HARQ_FULL,
    /* ENG: SEM FIFO full */
    LDPC_EC_SEM_FULL,
    /* ENG: SEM FIFO full */
    LDPC_EC_STS_FULL,
    /* ENG: Memory access faults */
    LDPC_EC_MEM_INVAL,
    /* ENG: Engine hung */
    LDPC_EC_ENG_HANG,
    /* Micro Controller exception */
    LDPC_EC_FW_EXC,
    /* Micro Controller FW hung */
    LDPC_EC_FW_HANG,
    /* KMD hung */
    LDPC_EC_KMD_HANG,
    /* KMD Operation Error */
    LDPC_EC_KMD_OPERR
};

enum power_operation
{
    /* Power On the engine */
    LDPC_POWER_ON = 1U,
    /* Power Off the engine */
    LDPC_POWER_OFF,
    /* Reset the engine */
    LDPC_POWER_RESET
};

struct ldpc_engine_cmd_arg
{
    /* IN: Type of command given to engine */
    __u64 riscv_cmd;
    /* OUT: RISCV status */
    __u64 riscv_status;
};


struct ldpc_buffer_op_arg
{
    /* IN: Buffer Operation e.g. LDPC_BUFFER_MAP or LDPC_BUFFER_UNMAP */
    __u64 buf_op;
    /* IN: Buffer Handle */
    __u64 buf_handle;
    /* IN: Buffer Size */
    __u32 buf_size;
    /* IN: Offset with respect to buffer address */
    __u32 buf_offset;
    /* OUT: Mapped buffer IOVA address */
    __u64 buf_iova;
    /* OUT: Result status of operation */
    __u64 buf_op_status;
};

struct ldpc_channel_setup_arg
{
    /* IN: Channel command version */
    __u32 ch_cmd_ver;
    /* Reserved */
    __u32 reserved1;
    /* IN: Side channel write address (IOVA) */
    __u64 ch_wr;
    /* IN: Side channel read address (IOVA) */
    __u64 ch_rd;
    /* IN: Desc table start address (IOVA) */
    __u64 desc_start;
    /* IN: Desc table end address (IOVA) */
    __u64 desc_end;
    /* OUT: Channel status */
    __u64 ch_status;
};

struct ldpc_error
{
    /* OUT: Error code updated by KMD/FW. It is one of the following
    * LDPC_EC_DESC_INVAL, LDPC_EC_HARQ_FULL, LDPC_EC_SEM_FULL,
    * LDPC_EC_STS_FULL, LDPC_EC_MEM_INVAL, LDPC_EC_ENG_HANG,
    * LDPC_EC_FW_EXC, LDPC_EC_FW_HANG, LDPC_EC_KMD_HANG, LDPC_EC_KMD_OPERR
    */
    __u64 err_code;
    /* OUT: Timestamp at which error has occurred */
    __u64 timestamp;
    /* Reserved */
    __u64 reserved1;
    /* Reserved */
    __u64 reserved2;
};

struct ldpc_eh_setup_arg
{
    /* IN: Error buffer IOVA address */
    struct ldpc_error *err_handle;
    /* IN: Number of error buffers */
    __u16 err_nr_handles;
};

struct ldpc_version
{
    /* OUT: Major version number */
    __u32 major;
    /* OUT: Minor version number */
    __u32 minor;
};

struct ldpc_get_version_arg
{
    /* OUT: LDPC KMD version */
    struct ldpc_version ldpc_kmd_ver;
    /* OUT: LDPC API version */
    struct ldpc_version ldpc_api_ver;
    /* OUT: LDPC FW version */
    struct ldpc_version ldpc_fw_ver;
    /* OUT: Output status */
    __u64 ver_status;
};

struct ldpc_pwrop_arg
{
    /* IN: Power operation to execute.
    * It is one of the LDPC_POWEROP_ON, LDPC_POWEROP_OFF,
    * LDPC_POWEROP_RESET */
    __u64 pwr_cmd;
    /* OUT: Output status of power operation execution */
    __u64 pwr_status;
};

#define LDPC_IOCTL_MAGIC 'l'

/**
 * IOCTL to directly communicate with LDPC HW via RISC-V from UMD.
 */
#define LDPC_IOCTL_ENGINE_OP _IOWR(LDPC_IOCTL_MAGIC, 1, struct ldpc_engine_cmd_arg)

/**
 * IOCTL to provide user space allocated buffer virtual address
 * alongwith buffer parameters and privileged kernel space operation
 * to perform.
 */
#define LDPC_IOCTL_BUFFER_OP _IOWR(LDPC_IOCTL_MAGIC, 2, struct ldpc_buffer_op_arg)

/**
 * IOCTL to create side channel between UMD to FW.
 */
#define LDPC_IOCTL_CHANNEL_OP _IOWR(LDPC_IOCTL_MAGIC, 3, struct ldpc_channel_setup_arg)

/**
 * IOCTL to setup buffer, which is populated by KMD and RISCV for any error.
 */
#define LDPC_IOCTL_EH_OP _IOWR(LDPC_IOCTL_MAGIC, 4, struct ldpc_eh_setup_arg)

/**
 * IOCTL to query LDPC KMD, API and Firmware version.
 */
#define LDPC_IOCTL_VERSION_OP _IOWR(LDPC_IOCTL_MAGIC, 5, struct ldpc_get_version_arg)

/**
 * IOCTL to power on/off the engine, reset engine.
 */
#define LDPC_IOCTL_POWER_OP _IOWR(LDPC_IOCTL_MAGIC, 6, struct ldpc_pwrop_arg)

#define LDPC_IOC_MAXNR 6
#endif /* _UAPI_LDPC_IOCTL_H_ */
