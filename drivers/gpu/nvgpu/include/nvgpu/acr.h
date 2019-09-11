/*
 * Copyright (c) 2016-2019, NVIDIA CORPORATION.  All rights reserved.
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

#ifndef NVGPU_ACR_H
#define NVGPU_ACR_H

/**
 * @file
 * @page unit-acr Unit ACR(Access Controlled Regions)
 *
 * Acronyms
 * ========
 * ACR     - Access Controlled Regions
 * ACR HS  - Access Controlled Regions Heavy-Secure ucode
 * FB      - Frame Buffer
 * non-WPR - non-Write Protected Region
 * WPR     - Write Protected Region
 * LS      - Light-Secure
 * HS      - Heavy-Secure
 * Falcon  - Fast Logic CONtroller
 * BLOB    - Binary Large OBject
 *
 * Overview
 * ========
 * The ACR unit is responsible for GPU secure boot. ACR unit divides its task
 * into two stages as below,
 *
 * + Blob construct:
 *   ACR unit creates LS ucode blob in system/FB's non-WPR memory. LS ucodes
 *   will be read from filesystem and added to blob as per ACR unit static
 *   config data. ACR unit static config data is set based on current chip.
 *   LS ucodes blob is required by the ACR HS ucode to authenticate & load LS
 *   ucode on to respective engine's LS Falcon.
 *
 * + ACR HS ucode load & bootstrap:
 *   ACR HS ucode is responsible for authenticating self(HS) & LS ucode.
 *
 *   ACR HS ucode is read from the filesystem based on the chip-id by the ACR
 *   unit. Read ACR HS ucode is loaded onto PMU/SEC2/GSP engines Falcon to
 *   bootstrap ACR HS ucode. ACR HS ucode does self-authentication using H/W
 *   based HS authentication methodology. Once authenticated the ACR HS ucode
 *   starts executing on the falcon.
 *
 *   Upon successful ACR HS ucode boot, ACR HS ucode performs a sanity check on
 *   WPR memory. If the WPR sanity check passes, then ACR HS ucode copies LS
 *   ucodes from system/FB's non-WPR memory to system/FB's WPR memory. The
 *   purpose of copying LS ucode to WPR memory is to protect ucodes from
 *   modification or tampering. The next step is to authenticate LS ucodes
 *   present in WPR memory using S/W based authentication methodology. If the
 *   LS ucode authentication passed, then ACR HS ucode loads LS ucode on to
 *   respective LS Falcons. If any of the LS ucode authentications fail, then
 *   ACR HS ucode updates error details in Falcon mailbox-0/1 & halts its
 *   execution. In the passing case, ACR HS ucode halts & updates mailbox-0 with
 *   ACR_OK(0x0) status.
 *
 *   ACR unit waits for ACR HS ucode to halt & checks for mailbox-0/1 to
 *   determine the status of ACR HS ucode. If there was an error then ACR unit
 *   returns an error else success.
 *
 * The ACR unit is a s/w unit which doesn't access any h/w registers by itself.
 * It depends on below units to access H/W resource to complete its task.
 *
 *   + PMU, SEC2 & GSP unit to access & load ucode on Engines Falcon.
 *   + Falcon unit to control/access Engines(PMU, SEC2 & GSP) Falcon to load &
 *     execute HS ucode
 *   + MM unit to fetch non-WPR/WPR info, allocate & read/write data in
 *     non-WPR memory.
 *
 * Data Structures
 * ===============
 *
 * There are no data structures exposed outside of ACR unit in nvgpu.
 *
 * Static Design
 * =============
 *
 * ACR Initialization
 * ------------------
 * ACR initialization happens as part of early NVGPU poweron sequence by calling
 * nvgpu_acr_init(). At ACR init stage memory gets allocated for ACR unit's
 * private data struct. The data struct holds static properties and ops of the
 * ACR unit and is populated based on the detected chip. These static properties
 * and ops will be used by blob-construct and load/bootstrap stage of ACR unit.
 *
 * ACR Teardown
 * ------------
 * The function nvgpu_acr_free() is called from nvgpu_remove() as part of
 * poweroff sequence to clear and free the memory space allocated for ACR unit.
 *
 * External APIs
 * -------------
 *   + nvgpu_acr_init()
 *   + nvgpu_acr_free()
 *
 * Dynamic Design
 * ==============
 *
 * After ACR unit init completion, the properties and ops of the ACR unit are
 * set to perform blob construction in non-wpr memory & load/bootstrap of HS ACR
 * ucode on specific engine's Falcon.
 *
 * Blob construct
 * --------------
 * The ACR unit creates blob for LS ucodes in nop-WPR memory & update
 * WPR/LS-ucode details in interface which is part of non-wpr region. Interface
 * will be accessed by ACR HS ucode to know in detail about WPR & LS ucodes.
 *
 * Load/Bootstrap ACR HS ucode
 * ---------------------------
 * The ACR unit loads ACR HS ucode onto PMU/SEC2/GSP engines Falcon as per
 * static config data & performs a bootstrap.
 *
 * ACR HS ucode does self-authentication using H/W based HS authentication
 * methodology. Once authenticated the ACR HS ucode starts executing on the
 * falcon. Upon successful ACR HS ucode boot, ACR HS ucode copies LS ucodes
 * from non-WPR memory to WPR memory. The next step is to authenticate LS ucodes
 * present in WPR memory and loads LS ucode on to respective LS Falcons.
 *
 * The ACR unit waits for ACR HS to halt within predefined timeout. Upon ACR HS
 * ucode halt, the ACR unit checks mailbox-0/1 to determine the status of ACR
 * HS ucode. If there is an error then ACR unit returns error else success.
 *
 * External APIs
 * -------------
 *   + nvgpu_acr_construct_execute()
 *   + nvgpu_acr_is_lsf_lazy_bootstrap()
 *   + nvgpu_acr_bootstrap_hs_acr()
 *
 */

