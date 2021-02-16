/*
 * Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
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

#ifndef CORE_LOCAL_INCLUDED
#define CORE_LOCAL_INCLUDED
#include <osi_core.h>

/**
 * @brief osi_read_reg - Read a MAC register.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] addr: MAC register
 *
 * @note
 * Traceability Details: TODO
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: Yes
 *
 * @retval data from MAC register on success
 * @retval -1 on failure
 */
static inline nveu32_t osi_read_reg(struct osi_core_priv_data *const osi_core,
		const nve32_t addr)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->read_reg != OSI_NULL) &&
	    (osi_core->base != OSI_NULL)) {
		return osi_core->ops->read_reg(osi_core, addr);
	}

	return 0;
}

/**
 * @brief osi_write_reg - Write a MAC register.
 *
 * @param[in] osi_core: OSI core private data structure.
 * @param[in] val: MAC register value
 * @param[in] addr: MAC register
 *
 * @note
 * Traceability Details: TODO
 *
 * @note
 * Classification:
 * - Interrupt: No
 * - Signal handler: No
 * - Thread safe: No
 * - Required Privileges: None
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @retval data from MAC register on success
 * @retval -1 on failure
 */
static inline nveu32_t osi_write_reg(struct osi_core_priv_data *const osi_core,
		const nveu32_t val, const nve32_t addr)
{
	if ((osi_core != OSI_NULL) && (osi_core->ops != OSI_NULL) &&
	    (osi_core->ops->write_reg != OSI_NULL) &&
	    (osi_core->base != OSI_NULL)) {
		return osi_core->ops->write_reg(osi_core, val, addr);
	}

	return 0;
}
#endif /* CORE_LOCAL_INCLUDED */
