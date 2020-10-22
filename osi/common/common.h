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
#ifndef INCLUDED_COMMON_H
#define INCLUDED_COMMON_H

#include <osi_common.h>

struct osi_core_priv_data;

/**
 * @brief osi_lock_init - Initialize lock to unlocked state.
 *
 * @note
 * Algorithm:
 *  - Set lock to unlocked state.
 *
 * @param[in] lock - Pointer to lock to be initialized
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 */
static inline void osi_lock_init(nveu32_t *lock)
{
	*lock = OSI_UNLOCKED;
}

/**
 * @brief osi_lock_irq_enabled - Spin lock. Busy loop till lock is acquired.
 *
 * @note
 * Algorithm:
 *  - Atomic compare and swap operation till lock is held.
 *
 * @param[in] lock - Pointer to lock to be acquired.
 *
 * @note
 *  - Does not disable irq. Do not call this API to acquire any
 *    lock that is shared between top/bottom half. It will result in deadlock.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 */
static inline void osi_lock_irq_enabled(nveu32_t *lock)
{
	/* __sync_val_compare_and_swap(lock, old value, new value) returns the
	 * old value if successful.
	 */
	while (__sync_val_compare_and_swap(lock, OSI_UNLOCKED, OSI_LOCKED) !=
	      OSI_UNLOCKED) {
		/* Spinning.
		 * Will deadlock if any ISR tried to lock again.
		 */
	}
}

/**
 * @brief osi_unlock_irq_enabled - Release lock.
 *
 * @note
 * Algorithm:
 *  - Atomic compare and swap operation to release lock.
 *
 * @param[in] lock - Pointer to lock to be released.
 *
 * @note
 *  - Does not disable irq. Do not call this API to release any
 *    lock that is shared between top/bottom half.
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 */
static inline void osi_unlock_irq_enabled(nveu32_t *lock)
{
	if (__sync_val_compare_and_swap(lock, OSI_LOCKED, OSI_UNLOCKED) !=
	    OSI_LOCKED) {
		/* Do nothing. Already unlocked */
	}
}

/**
 * @brief osi_readl - Read a memory mapped register.
 *
 * @param[in] addr: Memory mapped address.
 *
 * @pre Physical address has to be memory mapped.
 *
 * @return Data from memory mapped register - success.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: Yes
 */
static inline nveu32_t osi_readl(void *addr)
{
	return *(volatile nveu32_t *)addr;
}

/**
 * @brief osi_writel - Write to a memory mapped register.
 *
 * @param[in] val:  Value to be written.
 * @param[in] addr: Memory mapped address.
 *
 * @pre Physical address has to be memory mapped.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: Yes
 * - De-initialization: Yes
 */
static inline void osi_writel(nveu32_t val, void *addr)
{
	*(volatile nveu32_t *)addr = val;
}

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
nveu32_t osi_read_reg(struct osi_core_priv_data *const osi_core,
		      const nve32_t addr);

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
nveu32_t osi_write_reg(struct osi_core_priv_data *const osi_core,
		       const nveu32_t val, const nve32_t addr);

#ifdef ETHERNET_SERVER
nveu32_t osi_readla(void *priv, void *addr);

void osi_writela(void *priv, nveu32_t val, void *addr);
#else
/**
 * @brief osi_readla - Read a memory mapped register.
 *
 * @ note
 * The difference between osi_readla & osi_readl is osi_core argument.
 * In case of ethernet server, osi_core used to define policy for each VM.
 * In case of non virtualization osi_core argument is ignored.
 *
 * @param[in] priv: Priv address.
 * @param[in] addr: Memory mapped address.
 *
 * @note Physical address has to be memmory mapped.
 *
 * @return Data from memory mapped register - success.
 */
static inline nveu32_t osi_readla(void *priv, void *addr)
{
	return *(volatile nveu32_t *)addr;
}

/**
 *
 * @ note
 * @brief osi_writela - Write to a memory mapped register.
 * The difference between osi_writela & osi_writel is osi_core argument.
 * In case of ethernet server, osi_core used to define policy for each VM.
 * In case of non virtualization osi_core argument is ignored.
 *
 * @param[in] priv: Priv address.
 * @param[in] val:  Value to be written.
 * @param[in] addr: Memory mapped address.
 *
 * @note Physical address has to be memmory mapped.
 */
static inline void osi_writela(void *priv, nveu32_t val, void *addr)
{
	*(volatile nveu32_t *)addr = val;
}
#endif

/**
 * @brief is_valid_mac_version - Check if read MAC IP is valid or not.
 *
 * @param[in] mac_ver: MAC version read.
 *
 * @note MAC has to be out of reset.
 *
 * @note
 * API Group:
 * - Initialization: Yes
 * - Run time: No
 * - De-initialization: No
 *
 * @retval 0 - for not Valid MAC
 * @retval 1 - for Valid MAC
 */
static inline nve32_t is_valid_mac_version(nveu32_t mac_ver)
{
	if ((mac_ver == OSI_EQOS_MAC_4_10) ||
	    (mac_ver == OSI_EQOS_MAC_5_00) ||
	    (mac_ver == OSI_EQOS_MAC_5_30)) {
		return 1;
	}

	return 0;
}

/**
 * @brief osi_update_stats_counter - update value by increment passed
 *	as parameter
 *
 * @note
 * Algorithm:
 *  - Check for boundary and return sum
 *
 * @param[in] last_value: last value of stat counter
 * @param[in] incr: increment value
 *
 * @note Input parameter should be only nveu64_t type
 *
 * @note
 * API Group:
 * - Initialization: No
 * - Run time: Yes
 * - De-initialization: No
 *
 * @return nveu64_t value
 */
static inline nveu64_t osi_update_stats_counter(nveu64_t last_value,
						nveu64_t incr)
{
	nveu64_t temp = last_value + incr;

	if (temp < last_value) {
		/* Stats overflow, so reset it to zero */
		return 0UL;
	}

	return temp;
}
#endif
