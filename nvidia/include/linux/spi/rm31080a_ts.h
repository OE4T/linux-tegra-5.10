/*
 * Raydium RM31080 touchscreen header
 *
 * Copyright (C) 2012-2014, Raydium Semiconductor Corporation.
 * Copyright (C) 2012-2014, NVIDIA Corporation.  All Rights Reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#ifndef _RM31080A_TS_H_
#define _RM31080A_TS_H_

/***************************************************************************
 *	Kernel CTRL Define
 *	DO NOT MODIFY
 *	NOTE: Need to sync with HAL
 ***************************************************************************/
#define RETURN_OK				0
#define RETURN_FAIL				1
#define TRUE					1
#define FALSE					0
#define DEBUG_DRIVER			0x01
#define DEBUG_REGISTER			0x02
#define DEBUG_KTHREAD			0x04

#define RM_IOCTL_REPORT_POINT				0x1001
#define RM_IOCTL_SET_HAL_PID				0x1002
#define RM_IOCTL_INIT_START					0x1003
#define RM_IOCTL_INIT_END					0x1004
#define RM_IOCTL_FINISH_CALC				0x1005
#define RM_IOCTL_SCRIBER_CTRL				0x1006
#define RM_IOCTL_READ_RAW_DATA				0x1007
#define RM_IOCTL_SET_PARAMETER				0x100A
#define RM_IOCTL_SET_VARIABLE				0x1010

#define RM_VARIABLE_SELF_TEST_RESULT			0x01
#define RM_VARIABLE_SCRIBER_FLAG				0x02
#define RM_VARIABLE_AUTOSCAN_FLAG				0x03
#define RM_VARIABLE_VERSION						0x04
#define RM_VARIABLE_IDLEMODECHECK				0x05
#define RM_VARIABLE_REPEAT						0x06
#define RM_VARIABLE_WATCHDOG_FLAG				0x07
#define RM_VARIABLE_TEST_VERSION				0x08
#define RM_VARIABLE_SET_SPI_UNLOCK				0x09
#define RM_VARIABLE_SET_WAKE_UNLOCK				0x0A
#define RM_VARIABLE_DPW							0x0B
#define RM_VARIABLE_NS_MODE						0x0C
#define RM_VARIABLE_TOUCHFILE_STATUS			0x0D
#define RM_VARIABLE_TOUCH_EVENT					0x0E


#define RM_IOCTL_GET_VARIABLE				0x1011
#define RM_VARIABLE_PLATFORM_ID					0x01
#define RM_VARIABLE_GPIO_SELECT					0x02
#define RM_VARIABLE_CHECK_SPI_LOCK				0x03
#define RM_IOCTL_GET_SACN_MODE				0x1012
#define RM_IOCTL_SET_KRL_TBL				0x1013
#define RM_IOCTL_WATCH_DOG					0x1014
#define RM_IOCTL_SET_BASELINE				0x1015
#define RM_IOCTL_INIT_SERVICE				0x1016
#define RM_IOCTL_SET_CLK					0x1017

#define RM_INPUT_RESOLUTION_X				4096
#define RM_INPUT_RESOLUTION_Y				4096

#define RM_TS_SIGNAL						44
#define RM_TS_MAX_POINTS					16

#define RM_SIGNAL_INTR						0x00000001
#define RM_SIGNAL_SUSPEND					0x00000002
#define RM_SIGNAL_RESUME					0x00000003
#define RM_SIGNAL_CHANGE_PARA				0x00000004
#define RM_SIGNAL_WATCH_DOG_CHECK			0x00000005
#define RM_SIGNAL_REPORT_MODE_CHANGE		0x00000006
#define RM_SIGNAL_PARA_SMOOTH					0x00
#define RM_SIGNAL_PARA_SELF_TEST				0x01
#define RM_SIGNAL_PARA_REPORT_MODE_CHANGE		0x02


#define RM_SELF_TEST_STATUS_FINISH			0
#define RM_SELF_TEST_STATUS_TESTING			1
#define RM_SELF_TEST_RESULT_FAIL			0
#define RM_SELF_TEST_RESULT_PASS			1

/****************************************************************************
 * Platform define
 ***************************************************************************/
#define RM_PLATFORM_K007	0x00
#define RM_PLATFORM_K107	0x01
#define RM_PLATFORM_C210	0x02
#define RM_PLATFORM_D010	0x03
#define RM_PLATFORM_P005	0x04
#define RM_PLATFORM_R005	0x05
#define RM_PLATFORM_M010	0x06
#define RM_PLATFORM_P140	0x07
#define RM_PLATFORM_A010	0x08
#define RM_PLATFORM_L005	0x09
#define RM_PLATFORM_K156	0x0A
#define RM_PLATFORM_T008	0x0B
#define RM_PLATFORM_T008_2	0x0D
#define RM_PLATFORM_RAYPRJ	0x80

