/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Dingdong dongle configuration */

#ifndef __BOARD_H
#define __BOARD_H

/* 48 MHz SYSCLK clock frequency */
#define CPU_CLOCK 48000000

/* the UART console is on USART1 (PA9/PA10) */
#define CONFIG_UART_CONSOLE 1

/* Optional features */
#define CONFIG_STM_HWTIMER32
#define CONFIG_ADC
#define CONFIG_BOARD_PRE_INIT
#define CONFIG_HW_CRC
#define CONFIG_RSA
#define CONFIG_RWSIG
#define CONFIG_SHA256
#define CONFIG_USB
#define CONFIG_USB_CONSOLE
#define CONFIG_USB_BOS
/* #define CONFIG_USB_INHIBIT_CONNECT */
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_USB_PD_ALT_MODE
#define CONFIG_USB_PD_HW_DEV_ID_BOARD_MAJOR USB_PD_HW_DEV_ID_DINGDONG
#define CONFIG_USB_PD_HW_DEV_ID_BOARD_MINOR 2
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_CUSTOM_VDM
#define CONFIG_USB_PD_FLASH
#define CONFIG_USB_PD_INTERNAL_COMP
#define CONFIG_USB_PD_IDENTITY_HW_VERS 1
#define CONFIG_USB_PD_IDENTITY_SW_VERS 1
#define CONFIG_USB_PD_NO_VBUS_DETECT
#define CONFIG_USB_PD_LOGGING
#define CONFIG_USB_PD_LOG_SIZE 256
#define CONFIG_CMD_AUX
#undef CONFIG_WATCHDOG_HELP
#undef CONFIG_LID_SWITCH
#undef CONFIG_TASK_PROFILING

/* USB configuration */
#define CONFIG_USB_VENDOR_STR  "BizLink Technology Inc."
#define CONFIG_USB_PRODUCT_STR "Dingdong2"
#define CONFIG_USB_BB_URL      USB_BIZLINK_TYPEC_URL
#define CONFIG_USB_VID         USB_VID_BIZLINK
#define CONFIG_USB_PID 0x5011
#define CONFIG_USB_BCD_DEV 0x0001 /* v 0.01 */

/* No Write-protect GPIO, force the write-protection */
/* #define CONFIG_WP_ALWAYS */

#ifndef __ASSEMBLER__

/* Timer selection */
#define TIM_CLOCK32 2
#define TIM_ADC     3

#include "gpio_signal.h"

/* ADC signal */
enum adc_channel {
	ADC_CH_CC1_PD = 0,
	ADC_CH_AUX_N,
	ADC_CH_AUX_P,
	/* Number of ADC channels */
	ADC_CH_COUNT
};

/* USB string indexes */
enum usb_strings {
	USB_STR_DESC = 0,
	USB_STR_VENDOR,
	USB_STR_PRODUCT,
	USB_STR_VERSION,
	USB_STR_BB_URL,
	USB_STR_CONSOLE_NAME,
	USB_STR_COUNT
};

#endif /* !__ASSEMBLER__ */

/* USB Device class */
/*#define USB_DEV_CLASS USB_CLASS_BILLBOARD */

/* USB interface indexes (use define rather than enum to expand them) */
#define USB_IFACE_CONSOLE   0
#define USB_IFACE_COUNT     1

/* USB endpoint indexes (use define rather than enum to expand them) */
#define USB_EP_CONTROL   0
#define USB_EP_CONSOLE   1
#define USB_EP_COUNT     2


#endif /* __BOARD_H */