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

#ifndef LOCAL_COMMON_H
#define LOCAL_COMMON_H

#include <osi_core.h>

/**
 * @brief common_get_systime_from_mac - Get system time
 *
 * Algorithm: Gets the current system time
 *
 * @param[in] addr: Address of base register.
 * @param[in] mac: MAC HW type.
 * @param[out] sec: Value read in Seconds.
 * @param[out] nsec: Value read in Nano seconds.
 *
 * @note MAC should be init and started. see osi_start_mac()
 *
 * @retval 0 on success
 * @retval -1 on failure.
 */
void common_get_systime_from_mac(void *addr, unsigned int mac,
				 unsigned int *sec, unsigned int *nsec);

#endif /* LOCAL_COMMON_H */
