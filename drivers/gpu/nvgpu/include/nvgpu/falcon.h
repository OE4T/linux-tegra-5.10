/*
 * Copyright (c) 2017-2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef NVGPU_FALCON_H
#define NVGPU_FALCON_H

/**
 * @file
 * @page unit-falcon Unit Falcon
 *
 * Overview
 * ========
 *
 * Falcon unit is responsible for managing falcon engines/ controllers
 * that provide base support for GPU functions such as context switch
 * (FECS/GPCCS), power/perf management (PMU), secure load of other
 * falcons (ACR). These GPU functions are executed by a uCode
 * which runs on each falcon.
 *
 * Falcon unit provides interfaces to nvgpu driver to access falcon
 * controller through following interfaces:
 *
 *   + Falcon internal registers
 *     + Intrerrupt registers
 *     + Mailbox registers
 *     + Memory control registers etc.
 *   + IMEM (Instruction memory), DMEM (Data memory), EMEM (External memory)
 *
 * Data Structures
 * ===============
 *
 * The major data structures exposed to users of the Falcon unit in nvgpu are
 * following:
 *
 *   + struct nvgpu_falcon
 *
 *       struct nvgpu_falcon defines a Falcon's software state. It contains the
 *       hardware ID, base address for registers access, memory access locks,
 *       engine specific functions.
 *
 *   + struct nvgpu_falcon_bl_info
 *
 *       struct nvgpu_falcon_bl_info specifies the bootloader bootstrap
 *       parameters.
 *
 * Static Design
 * =============
 *
 * Falcon Initialization
 * ---------------------
 * Before accessing the falcon's registers and memory for various tasks like
 * loading the firmwares or check the falcon status nvgpu driver needs to
 * initialize the falcon software state. This sets up the base address for
 * falcon register access, initializes the memory access locks and links
 * the hardware specific functions.
 *
 * Falcon Teardown
 * ---------------------
 * While powering down the device, falcon software state that is setup by
 * nvgpu_falcon_sw_init is destroyed.
 *
 * External APIs
 * -------------
 *   + nvgpu_falcon_sw_init()
 *   + nvgpu_falcon_sw_free()
 *
 * Dynamic Design
 * ==============
 *
 * General operation
 * -----------------
 *   + During nvgpu driver power on, various falcons are initialized with
 *     nvgpu_falcon_sw_init (PMU, SEC2, NVDEC, GSPLITE, FECS) and then
 *     ACR is initialized.
 *   + ACR HS bootloader is executed through nvgpu_falcon_bl_bootstrap.
 *     After bl_bootstrap falcon is reset by nvgpu_falcon_reset.
 *
 * Sequence for loading any uCode on the falcon
 * --------------------------------------------
 *   + Reset the falcon
 *   + Clear the interrupts
 *   + Copy secure/non-secure code to IMEM and data to DMEM
 *   + Update mailbox registers as required for ACK or reading capabilities
 *   + Bootstrap falcon
 *   + Wait for halt
 *   + Read mailbox registers
 *
 * External APIs
 * -------------
 *   + nvgpu_falcon_reset()
 *   + nvgpu_falcon_wait_for_halt()
 *   + nvgpu_falcon_wait_idle()
 *   + nvgpu_falcon_mem_scrub_wait()
 *   + nvgpu_falcon_copy_to_dmem()
 *   + nvgpu_falcon_copy_to_imem()
 *   + nvgpu_falcon_bootstrap()
 *   + nvgpu_falcon_mailbox_read()
 *   + nvgpu_falcon_mailbox_write()
 *   + nvgpu_falcon_bl_bootstrap()
 *   + nvgpu_falcon_get_mem_size()
 *   + nvgpu_falcon_get_id()
 */

#include <nvgpu/types.h>
#include <nvgpu/lock.h>

/** Falcon ID for PMU engine */
#define FALCON_ID_PMU       (0U)
/** Falcon ID for GSPLITE engine */
#define FALCON_ID_GSPLITE   (1U)
/** Falcon ID for FECS engine */
#define FALCON_ID_FECS      (2U)
/** Falcon ID for GPCCS engine */
#define FALCON_ID_GPCCS     (3U)
/** Falcon ID for NVDEC engine */
#define FALCON_ID_NVDEC     (4U)
/** Falcon ID for SEC2 engine */
#define FALCON_ID_SEC2      (7U)
/** Falcon ID for MINION engine */
#define FALCON_ID_MINION    (10U)
#define FALCON_ID_END	    (11U)
#define FALCON_ID_INVALID   0xFFFFFFFFU

