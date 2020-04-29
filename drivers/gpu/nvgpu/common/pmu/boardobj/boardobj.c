/*
 * Copyright (c) 2016-2020, NVIDIA CORPORATION.  All rights reserved.
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

#include <nvgpu/kmem.h>
#include <nvgpu/gk20a.h>

#include "boardobj.h"

/*
* Destructor for the base board object. Called by each device-Specific
* implementation of the BOARDOBJ interface to destroy the board object.
* This has to be explicitly set by each device that extends from the
* board object.
*/
static int destruct_super(struct boardobj *pboardobj)
{
	struct gk20a *g = pboardobj->g;

	nvgpu_log_info(g, " ");
	if (pboardobj == NULL) {
		return -EINVAL;
	}

	nvgpu_list_del(&pboardobj->node);
	if (pboardobj->allocated) {
		nvgpu_kfree(pboardobj->g, pboardobj);
	}

	return 0;
}

/*
* check whether the specified BOARDOBJ object implements the queried
* type/class enumeration.
*/
static bool implements_super(struct gk20a *g, struct boardobj *pboardobj,
	u8 type)
{
	nvgpu_log_info(g, " ");

	return (0U != (pboardobj->type_mask & BIT32(type)));
}

int nvgpu_boardobj_pmu_data_init_super(struct gk20a *g,
		struct boardobj *pboardobj, struct nv_pmu_boardobj *pmudata)
{
	nvgpu_log_info(g, " ");
	if (pboardobj == NULL) {
		return -EINVAL;
	}
	if (pmudata == NULL) {
		return -EINVAL;
	}
	pmudata->type = pboardobj->type;
	nvgpu_log_info(g, " Done");
	return 0;
}

int nvgpu_boardobj_construct_super(struct gk20a *g, struct boardobj
				**ppboardobj, size_t size, void *args)
{
	struct boardobj  *pboardobj = NULL;
	struct boardobj  *devtmp = (struct boardobj *)args;

	nvgpu_log_info(g, " ");

	if (devtmp == NULL) {
		return -EINVAL;
	}

	if (*ppboardobj == NULL) {
		*ppboardobj = nvgpu_kzalloc(g, size);
		if (*ppboardobj == NULL) {
			return -ENOMEM;
		}
		(*ppboardobj)->allocated = true;
	}

	pboardobj = *ppboardobj;
	pboardobj->g = g;
	pboardobj->type = devtmp->type;
	pboardobj->idx = CTRL_BOARDOBJ_IDX_INVALID;
	pboardobj->type_mask   = BIT32(pboardobj->type) | devtmp->type_mask;

	pboardobj->implements  = implements_super;
	pboardobj->destruct    = destruct_super;
	pboardobj->pmudatainit = nvgpu_boardobj_pmu_data_init_super;

	nvgpu_list_add(&pboardobj->node, &g->boardobj_head);

	return 0;
}


