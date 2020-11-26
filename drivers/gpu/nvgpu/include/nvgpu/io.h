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
#ifndef NVGPU_IO_H
#define NVGPU_IO_H

#include <nvgpu/types.h>

/**
 * @file
 *
 * Interface for mmio access.
 */

/* Legacy defines - should be removed once everybody uses nvgpu_* */
#define gk20a_writel nvgpu_writel
#define gk20a_readl nvgpu_readl
#define gk20a_writel_check nvgpu_writel_check
#define gk20a_bar1_writel nvgpu_bar1_writel
#define gk20a_bar1_readl nvgpu_bar1_readl
#define gk20a_io_exists nvgpu_io_exists
#define gk20a_io_valid_reg nvgpu_io_valid_reg

struct gk20a;

/**
 * @brief Write a value to a GPU register with an ordering constraint.
 *
 * @param g [in]		GPU super structure.
 * @param r [in]		Register offset in GPU IO space.
 * @param v [in]		Value to write at the offset.
 *
 * Write a 32-bit value to register offset in GPU IO space with an
 * ordering constraint on memory operations.
 *
 * @return None.
 */
void nvgpu_writel(struct gk20a *g, u32 r, u32 v);

#ifdef CONFIG_NVGPU_DGPU
/**
 * @brief Write a value to GPU register without an ordering constraint.
 *
 * @param g [in]		GPU super structure.
 * @param r [in]		Register offset in GPU IO space.
 * @param v [in]		Value to write at the offset.
 *
 * Write a 32-bit value to register offset in GPU IO space without
 * an ordering constraint on memory operations. This function is
 * implemented by the OS layer.
 *
 * @return None.
 */
void nvgpu_writel_relaxed(struct gk20a *g, u32 r, u32 v);
#endif

/**
 * @brief Read a value from a GPU register.
 *
 * @param g [in]		GPU super structure.
 * @param r [in]		Register offset in GPU IO space.
 *
 * Read a 32-bit value from register offset in GPU IO space. If all
 * the bits are set in the value read then check for gpu state validity.
 * Refer #nvgpu_check_gpu_state() for gpu state validity check.
 *
 * @return Value at the given register offset in GPU IO space.
 */
u32 nvgpu_readl(struct gk20a *g, u32 r);

/**
 * @brief Read a value from a GPU register.
 *
 * @param g [in]		GPU super structure.
 * @param r [in]		Register offset in GPU IO space.
 *
 * Read a 32-bit to register offset from a GPU IO space. nvgpu_readl() is
 * called from this function. This function is implemented by the OS layer.
 *
 * @return Value at the given register offset in GPU IO space.
 */
u32 nvgpu_readl_impl(struct gk20a *g, u32 r);

/**
 * @brief Write validate to a GPU register.
 *
 * @param g [in]		GPU super structure.
 * @param r [in]		Register offset in GPU IO space.
 * @param v [in]		Value to write at the offset.
 *
 * Write a 32-bit value to register offset in GPU IO space and reads it
 * back. Check whether the write/read values match and logs the event on
 * a mismatch.
 *
 * @return None.
 */
void nvgpu_writel_check(struct gk20a *g, u32 r, u32 v);

#ifdef CONFIG_NVGPU_NON_FUSA
/**
 * @brief Ensure write to a GPU register.
 *
 * @param g [in]		GPU super structure.
 * @param r [in]		Register offset in GPU IO space.
 * @param v [in]		Value to write at the offset.
 *
 * This is a blocking call. It keeps on writing a 32-bit value to a GPU
 * register and reads it back until read/write values are not match.
 *
 * @return None.
 */
void nvgpu_writel_loop(struct gk20a *g, u32 r, u32 v);
#endif

/**
 * @brief Write a value to an already mapped bar1 io-region.
 *
 * @param g [in]		GPU super structure.
 * @param r [in]		Register offset in io-region.
 * @param v [in]		Value to write at the offset.
 *
 * - Write a 32-bit value to register offset of region bar1.
 *
 * @return None.
 */
void nvgpu_bar1_writel(struct gk20a *g, u32 b, u32 v);

/**
 * @brief Read a value from an already mapped bar1 io-region.
 *
 * @param g [in]		GPU super structure.
 * @param b [in]		Register offset in io-region.
 *
 * - Read a 32-bit value from a region bar1.
 *
 * @return Value at the given offset of the io-region.
 */
u32 nvgpu_bar1_readl(struct gk20a *g, u32 b);

/**
 * @brief Check bar0 io-region is mapped or not
 *
 * @param g [in]		GPU super structure.
 *
 * - io mapping exists if bar0 address is assigned to regs.
 *
 * @return TRUE if bar0 is mapped or else FALSE.
 */
bool nvgpu_io_exists(struct gk20a *g);

/**
 * @brief Validate BAR0 io-mapped offset.
 *
 * @param g [in]		GPU super structure.
 * @param r [in]		Register offset in io-region.
 *
 * - BAR0 Offset is valid if it falls into BAR0 range.
 *
 * @return TRUE if bar0 offset is valid or else FALSE.
 */
bool nvgpu_io_valid_reg(struct gk20a *g, u32 r);

/**
 * @brief Write value to register at phys offset.
 *
 * @param g [in]		GPU super structure.
 * @param r [in]		Register offset in GPU IO-space.
 * @param v [in]		Value to write at the offset.
 *
 * - Write a 32-bit value at phys offset. Phys_offset can be retrieved using
 * gops.func.get_full_phys_offset().
 *
 * @return None.
 */
void nvgpu_func_writel(struct gk20a *g, u32 r, u32 v);

/**
 * @brief Read value from register at phys offset.
 *
 * @param g [in]		GPU super structure.
 * @param b [in]		Register offset in GPU IO-space.
 *
 * - Read a 32-bit value from phys offset. Phys_offset can be retrieved using
 * gops.func.get_full_phys_offset().
 *
 * @return Value at the given offset.
 */
u32 nvgpu_func_readl(struct gk20a *g, u32 r);

#endif /* NVGPU_IO_H */