#define FALCON_MAILBOX_0	0x0U
#define FALCON_MAILBOX_1	0x1U
/** Total Falcon mailbox registers */
#define FALCON_MAILBOX_COUNT	0x02U
/** Falcon IMEM block size in bytes */
#define FALCON_BLOCK_SIZE	0x100U

#define GET_IMEM_TAG(IMEM_ADDR) ((IMEM_ADDR) >> 8U)

#define GET_NEXT_BLOCK(ADDR) \
	(((((ADDR) + (FALCON_BLOCK_SIZE - 1U)) & ~(FALCON_BLOCK_SIZE-1U)) \
		/ FALCON_BLOCK_SIZE) << 8U)

/**
 * Falcon ucode header format
 *   OS Code Offset
 *   OS Code Size
 *   OS Data Offset
 *   OS Data Size
 *   NumApps (N)
 *   App   0 Code Offset
 *   App   0 Code Size
 *   .  .  .  .
 *   App   N - 1 Code Offset
 *   App   N - 1 Code Size
 *   App   0 Data Offset
 *   App   0 Data Size
 *   .  .  .  .
 *   App   N - 1 Data Offset
 *   App   N - 1 Data Size
 *   OS Ovl Offset
 *   OS Ovl Size
 */
#define OS_CODE_OFFSET 0x0U
#define OS_CODE_SIZE   0x1U
#define OS_DATA_OFFSET 0x2U
#define OS_DATA_SIZE   0x3U
#define NUM_APPS       0x4U
#define APP_0_CODE_OFFSET 0x5U
#define APP_0_CODE_SIZE   0x6U

struct gk20a;
struct nvgpu_falcon;

/**
 * Falcon memory types.
 */
enum falcon_mem_type {
	/** Falcon data memory */
	MEM_DMEM = 0,
	/** Falcon instruction memory */
	MEM_IMEM
};

/**
 * This struct holds the firmware bootloader parameters such as pointer and size
 * of bootloader descriptor and source instructions. It also contains the
 * address to be programmed as boot vector.
 */
struct nvgpu_falcon_bl_info {
	/** bootloader source instructions */
	void *bl_src;
	/** bootloader descriptor */
	u8 *bl_desc;
	/** bootloader descriptor size */
	u32 bl_desc_size;
	/** bootloader source instructions size */
	u32 bl_size;
	/** boot vector/address to start execution */
	u32 bl_start_tag;
};

/**
 * This struct holds the falcon ops which are falcon engine specific.
 */
struct nvgpu_falcon_engine_dependency_ops {
	/** reset function specific to engine */
	int (*reset_eng)(struct gk20a *g);
	/** Falcon bootstrap config function specific to engine */
	void (*setup_bootstrap_config)(struct gk20a *g);
	/** copy functions for SEC2 falcon engines on dGPU that supports EMEM */
	int (*copy_from_emem)(struct gk20a *g, u32 src, u8 *dst,
		u32 size, u8 port);
	int (*copy_to_emem)(struct gk20a *g, u32 dst, u8 *src,
		u32 size, u8 port);
};

/**
 * This struct holds the software state of the underlying falcon engine.
 * Falcon interfaces rely on this state.
 */
struct nvgpu_falcon {
	/** The GPU driver struct */
	struct gk20a *g;
	/** Falcon ID for the engine */
	u32 flcn_id;
	/** Base address to access falcon registers */
	u32 flcn_base;
	/* Indicates if the falcon is supported and initialized for use. */
	bool is_falcon_supported;
	/* Indicates if the falcon interrupts are enabled. */
	bool is_interrupt_enabled;
	/* Lock to access the falcon's IMEM. */
	struct nvgpu_mutex imem_lock;
	/* Lock to access the falcon's DMEM. */
	struct nvgpu_mutex dmem_lock;
#ifdef CONFIG_NVGPU_DGPU
	/* Indicates if the falcon supports EMEM. */
	bool emem_supported;
	/* Lock to access the falcon's EMEM. */
	struct nvgpu_mutex emem_lock;
#endif
	/* Functions for engine specific reset and memory access. */
	struct nvgpu_falcon_engine_dependency_ops flcn_engine_dep_ops;
};

