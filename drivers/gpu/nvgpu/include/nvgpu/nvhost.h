/*
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_NVHOST_H
#define NVGPU_NVHOST_H

#ifdef CONFIG_TEGRA_GK20A_NVHOST

#include <nvgpu/types.h>

struct nvgpu_nvhost_dev;
struct gk20a;
struct sync_pt;
struct sync_fence;
struct timespec;

/**
 * @file Functions that initialize the sync points
 *  and describe other functionalities.
 */

/**
 * @brief Initialzes the nvhost device for nvgpu.
 *
 * @param g [in]	The GPU super structure.
 *
 * - Allocate memory for g.nvhost_dev.
 * - Get the type of the hardware(t186/t194) by calling
 *   #NvTegraSysGetChipId().
 * - Initialize the number of synpoints according to the
 *   associated hardware.
 * - Allocate and initialize different fields associated with
 *   nvhost device.
 *
 * @return		0, if success.
 *			-ENOMEM, if it fails.
 */
int nvgpu_get_nvhost_dev(struct gk20a *g);

/**
 * @brief Free the nvhost device.
 *
 * @param g [in]	The GPU super structure.
 *
 * - Free the different fields of nvhost device initialized by
 *   #nvgpu_get_nvhost_dev().
 *
 * @return		None.
 */
void nvgpu_free_nvhost_dev(struct gk20a *g);

#ifdef CONFIG_NVGPU_KERNEL_MODE_SUBMIT
int nvgpu_nvhost_module_busy_ext(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev);
void nvgpu_nvhost_module_idle_ext(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev);

void nvgpu_nvhost_debug_dump_device(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev);

int nvgpu_nvhost_intr_register_notifier(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
	u32 id, u32 thresh, void (*callback)(void *priv, int nr_completed),
	void *private_data);

bool nvgpu_nvhost_syncpt_is_expired_ext(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
	u32 id, u32 thresh);
int nvgpu_nvhost_syncpt_wait_timeout_ext(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
	u32 id, u32 thresh, u32 timeout);

u32 nvgpu_nvhost_syncpt_incr_max_ext(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
	u32 id, u32 incrs);

int nvgpu_nvhost_syncpt_read_ext_check(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
	u32 id, u32 *val);

u32 nvgpu_nvhost_get_syncpt_host_managed(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
	u32 param,
	const char *syncpt_name);

int nvgpu_nvhost_create_symlink(struct gk20a *g);
void nvgpu_nvhost_remove_symlink(struct gk20a *g);

#endif

/**
 * @brief Get the sync_pt name of given sync point id.
 *
 * @param nvgpu_syncpt_dev [in]	Sync point device.
 * @param id [in]		Sync point id.
 *
 * - Read the sync_pt name of the given sync point by de-referring the
 *   #nvgpu_syncpt_dev->syncpt_names.
 *
 * @return			sync_pt name of given sync point id.
 */
const char *nvgpu_nvhost_syncpt_get_name(
	struct nvgpu_nvhost_dev *nvgpu_syncpt_dev, int id);

/**
 * @brief Increment the value of given sync point to the maximum value.
 *
 * @param nvgpu_syncpt_dev [in]	Sync point device.
 * @param id [in]		Sync point id.
 *
 * - Read the current value of the given sync point by calling
 *   #NvRmHost1xSyncpointRead().
 * - Read the max value of the sync point set at allocation of the
 *   sync point.
 * - If the max value is less than current, increment the syncpoint
 *   by the difference(max - current) using #NvRmHost1xSyncpointIncrement().
 *
 * @return			None.
 */
void nvgpu_nvhost_syncpt_set_min_eq_max_ext(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
	u32 id);

/**
 * @brief Read the maximum value of given sync point id.
 *
 * @param nvgpu_syncpt_dev [in]	Sync point device.
 * @param id [in]		Sync point id.
 *
 * - Read the max value of the given sync point by reading one of
 *   the max value array stored in nvhost sync point device structure.
 *
 * @return			Maximum value of the sync point.
 */
u32 nvgpu_nvhost_syncpt_read_maxval(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
	u32 id);

