/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* kronitech board configuration */

#ifndef __BOARD_H
#define __BOARD_H

/*==============================================================================*/
/* 4 board types configurations
 * Emulation / Host  : CONFIG_BIZ_EMU_HOST (auto set: CONFIG_BIZ_DUAL_CC)
 * Emulation / Dock  : CONFIG_BIZ_EMU_DOCK
 * Kronitech, P-DC   : 
 * Kronitech, P-TypC : CONFIG_BIZ_DUAL_CC
 */
#if 0
#define CONFIG_BIZ_EMU_HOST    /* HOST emulation over STM32-Discovery board */
#define CONFIG_BIZ_EMU_DOCK    /* HOST emulation over STM32-Discovery board */
#define CONFIG_BIZ_DUAL_CC     /* there are 2 CC ports */
#endif
#include "biz_board.conf"

/*--------------------------------------------------------------------------*/
#ifdef CONFIG_BIZ_EMU_HOST
#define __BIZ_EMU_BUILD__      /* emulation over STM32-Discovery board */
#define CONFIG_BIZ_DUAL_CC     /* there are 2 CC ports */
#define CONFIG_USB_PD_ALT_MODE_DFP
#endif

#ifdef CONFIG_BIZ_EMU_DOCK
#define __BIZ_EMU_BUILD__      /* emulation over STM32-Discovery board */
#endif

#define GPIO_PD_SBU_ENABLE     GPIO_USB_P0_SBU_ENABLE   /* get same SBU_ENABLE gpio name as HOHO */

#if 0
#define __BIZ_SPICLK_USE_PB9__
#else
#endif

/*==============================================================================*/
/* 48 MHz SYSCLK clock frequency */
#define CPU_CLOCK 48000000

/* the UART console is on USART1 (PA9/PA10) */
#undef  CONFIG_UART_CONSOLE
#define CONFIG_UART_CONSOLE 1

/* Optional features */
#define CONFIG_ADC
#define CONFIG_BOARD_PRE_INIT
/* Minimum ilim = 500 mA */
#define CONFIG_CHARGER_INPUT_CURRENT PWM_0_MA
#define CONFIG_HW_CRC
#define CONFIG_RSA
#define CONFIG_RWSIG
#define CONFIG_SHA256
#define CONFIG_I2C
#undef  CONFIG_LID_SWITCH
#define CONFIG_STM_HWTIMER32
#undef  CONFIG_TASK_PROFILING
#define CONFIG_USB
#define CONFIG_USB_CONSOLE
#define CONFIG_USB_BOS
#undef  CONFIG_USB_INHIBIT_CONNECT
#define CONFIG_USB_POWER_DELIVERY

#if CONFIG_BIZ_HULK_V2_0_HW_TYPE !=  CONFIG_BIZ_HULK_V2_0_TYPE_RJ45
#define CONFIG_USB_PD_ALT_MODE
#endif

#define CONFIG_USB_PD_HW_DEV_ID_BOARD_MAJOR USB_PD_HW_DEV_ID_HOHO
#define CONFIG_USB_PD_HW_DEV_ID_BOARD_MINOR 2
#define CONFIG_USB_PD_CUSTOM_VDM
#define CONFIG_USB_PD_DUAL_ROLE
#define CONFIG_USB_PD_INTERNAL_COMP
#define CONFIG_USB_PD_IDENTITY_HW_VERS 1
#define CONFIG_USB_PD_IDENTITY_SW_VERS 1
#define CONFIG_USBC_SS_MUX
#define CONFIG_USBC_VCONN
#undef  CONFIG_WATCHDOG_HELP
#define CONFIG_USB_PD_DYNAMIC_SRC_CAP
#define CONFIG_BIZLINK_DEFINE_DUAL_DRP_STATE
/*
 * DFP advised power (Type-C spec: Table 4-18 ~ 4-20)
 *-----------------------------------------------------
#define CONFIG_USB_PD_ADVPWR_DEFAULT    // Default USB Power
#define CONFIG_USB_PD_ADVPWR_1A5
 */
#define CONFIG_USB_PD_ADVPWR_3A

#define CONFIG_PD_USE_DAC_AS_REF
#ifdef CONFIG_BIZ_EMU_DOCK
#define CONFIG_PD_DAC_CR           STM32_DAC_CR_EN2      /* DAC_OUT2 PA5 */
#define CONFIG_PD_DAC_VOUT         (550 * 4096 / 3300) << 16   /* STD: 550 */
#else
#define CONFIG_PD_DAC_CR           STM32_DAC_CR_EN1      /* DAC_OUT1 PA4 */
#define CONFIG_PD_DAC_VOUT         (550 * 4096 / 3300)   /* STD: 550 */
#endif

/* I2C ports configuration */
#define I2C_PORT_MASTER 1
#define I2C_PORT_SLAVE  0
#define I2C_PORT_EC I2C_PORT_SLAVE

/* USB configuration */
#define CONFIG_USB_VENDOR_STR  "BizLink Technology Inc."
#define CONFIG_USB_PRODUCT_STR "Hulk"
#define CONFIG_USB_BB_URL      USB_BIZLINK_TYPEC_URL
#define CONFIG_USB_VID         USB_VID_BIZLINK
#define CONFIG_USB_PID 0x6001
#define CONFIG_USB_BCD_DEV 0x0001 /* v 0.01 */

/* slave address for host commands */
#ifdef HAS_TASK_HOSTCMD
#define CONFIG_HOSTCMD_I2C_SLAVE_ADDR CONFIG_USB_PD_I2C_SLAVE_ADDR
#endif

/*
 * Allow dangerous commands all the time, since we don't have a write protect
 * switch.
 */
#define CONFIG_SYSTEM_UNLOCKED

#ifndef __ASSEMBLER__

/* Timer selection */
#define TIM_CLOCK32 2
#define TIM_ADC     3

#include "gpio_signal.h"

/* ADC signal */
enum adc_channel {
	ADC_P0_CC_PD = 0,
	ADC_P1_CC1_PD,
	ADC_P1_CC2_PD,
	ADC_BOOSTIN,
	ADC_P0_VBUS_DT,
	ADC_P1_VBUS_DT,
	ADC_P1_20VBUS_DT,
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

/* Charge current limit min / max, based on PWM duty cycle */
#define PWM_0_MA	500
#define PWM_100_MA	4000

/* Map current in milli-amps to PWM duty cycle percentage */
#define MA_TO_PWM(curr) (((curr) - PWM_0_MA) * 100 / (PWM_100_MA - PWM_0_MA))

#endif /* __BOARD_H */
