/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* ryu board configuration */

#ifndef __BOARD_H
#define __BOARD_H

/* 48 MHz SYSCLK clock frequency */
#define CPU_CLOCK 48000000

/* the UART console is on USART2 (PD4/PD5) */
#undef CONFIG_UART_CONSOLE
#define CONFIG_UART_CONSOLE 2

/* By default, enable all console messages excepted USB */
#define CC_DEFAULT     (CC_ALL & ~(CC_MASK(CC_USB) | CC_MASK(CC_LIGHTBAR)))

/* Optional features */
#undef CONFIG_CMD_HASH
#define CONFIG_CHARGE_MANAGER
#define CONFIG_FORCE_CONSOLE_RESUME
#define CONFIG_STM_HWTIMER32
#define CONFIG_USB_POWER_DELIVERY
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_FLASH_ERASE_CHECK
#define CONFIG_USB_PD_INTERNAL_COMP
#define CONFIG_USB_SWITCH_PI3USB9281
#define CONFIG_USBC_SS_MUX
#define CONFIG_USBC_VCONN
#define CONFIG_ADC
#define CONFIG_ADC_SAMPLE_TIME 3
#define CONFIG_HW_CRC
#define CONFIG_I2C
#define CONFIG_LID_SWITCH
#define CONFIG_LOW_POWER_IDLE
#define CONFIG_VBOOT_HASH
#define CONFIG_WATCHDOG_HELP
#undef CONFIG_TASK_PROFILING
#define CONFIG_INDUCTIVE_CHARGING
#undef CONFIG_HIBERNATE
#undef CONFIG_UART_TX_DMA /* DMAC_CH7 is used by USB PD */
#define CONFIG_UART_RX_DMA
#define CONFIG_UART_RX_DMA_CH STM32_DMAC_USART2_RX

/* Charging/Power configuration */
#define CONFIG_BATTERY_RYU
#define CONFIG_BATTERY_BQ27541
#define CONFIG_BATTERY_CUT_OFF
#define CONFIG_BATTERY_REQUESTS_NIL_WHEN_DEAD
#define CONFIG_BATTERY_REVIVE_DISCONNECT
#define CONFIG_CHARGER
#define CONFIG_CHARGER_V2
#define CONFIG_CHARGER_BQ25892
#define CONFIG_CHARGER_BQ2589X_BOOST (BQ2589X_BOOSTV_MV(4998) | \
				      BQ2589X_BOOST_LIM_1650MA)
#define CONFIG_CHARGER_ILIM_PIN_DISABLED
#define CONFIG_CHARGER_PROFILE_OVERRIDE
#define CONFIG_CHARGER_INPUT_CURRENT 512
#define CONFIG_CHARGER_DISCHARGE_ON_AC
#define CONFIG_CHIPSET_TEGRA
#define CONFIG_PMIC_FW_LONG_PRESS_TIMER
#define CONFIG_POWER_COMMON
#define CONFIG_POWER_BUTTON
#define CONFIG_POWER_BUTTON_ACTIVE_STATE 1

/* I2C ports configuration */
#define I2C_PORT_MASTER 0
#define I2C_PORT_SLAVE  1
#define I2C_PORT_EC I2C_PORT_SLAVE
#define I2C_PORT_CHARGER I2C_PORT_MASTER
#define I2C_PORT_BATTERY I2C_PORT_MASTER
#define I2C_PORT_LIGHTBAR I2C_PORT_MASTER

/* slave address for host commands */
#ifdef HAS_TASK_HOSTCMD
#define CONFIG_HOSTCMD_I2C_SLAVE_ADDR 0x3c
#endif

/* USART and USB stream drivers */
#define CONFIG_STREAM_USART
#define CONFIG_STREAM_USART1
#define CONFIG_STREAM_USART3
#define CONFIG_STREAM_USB

/* USB Configuration */
#define CONFIG_USB
#define CONFIG_USB_PID 0x500f

/* Prevent the USB driver from initializing at boot */
#define CONFIG_USB_INHIBIT_INIT

/* USB interface indexes (use define rather than enum to expand them) */
#define USB_IFACE_CONSOLE   0
#define USB_IFACE_AP_STREAM 1
#define USB_IFACE_SH_STREAM 2
#define USB_IFACE_SPI       3
#define USB_IFACE_COUNT     4

/* USB endpoint indexes (use define rather than enum to expand them) */
#define USB_EP_CONTROL   0
#define USB_EP_CONSOLE   1
#define USB_EP_AP_STREAM 2
#define USB_EP_SH_STREAM 3
#define USB_EP_SPI       4
#define USB_EP_COUNT     5

/* Enable console over USB */
#define CONFIG_USB_CONSOLE

/* Enable control of SPI over USB */
#define CONFIG_SPI_MASTER_PORT 2
#define CONFIG_SPI_CS_GPIO     GPIO_SPI_FLASH_NSS

#define CONFIG_USB_SPI

/* Enable Case Closed Debugging */
#define CONFIG_CASE_CLOSED_DEBUG

/* Maximum number of deferrable functions */
#undef  DEFERRABLE_MAX_COUNT
#define DEFERRABLE_MAX_COUNT 14

#ifndef __ASSEMBLER__

int board_get_version(void);

/* Timer selection */
#define TIM_CLOCK32 5
#define TIM_WATCHDOG 19

#include "gpio_signal.h"

enum power_signal {
	TEGRA_XPSHOLD = 0,
	TEGRA_SUSPEND_ASSERTED,

	/* Number of power signals */
	POWER_SIGNAL_COUNT
};

/* ADC signal */
enum adc_channel {
	ADC_VBUS = 0,
	ADC_CC1_PD,
	ADC_CC2_PD,
	/* Number of ADC channels */
	ADC_CH_COUNT
};

/* Charge suppliers */
enum charge_supplier {
	CHARGE_SUPPLIER_PD,
	CHARGE_SUPPLIER_TYPEC,
	CHARGE_SUPPLIER_BC12_DCP,
	CHARGE_SUPPLIER_BC12_CDP,
	CHARGE_SUPPLIER_BC12_SDP,
	CHARGE_SUPPLIER_PROPRIETARY,
	CHARGE_SUPPLIER_OTHER,
	CHARGE_SUPPLIER_VBUS,
	CHARGE_SUPPLIER_COUNT
};

/* supplier_priority table defined in board.c */
extern const int supplier_priority[];

/* USB string indexes */
enum usb_strings {
	USB_STR_DESC = 0,
	USB_STR_VENDOR,
	USB_STR_PRODUCT,
	USB_STR_VERSION,
	USB_STR_CONSOLE_NAME,
	USB_STR_AP_STREAM_NAME,
	USB_STR_SH_STREAM_NAME,

	USB_STR_COUNT
};

/* Discharge battery when on AC power for factory test. */
int board_discharge_on_ac(int enable);

/* Set the charge current limit. */
void board_set_charge_limit(int charge_ma);

/* PP1800 transition GPIO interrupt handler */
void pp1800_on_off_evt(enum gpio_signal signal);

#endif /* !__ASSEMBLER__ */

#endif /* __BOARD_H */
