/*
 * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
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

/*
 * This header provides constants for binding nvidia,tegra186-hsp.
 */

#ifndef _DT_BINDINGS_SOC_NVIDIA_TEGRA186_HSP_H_
#define _DT_BINDINGS_SOC_NVIDIA_TEGRA186_HSP_H_

#include <dt-bindings/mailbox/tegra186-hsp.h>

/*
 * These define the type of mailbox that is to be used (doorbell, shared
 * mailbox, shared semaphore or arbitrated semaphore).
 */
#ifndef TEGRA_HSP_MBOX_TYPE_DB
#define TEGRA_HSP_MBOX_TYPE_DB 0x0
#endif
#ifndef TEGRA_HSP_MBOX_TYPE_SM
#define TEGRA_HSP_MBOX_TYPE_SM 0x1
#endif
#ifndef TEGRA_HSP_MBOX_TYPE_SS
#define TEGRA_HSP_MBOX_TYPE_SS 0x2
#endif
#ifndef TEGRA_HSP_MBOX_TYPE_AS
#define TEGRA_HSP_MBOX_TYPE_AS 0x3
#endif

/*
* Shared mailboxes are unidirectional, so the direction needs to be specified
* in the device tree.
*/
#ifndef TEGRA_HSP_SM_FLAG_RXTX_MASK
#define TEGRA_HSP_SM_FLAG_RXTX_MASK (1 << 31)
#endif
#ifndef TEGRA_HSP_SM_FLAG_RX
#define TEGRA_HSP_SM_FLAG_RX (0 << 31)
#endif
#ifndef TEGRA_HSP_SM_FLAG_TX
#define TEGRA_HSP_SM_FLAG_TX (1 << 31)
#endif

#ifndef TEGRA_HSP_SM_MASK
#define TEGRA_HSP_SM_MASK 0x00ffffff
#endif

#ifndef TEGRA_HSP_SM_RX
#define TEGRA_HSP_SM_RX(x) (TEGRA_HSP_SM_FLAG_RX | ((x) & TEGRA_HSP_SM_MASK))
#endif
#ifndef TEGRA_HSP_SM_TX
#define TEGRA_HSP_SM_TX(x) (TEGRA_HSP_SM_FLAG_TX | ((x) & TEGRA_HSP_SM_MASK))
#endif

#endif	/* _DT_BINDINGS_SOC_NVIDIA_TEGRA186_HSP_H_ */
