/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef NVGUARD_SERVICEID_H
#define NVGUARD_SERVICEID_H

/* *************** SERVICE ID for CCPLEX_FPS*************** */
/*  IDs for SW Error Handler */
#define NVGUARD_SERVICE_CCPLEX_FPS_MAX_SWERR 0U
/* IDs for Diagnostics Test Services */
/** DESCRIPTION : Periodic test for permanent fault diagnostic of FPS in CCPLEX 0x0121c400 */
#define NVGUARD_SERVICE_CCPLEX_FPS_DIAG_PF_PERIODIC_TEST0 0x0121c400
/** DESCRIPTION : Periodic test for permanent fault diagnostic of FPS in CCPLEX 0x0121c401 */
#define NVGUARD_SERVICE_CCPLEX_FPS_DIAG_PF_PERIODIC_TEST1 0x0121c401
/** DESCRIPTION : Periodic test for permanent fault diagnostic of FPS in CCPLEX 0x0121c402 */
#define NVGUARD_SERVICE_CCPLEX_FPS_DIAG_PF_PERIODIC_TEST2 0x0121c402
/** DESCRIPTION : Periodic test for permanent fault diagnostic of FPS in CCPLEX 0x0121c403 */
#define NVGUARD_SERVICE_CCPLEX_FPS_DIAG_PF_PERIODIC_TEST3 0x0121c403
/** DESCRIPTION : Periodic test for permanent fault diagnostic of FPS in CCPLEX 0x0121c404 */
#define NVGUARD_SERVICE_CCPLEX_FPS_DIAG_PF_PERIODIC_TEST4 0x0121c404
/** DESCRIPTION : Periodic test for permanent fault diagnostic of FPS in CCPLEX 0x0121c405 */
#define NVGUARD_SERVICE_CCPLEX_FPS_DIAG_PF_PERIODIC_TEST5 0x0121c405
/** DESCRIPTION : Periodic test for permanent fault diagnostic of FPS in CCPLEX 0x0121c406 */
#define NVGUARD_SERVICE_CCPLEX_FPS_DIAG_PF_PERIODIC_TEST6 0x0121c406
/** DESCRIPTION : Periodic test for permanent fault diagnostic of FPS in CCPLEX 0x0121c407 */
#define NVGUARD_SERVICE_CCPLEX_FPS_DIAG_PF_PERIODIC_TEST7 0x0121c407
#define NVGUARD_SERVICE_CCPLEX_FPS_MAX_DIAG 8U
/* IDs for HW Error Handlers */
#define NVGUARD_SERVICE_CCPLEX_FPS_MAX_HWERR 0U
/* *************** END SERVICE ID for CCPLEX_FPS*************** */

/* *************** SERVICE ID for CCPLEX_GIC*************** */
/*  IDs for SW Error Handler */
#define NVGUARD_SERVICE_CCPLEX_GIC_MAX_SWERR 0U
/* IDs for Diagnostics Test Services */
/** DESCRIPTION : Periodic test for GIC HW diagnostic in CCPLEX 0x0121c800 */
#define NVGUARD_SERVICE_CCPLEX_GIC_DIAG_SDL_PERIODIC_TEST0 0x0121c800
/** DESCRIPTION : Periodic test for GIC HW diagnostic in CCPLEX 0x0121c801 */
#define NVGUARD_SERVICE_CCPLEX_GIC_DIAG_SDL_PERIODIC_TEST1 0x0121c801
/** DESCRIPTION : Periodic test for GIC HW diagnostic in CCPLEX 0x0121c802 */
#define NVGUARD_SERVICE_CCPLEX_GIC_DIAG_SDL_PERIODIC_TEST2 0x0121c802
/** DESCRIPTION : Periodic test for GIC HW diagnostic in CCPLEX 0x0121c803 */
#define NVGUARD_SERVICE_CCPLEX_GIC_DIAG_SDL_PERIODIC_TEST3 0x0121c803
/** DESCRIPTION : Periodic test for GIC HW diagnostic in CCPLEX 0x0121c804 */
#define NVGUARD_SERVICE_CCPLEX_GIC_DIAG_SDL_PERIODIC_TEST4 0x0121c804
/** DESCRIPTION : Periodic test for GIC HW diagnostic in CCPLEX 0x0121c805 */
#define NVGUARD_SERVICE_CCPLEX_GIC_DIAG_SDL_PERIODIC_TEST5 0x0121c805
/** DESCRIPTION : Periodic test for GIC HW diagnostic in CCPLEX 0x0121c806 */
#define NVGUARD_SERVICE_CCPLEX_GIC_DIAG_SDL_PERIODIC_TEST6 0x0121c806
/** DESCRIPTION : Periodic test for GIC HW diagnostic in CCPLEX 0x0121c807 */
#define NVGUARD_SERVICE_CCPLEX_GIC_DIAG_SDL_PERIODIC_TEST7 0x0121c807
#define NVGUARD_SERVICE_CCPLEX_GIC_MAX_DIAG 8U
/* IDs for HW Error Handlers */
#define NVGUARD_SERVICE_CCPLEX_GIC_MAX_HWERR 0U
/* *************** END SERVICE ID for CCPLEX_GIC*************** */
#endif