/**
 * @brief Set the value of given syncpoint to a value where all waiters of the
 *  sync point can be safely released.
 *
 * @param nvgpu_syncpt_dev [in]	Sync point device.
 * @param id [in]		Sync point id.
 *
 * - Read the current value of the sync point by #NvRmHost1xSyncpointRead().
 * - Increment the value by 256. This is just to make the sync point safe
 *   where all waiters of the sync point can be safely released.
 *
 * @return			None.
 */
void nvgpu_nvhost_syncpt_set_safe_state(
	struct nvgpu_nvhost_dev *nvgpu_syncpt_dev, u32 id);

/**
 * @brief Check the given sync point id is valid or not.
 *
 * @param nvgpu_syncpt_dev [in]	Sync point device.
 * @param id [in]		Sync point id.
 *
 * - Validate the sync point id.
 *
 * @return			TRUE, If id is valid.
 *				FALSE, If id is not valid.
 */
bool nvgpu_nvhost_syncpt_is_valid_pt_ext(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
	u32 id);

/**
 * @brief Free the sync point created by #nvgpu_nvhost_get_syncpt_client_managed().
 *
 * @param nvgpu_syncpt_dev [in]	Sync point device.
 * @param id [in]		Sync point id.
 *
 * - Check the validity of the given sync point.
 * - Free the fields allocated by #nvgpu_nvhost_get_syncpt_client_managed().
 *
 * @return			None.
 */
void nvgpu_nvhost_syncpt_put_ref_ext(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
	u32 id);

/**
 * @brief Allocate a sync point managed by a client.
 *
 * @param nvgpu_syncpt_dev [in]	Sync point device.
 * @param name [in]		Name of the sync point.
 *
 * - Call #nvgpu_nvhost_allocate_syncpoint() to allocate sync point.
 *   -- nvgpu_nvhost_allocate_syncpoint() will do the following
 *      - Call #NvRmHost1xSyncpointAllocate() to allocate sync point.
 *      - Call #NvRmHost1xSyncpointGetId() to get the ID.
 *      - Call #NvRmHost1xWaiterAllocate() to get waiter handle if needed.
 *      - Store the above datas in to the nvgpu nvhost device.
 *
 * @return			Sync point id allocated.
 */
u32 nvgpu_nvhost_get_syncpt_client_managed(struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
	const char *syncpt_name);

#ifdef CONFIG_SYNC
u32 nvgpu_nvhost_sync_pt_id(struct sync_pt *pt);
u32 nvgpu_nvhost_sync_pt_thresh(struct sync_pt *pt);
int nvgpu_nvhost_sync_num_pts(struct sync_fence *fence);

struct sync_fence *nvgpu_nvhost_sync_fdget(int fd);
struct sync_fence *nvgpu_nvhost_sync_create_fence(
	struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
	u32 id, u32 thresh, const char *name);
#endif /* CONFIG_SYNC */

#ifdef CONFIG_TEGRA_T19X_GRHOST

/**
 * @brief Initializes the address and size of memory mapped
 * sync point unit region(MSS).
 *
 * @param nvgpu_syncpt_dev [in]	Sync point device.
 * @param base [out]		Base address where it mapped.
 * @param size [out]		Size of the mapping.
 *
 * - Retrieve the value of base and size from the given
 *   sync point device.
 *
 * @return			Zero.
 */
int nvgpu_nvhost_syncpt_unit_interface_get_aperture(
		struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
		u64 *base, size_t *size);

/**
 * @brief Get offset of the sync point from MSS aperture base.
 *
 * @param syncpt_id [in]	Sync point id.
 *
 * - Multiply the id with 0x1000.
 *
 * @return			Sync point offset.
 */
u32 nvgpu_nvhost_syncpt_unit_interface_get_byte_offset(u32 syncpt_id);
#ifdef __KERNEL__
int nvgpu_nvhost_syncpt_init(struct gk20a *g);
#endif
#else
static inline int nvgpu_nvhost_syncpt_unit_interface_get_aperture(
		struct nvgpu_nvhost_dev *nvgpu_syncpt_dev,
		u64 *base, size_t *size)
{
	return -EINVAL;
}
static inline u32 nvgpu_nvhost_syncpt_unit_interface_get_byte_offset(u32 syncpt_id)
{
	return 0;
}
static inline int nvgpu_nvhost_syncpt_init(struct gk20a *g)
{
	return 0;
}
#endif
#endif /* CONFIG_TEGRA_GK20A_NVHOST */
#endif /* NVGPU_NVHOST_H */
