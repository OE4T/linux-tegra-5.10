/*
 * Copyright (c) 2021-2022, NVIDIA CORPORATION, All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/**
 * @file tegra-epl.h
 * @brief <b> epl driver header file</b>
 *
 * This file will expose API prototype for epl kernel
 * space APIs.
 */


#ifndef TEGRA_EPL_H
#define TEGRA_EPL_H

/* ==================[Includes]============================================= */

/* ==================[MACROS]=============================================== */

/**
 * @brief API to check if SW error can be reported via Misc EC
 *        by reading and checking Misc EC error status register value.
 *
 * @param[in]   dev                     pointer to the device structure for the kernel driver
 *                                      from where API is called.
 * @param[in]   err_number              Generic SW error number for which status needs to
 *                                      enquired - [0 to 4].
 * @param[out]  status                  out param updated by API as follows:
 *                                      true - SW error can be reported
 *                                      false - SW error can not be reported because previous error
 *                                              is still active. Client needs to retry later.
 *
 * @returns
 *	0			(success)
 *	-EINVAL		(On invalid arguments)
 *	-ENODEV		(On device driver not loaded or Misc EC not configured)
 *	-EACCESS	(On client not allowed to report error via given Misc EC)
 *	-EAGAIN		(On Misc EC busy, client should retry)
 */
int epl_get_misc_ec_err_status(struct device *dev, uint8_t err_number, bool *status);


/**
 * @brief API to report SW error to FSI using Misc Generic SW error lines connected to
 *        the Misc error collator.
 *
 * @param[in]   dev                     pointer to the device structure for the kernel driver
 *                                      from where API is called.
 * @param[in]   err_number              Generic SW error number through which error
 *                                      needs to be reported.
 * @param[in]   sw_error_code           Client Defined Error Code, which will be
 *                                      forwarded to the application on FSI.
 *
 * @returns
 *	0			(success)
 *	-EINVAL		(On invalid arguments)
 *	-ENODEV		(On device driver not loaded or Misc EC not configured)
 *	-EACCESS	(On client not allowed to report error via given Misc EC)
 *	-EAGAIN		(On Misc EC busy, client should retry)
 */
int epl_report_misc_ec_error(struct device *dev, uint8_t err_number, uint32_t sw_error_code);

#endif /* TEGRA_EPL_H */
