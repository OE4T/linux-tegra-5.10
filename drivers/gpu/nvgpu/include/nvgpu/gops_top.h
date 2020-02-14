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
#ifndef NVGPU_GOPS_TOP_H
#define NVGPU_GOPS_TOP_H

#include <nvgpu/types.h>

/**
 * @file
 *
 * TOP unit HAL interface
 *
 */
struct gk20a;
struct nvgpu_device_info;

/**
 * TOP unit HAL operations
 *
 * @see gpu_ops
 */
struct gops_top {
	/**
	 * @brief Get the number of entries of particular engine type in
	 *        device_info table.
	 *
	 * @param g [in]		GPU driver struct pointer.
	 * @param engine_type [in]	Engine enumeration value.
	 *
	 * 1. Some engines have multiple entries in device_info table
	 *    corresponding to each instance of the engine. All such entries
         *    corresponding to same engine will have same \a engine_type, but
	 *    a unique instance id.
	 * 2. Traverse through the device_info table and get the total
	 *    number of entries corresponding to input \a engine_type.
	 * 3. This HAL is valid for Pascal and chips beyond.
	 * 4. Prior to Pascal, each instance of the engine was denoted by a
	 *    different engine_type.
	 *
	 * List of valid engine enumeration values:
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 *  NVGPU_ENGINE_GRAPHICS    -        0
	 *  NVGPU_ENGINE_COPY0       -        1
	 *  NVGPU_ENGINE_COPY1       -        2
	 *  NVGPU_ENGINE_COPY2       -        3
	 *  NVGPU_ENGINE_IOCTRL      -        18
	 *  NVGPU_ENGINE_LCE         -        19
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 *
	 * @return	Number of instances of \a engine_type in device_info
	 * 		table
	 */
	u32 (*get_num_engine_type_entries)(struct gk20a *g, u32 engine_type);

	/**
	 * @brief Get all the engine related information from device_info table
	 *
	 * @param g [in]		GPU device struct pointer
	 * @param dev_info [out]	Pointer to device information struct
	 *				which gets populated with all the
	 *				engine related information.
	 * @param engine_type [in]	Engine enumeration value
	 * @param inst_id [in]		Engine's instance identification number
	 *
	 * 1. Device_info table is an array of registers which contains engine
	 *    specific data like interrupt enum, reset enum, pri_base etc.
	 * 2. This HAL reads such engine information from table after matching
	 *    the \a engine_type and \a inst_id and then populates the read
	 *    information in \a dev_info struct.
	 * 3. In the device_info table, more than one register is required to
	 *    denote information for a specific engine. So they use multiple
	 *    consecutive registers in the array to represent a specific engine.
	 * 4. The MSB (called chain bit) in each register denotes if the next
	 *    register talks the same engine as present the one. All the
	 *    registers in the device info table can be classified in one of 4
	 *    types:
	 *      - Not_valid: Ignore these registers
	 *      - Data: This type of register contains pri_base, fault_id etc
	 *      - Enum: This type of register contains intr_enum, reset_enum
	 *      - Engine_type: This type of register contains the engine name
	 *               which is being described.
	 * 5. So, in the parsing code,
	 * 	- Loop through the array
	 * 	- Ignore the invalid entries
	 * 	- Store the “linked” register values in temporary variables
	 *	   until chain_bit is set. This helps us get all the data for
	 *	   particular engine type. [This is needed because the engine
	 *         name may not be part of the first register representing the
	 *	   engine. So, reading the first register is not sufficient to
	 *	   determine if the group represents the \a engine_type.
	 *         Chain_bit being disabled implies the next register read
         *         would represent a new engine.
	 * 	- Parse the stored variables to get engine_name,
	 *         intr/reset_enums, pri base etc. Check if the engine
	 *    	   type read from the table equals \a engine_type.
	 *
	 * List of valid engine enumeration values:
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 *  NVGPU_ENGINE_GRAPHICS    -        0
	 *  NVGPU_ENGINE_COPY0       -        1
	 *  NVGPU_ENGINE_COPY1       -        2
	 *  NVGPU_ENGINE_COPY2       -        3
	 *  NVGPU_ENGINE_IOCTRL      -        18
	 *  NVGPU_ENGINE_LCE         -        19
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 *
	 * @return 0 in case of success and < 0 in case of failure
	 */
	int (*get_device_info)(struct gk20a *g,
				struct nvgpu_device_info *dev_info,
				u32 engine_type, u32 inst_id);