/**
 * @brief Reset the falcon CPU or Engine.
 *
 * @param flcn [in] The falcon
 *
 * Does the falcon #flcn reset through CPUCTL control register if not being
 * controlled by an engine, else does engine dependent reset and completes by
 * waiting for memory scrub completion.
 *
 * @return 0 in case of success, < 0 in case of failure.
 * @retval -ETIMEDOUT in case the timeout expired waiting for memory scrub.
 */
int nvgpu_falcon_reset(struct nvgpu_falcon *flcn);

/**
 * @brief Wait for the falcon CPU to be halted.
 *
 * @param flcn [in] The falcon
 * @param timeout [in] Duration to wait for halt
 *
 * Return the falcon #flcn's halt status waiting for passed timeout duration.
 *
 * @return 0 in case of success, < 0 in case of failure.
 * @retval -ETIMEDOUT in case the timeout expired waiting for halt.
 */
int nvgpu_falcon_wait_for_halt(struct nvgpu_falcon *flcn, unsigned int timeout);

/**
 * @brief Wait for the falcon to be idle.
 *
 * @param flcn [in] The falcon
 *
 * Wait for falcon #flcn's HW units to go idle.
 *
 * @return 0 in case of success, < 0 in case of failure.
 * @retval -ETIMEDOUT in case the timeout expired waiting for idle.
 */
int nvgpu_falcon_wait_idle(struct nvgpu_falcon *flcn);


/**
 * @brief Wait for the falcon memory scrub.
 *
 * @param flcn [in] The falcon
 *
 * Wait for falcon #flcn's IMEM and DMEM scrubbing to complete.
 *
 * @return 0 in case of success, < 0 in case of failure.
 * @retval -ETIMEDOUT in case the timeout expired waiting for scrub completion.
 */
int nvgpu_falcon_mem_scrub_wait(struct nvgpu_falcon *flcn);

/**
 * @brief Copy data to falcon's DMEM.
 *
 * @param flcn [in] The falcon
 * @param dst  [in] Address in the DMEM (Block and offset)
 * @param src  [in] Source data to be copied to DMEM
 * @param size [in] Size in bytes of the source data
 * @param port [in] DMEM port to be used for copy
 *
 * Validates the parameters for DMEM alignment and size restrictions.
 * Copy source data #src of #size though #port at offset #dst of dmem
 * of #flcn.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_falcon_copy_to_dmem(struct nvgpu_falcon *flcn,
	u32 dst, u8 *src, u32 size, u8 port);

/**
 * @brief Copy data to falcon's IMEM.
 *
 * @param flcn [in] The falcon
 * @param dst  [in] Address in the IMEM (Block and offset)
 * @param src  [in] Source data to be copied to IMEM
 * @param size [in] Size in bytes of the source data
 * @param port [in] IMEM port to be used for copy
 * @param sec  [in] Indicates if blocks are to be marked as secure
 * @param tag  [in] Tag to be set for the blocks
 *
 * Validates the parameters for IMEM alignment and size restrictions.
 * Copy source data #src of #size though #port at offset #dst of imem
 * of #flcn. Optionally set the tag and mark blocks secure.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_falcon_copy_to_imem(struct nvgpu_falcon *flcn,
	u32 dst, u8 *src, u32 size, u8 port, bool sec, u32 tag);

/**
 * @brief Bootstrap the falcon.
 *
 * @param flcn [in] The falcon
 * @param boot_vector [in] Address to start the falcon execution
 *
 * Set the boot vector address, DMA control and start the falcon CPU
 * execution.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_falcon_bootstrap(struct nvgpu_falcon *flcn, u32 boot_vector);

/**
 * @brief Sets bootstrap configuration required for the Falcon boot.
 *
 * @param flcn [in] The falcon
 *
 * Set the virtual and physical apertures, context interface attributes &
 * instance block address.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_falcon_setup_bootstrap_config(struct nvgpu_falcon *flcn);

/**
 * @brief Read the falcon mailbox register.
 *
 * @param flcn [in] The falcon
 * @param mailbox_index [in] Index of the mailbox register
 *
 * Reads data from mailbox register of the falcon #flcn.
 *
 * @return register data in case of success, 0 in case of failure.
 */
u32 nvgpu_falcon_mailbox_read(struct nvgpu_falcon *flcn, u32 mailbox_index);

/**
 * @brief Write the falcon mailbox register.
 *
 * @param flcn [in] The falcon
 * @param mailbox_index [in] Index of the mailbox register
 * @param data [in] Data to be written to mailbox register
 *
 * Writes #data to mailbox register #mailbox_index of the falcon #flcn.
 */
