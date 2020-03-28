/*
 * Copyright (c) 2019-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_ENGINE_H
#define NVGPU_ENGINE_H
/**
 * @file
 *
 * Abstract interface for engine related functionality.
 */

#include <nvgpu/types.h>

/** @cond DOXYGEN_SHOULD_SKIP_THIS */
#if defined(CONFIG_NVGPU_NON_FUSA) && defined(CONFIG_NVGPU_NEXT)
#include "include/nvgpu/nvgpu_next_engines.h"
#endif
/** @endcond DOXYGEN_SHOULD_SKIP_THIS */

/**
 * Invalid engine id value.
 */
#define NVGPU_INVALID_ENG_ID		(~U32(0U))

struct gk20a;
struct nvgpu_fifo;
struct nvgpu_device;

/**
 * Engine enum types used for s/w purpose. These enum values are
 * different as compared to engine enum types defined by h/w.
 * Refer device.h header file.
 */
enum nvgpu_fifo_engine {
	/** GR engine enum */
	NVGPU_ENGINE_GR        = 0U,
	/** GR ce engine enum */
	NVGPU_ENGINE_GRCE      = 1U,
	/** Async CE engine enum */
	NVGPU_ENGINE_ASYNC_CE  = 2U,
	/** Invalid engine enum */
	NVGPU_ENGINE_INVAL     = 3U,
};

struct nvgpu_engine_info {
	/**
	 * Contains valid engine id read from device info h/w register or
	 * invalid engine id, U32_MAX.
	 */
	u32 engine_id;
	/**
	 * Contains valid runlist id read from device info h/w register or
	 * invalid runlist id, U32_MAX.
	 */
	u32 runlist_id;
	/**
	 * Contains bit mask for the intr id read from device info h/w register
	 * or invalid intr id mask, U32_MAX. This is used to check pending
	 * interrupt for the engine_id.
	 */
	u32 intr_mask;
	/**
	 * Contains bit mask for the reset id read from device info h/w register
	 * or invalid reset id mask, U32_MAX.
	 */
	u32 reset_mask;
	/** Pbdma id servicing #runlist_id. */
	u32 pbdma_id;
	/**
	 * Specifies instance of a device, allowing s/w to distinguish between
	 * multiple copies of a device present on the chip.
	 */
	u32 inst_id;
	/**
	 * Used to determine the start of the h/w register address space
	 * for #inst_id 0.
	 */
	u32 pri_base;
	/**
	 * Contains valid mmu fault id read from device info h/w register or
	 * invalid mmu fault id, U32_MAX.
	 */
	u32 fault_id;
	/** Engine enum type used for s/w purpose. */
	enum nvgpu_fifo_engine engine_enum;

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */
#if defined(CONFIG_NVGPU_NON_FUSA) && defined(CONFIG_NVGPU_NEXT)
	/* nvgpu next engine info additions */
	struct nvgpu_next_engine_info nvgpu_next;
#endif
	/** @endcond DOXYGEN_SHOULD_SKIP_THIS */
};
/**
 * @brief Get s/w defined engine enum type for engine enum type defined by h/w.
 *        See device.h for engine enum types defined by h/w.
 *
 * @param g [in]		The GPU driver struct.
 * @param dev [in]		Device to check.
 *
 * This is used to map engine enum type defined by h/w to engine enum type
 * defined by s/w.
 *
 * @return S/w defined valid engine enum type < #NVGPU_ENGINE_INVAL.
 * @retval #NVGPU_ENGINE_INVAL in case h/w #engine_type
 *         does not support gr (graphics) and ce (copy engine) engine enum
 *         types or if #engine_type does not match with h/w defined engine enum
 *         types for gr and/or ce engines.
 */
enum nvgpu_fifo_engine nvgpu_engine_enum_from_dev(struct gk20a *g,
					const struct nvgpu_device *dev);
/**
 * @brief Get pointer to #nvgpu_engine_info for the h/w engine id.
 *
 * @param g [in]		The GPU driver struct.
 * @param engine_id [in]	Active (h/w) Engine id.
 *
 * If #engine_id is one of the supported h/w engine ids, get pointer to
 * #nvgpu_engine_info from an array of structures that is indexed by h/w
 * engine id.
 *
 * @return Pointer to #nvgpu_engine_info.
 * @retval NULL if #g is NULL.
 * @retval NULL if engine_id is not less than max supported number of engines
 *         i.e. #nvgpu_fifo.max_engines or if #engine_id does not match with
 *         any engine id supported by h/w or number of available engines
 *         i.e. #nvgpu_fifo.num_engines is 0.
 */
