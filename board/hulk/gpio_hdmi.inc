/* -*- mode:c -*-
 *
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

//-------------------------------------------------------------------------------------
// ALL GPIOs with interrupt handlers must be declared at the top of the gpio.inc
//-------------------------------------------------------------------------------------
#if defined(CONFIG_USB_PD_ALT_MODE) && !defined(CONFIG_USB_PD_ALT_MODE_DFP)
GPIO_INT(USB_P0_DP_HPD,       B, 5,  GPIO_INT_BOTH, hpd_event) 
#endif


//-------------------------------------------------------------------------------------
// Dual CC configuration:
//    port 0: UFP / plug
//    port 1: DFP / receptacle
//-------------------------------------------------------------------------------------
/* CC Signal RX/TX Port 0 */
GPIO(USB_P0_CC_PD,           A, 0,  GPIO_ANALOG)
GPIO(USB_P0_CC_TX_DATA,      B, 4,  GPIO_OUT_LOW)
GPIO(USB_P0_CC_TX_EN,        B, 0,  GPIO_OUT_LOW)
GPIO(USB_CCRX_REF,           A, 1,  GPIO_ANALOG)
GPIO(USB_CCRX_REF_OUT,       A, 4,  GPIO_ANALOG)

/* CC Signal RX/TX Port 1 */
#ifdef CONFIG_BIZ_DUAL_CC
GPIO(USB_P1_CC1_PD,          A, 2,  GPIO_ANALOG)
GPIO(USB_P1_CC2_PD,          A, 5,  GPIO_ANALOG)
GPIO(USB_P1_CC_TX_DATA,      B, 14, GPIO_OUT_LOW)
GPIO(USB_P1_CC_TX_EN,        B, 10, GPIO_OUT_LOW)
GPIO(USB_P1_CC_POLARITY,     B, 12, GPIO_OUT_LOW)/* USB CC1/CC2 Polarity (orientation) */
#endif

//-------------------------------------------------------------------------------------
// CC Port 0
//-------------------------------------------------------------------------------------
/* USB/DP mux signals */
GPIO(USB_P0_SBU_ENABLE,      B, 9, GPIO_OUT_LOW)
GPIO(USB_P0_PWROLE_SRC,      A, 8, GPIO_OUT_LOW)

/* UFP Power Provider */    
GPIO(USB_P0_PWR_20V_EN,      B, 2, GPIO_OUT_LOW)
GPIO(USB_P0_PWR_5V_EN,       A, 7, GPIO_OUT_LOW) 
//-------------------------------------------------------------------------------------
// CC Port 1
//-------------------------------------------------------------------------------------
GPIO(USB_VBUS_DETECT,        A, 3, GPIO_ANALOG)
GPIO(USB_C_RECEPTACLE_DETECT,A, 6, GPIO_ANALOG)
GPIO(USB_C_VBUS_DETECT,      A, 15,GPIO_ANALOG)

/* Discarge signals */
GPIO(UP_Discharge,           C, 13, GPIO_OUT_LOW)
GPIO(VBUS_UP_CTRL1,          C, 15, GPIO_OUT_HIGH)
GPIO(DS_Discharge,           B, 15, GPIO_OUT_LOW)
GPIO(VBUS_DS_CTRL1,          C, 14, GPIO_OUT_LOW)
GPIO(LED_CONTROL,            B, 11, GPIO_OUT_HIGH)	
GPIO(MCDP_RESET_L,           B, 8,  GPIO_OUT_HIGH)

GPIO(MASTER_I2C_SCL,         B, 6, GPIO_INPUT)
GPIO(MASTER_I2C_SDA,         B, 7, GPIO_INPUT)

GPIO(USB_DM,                 A, 11, GPIO_ANALOG)
GPIO(USB_DP,                 A, 12, GPIO_ANALOG)

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
#ifdef CONFIG_BIZ_HULK_V2_0
ALTERNATE(B, 0x00C0, 1, MODULE_I2C,    0) /* I2C PB6/7 */
#endif

