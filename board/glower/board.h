/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* Glower board configuration */

#ifndef __BOARD_H
#define __BOARD_H

/* Optional features */
/* #define CONFIG_BACKLIGHT_LID */
/* #define CONFIG_BATTERY_SMART */
#define CONFIG_BOARD_VERSION

#if 0
#define CONFIG_CHARGER
#define CONFIG_CHARGER_V1
#define CONFIG_CHARGER_BQ24715
#define CONFIG_CHARGER_DISCHARGE_ON_AC
#define CONFIG_CHARGER_INPUT_CURRENT 1700   /* 33 W adapter, 19 V, 1.75 A */
#define CONFIG_CHARGER_SENSE_RESISTOR 10    /* Charge sense resistor, mOhm */
#define CONFIG_CHARGER_SENSE_RESISTOR_AC 10 /* Input senso resistor, mOhm */
#endif

#define CONFIG_CHIPSET_BAYTRAIL
#define CONFIG_CHIPSET_CAN_THROTTLE
#define CONFIG_POWER_COMMON
/* #define CONFIG_CMD_GSV */
#define CONFIG_EXTPOWER_GPIO
#define CONFIG_I2C_PASSTHRU_RESTRICTED
#define CONFIG_KEYBOARD_COL2_INVERTED
#define CONFIG_KEYBOARD_IRQ_GPIO GPIO_KBD_IRQ_L
#define CONFIG_KEYBOARD_PROTOCOL_8042
/* #define CONFIG_LED_COMMON */
#define CONFIG_POWER_BUTTON
#define CONFIG_POWER_BUTTON_X86
#define CONFIG_USB_PORT_POWER_IN_S3
#define CONFIG_USB_PORT_POWER_SMART
#define CONFIG_USB_PORT_POWER_SMART_SIMPLE
/* #define CONFIG_VBOOT_HASH */
/*
#define CONFIG_WIRELESS
#define CONFIG_WIRELESS_SUSPEND \
	(EC_WIRELESS_SWITCH_WLAN | EC_WIRELESS_SWITCH_WLAN_POWER)
	*/

#define CONFIG_SYSTEM_UNLOCKED  /* Allow dangerous commands */
#define CONFIG_WATCHDOG_HELP
#define CONFIG_CLOCK_CRYSTAL
#define CONFIG_WAKE_PIN GPIO_POWER_BUTTON_L
#define CONFIG_SPI_PORT 1
#define CONFIG_SPI_CS_GPIO GPIO_PVT_CS0

#undef  CONFIG_CONSOLE_CMDHELP
#undef CONFIG_TASK_PROFILING

/* Modules we want to exclude */
#undef CONFIG_ADC
#undef CONFIG_EEPROM
#undef CONFIG_EOPTION
#undef CONFIG_PECI
#undef CONFIG_PSTORE
#undef CONFIG_PWM

#ifndef __ASSEMBLER__

/* I2C ports */
#define I2C_PORT_BATTERY 1
#define I2C_PORT_CHARGER 1
#define I2C_PORT_THERMAL 2

/* USB ports */
#define USB_PORT_COUNT 2

/* Wireless signals */
#define WIRELESS_GPIO_WLAN GPIO_WLAN_OFF_L
#define WIRELESS_GPIO_WWAN GPIO_PP3300_LTE_EN
#define WIRELESS_GPIO_WLAN_POWER GPIO_PP3300_WLAN_EN

#include "gpio_signal.h"

/* power signal definitions */
enum power_signal {
	X86_PGOOD_PP1050 = 0,
	X86_PGOOD_PP3300_PCH,
	X86_PGOOD_PP5000,
	X86_PGOOD_S5,
	X86_PGOOD_VCORE,
	X86_PGOOD_PP1000_S0IX,
	X86_SLP_S3_DEASSERTED,
	X86_SLP_S4_DEASSERTED,
#ifdef CONFIG_CHIPSET_DEBUG
	X86_SLP_SX_DEASSERTED,
	X86_SUS_STAT_ASSERTED,
	X86_SUSPWRDNACK_ASSERTED,
#endif

	/* Number of X86 signals */
	POWER_SIGNAL_COUNT
};

/* Discharge battery when on AC power for factory test. */
int board_discharge_on_ac(int enable);

#endif /* !__ASSEMBLER__ */

#endif /* __BOARD_H */