	/**
	 * @brief Checks if \a engine_type corresponds to graphics engine
	 *
	 * @param g [in]		GPU device struct pointer
	 * @param engine_type [in]	Engine enumeration value
	 *
	 * 1. This HAL checks if the input \a engine_type is the enumeration
	 *    value corresponding to graphics engine.
	 * 2. The enumeration value for graphics engine for device_info table
	 *    is 0.
	 *
	 * @return true if \a engine_type is equal to 0, false otherwise
	 */
	bool (*is_engine_gr)(struct gk20a *g, u32 engine_type);

	/**
	 * @brief Checks if \a engine_type corresponds to copy engine
	 *
	 * @param g [in]		GPU device struct pointer
	 * @param engine_type [in]	Engine enumeration value
	 *
	 * 1. This HAL checks if the input \a engine_type is the enumeration value
	 *    corresponding to copy engine.
	 * 2. Prior to Pascal, each instance of copy engine was denoted by
	 *    different engine_type as below:
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 *  COPY_ENGINE_INSTANCE0 enum value -  1
	 *  COPY_ENGINE_INSTANCE1 enum value -  2
	 *  COPY_ENGINE_INSTANCE2 enum value -  3
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * 3. For Pascal and chips beyond, all instances of copy engine have same
	 *    engine_type as below:
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 *  COPY_ENGINE enum value -  19
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 *
	 * @return true if \a engine_type is equal to enum value specified above
	 * 	or false otherwise
	 */
	bool (*is_engine_ce)(struct gk20a *g, u32 engine_type);

	/**
	 * @brief Get the instance ID for particular copy engine
	 *
	 * @param g [in]		GPU device struct pointer
	 * @param engine_type [in]	Engine enumeration value
	 *
	 * 1. This HAL is valid for chips prior to Pascal when each instance of
	 *    copy engine had unique engine_type. The three instances of copy
	 *    engine are allocated engine_type in ascending starting from 1.
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 *  COPY_ENGINE_INSTANCE0 engine_type -  1
	 *  COPY_ENGINE_INSTANCE1 engine_type -  2
	 *  COPY_ENGINE_INSTANCE2 engine_type -  3
	 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	 * 2. Calculate the instance id by subtracting COPY_ENGINE_INSTANCE0
	 *    enum value from \a engine_type.
	 *
	 * @return Calculated instance ID as explained above.
	 */
	u32 (*get_ce_inst_id)(struct gk20a *g, u32 engine_type);

	/**
	 * @brief Gets maximum number of GPCs in a GPU as programmed in HW
	 *
	 * @param g [in]		GPU device struct pointer
	 *
	 * This HAL reads the NV_PTOP_SCAL_NUM_GPCS HW register, extracts the
	 * NV_PTOP_SCAL_NUM_GPCS_VALUE field and returns it.
	 *
	 * @return The number of GPCs as read from above mentioned HW register.
	 */
	u32 (*get_max_gpc_count)(struct gk20a *g);

	/**
	 * @brief Gets the maximum number of TPCs per GPC in a GPU as programmed
	 * 	in HW.
	 *
	 * @param g [in]		GPU device struct pointer
	 *
	 * This HAL reads the NV_PTOP_SCAL_TPC_PER_GPC HW register, extracts the
	 * NV_PTOP_SCAL_NUM_TPC_PER_GPC_VALUE field and returns it.
	 *
	 * @return The number of TPC per GPC as read from the above mentioned
	 *	HW register.
	 */
	u32 (*get_max_tpc_per_gpc_count)(struct gk20a *g);

