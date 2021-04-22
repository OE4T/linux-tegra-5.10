/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_CIC_H
#define NVGPU_CIC_H

#include <nvgpu/log.h>

struct nvgpu_err_desc;
/**
 * @file
 *
 * Public structs and APIs exposed by Central Interrupt Controller
 * (CIC) unit.
 */

/*
 * Requires a string literal for the format - notice the string
 * concatination.
 */
#define cic_dbg(g, fmt, args...)					\
	nvgpu_log((g), gpu_dbg_cic, "CIC | " fmt, ##args)

/**
 * @brief Initialize the CIC unit's data structures
 *
 * @param g [in]	- The GPU driver struct.
 *
 * - Check if CIC unit is already initialized by checking its
 *   reference in struct gk20a.
 * - If not initialized, allocate memory for CIC's private data
 *   structure.
 * - Initialize the members of this private structure.
 * - Store a reference pointer to the CIC struct in struct gk20a.
 *
 * @return 0 if Initialization had already happened or was
 *           successful in this call.
 *	  < 0 if any steps in initialization fail.
 *
 * @retval -ENOMEM if sufficient memory is not available for CIC
 *            struct.
 *
 */
int nvgpu_cic_init_common(struct gk20a *g);

/**
 * @brief De-initialize the CIC unit's data structures
 *
 * @param g [in]	- The GPU driver struct.
 *
 * - Check if CIC unit is already deinitialized by checking its
 *   reference in struct gk20a.
 * - If not deinitialized, set the LUT pointer to NULL and set the
 *   num_hw_modules to 0.
 * - Free the memory allocated for CIC's private data structure.
 * - Invalidate reference pointer to the CIC struct in struct gk20a.
 *
 * @return 0 if Deinitialization had already happened or was
 *           successful in this call.
 *
 * @retval None.
 */
int nvgpu_cic_deinit_common(struct gk20a *g);

/**
 * @brief Check if the input HW unit ID is valid CIC HW unit.
 *
 * @param g [in]	      - The GPU driver struct.
 * @param hw_unit_id [in]     - HW unit ID to be verified
 *
 * - Check if the CIC unit is initialized so that the LUT is
 *   available to verify the hw_unit_id.
 * - LUT is an array of nvgpu_err_hw_module struct which contains the
 *   hw_unit_id for a specific unit.
 * - The hw_unit_id starts from 0 and ends at
 *   (g->cic->num_hw_modules -1) and hence effectively can serve as
 *   index into the LUT array.
 *
 * @return 0 if input hw_unit_id is valid,
 *       < 0 if input hw_unit_id is invalid
 * @retval -EINVAL if CIC is not initialized and
 *                 if input hw_unit_id is invalid.
 */
int nvgpu_cic_check_hw_unit_id(struct gk20a *g, u32 hw_unit_id);

/**
 * @brief Check if the input error ID is valid in CIC domain.
 *
 * @param g [in]	      - The GPU driver struct.
 * @param hw_unit_id [in]     - HW unit ID corresponding to err_id
 * @param err_id [in]         - Error ID to be verified
 *
 * - Check if the CIC unit is initialized so that the LUT is
 *   available to verify the hw_unit_id.
 * - LUT is an array of nvgpu_err_hw_module struct which contains the
 *   hw_unit_id for a specific unit and also the number of errors
 *   reported by the unit.
 * - The hw_unit_id starts from 0 and ends at
 *   (g->cic->num_hw_modules -1) and hence effectively can serve as
 *   index into the LUT array.
 * - Before using the input hw_unit_id to index into LUT, verify that
 *   the hw_unit_id is valid.
 * - Index using hw_unit_id and derive the num_errs from LUT for the
 *   given HW unit
 * - Check if the input err_id lies between 0 and (num_errs-1).
 *
 * @return 0 if input err_id is valid, < 0 if input err_id is invalid
 * @retval -EINVAL if CIC is not initialized and
 *                 if input hw_unit_id or err_id is invalid.
 */
int nvgpu_cic_check_err_id(struct gk20a *g, u32 hw_unit_id,
						u32 err_id);

/**
 * @brief Get the LUT data for input HW unit ID and error ID
 *
 * @param g [in]	      - The GPU driver struct.
 * @param hw_unit_id [in]     - HW unit ID corresponding to err_id
 * @param err_id [in]         - Error ID whose LUT data is required.
 * @param err_desc [out]      - Pointer to store LUT data into.
 *
 * - LUT is an array of nvgpu_err_hw_module struct which contains the
 *   all the static data for each HW unit reporting error to CIC.
 * - nvgpu_err_hw_module struct is inturn an array of struct
 *   nvgpu_err_desc which stores static data per error ID.
 * - Use the nvgpu_cic_check_err_id() API to
 *     - Check if the CIC unit is initialized so that the LUT is
 *       available to read the static data for input err_id.
 *     - Check if input HW unit ID and error ID are valid.
 * - The hw_unit_id starts from 0 and ends at
 *   (g->cic->num_hw_modules -1) and hence effectively can serve as
 *   index into the LUT array.
 * - The err_id starts from 0 and ends at
 *   [lut[hw_unit_id].num_err) - 1], and hence effectively can serve
 *   as index into array of errs[].
 * - Index using hw_unit_id and err_id and store the LUT data into
 *
 * @return 0 if err_desc was successfully filled with LUT data,
 *       < 0 otherwise.
 * @retval -EINVAL if CIC is not initialized and
 *                 if input hw_unit_id or err_id is invalid.
 */
int nvgpu_cic_get_err_desc(struct gk20a *g, u32 hw_unit_id,
		u32 err_id, struct nvgpu_err_desc **err_desc);

/**
 * @brief GPU HW errors are reported to Safety_Services via SDL unit.
 *        This function provides an interface between error reporting functions
 *        used by sub-units in nvgpu-rm and SDL unit.
 *
 * @param g [in]		- The GPU driver struct.
 * @param err_info [in]		- Error message.
 * @param err_size [in]		- Size of the error message.
 * @param is_critical [in]	- Criticality of the error being reported.
 *
 * On QNX:
 *  - Checks whether SDL is initialized.
 *  - Enqueues \a err_info into error message queue.
 *  - Signals the workqueue condition variable.
 *  - If the reported error is critical, invokes #nvgpu_sw_quiesce() api.
 *
 * on Linux:
 *  - NOP currently as safety services are absent in Linux
 *
 * @return 0 in case of success, <0 in case of failure.
 * @retval -EAGAIN if SDL not initialized.
 * @retval -ENOMEM if sufficient memory is not available.
 */
int nvgpu_cic_report_err_safety_services(struct gk20a *g,
		void *err_info, size_t err_size, bool is_critical);

/**
 * @brief Get the number of HW modules supported by CIC.
 *
 * @param g [in]	      - The GPU driver struct.
 *
 * - Check if the CIC unit is initialized so that num_hw_modules is
 *   initialized.
 * - Return the num_hw_modules variable stored in CIC's private
 *   struct.
 *
 * @return 0 or >0 value of num_hw_modules if successful;
 *       < 0 otherwise.
 * @retval -EINVAL if CIC is not initialized.
 */
int nvgpu_cic_get_num_hw_modules(struct gk20a *g);
#endif /* NVGPU_CIC_H */