/***************************************************************************
 *	DO NOT MODIFY - Kernel CTRL Define
 *	NOTE: Need to sync with HAL
 ***************************************************************************/



/***************************************************************************
 *	Kernel Command Set
 *	DO NOT MODIFY
 *	NOTE: Need to sync with HAL
 ***************************************************************************/
#define KRL_TBL_CMD_LEN					3

#define KRL_INDEX_FUNC_SET_IDLE			0
#define KRL_INDEX_FUNC_PAUSE_AUTO		1
#define KRL_INDEX_RM_RESUME				2
#define KRL_INDEX_RM_SUSPEND			3
#define KRL_INDEX_RM_READ_IMG			4
#define KRL_INDEX_RM_WATCHDOG			5
#define KRL_INDEX_RM_TESTMODE			6
#define KRL_INDEX_RM_SLOWSCAN			7
#define KRL_INDEX_RM_CLEARINT			8
#define KRL_INDEX_RM_SCANSTART			9
#define KRL_INDEX_RM_WAITSCANOK			10
#define KRL_INDEX_RM_SETREPTIME			11
#define KRL_INDEX_RM_NSPARA				12
#define KRL_INDEX_RM_WRITE_IMG			13
#define KRL_INDEX_RM_TLK				14
#define KRL_INDEX_RM_KL_TESTMODE		15
#define KRL_INDEX_RM_NS_SCF				16

#define KRL_SIZE_SET_IDLE				128
#define KRL_SIZE_PAUSE_AUTO				64
#define KRL_SIZE_RM_RESUME				64
#define KRL_SIZE_RM_SUSPEND				64
#define KRL_SIZE_RM_READ_IMG			64
#define KRL_SIZE_RM_WATCHDOG			96
#define KRL_SIZE_RM_TESTMODE			96
#define KRL_SIZE_RM_SLOWSCAN			128
#define KRL_SIZE_RM_CLEARINT			32
#define KRL_SIZE_RM_SCANSTART			32
#define KRL_SIZE_RM_WAITSCANOK			32
#define KRL_SIZE_RM_SETREPTIME			32
#define KRL_SIZE_RM_NS_PARA				64
#define KRL_SIZE_RM_WRITE_IMAGE			64
#define KRL_SIZE_RM_TLK					128
#define KRL_SIZE_RM_KL_TESTMODE			128
#define KRL_SIZE_RM_SCF_PARA			64

#define KRL_TBL_FIELD_POS_LEN_H				0
#define KRL_TBL_FIELD_POS_LEN_L				1
#define KRL_TBL_FIELD_POS_CASE_NUM			2
#define KRL_TBL_FIELD_POS_CMD_NUM			3

#define KRL_CMD_READ						0x11
#define KRL_CMD_WRITE_W_DATA				0x12
#define KRL_CMD_WRITE_WO_DATA				0x13
#define KRL_CMD_IF_AND_OR					0x14
#define KRL_CMD_AND							0x18
#define KRL_CMD_OR							0x19
#define KRL_CMD_NOT							0x1A
#define KRL_CMD_XOR							0x1B
#define KRL_CMD_WRITE_W_COUNT				0x1C
#define KRL_CMD_RETURN_RESULT				0x1D
#define KRL_CMD_RETURN_VALUE				0x1E
#define KRL_CMD_DRAM_INIT					0x1F


#define KRL_CMD_SEND_SIGNAL					0x20
#define KRL_CMD_CONFIG_RST					0x21
#define KRL_SUB_CMD_SET_RST_GPIO				0x00
#define KRL_SUB_CMD_SET_RST_VALUE				0x01

#define KRL_CMD_SET_TIMER					0x22
#define KRL_SUB_CMD_INIT_TIMER					0x00
#define KRL_SUB_CMD_ADD_TIMER					0x01
#define KRL_SUB_CMD_DEL_TIMER					0x02

#define KRL_CMD_CONFIG_3V3					0x23
#define KRL_SUB_CMD_SET_3V3_GPIO				0x00
#define KRL_SUB_CMD_SET_3V3_REGULATOR			0x01

#define KRL_CMD_CONFIG_1V8					0x24
#define KRL_SUB_CMD_SET_1V8_GPIO				0x00
#define KRL_SUB_CMD_SET_1V8_REGULATOR			0x01

#define KRL_CMD_CONFIG_CLK					0x25
#define KRL_SUB_CMD_SET_CLK						0x00

#define KRL_CMD_CONFIG_CS					0x26
#define KRL_SUB_CMD_SET_CS_LOW					0x00