	/**
	 * @brief Gets the maximum number of FBPs in a GPU as programmed in HW
	 *
	 * @param g [in]		GPU device struct pointer
	 *
	 * This HAL reads the NV_PTOP_SCAL_NUM_FBPS HW register, extracts the
	 * NV_PTOP_SCAL_NUM_FBPS_VALUE field and returns it.
	 *
	 * @return The number of FBPs as read from above mentioned HW register
	 */
	u32 (*get_max_fbps_count)(struct gk20a *g);

	/**
	 * @brief Gets the maximum number of LTCs per FBP in a GPU as programmed
	 * 	in HW.
	 *
	 * @param g [in]		GPU device struct pointer
	 *
	 * This HAL reads the NV_PTOP_SCAL_LTC_PER_FBP HW register, extracts the
	 * NV_PTOP_SCAL_NUM_LTC_PER_FBP_VALUE field and returns it.
	 *
	 * @return The number of LTC per FBP as read from the above mentioned
	 *	HW register.
	 */
	u32 (*get_max_ltc_per_fbp)(struct gk20a *g);

	/**
	 * @brief Gets the number of LTCs in a GPU as programmed in HW
	 *
	 * @param g [in]		GPU device struct pointer
	 *
	 * This HAL reads the NV_PTOP_SCAL_NUM_LTCS HW register, extracts the
	 * NV_PTOP_SCAL_NUM_LTCS_VALUE field and returns it.
	 *
	 * @return The number of LTCs as read from above mentioned HW register
	 */
	u32 (*get_num_ltcs)(struct gk20a *g);

	/**
	 * @brief Gets the number of copy engines as programmed in HW
	 *
	 * @param g [in]		GPU device struct pointer
	 *
	 * This HAL reads the NV_PTOP_SCAL_NUM_CES HW register, extracts the
	 * NV_PTOP_SCAL_NUM_CES_VALUE field and returns it.
	 *
	 * @return The number of copy engines as read from above mentioned HW
	 * 	register
	 */
	u32 (*get_num_lce)(struct gk20a *g);

	/**
	 * @brief Gets the maximum number of LTS per LTC in a GPU as programmed
	 * 	in HW.
	 *
	 * @param g [in]		GPU device struct pointer
	 *
	 * This HAL reads the NV_PTOP_SCAL_NUM_SLICES_PER_LTC HW register,
	 * extracts the NV_PTOP_SCAL_NUM_SLICES_PER_LTC_VALUE field and
	 * returns it.
	 *
	 * @return The number of LTS per LTC as read from the above mentioned
	 *	HW register.
	 */
	u32 (*get_max_lts_per_ltc)(struct gk20a *g);

	/** @cond DOXYGEN_SHOULD_SKIP_THIS */

	/**
	 * NON-FUSA HALs
	 */
	u32 (*get_nvhsclk_ctrl_e_clk_nvl)(struct gk20a *g);
	void (*set_nvhsclk_ctrl_e_clk_nvl)(struct gk20a *g, u32 val);
	u32 (*get_nvhsclk_ctrl_swap_clk_nvl)(struct gk20a *g);
	void (*set_nvhsclk_ctrl_swap_clk_nvl)(struct gk20a *g, u32 val);
	u32 (*get_max_fbpas_count)(struct gk20a *g);
	u32 (*read_top_scratch1_reg)(struct gk20a *g);
	u32 (*top_scratch1_devinit_completed)(struct gk20a *g,
					u32 value);
	/**
	 * HALs used within "Top" unit. Private HALs.
	 */
	void (*device_info_parse_enum)(struct gk20a *g,
					u32 table_entry,
					u32 *engine_id, u32 *runlist_id,
					u32 *intr_id, u32 *reset_id);
	int (*device_info_parse_data)(struct gk20a *g,
					u32 table_entry, u32 *inst_id,
					u32 *pri_base, u32 *fault_id);

#if defined(CONFIG_NVGPU_HAL_NON_FUSA) && defined(CONFIG_NVGPU_NEXT)
#include "include/nvgpu/nvgpu_next_gops_top.h"
#endif
	/** @endcond DOXYGEN_SHOULD_SKIP_THIS */

};

#endif /* NVGPU_GOPS_TOP_H */