struct gk20a;
struct nvgpu_falcon;
struct nvgpu_firmware;
struct nvgpu_acr;

/**
 * @brief ACR initialization to allocate memory for ACR unit & set static
 *        properties and ops for LS ucode blob construction as well as for
 *        ACR HS ucode bootstrap.
 *
 * @param g   [in] The GPU driver struct.
 * @param acr [in] The ACR private data struct.
 *
 * Initializes ACR unit private data struct in the GPU driver based on current
 * chip. Allocate memory for #nvgpu_acr data struct & sets the static properties
 * and ops for LS ucode blob construction as well as for ACR HS ucode bootstrap.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_acr_init(struct gk20a *g, struct nvgpu_acr **acr);

#ifdef CONFIG_NVGPU_DGPU
int nvgpu_acr_alloc_blob_prerequisite(struct gk20a *g, struct nvgpu_acr *acr,
	size_t size);
#endif

/**
 * @brief Construct blob of LS ucode's in non-wpr memory. Load and bootstrap HS
 *        ACR ucode on specified engine Falcon
 *
 * @param g   [in] The GPU driver struct.
 * @param acr [in] The ACR private data struct
 *
 * Construct blob of LS ucode in non-wpr memory. Allocation happens in non-WPR
 * system/FB memory based on type of GPU iGPU/dGPU currently in execution. Next,
 * ACR unit loads & bootstrap ACR HS ucode on specified engine Falcon.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_acr_construct_execute(struct gk20a *g, struct nvgpu_acr *acr);

/**
 * @brief Read, Load and Bootstrap HS ACR ucode on Engine's Falcon.
 *
 * @param g   [in] The GPU driver struct.
 * @param acr [in] The ACR private data struct
 *
 * Load HS ucode on specified engine Falcon as per static config data & does
 * bootstrap to self-HS-authenticate(H/W based) followed by ACR HS execution.
 * ACR unit waits for ACR HS ucode to halt & check mailbox-0/1 to know the
 * status of ACR HS ucode, if there is an error then ACR unit returns an error
 * else success.
 *
 * @return 0 in case of success, < 0 in case of failure.
 */
int nvgpu_acr_bootstrap_hs_acr(struct gk20a *g, struct nvgpu_acr *acr);

/**
 * @brief Check if ls-Falcon lazy-bootstrap status to load & bootstrap from
 *        LS-RTOS or not
 *
 * @param g   [in] The GPU driver struct.
 * @param acr [in] The ACR private data struct
 *
 * Chek if ls-Falcon lazy-bootstrap status to load & bootstrap from
 * LS-RTOS or not.
 *
 * @return True in case of success, False in case of failure.
 */
bool nvgpu_acr_is_lsf_lazy_bootstrap(struct gk20a *g, struct nvgpu_acr *acr,
	u32 falcon_id);

#endif /* NVGPU_ACR_H */

