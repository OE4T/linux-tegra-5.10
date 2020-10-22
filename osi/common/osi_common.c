/*
 * Copyright (c) 2018-2020, NVIDIA CORPORATION. All rights reserved.
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

#include <osd.h>
#include "eqos_common.h"
#include "../osi/common/common.h"



void osi_memset(void *s, nveu32_t c, nveu64_t count)
{
	nveu8_t *xs = OSI_NULL;
	nveu64_t temp = count;

	if (s == OSI_NULL) {
		return;
	}
	xs = (nveu8_t *)s;
	while (temp != 0UL) {
		if (c < OSI_UCHAR_MAX) {
			*xs = (nveu8_t)c;
			xs++;
		}
		temp--;
	}
}

void osi_memcpy(void *dest, void *src, int n)
{
	char *csrc = (char *)src;
	char *cdest = (char *)dest;
	int i = 0;

	if (src == OSI_NULL || dest == OSI_NULL) {
		return;
	}
	for (i = 0; i < n; i++) {
		cdest[i] = csrc[i];
	}
}

void common_get_systime_from_mac(void *addr, nveu32_t mac, nveu32_t *sec,
				 nveu32_t *nsec)
{
	nveu64_t temp;
	nveu64_t remain;
	nveul64_t ns;

	if (mac == OSI_MAC_HW_EQOS) {
		ns = eqos_get_systime_from_mac(addr);
	} else {
		/* Non EQOS HW is supported yet */
		return;
	}

	temp = div_u64_rem((nveu64_t)ns, OSI_NSEC_PER_SEC, &remain);
	if (temp < UINT_MAX) {
		*sec = (nveu32_t)temp;
	} else {
		/* do nothing here */
	}
	if (remain < UINT_MAX) {
		*nsec = (nveu32_t)remain;
	} else {
		/* do nothing here */
	}
}

nveu32_t common_is_mac_enabled(void *addr, nveu32_t mac)
{
	if (mac == OSI_MAC_HW_EQOS) {
		return eqos_is_mac_enabled(addr);
	} else {
		/* Non EQOS HW is supported yet */
		return OSI_DISABLE;
	}
}

nveu64_t div_u64_rem(nveu64_t dividend, nveu64_t divisor,
		     nveu64_t *remain)
{
	nveu64_t ret = 0;

	if (divisor != 0U) {
		*remain = dividend % divisor;
		ret = dividend / divisor;
	} else {
		ret = 0;
	}

	return ret;
}
