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
/* CC Signal RX */
GPIO(USB_P0_CC_PD,           A, 0,  GPIO_ANALOG,   NULL)
GPIO(USB_P1_CC1_PD,          A, 2,  GPIO_ANALOG,   NULL)
GPIO(USB_P1_CC2_PD,          A, 5,  GPIO_ANALOG,   NULL)
GPIO(USB_CCRX_REF,           A, 1,  GPIO_ANALOG,   NULL)
GPIO(USB_CCRX_REF_OUT,       A, 4,  GPIO_ANALOG,   NULL)

/* CC Signal TX */
GPIO(USB_P0_CC_TX_DATA,      B, 4,  GPIO_OUT_LOW,  NULL)
GPIO(USB_P1_CC_TX_DATA,      B, 14, GPIO_OUT_LOW,  NULL)
GPIO(USB_P0_CC_TX_EN,        B, 0,  GPIO_OUT_LOW,  NULL)
GPIO(USB_P1_CC_TX_EN,        B, 1,  GPIO_OUT_LOW,  NULL)

/* USB CC1/CC2 Polarity (orientation) */
GPIO(USB_P1_CC_POLARITY,     A, 8,  GPIO_OUT_LOW,  NULL)

//-------------------------------------------------------------------------------------
// CC Port 0
//-------------------------------------------------------------------------------------
/* USB/DP mux signals */
GPIO(USB_P0_DP_SS_LANE,      B, 7,  GPIO_OUT_LOW,  NULL)
GPIO(USB_P0_SBU_ENABLE,      A, 15, GPIO_OUT_LOW,  NULL)
#if defined(CONFIG_USB_PD_ALT_MODE) && !defined(CONFIG_USB_PD_ALT_MODE_DFP)
GPIO(USB_P0_DP_HPD,          A, 3,  GPIO_INT_BOTH, hpd_event)
#endif

/* Host/Device mode handling (presentation of Rp or Rd by switch) */
GPIO(USB_P0_PWROLE_SRC,      B, 10, GPIO_ODR_LOW,  NULL)
GPIO(USB_P0_VBUS_WAKE,       B, 12, GPIO_INT_BOTH, vbus0_evt)

/* UFP Power Provider */    
GPIO(USB_P0_PWR_5V_EN,       A, 6,  GPIO_OUT_HIGH, NULL)
GPIO(USB_P0_PWR_20V_EN,      B, 2,  GPIO_OUT_HIGH, NULL)
GPIO(USB_P0_PWR_VBUS_EN,     B, 8,  GPIO_OUT_HIGH, NULL)

//-------------------------------------------------------------------------------------
// CC Port 1
//-------------------------------------------------------------------------------------
#ifdef CONFIG_BIZ_EMU_HOST
/* USB/DP mux signals */
GPIO(USB_P1_DP_SS_LANE,      C, 0,  GPIO_OUT_LOW,  NULL)
GPIO(USB_P1_DP_HPD,          C, 1,  GPIO_OUT_LOW,  NULL)
GPIO(USB_P1_SBU_ENABLE,      C, 2,  GPIO_OUT_LOW,  NULL)

/* DFP VBUS/VCONN signals */
GPIO(USB_P1_CC1_VCONN_EN_L,  C, 3,  GPIO_OUT_LOW,  NULL)
GPIO(USB_P1_CC2_VCONN_EN_L,  C, 4,  GPIO_OUT_LOW,  NULL)

/* Host/Device mode handling (presentation of Rp or Rd by switch) */
GPIO(USB_P1_CC1_PWROLE_SRC,  C, 5, GPIO_ODR_LOW,  NULL)
GPIO(USB_P1_CC2_PWROLE_SRC,  C, 6, GPIO_ODR_LOW,  NULL)

/* DFP Power Provider */   
GPIO(USB_P1_PWR_5V_EN,       C, 7,  GPIO_OUT_HIGH, NULL)
GPIO(USB_P1_PWR_20V_EN,      C, 8,  GPIO_OUT_HIGH, NULL)
GPIO(USB_P1_PWR_VBUS_EN,     C, 9,  GPIO_OUT_HIGH, NULL)
#endif

#ifdef CONFIG_BIZ_DUAL_CC
GPIO(USB_P1_VBUS_WAKE,       C, 10, GPIO_INT_BOTH, vbus1_evt)
#endif

/* Misc signals */
GPIO(MCU_CHIPS_RESET_EN,     B, 11, GPIO_OUT_LOW,  NULL)
GPIO(MCU_PWR_DC_IN_DET,      B, 5,  GPIO_INT_BOTH, pwr_in_event)
GPIO(MCU_PWR_STDBY_HUB,      A, 7,  GPIO_OUT_HIGH, NULL)
GPIO(MCU_PWR_STDBY_LAN,      B, 6,  GPIO_OUT_HIGH, NULL)
GPIO(MCU_PWR_STDBY_DP,       B, 9,  GPIO_OUT_HIGH, NULL)

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
ALTERNATE(B, 0x0200, 2, MODULE_USB_PD, 0) /* TIM17_CH1: PB9,  USB_CCTX_SPICLK_SRC */
ALTERNATE(A, 0x0600, 1, MODULE_UART,   0) /* USART1: PA9/PA10 */
#if 0
ALTERNATE(B, 0x0cc0, 1, MODULE_I2C,    0) /* I2C SLAVE:PB6/7 MASTER:PB10/11 */
#endif