struct nvgpu_engine_info *nvgpu_engine_get_active_eng_info(
		struct gk20a *g, u32 engine_id);
/**
 * @brief Get instance count and h/w engine id/s for s/w defined engine
 *        enum type. See #nvgpu_fifo_engine for s/w defined engine enum types.
 *
 * @param g [in]		The GPU driver struct.
 * @param engine_ids [in,out]	Pointer to memory area to store h/w engine ids.
 * @param engine_id_sz [in]	Number of h/w engine ids to be stored in
 *                              memory area pointed by #engine_ids.
 * @param engine_enum [in]	Engine enum types defined by #nvgpu_fifo_engine.
 *
 * - Check validity of input parameters.
 * - Get #nvgpu_engine_info for each of #nvgpu_fifo.num_engines.
 *   - Increase instance count and store h/w engine id in #engine_ids if
 *     #nvgpu_engine_info.engine_enum matches with #engine_enum until
 *     instance count is less than #engine_id_sz.
 *
 * @return Instance count.
 * @retval 0 if #g is NULL or #engine_id_sz is 0 or #engine_enum is
 *         #NVGPU_ENGINE_INVAL
 * @retval 0 if #nvgpu_fifo.num_engines is 0.
 */
u32 nvgpu_engine_get_ids(struct gk20a *g,
		u32 *engine_ids, u32 engine_id_sz,
		enum nvgpu_fifo_engine engine_enum);
/**
 * @brief Check if engine id is one of the supported h/w engine ids.
 *
 * @param g [in]		The GPU driver struct.
 * @param engine_id [in]	Engine id.
 *
 * Check if #engine_id is one of the supported active engine ids.
 *
 * @return True if #engine_id is valid.
 * @retval False if engine_id is not less than maximum number of supported
 *         engines on the chip i.e. #nvgpu_fifo.max_engines or if engine id
 *         does not match with any of the engine ids supported by h/w.
 */
bool nvgpu_engine_check_valid_id(struct gk20a *g, u32 engine_id);
/**
 * @brief Get instance count and first available h/w engine id for
 *        #NVGPU_ENGINE_GR engine enum type.
 *
 * @param g [in]		The GPU driver struct.
 *
 * Call #nvgpu_engine_get_ids to get first available #NVGPU_ENGINE_GR
 * engine enum type.
 *
 * @return First available h/w engine id for #NVGPU_ENGINE_GR engine enum type.
 * @retval #NVGPU_INVALID_ENG_ID if #NVGPU_ENGINE_GR engine enum type could not
 *         be found in the set of available h/w engine ids.
 */
u32 nvgpu_engine_get_gr_id(struct gk20a *g);
/**
 * @brief Get intr mask for the GR engine supported by the chip.
 *
 * @param g[in]			The GPU driver struct.
 *
 * For each of #nvgpu_fifo.num_engines, get pointer to
 * #nvgpu_engine_info. Use this to get #nvgpu_engine_info.intr_mask.
 * If #nvgpu_engine_info.engine_num type matches with
 * #NVGPU_ENGINE_GR, local intr_mask variable is logically ORed with
 * #nvgpu_engine_info.intr_mask.
 *
 * @return Interrupt mask for GR engine.
 * @retval 0 if #nvgpu_fifo.num_engines is 0.
 * @retval 0 if all of the supported engine enum types don't match with
 *         #NVGPU_ENGINE_GR.
 */
u32 nvgpu_gr_engine_interrupt_mask(struct gk20a *g);
/**
 * @brief Get intr mask for the CE engines supported by the chip.
 *
 * @param g [in]		The GPU driver struct.
 *
 * For each of #nvgpu_fifo.num_engines, get pointer to
 * #nvgpu_engine_info. Use this to get #nvgpu_engine_info.intr_mask.
 * If #nvgpu_engine_info.engine_num type matches with
 * #NVGPU_ENGINE_GRCE or #NVGPU_ENGINE_ASYNC_CE but interrupt handlers
 * are not supported or engine_enum type matches with #NVGPU_ENGINE_GR
 * , local intr_mask variable is not logically ORed with
 * #nvgpu_engine_info.intr_mask.
 *
 * @return Interrupt mask for CE engines that support interrupt handlers.
 * @retval 0 if #nvgpu_fifo.num_engines is 0.
 * @retval 0 if all of the supported engine enum types match with
 *         #NVGPU_ENGINE_GRCE or #NVGPU_ENGINE_ASYNC_CE and does not
 *         support interrupt handler or engine enum type matches with
 *         #NVGPU_ENGINE_GR.
 */
