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

/* *************** SERVICE ID for CCPLEX*************** */
/*  IDs for SW Error Handler */
#define NVGUARD_SERVICE_CCPLEX_MAX_SWERR 0U
/* IDs for Diagnostics Test Services */
/** DESCRIPTION : test for permanent fault diagnostic of FPS in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_FPS_PF_TEST0 0x0120b000U
/** DESCRIPTION : test for permanent fault diagnostic of FPS in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_FPS_PF_TEST1 0x0120b001U
/** DESCRIPTION : test for permanent fault diagnostic of FPS in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_FPS_PF_TEST2 0x0120b002U
/** DESCRIPTION : test for permanent fault diagnostic of FPS in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_FPS_PF_TEST3 0x0120b003U
/** DESCRIPTION : test for permanent fault diagnostic of FPS in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_FPS_PF_TEST4 0x0120b004U
/** DESCRIPTION : test for permanent fault diagnostic of FPS in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_FPS_PF_TEST5 0x0120b005U
/** DESCRIPTION : test for permanent fault diagnostic of FPS in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_FPS_PF_TEST6 0x0120b006U
/** DESCRIPTION : test for permanent fault diagnostic of FPS in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_FPS_PF_TEST7 0x0120b007U
/** DESCRIPTION : test for GIC HW diagnostic in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_GIC_SDL_TEST0 0x0120b008U
/** DESCRIPTION : test for GIC HW diagnostic in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_GIC_SDL_TEST1 0x0120b009U
/** DESCRIPTION : test for GIC HW diagnostic in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_GIC_SDL_TEST2 0x0120b00aU
/** DESCRIPTION : test for GIC HW diagnostic in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_GIC_SDL_TEST3 0x0120b00bU
/** DESCRIPTION : test for GIC HW diagnostic in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_GIC_SDL_TEST4 0x0120b00cU
/** DESCRIPTION : test for GIC HW diagnostic in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_GIC_SDL_TEST5 0x0120b00dU
/** DESCRIPTION : test for GIC HW diagnostic in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_GIC_SDL_TEST6 0x0120b00eU
/** DESCRIPTION : test for GIC HW diagnostic in CCPLEX */
#define NVGUARD_SERVICE_CCPLEX_DIAG_GIC_SDL_TEST7 0x0120b00fU
#define NVGUARD_SERVICE_CCPLEX_MAX_DIAG 16U
/* IDs for HW Error Handlers */
/** DESCRIPTION :   */
#define NVGUARD_SERVICE_CCPLEX_HSMERR_CCPLEX_UE 0x0100b0fcU
/** DESCRIPTION :   */
#define NVGUARD_SERVICE_CCPLEX_HSMERR_CCPLEX_CE 0x0100b1e3U
#define NVGUARD_SERVICE_CCPLEX_MAX_HWERR 2U
/* *************** END SERVICE ID for CCPLEX*************** */

#endif //NVGUARD_SERVICEID_H
