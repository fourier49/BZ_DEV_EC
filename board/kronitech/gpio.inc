/* -*- mode:c -*-
 *
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

//-------------------------------------------------------------------------------------
// Dual CC configuration:
//    port 0: UFP / plug
//    port 1: DFP / receptacle
//-------------------------------------------------------------------------------------
/* CC Signal RX/TX Port 0 */
#ifndef CONFIG_BIZ_EMU_HOST
GPIO(USB_P0_CC_PD,           A, 0,  GPIO_ANALOG,   NULL)
GPIO(USB_P0_CC_TX_DATA,      B, 4,  GPIO_OUT_LOW,  NULL)
GPIO(USB_P0_CC_TX_EN,        B, 0,  GPIO_OUT_LOW,  NULL)
#endif
GPIO(USB_CCRX_REF,           A, 1,  GPIO_ANALOG,   NULL)
GPIO(USB_CCRX_REF_OUT,       A, 4,  GPIO_ANALOG,   NULL)

/* CC Signal RX/TX Port 1 */
#ifdef CONFIG_BIZ_DUAL_CC
GPIO(USB_P1_CC1_PD,          A, 2,  GPIO_ANALOG,   NULL)
GPIO(USB_P1_CC2_PD,          A, 5,  GPIO_ANALOG,   NULL)
GPIO(USB_P1_CC_TX_DATA,      B, 14, GPIO_OUT_LOW,  NULL)
GPIO(USB_P1_CC_TX_EN,        A, 7,  GPIO_OUT_LOW,  NULL)

/* USB CC1/CC2 Polarity (orientation) */
GPIO(USB_P1_CC_POLARITY,     A, 8,  GPIO_OUT_LOW,  NULL)
#endif

//-------------------------------------------------------------------------------------
// CC Port 0
//-------------------------------------------------------------------------------------
/* USB/DP mux signals */
#ifndef CONFIG_BIZ_EMU_HOST
GPIO(USB_P0_DP_SS_LANE,      B, 7,  GPIO_OUT_LOW,  NULL)
GPIO(USB_P0_SBU_ENABLE,      A, 15, GPIO_OUT_LOW,  NULL)
#if defined(CONFIG_USB_PD_ALT_MODE) && !defined(CONFIG_USB_PD_ALT_MODE_DFP)
GPIO(USB_P0_DP_HPD,          A, 3,  GPIO_INT_BOTH, hpd_event)
#endif

/* Host/Device mode handling (presentation of Rp or Rd by switch) */
GPIO(USB_P0_PWROLE_SRC,      B, 10, GPIO_OUT_LOW,  NULL)   /* active HIGH */
GPIO(USB_P0_VBUS_WAKE,       B, 12, GPIO_INT_BOTH, vbus0_evt)

/* UFP Power Provider */    
GPIO(USB_P0_PWR_5V_EN,       A, 6,  GPIO_OUT_HIGH, NULL)   /* active HIGH, PD Provider */
GPIO(USB_P0_PWR_20V_EN,      B, 2,  GPIO_OUT_HIGH, NULL)   /* active HIGH, PD Provider */
GPIO(USB_P0_PWR_VBUS_EN,     B, 8,  GPIO_OUT_HIGH, NULL)   /* active HIGH, PD Consumer */
#endif

//-------------------------------------------------------------------------------------
// CC Port 1
//-------------------------------------------------------------------------------------
#ifdef CONFIG_BIZ_EMU_HOST
/* USB/DP mux signals */
GPIO(USB_P1_DP_SS_LANE,      A, 3,  GPIO_OUT_LOW,  NULL)
GPIO(USB_P1_DP_HPD,          B, 0,  GPIO_OUT_LOW,  NULL)
GPIO(USB_P1_SBU_ENABLE,      B, 2,  GPIO_OUT_LOW,  NULL)

/* DFP VBUS/VCONN signals */
GPIO(USB_P1_CC1_VCONN_EN,    B, 4,  GPIO_OUT_LOW,  NULL)   /* active HIGH */
GPIO(USB_P1_CC2_VCONN_EN,    B, 5,  GPIO_OUT_LOW,  NULL)