u32 nvgpu_ce_engine_interrupt_mask(struct gk20a *g);
/**
 * @brief Get intr mask for the h/w engine id.
 *
 * @param g [in]		The GPU driver struct.
 * @param engine_id [in]	H/w Engine id.
 *
 * Get pointer to #nvgpu_engine_info for the #engine_id. Use this to
 * get intr mask for the #engine_id.
 *
 * @return Intr mask for the #engine_id.
 * @retval 0 if pointer to #nvgpu_engine_info is NULL for the
 *         #engine_id.
 */
u32 nvgpu_engine_act_interrupt_mask(struct gk20a *g, u32 engine_id);
/**
 * @brief Get engine reset mask for CE engines.
 *
 * @param g [in]		The GPU driver struct.
 *
 * For each #nvgpu_fifo.num_engines, get pointer to #nvgpu_engine_info.
 * Use this pointer to check if engine enum type matches with
 * #NVGPU_ENGINE_GRCE or #NVGPU_ENGINE_ASYNC_CE. Upon match, logical OR
 * the reset mask (init to 0) with reset_mask read from pointer to
 * #nvgpu_engine_info.
 *
 * @return Reset mask for all the supported CE engine enum types.
 * @retval NULL if #g is NULL.
 * @retval 0 if none of the supported engine enum types match with
 *         #NVGPU_ENGINE_GRCE or #NVGPU_ENGINE_ASYNC_CE.
 */
u32 nvgpu_engine_get_all_ce_reset_mask(struct gk20a *g);
/**
 * @brief Allocate and initialize s/w context for engine related info.
 *
 * @param g [in]		The GPU driver struct.
 *
 * - Get max number of engines supported on the chip from h/w config register.
 * - Allocate kernel memory area for storing engine info for max number of
 *   engines supported on the chip. This allocated area of memory is set to 0
 *   and then filled with value read from device info h/w registers.
 *   Also this area of memory is indexed by h/w engine id.
 * - Allocate kernel memory area for max number of engines supported on the
 *   chip to map s/w defined engine ids starting from 0 to h/w (active) engine
 *   id read from device info h/w register. This allocated area of memory is
 *   set to 0xff and then filled with engine id read from device info h/w
 *   registers. This area of memory is indexed by s/w defined engine id starting
 *   from 0.
 * - Initialize engine info related s/w context by calling hal that will read
 *   device info h/w registers and also initialize s/w variable
 *   #nvgpu_fifo.num_engines that is used to count total number of valid h/w
 *   engine ids read from device info h/w registers.
 *
 * @return 0 upon success.
 * @retval Valid error codes upon failure to allocate memory or
 *         failure to get engine info from device info h/w registers.
 */
int nvgpu_engine_setup_sw(struct gk20a *g);
/**
 * @brief Clean up s/w context for engine related info.
 *
 * @param g [in]		The GPU driver struct.
 *
 * - Free up kernel memory area that is used for storing engine info read
 *   from device info h/w registers.
 * - Free up kernel memory area to map s/w defined engine ids (starting with 0)
 *   to active (h/w) engine ids read from device info h/w register.
 */
void nvgpu_engine_cleanup_sw(struct gk20a *g);

#ifdef CONFIG_NVGPU_FIFO_ENGINE_ACTIVITY
int nvgpu_engine_enable_activity(struct gk20a *g,
			struct nvgpu_engine_info *eng_info);
int nvgpu_engine_enable_activity_all(struct gk20a *g);
int nvgpu_engine_disable_activity(struct gk20a *g,
			struct nvgpu_engine_info *eng_info,
			bool wait_for_idle);
int nvgpu_engine_disable_activity_all(struct gk20a *g,
				bool wait_for_idle);

int nvgpu_engine_wait_for_idle(struct gk20a *g);
#endif
#ifdef CONFIG_NVGPU_ENGINE_RESET
/**
 * Called from recovery. This will not be part of the safety build after
 * recovery is not supported in the safety build.
 */