#define KRL_CMD_MSLEEP						0x40

#define KRL_CMD_FLUSH_QU					0x52
#define KRL_SUB_CMD_SENSOR_QU					0x00
#define KRL_SUB_CMD_TIMER_QU					0x01

#define KRL_CMD_READ_IMG					0x60
#define KRL_CMD_WRITE_IMG					0x61

#define KRL_CMD_CONFIG_IRQ					0x70
#define KRL_SUB_CMD_SET_IRQ						0x00

#define KRL_CMD_DUMMY						0xFF


/***************************************************************************
 *	DO NOT MODIFY - Kernel Command Set
 *	NOTE: Need to sync with HAL
 ***************************************************************************/


/***************************************************************************
 *	Kernel Point Report Definition
 *	DO NOT MODIFY
 *	NOTE: Need to sync with HAL
 ***************************************************************************/
#define INPUT_PROTOCOL_TYPE_A	0x01
#define INPUT_PROTOCOL_TYPE_B	0x02
#define INPUT_PROTOCOL_CURRENT_SUPPORT INPUT_PROTOCOL_TYPE_B

#define INPUT_SLOT_RESET	0x80
#define INPUT_ID_RESET		0xFF
#define MAX_REPORT_TOUCHED_POINTS	10

#define POINT_TYPE_NONE			0x00
#define POINT_TYPE_STYLUS		0x01
#define POINT_TYPE_ERASER		0x02
#define POINT_TYPE_FINGER		0x03
#define POINT_TYPE_THUMB		0x04
#define POINT_TYPE_NUM			0x05

#define EVENT_REPORT_MODE_STYLUS_ERASER_FINGER		0x00
#define EVENT_REPORT_MODE_FINGER_ONLY				0x01
#define EVENT_REPORT_MODE_FINGER_ONLY_ALL_FINGERS	0x02
#define EVENT_REPORT_MODE_STYLUS_FINGER				0x03
#define EVENT_REPORT_MODE_STYLUS_ERASER				0x04
#define EVENT_REPORT_MODE_STYLUS_ONLY				0x05
#define EVENT_REPORT_MODE_ERASER_ONLY				0x06
#define EVENT_REPORT_MODE_TYPE_NUM					0x07
/***************************************************************************
 *	DO NOT MODIFY - Kernel Point Report Definition
 *	NOTE: Need to sync with HAL
 ***************************************************************************/

/*#define ENABLE_CALC_QUEUE_COUNT*/
#define ENABLE_SLOW_SCAN
#define ENABLE_SMOOTH_LEVEL
#define ENABLE_SPI_SETTING		0
#define ENABLE_FREQ_HOPPING		1
#define ENABLE_QUEUE_GUARD		0
#define ENABLE_EVENT_QUEUE		0
#define CS_SUPPORT

#define ISR_POST_HANDLER WORK_QUEUE		/*or KTHREAD*/
#define WORK_QUEUE	0
#define KTHREAD		1

enum tch_update_reason {
	STYLUS_DISABLE_BY_WATER = 0x01,
	STYLUS_DISABLE_BY_NOISE,
	STYLUS_IS_ENABLED = 0xFF,
};

struct rm_touch_event {
	unsigned char uc_touch_count;
	unsigned char uc_id[RM_TS_MAX_POINTS];
	unsigned char uc_tool_type[RM_TS_MAX_POINTS];
	unsigned short us_x[RM_TS_MAX_POINTS];
	unsigned short us_y[RM_TS_MAX_POINTS];
	unsigned short us_z[RM_TS_MAX_POINTS];
	unsigned short us_tilt_x[RM_TS_MAX_POINTS];
	unsigned short us_tilt_y[RM_TS_MAX_POINTS];
	unsigned char uc_slot[RM_TS_MAX_POINTS];
	unsigned char uc_pre_tool_type[RM_TS_MAX_POINTS];
};

#if ENABLE_EVENT_QUEUE
struct rm_touch_event_list {
	struct list_head next_event;
	struct rm_touch_event *event_record;
};
#endif

struct rm_spi_ts_platform_data {
	int gpio_reset;
	int gpio_1v8;
	int gpio_3v3;
	int x_size;
	int y_size;
	unsigned char *config;
	int platform_id;
	unsigned char *name_of_clock;
	unsigned char *name_of_clock_con;
	bool gpio_sensor_select0;
	bool gpio_sensor_select1;
};

int rm_tch_spi_byte_write(unsigned char u8_addr, unsigned char u8_value);
int rm_tch_spi_byte_read(unsigned char u8_addr, unsigned char *p_u8_value);

#endif				/*_RM31080A_TS_H_*/