/* Host/Device mode handling (presentation of Rp or Rd by switch) */
GPIO(USB_P1_CC1_PWROLE_SRC,  A, 15, GPIO_OUT_HIGH,  NULL)
GPIO(USB_P1_CC2_PWROLE_SRC,  A, 6,  GPIO_OUT_HIGH,  NULL)

/* DFP Power Provider */   
GPIO(USB_P1_PWR_5V_EN,       B, 7,  GPIO_OUT_HIGH,  NULL)   /* DFP providing 5V  over VBUS to UFP, active HIGH */
GPIO(USB_P1_PWR_20V_EN,      B, 6,  GPIO_OUT_LOW,  NULL)    /* DFP providing 20V over VBUS to UFP, active HIGH */
GPIO(USB_P1_PWR_VBUS_EN,     B, 8,  GPIO_OUT_LOW,  NULL)    /* DFP get power from VBUS by UFP,     active HIGH */
#endif

#ifdef CONFIG_BIZ_DUAL_CC
GPIO(USB_P1_VBUS_WAKE,       C, 10, GPIO_INT_BOTH, vbus1_evt)
#endif
GPIO(MCU_DBG_TRIGGER,        C, 11, GPIO_OUT_LOW, NULL)

/* Misc signals */
#ifndef CONFIG_BIZ_EMU_HOST
GPIO(MCU_CHIPS_RESET_EN,     B, 11, GPIO_OUT_HIGH, NULL)
GPIO(MCU_PWR_DC_IN_DET,      B, 5,  GPIO_INT_BOTH, pwr_in_event)
GPIO(MCU_PWR_STDBY_HUB,      A, 7,  GPIO_OUT_HIGH, NULL)
GPIO(MCU_PWR_STDBY_LAN,      B, 6,  GPIO_OUT_HIGH, NULL)
#ifdef __BIZ_SPICLK_USE_PB9__
GPIO(MCU_PWR_STDBY_DP,       B, 1,  GPIO_OUT_HIGH, NULL)
#else
GPIO(MCU_PWR_STDBY_DP,       B, 9,  GPIO_OUT_HIGH, NULL)
#endif
#endif

#if 0
/*
 * I2C pins should be configured as inputs until I2C module is
 * initialized. This will avoid driving the lines unintentionally.
 */
GPIO(SLAVE_I2C_SCL,          B, 6,  GPIO_INPUT,    NULL)
GPIO(SLAVE_I2C_SDA,          B, 7,  GPIO_INPUT,    NULL)
GPIO(MASTER_I2C_SCL,         B, 10, GPIO_INPUT,    NULL)
GPIO(MASTER_I2C_SDA,         B, 11, GPIO_INPUT,    NULL)
#endif

GPIO(USB_DM,                 A, 11, GPIO_ANALOG,   NULL)
GPIO(USB_DP,                 A, 12, GPIO_ANALOG,   NULL)

/* Unimplemented signals which we need to emulate for now */
UNIMPLEMENTED(ENTERING_RW)
UNIMPLEMENTED(WP_L)

//-------------------------------------------------------------------------------------
ALTERNATE(B, 0x0008, 0, MODULE_USB_PD, 0) /* SPI1: SCK(PB3),  USB_P0_SPI1_SCK */
ALTERNATE(B, 0x2000, 0, MODULE_USB_PD, 0) /* SPI2: SCK(PB13), USB_P1_SPI2_SCK */
#ifdef __BIZ_SPICLK_USE_PB9__
ALTERNATE(B, 0x0200, 2, MODULE_USB_PD, 0) /* TIM17_CH1: PB9,  USB_CCTX_SPICLK_SRC */
#else
ALTERNATE(B, 0x0002, 0, MODULE_USB_PD, 0) /* TIM14_CH1: PB1,  USB_CCTX_SPICLK_SRC */
#endif
ALTERNATE(A, 0x0600, 1, MODULE_UART,   0) /* USART1: PA9/PA10 */
#if 0
ALTERNATE(B, 0x0cc0, 1, MODULE_I2C,    0) /* I2C SLAVE:PB6/7 MASTER:PB10/11 */
#endif