void nvgpu_engine_reset(struct gk20a *g, u32 engine_id);
#endif
/**
 * @brief Get runlist id for the last available #NVGPU_ENGINE_ASYNC_CE
 *         engine enum type.
 *
 * @param g [in]		The GPU driver struct.
 *
 * - Call #nvgpu_engine_get_gr_runlist_id to get runlist id for the first
 *   available #NVGPU_ENGINE_GR engine type.
 * - Get #nvgpu_engine_info.runlist_id for last available #NVGPU_ENGINE_ASYNC_CE
 *   engine enum type.
 *   - Get pointer to #nvgpu_engine_info for each of #nvgpu_fifo.num_engines.
 *   - Get #nvgpu_engine_info.runlist_id for the #nvgpu_engine_info.engine_enum
 *     type matching with #NVGPU_ENGINE_ASYNC_CE.
 *
 * @return #nvgpu_engine_info.runlist_id for the last available
 *         #NVGPU_ENGINE_ASYNC_CE engine enum type.
 * @retval Return value of #nvgpu_engine_get_gr_runlist_id if #g is NULL.
 * @retval Return value of #nvgpu_engine_get_gr_runlist_id if
 *         #NVGPU_ENGINE_ASYNC_CE engine enum type is not available.
 * @retval Return value of #nvgpu_engine_get_gr_runlist_id if
 *         #nvgpu_fifo.num_engines is 0.
 */
u32 nvgpu_engine_get_fast_ce_runlist_id(struct gk20a *g);
/**
 * @brief Get runlist id for the first available #NVGPU_ENGINE_GR engine enum
 *        type.
 *
 * @param g [in]		The GPU driver struct.
 *
 * - Get h/w engine id for the first available #NVGPU_ENGINE_GR engine enum
 *   type.
 * -- Get #nvgpu_engine_info for the first available gr engine id.
 * -- Get #nvgpu_engine_info.runlist_id for first available gr engine id.
 *
 * @return #nvgpu_engine_info.runlist_id for the first available gr engine id.
 * @retval U32_MAX if #NVGPU_ENGINE_GR engine enum type is not available.
 * @retval U32_MAX if pointer to #nvgpu_engine_info for the first available
 *         gr h/w engine id is NULL.
 */
u32 nvgpu_engine_get_gr_runlist_id(struct gk20a *g);
/**
 * @brief Check if runlist id corresponds to runlist id of one of the
 *        engine ids supported by h/w.
 *
 * @param g [in]		The GPU driver struct.
 * @param runlist_id [in]	Runlist id.
 *
 * Check if #runlist_id corresponds to runlist id of one of the engine
 * ids supported by h/w by checking #nvgpu_engine_info for each of
 * #nvgpu_fifo.num_engines engines.
 *
 * @return True if #runlist_id is valid.
 * @return False if #nvgpu_engine_info is NULL for all the engine ids starting
 *         with 0 upto #nvgpu_fifo.num_engines or #runlist_id did not match with
 *         any of the runlist ids of engine ids supported by h/w.
 */
bool nvgpu_engine_is_valid_runlist_id(struct gk20a *g, u32 runlist_id);
/**
 * @brief Get mmu fault id for the engine id.
 *
 * @param g [in]		The GPU driver struct.
 * @param engine_id [in]	Engine id.
 *
 * Get pointer to #nvgpu_engine_info for the #engine_id. Use this pointer to
 * get mmu fault id for the #engine_id.
 *
 * @return Mmu fault id for #engine_id.
 * @retval Invalid mmu fault id, #NVGPU_INVALID_ENG_ID.
 */
u32 nvgpu_engine_id_to_mmu_fault_id(struct gk20a *g, u32 engine_id);
/**
 * @brief Get engine id from mmu fault id.
 *
 * @param g [in]		The GPU driver struct.
 * @param fault_id [in]		Mmu fault id.
 *
 * Check if #fault_id corresponds to fault id of one of the active engine
 * ids supported by h/w by checking #nvgpu_engine_info for each of
 * #nvgpu_fifo.num_engines engines.
 *
 * @return Valid engine id corresponding to #fault_id.
 * @retval Invalid engine id, #NVGPU_INVALID_ENG_ID if pointer to
 *         #nvgpu_engine_info is NULL for the  engine ids starting with
 *         0 upto #nvgpu_fifo.num_engines or
 *         #fault_id did not match with any of the fault ids of h/w engine ids.
 */
u32 nvgpu_engine_mmu_fault_id_to_engine_id(struct gk20a *g, u32 fault_id);
/**
 * Called from recovery. This will not be part of the safety build after
 * recovery is not supported in the safety build.
 */
