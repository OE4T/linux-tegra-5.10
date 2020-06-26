/*
 * Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
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

#ifndef EQOS_COMMON_H
#define EQOS_COMMON_H

#include <local_common.h>

/**
 * @brief PTP Time read registers
 * @{
 */
#define EQOS_MAC_STSR			0x0B08
#define EQOS_MAC_STNSR			0x0B0C
#define EQOS_MAC_STNSR_TSSS_MASK	0x7FFFFFFFU
/** @} */

/**
 * @brief eqos_get_systime - Get system time from MAC
 *
 * Algorithm: Get current system time
 *
 * @param[in] addr: Base address indicating the start of
 *		    memory mapped IO region of the MAC.
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
unsigned long long eqos_get_systime_from_mac(void *addr);

#endif /* EQOS_COMMON_H */
