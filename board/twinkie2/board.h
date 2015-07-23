/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Twinkie dongle configuration */

#ifndef __BOARD_H
#define __BOARD_H

/* 48 MHz SYSCLK clock frequency */
#define CPU_CLOCK 48000000

/* the UART console is on USART1 (PA9/PA10) */
#define CONFIG_UART_CONSOLE 1

/* Optional features */
#define CONFIG_USB
#define CONFIG_USB_CONSOLE

#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_INTERNAL_COMP
#define CONFIG_PD_USE_DAC_AS_REF
#define CONFIG_HW_CRC

#ifndef HAS_TASK_PD /* PD sniffer mode */
#undef CONFIG_DMA_DEFAULT_HANDLERS
#define CONFIG_USB_PD_TX_PHY_ONLY
/* override the comparator interrupt handler */
#undef CONFIG_USB_PD_RX_COMP_IRQ
#endif

#define CONFIG_ADC
#define CONFIG_BOARD_PRE_INIT
#define CONFIG_I2C
#define CONFIG_INA231
#undef CONFIG_WATCHDOG_HELP
#undef CONFIG_LID_SWITCH
#undef CONFIG_TASK_PROFILING

/* I2C ports configuration */
#define I2C_PORT_MASTER 0

/* USB configuration */
#define CONFIG_USB_VID         USB_VID_BIZLINK
#define CONFIG_USB_PID 0x500A

#ifdef CONFIG_BIZ_EMU_DOCK
#define CONFIG_PD_DAC_CR           STM32_DAC_CR_EN2      /* DAC_OUT2 PA5 */
#define CONFIG_PD_DAC_VOUT         (550 * 4096 / 3300) << 16   /* STD: 550 */
#else
#define CONFIG_PD_DAC_CR           STM32_DAC_CR_EN1      /* DAC_OUT1 PA4 */
#define CONFIG_PD_DAC_VOUT         (550 * 4096 / 3300)   /* STD: 550 */
#endif

/* By default, enable all console messages excepted USB */
#define CC_DEFAULT     (CC_ALL & ~CC_MASK(CC_USB))

/*
 * Allow dangerous commands all the time, since we don't have a write protect
 * switch.
 */
#define CONFIG_SYSTEM_UNLOCKED

#ifndef __ASSEMBLER__

void sniffer_init(void);

int wait_packet(int pol, uint32_t min_edges, uint32_t timeout_us);

int expect_packet(int pol, uint8_t cmd, uint32_t timeout_us);

uint8_t recording_enable(uint8_t mask);

void trace_packets(void);

void set_trace_mode(int mode);

/* Timer selection */
#define TIM_CLOCK_MSB  3
#define TIM_CLOCK_LSB 15
#define TIM_ADC       16

#include "gpio_signal.h"

/* ADC signal */
enum adc_channel {
	ADC_CH_CC1_PD = 0,
	ADC_CH_CC2_PD,
	/* Number of ADC channels */
	ADC_CH_COUNT
};

/* USB string indexes */
enum usb_strings {
	USB_STR_DESC = 0,
	USB_STR_VENDOR,
	USB_STR_PRODUCT,
	USB_STR_VERSION,
	USB_STR_SNIFFER,
	USB_STR_CONSOLE_NAME,

	USB_STR_COUNT
};

#endif /* !__ASSEMBLER__ */

/* USB interface indexes (use define rather than enum to expand them) */
#define USB_IFACE_CONSOLE 0
#define USB_IFACE_VENDOR  1
#define USB_IFACE_COUNT   2

/* USB endpoint indexes (use define rather than enum to expand them) */
#define USB_EP_CONTROL   0
#define USB_EP_CONSOLE   1

/*
 * Endpoint 2 is missing because the console used to use two bidirectional
 * endpoints.  It now uses a single bidirectional endpoint relying on the
 * direction bit as an additional bit identifying the endpoint used.  It is
 * safe to reallocate endpoint 2 in the future.
 */

#ifdef HAS_TASK_SNIFFER
#define USB_EP_SNIFFER   3
#define USB_EP_COUNT     4
#else
#define USB_EP_COUNT     2
#endif

#endif /* __BOARD_H */
