/*
 * Copyright (c) 2019, NVIDIA CORPORATION.  All rights reserved.
 *
 * NVIDIA CORPORATION and its licensors retain all intellectual property
 * and proprietary rights in and to this software, related documentation
 * and any modifications thereto.  Any use, reproduction, disclosure or
 * distribution of this software and related documentation without an express
 * license agreement from NVIDIA CORPORATION is strictly prohibited.
 */

#ifndef INCLUDE_CAMRTC_DIAG_H
#define INCLUDE_CAMRTC_DIAG_H

#include <camrtc-common.h>

#pragma GCC diagnostic error "-Wpadded"

#define CAMRTC_DIAG_IVC_ALIGN	__aligned(8)
#define CAMRTC_DIAG_DMA_ALIGN	__aligned(64)

/**
 * Camera SDL - ISP5 SDL test vectors binary
 */

#define ISP5_SDL_PARAM_UNSPECIFIED	U32_C(0xFFFFFFFF)

/**
 * ISP5 SDL binary - header
 *
 * @param version		Monotonically-increasing version number
 * @param num_vectors		No. of test descriptors (vectors)
 * @param payload_crc32		CRC32 (unsigned) on binary payload (after
 *				header)
 * @param payload_offset	Offset [byte] to the payload region (also the
 *				header size)
 * @param input_base_offset     Offset [byte] to input images region from
 *				payload_offset
 * @param push_buffer2_offset	Offset [byte] to push_buffer2 allocation from
 *				payload_offset
 * @param output_buffers_offset	Offset [byte] to MW[0/1/2] output buffers
 *				scratch surface from payload_offset
 */
struct isp5_sdl_header {
	uint32_t version;
	uint32_t num_vectors;
	uint32_t payload_crc32;
	uint32_t payload_offset;
	uint32_t input_base_offset;
	uint32_t push_buffer2_offset;
	uint32_t output_buffers_offset;
	uint32_t __reserved[9];
} CAMRTC_DIAG_DMA_ALIGN;

/**
 * ISP5 SDL binary - test descriptor
 *
 * @param test_index		Zero-index test number (0...num_vectors-1)
 * @param input_width		Input image width [px] (same for all inputs)
 * @param input_height		Input image height [px] (same for all inputs)
 * @param input_offset[3]	Offset [byte] to the nth input image of the
 *				test vector from input_base_offset
 * @param output_crc32[3]	Golden CRC32 values for MW0, MW1 and MW2 output
 * @param push_buffer1_size	ISP5 push buffer 1 size [dword]
 * @param push_buffer1		ISP5 frame push buffer 1
 * @param config_buffer		ISP5 frame config buffer (isp5_configbuffer)
 */
struct isp5_sdl_test_descriptor {
	uint32_t test_index;
	uint16_t input_width;
	uint16_t input_height;
	uint32_t input_offset[3];
	uint32_t output_crc32[3];
	uint32_t __reserved[7];
	uint32_t push_buffer1_size;
	uint32_t push_buffer1[4096] CAMRTC_DIAG_DMA_ALIGN;
	uint8_t config_buffer[128] CAMRTC_DIAG_DMA_ALIGN;
} CAMRTC_DIAG_DMA_ALIGN;

#pragma GCC diagnostic ignored "-Wpadded"

#endif /* INCLUDE_CAMRTC_DIAG_H */