void nvgpu_falcon_mailbox_write(struct nvgpu_falcon *flcn, u32 mailbox_index,
	u32 data);

/**
 * @brief Bootstrap the falcon with bootloader.
 *
 * @param flcn [in] The falcon
 * @param bl_info [in] Bootloader input parameters
 *
 * Copies bootloader source and descriptor to IMEM and DMEM and then
 * bootstraps the falcon.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_falcon_bl_bootstrap(struct nvgpu_falcon *flcn,
	struct nvgpu_falcon_bl_info *bl_info);

/**
 * @brief Bootstrap the falcon with HS ucode.
 *
 * @param flcn  [in] The falcon
 * @param ucode [in] ucode to be copied
 * @param ucode_header [in] ucode header
 *
 * Copies HS ucode source and descriptor to IMEM and DMEM and then
 * bootstraps the falcon.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_falcon_hs_ucode_load_bootstrap(struct nvgpu_falcon *flcn, u32 *ucode,
	u32 *ucode_header);

/**
 * @brief Get the size of falcon's memory.
 *
 * @param flcn [in] The falcon
 * @param type [in] Falcon memory type (IMEM, DMEM)
 * @param size [out] Size of the falcon memory type
 *
 * Retrieves the size of the falcon memory in bytes from the HW config
 * registers in output parameter #size.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_falcon_get_mem_size(struct nvgpu_falcon *flcn,
			      enum falcon_mem_type type, u32 *size);

/**
 * @brief Get the falcon ID.
 *
 * @param flcn [in] The falcon
 *
 * @return the falcon ID of #flcn, flcn->flcn_id.
 */
u32 nvgpu_falcon_get_id(struct nvgpu_falcon *flcn);

/**
 * @brief Get the reference to falcon struct in GPU driver struct.
 *
 * @param g [in] The GPU driver struct
 * @param flcn_id [id] falcon ID
 *
 * @return the falcon struct of #g corresponding to #flcn_id.
 */
struct nvgpu_falcon *nvgpu_falcon_get_instance(struct gk20a *g, u32 flcn_id);

/**
 * @brief Initialize the falcon software state.
 *
 * @param g [in] The GPU driver struct
 * @param flcn_id [id] falcon ID
 *
 * Initializes the nvgpu_falcon structure in device structure #g based on
 * #flcn_id. Sets falcon specific HAL values and methods by calling HAL
 * functions. Initializes lock for memory copy operations.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_falcon_sw_init(struct gk20a *g, u32 flcn_id);

/**
 * @brief Free the falcon software state.
 *
 * @param g [in] The GPU driver struct
 * @param flcn_id [id] falcon ID
 *
 * Destroys the nvgpu_falcon structure state in device structure #g based on
 * #flcn_id.
 */
void nvgpu_falcon_sw_free(struct gk20a *g, u32 flcn_id);

#ifdef CONFIG_NVGPU_DGPU
int nvgpu_falcon_copy_from_emem(struct nvgpu_falcon *flcn,
	u32 src, u8 *dst, u32 size, u8 port);
int nvgpu_falcon_copy_to_emem(struct nvgpu_falcon *flcn,
	u32 dst, u8 *src, u32 size, u8 port);
#endif

#ifdef CONFIG_NVGPU_FALCON_DEBUG
void nvgpu_falcon_dump_stats(struct nvgpu_falcon *flcn);
#endif

#ifdef CONFIG_NVGPU_FALCON_NON_FUSA
int nvgpu_falcon_clear_halt_intr_status(struct nvgpu_falcon *flcn,
		unsigned int timeout);
void nvgpu_falcon_set_irq(struct nvgpu_falcon *flcn, bool enable,
	u32 intr_mask, u32 intr_dest);
int nvgpu_falcon_copy_from_dmem(struct nvgpu_falcon *flcn,
	u32 src, u8 *dst, u32 size, u8 port);
int nvgpu_falcon_copy_from_imem(struct nvgpu_falcon *flcn,
	u32 src, u8 *dst, u32 size, u8 port);
void nvgpu_falcon_print_dmem(struct nvgpu_falcon *flcn, u32 src, u32 size);
void nvgpu_falcon_print_imem(struct nvgpu_falcon *flcn, u32 src, u32 size);
void nvgpu_falcon_get_ctls(struct nvgpu_falcon *flcn, u32 *sctl, u32 *cpuctl);
#endif

#endif /* NVGPU_FALCON_H */
