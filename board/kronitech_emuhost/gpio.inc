/* -*- mode:c -*-
 *
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

//-------------------------------------------------------------------------------------
// ALL GPIOs with interrupt handlers must be declared at the top of the gpio.inc
//-------------------------------------------------------------------------------------
#ifdef CONFIG_BIZ_DUAL_CC
GPIO_INT(USB_P1_VBUS_WAKE,       C, 10, GPIO_INT_BOTH, vbus1_evt)
#endif

//-------------------------------------------------------------------------------------
// Dual CC configuration:
//    port 0: UFP / plug
//    port 1: DFP / receptacle
//-------------------------------------------------------------------------------------
/* CC Signal RX/TX Port 0 */
GPIO(USB_CCRX_REF,           A, 1,  GPIO_ANALOG)
GPIO(USB_CCRX_REF_OUT,       A, 4,  GPIO_ANALOG)

/* CC Signal RX/TX Port 1 */
#ifdef CONFIG_BIZ_DUAL_CC
GPIO(USB_P1_CC1_PD,          A, 2,  GPIO_ANALOG)
GPIO(USB_P1_CC2_PD,          A, 5,  GPIO_ANALOG)
GPIO(USB_P1_CC_TX_DATA,      B, 14, GPIO_OUT_LOW)
GPIO(USB_P1_CC_TX_EN,        A, 7,  GPIO_OUT_LOW)

/* USB CC1/CC2 Polarity (orientation) */
GPIO(USB_P1_CC_POLARITY,     A, 8,  GPIO_OUT_LOW)
#endif

//-------------------------------------------------------------------------------------
// CC Port 1
//-------------------------------------------------------------------------------------
#ifdef CONFIG_BIZ_EMU_HOST
/* USB/DP mux signals */
GPIO(USB_P1_DP_SS_LANE,      A, 3,  GPIO_OUT_LOW)
GPIO(USB_P1_DP_HPD,          B, 0,  GPIO_OUT_LOW)
GPIO(USB_P1_SBU_ENABLE,      B, 2,  GPIO_OUT_LOW)

/* DFP VBUS/VCONN signals */
GPIO(USB_P1_CC1_VCONN_EN,    B, 4,  GPIO_OUT_LOW)   /* active HIGH */
GPIO(USB_P1_CC2_VCONN_EN,    B, 5,  GPIO_OUT_LOW)

/* Host/Device mode handling (presentation of Rp or Rd by switch) */
GPIO(USB_P1_CC1_PWROLE_SRC,  A, 15, GPIO_OUT_HIGH)
GPIO(USB_P1_CC2_PWROLE_SRC,  A, 6,  GPIO_OUT_HIGH)

/* DFP Power Provider */   
GPIO(USB_P1_PWR_5V_EN,       B, 7,  GPIO_OUT_HIGH)   /* DFP providing 5V  over VBUS to UFP, active HIGH */
GPIO(USB_P1_PWR_20V_EN,      B, 6,  GPIO_OUT_LOW)    /* DFP providing 20V over VBUS to UFP, active HIGH */
GPIO(USB_P1_PWR_VBUS_EN,     B, 8,  GPIO_OUT_LOW)    /* DFP get power from VBUS by UFP,     active HIGH */
#endif

//-------------------------------------------------------------------------------------
// Misc signals
//-------------------------------------------------------------------------------------
GPIO(MCU_DBG_TRIGGER,        C, 11, GPIO_OUT_LOW)

GPIO(USB_DM,                 A, 11, GPIO_ANALOG)
GPIO(USB_DP,                 A, 12, GPIO_ANALOG)

/* Unimplemented signals which we need to emulate for now */
UNIMPLEMENTED(ENTERING_RW)
UNIMPLEMENTED(WP_L)

//-------------------------------------------------------------------------------------
ALTERNATE(B, 0x0008, 0, MODULE_USB_PD, 0) /* SPI1: SCK(PB3),  USB_P0_SPI1_SCK */
ALTERNATE(B, 0x2000, 0, MODULE_USB_PD, 0) /* SPI2: SCK(PB13), USB_P1_SPI2_SCK */
ALTERNATE(B, 0x0002, 0, MODULE_USB_PD, 0) /* TIM14_CH1: PB1,  USB_CCTX_SPICLK_SRC */
ALTERNATE(A, 0x0600, 1, MODULE_UART,   0) /* USART1: PA9/PA10 */
#if 0
ALTERNATE(B, 0x0cc0, 1, MODULE_I2C,    0) /* I2C SLAVE:PB6/7 MASTER:PB10/11 */
#endif