u32 nvgpu_engine_get_mask_on_id(struct gk20a *g, u32 id, bool is_tsg);
/**
 * @brief Read device info h/w registers to get engine info.
 *
 * @param f [in]	Pointer to #nvgpu_fifo struct.
 *
 * - Get device info related info for h/w engine enum type,
 *   #NVGPU_DEVTYPE_GRAPHICS.
 * - Get PBDMA id serving the runlist id of the h/w engine enum type,
 *   #NVGPU_DEVTYPE_GRAPHICS.
 * - Get s/w defined engine enum type for the h/w engine enum type read
 *   from device info registers.
 * - Initialize #nvgpu_fifo.engine_info and #nvgpu_fifo.active_engines_list
 *   with the data from local device info struct and also initialize s/w
 *   variable.
 *   #nvgpu_fifo.num_engines that is used to count total number of valid h/w
 *   engine ids read from device info h/w registers.
 * - Call function to initialize CE engine info.
 *
 * @return 0 upon success.
 * @retval -EINVAL if call to function to get device info related info for
 *         h/w engine enum type, #NVGPU_DEVTYPE_GRAPHICS returned failure.
 * @retval -EINVAL if call to function to get pbdma id for runlist id of
 *         h/w engine enum type, #NVGPU_DEVTYPE_GRAPHICS returned failure.
 * @retval Return value of function called to initialize CE engine info.
 */
int nvgpu_engine_init_info(struct nvgpu_fifo *f);

/**
 * Called from recovery handling for architectures before volta. This will
 * not be part of safety build after recovery is not supported in the safety
 * build.
 */
void nvgpu_engine_get_id_and_type(struct gk20a *g, u32 engine_id,
					  u32 *id, u32 *type);
/**
 * Called from ctxsw timeout intr handling. This function will not be part
 * of safety build after recovery is not supported in the safety build.
 */
u32 nvgpu_engine_find_busy_doing_ctxsw(struct gk20a *g,
		u32 *id_ptr, bool *is_tsg_ptr);
/**
 * Called from runlist update timeout handling. This function will not be part
 * of safety build after recovery is not supported in safety build.
 */
u32 nvgpu_engine_get_runlist_busy_engines(struct gk20a *g, u32 runlist_id);

#ifdef CONFIG_NVGPU_DEBUGGER
bool nvgpu_engine_should_defer_reset(struct gk20a *g, u32 engine_id,
			u32 engine_subid, bool fake_fault);
#endif
/**
 * @brief Get veid from mmu fault id.
 *
 * @param g [in]		The GPU driver struct.
 * @param mmu_fault_id [in]	Mmu fault id.
 * @param gr_eng_fault_id [in]	GR engine's mmu fault id.
 *
 * Get valid veid by subtracting #gr_eng_fault_id from #mmu_fault_id,
 * if #mmu_fault_id is greater than or equal to #gr_eng_fault_id and less
 * than #gr_eng_fault_id + #nvgpu_fifo.max_subctx_count.
 *
 * @return Veid.
 * @retval Invalid veid, #INVAL_ID.
 */
u32 nvgpu_engine_mmu_fault_id_to_veid(struct gk20a *g, u32 mmu_fault_id,
			u32 gr_eng_fault_id);
/**
 * @brief Get h/w engine id and veid from mmu fault id.
 *
 * @param g [in]		The GPU driver struct.
 * @param mmu_fault_id [in]	Mmu fault id.
 * @param veid [in,out]		Pointer to store veid.
 *
 * Get valid h/w engine id for given #mmu_fault_id. Also get veid if engine
 * enum for h/w engine id is of type #NVGPU_ENGINE_GR.
 *
 * @return Valid h/w (active) engine id. Updated #veid with valid veid
 *         if engine enum type for h/w engine id is of type #NVGPU_ENGINE_GR.
 * @retval #INVAL_ID.
 */
u32 nvgpu_engine_mmu_fault_id_to_eng_id_and_veid(struct gk20a *g,
			 u32 mmu_fault_id, u32 *veid);
/**
 * @brief Get engine id, veid and pbdma id from mmu fault id.
 *
 * @param g [in]		The GPU driver struct.
 * @param mmu_fault_id [in]	Mmu fault id.
 * @param engine_id [in,out]	Pointer to store active engine id.
 * @param veid [in,out]		Pointer to store veid.
 * @param pbdma_id [in,out]	Pointer to store pbdma id.
 *
 * Calls function to get h/w engine id and veid for given #mmu_fault_id.
 * If h/w (active) engine id is not #INVAL_ID, call function to get pbdma id for
 * the engine having fault id as #mmu_fault_id.
 *
 * @return Updated #engine_id, #veid and #pbdma_id pointers
 */
void nvgpu_engine_mmu_fault_id_to_eng_ve_pbdma_id(struct gk20a *g,
	u32 mmu_fault_id, u32 *engine_id, u32 *veid, u32 *pbdma_id);
#endif /*NVGPU_ENGINE_H*/
