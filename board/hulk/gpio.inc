/* -*- mode:c -*-
 *
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

//-------------------------------------------------------------------------------------
// ALL GPIOs with interrupt handlers must be declared at the top of the gpio.inc
//-------------------------------------------------------------------------------------
#ifndef CONFIG_BIZ_EMU_HOST
#if defined(CONFIG_USB_PD_ALT_MODE) && !defined(CONFIG_USB_PD_ALT_MODE_DFP)
GPIO_INT(USB_P0_DP_HPD,          B, 5,  GPIO_INT_BOTH, hpd_event) /*Hulk , high means plug-in*/
#endif
//GPIO_INT(USB_P0_VBUS_WAKE,       B, 12, GPIO_INT_BOTH, vbus0_evt)
//GPIO_INT(MCU_PWR_DC_IN_DET,      B, 12,  GPIO_INT_BOTH, pwr_in_event) /*Hulk didn't use this pin*/
#endif

#ifdef CONFIG_BIZ_DUAL_CC
//GPIO_INT(USB_P1_VBUS_WAKE,       C, 10, GPIO_INT_BOTH, vbus1_evt) //hulk nee p1 vbus wake up. add?
#endif

//-------------------------------------------------------------------------------------
// Dual CC configuration:
//    port 0: UFP / plug
//    port 1: DFP / receptacle
//-------------------------------------------------------------------------------------
/* CC Signal RX/TX Port 0 */
#ifndef CONFIG_BIZ_EMU_HOST
GPIO(USB_P0_CC_PD,           A, 0,  GPIO_ANALOG)	/*Hulk PA0 Port0(Plug CC)*/
GPIO(USB_P0_CC_TX_DATA,      B, 4,  GPIO_OUT_LOW)	/*Hulk PB4 Port0 TX Data , default low will enter receiving mode*/
GPIO(USB_P0_CC_TX_EN,        B, 0,  GPIO_OUT_LOW)    /*Hulk PB0 Port0 TX Enable , default low will enter receiving mode*/
#endif
GPIO(USB_CCRX_REF,           A, 1,  GPIO_ANALOG)	
GPIO(USB_CCRX_REF_OUT,       A, 4,  GPIO_ANALOG)	

/* CC Signal RX/TX Port 1 */
#ifdef CONFIG_BIZ_DUAL_CC
GPIO(USB_P1_CC1_PD,          A, 2,  GPIO_ANALOG)    /*Hulk PA2 Port1(Receptable) CC1 PD*/
GPIO(USB_P1_CC2_PD,          A, 5,  GPIO_ANALOG)    /*Hulk PA5 Port1(Receptable) CC2 PD*/
GPIO(USB_P1_CC_TX_DATA,      B, 14, GPIO_OUT_LOW)    /*Hulk PB14 Port1(Receptable) TX data*/
GPIO(USB_P1_CC_TX_EN,        A, 7,  GPIO_OUT_LOW)    /*Hulk PA7  Port1(Receptable) TX data enable*/

/* USB CC1/CC2 Polarity (orientation) */
GPIO(USB_P1_CC_POLARITY,     A, 8,  GPIO_OUT_LOW)   /*Hulk PA8  Port1(Receptable) orientation ,L:CC1 on ,H:CC2 on */
#endif

//-------------------------------------------------------------------------------------
// CC Port 0
//-------------------------------------------------------------------------------------
/* USB/DP mux signals */
#ifndef CONFIG_BIZ_EMU_HOST
//GPIO(USB_P0_DP_SS_LANE,      B, 7,  GPIO_OUT_LOW)  /*What's the purpose for this pin,don't use it on Hulk*/
GPIO(USB_P0_SBU_ENABLE,      B, 9, GPIO_OUT_LOW)		/*Hulk , high enable*/


/* Host/Device mode handling (presentation of Rp or Rd by switch) */
GPIO(USB_P0_PWROLE_SRC,      B, 7, GPIO_OUT_LOW)   /* Hulk , active HIGH */


/* UFP Power Provider */    
GPIO(USB_P0_PWR_5V_EN,       B, 6,  GPIO_OUT_LOW)   /* Hulk active HIGH, enable 5V to VBUS of port 0*/
GPIO(USB_P0_PWR_20V_EN,      B, 2,  GPIO_OUT_LOW)   /* Hulk active HIGH, enable 12V - 20V to VBUS of  port 0 */
//GPIO(USB_P0_PWR_VBUS_EN,     B, 8,  GPIO_OUT_LOW)   /* Hulk active HIGH, enable vbus to VBUS of  port 0 */      remove in v2.0
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
GPIO(USB_P1_PWR_5V_EN,         B, 7,  GPIO_OUT_LOW)   /* DFP providing 5V  over VBUS to UFP, active HIGH */
GPIO(USB_P1_PWR_20V_EN,        B, 6,  GPIO_OUT_LOW)    /* DFP providing 20V over VBUS to UFP, active HIGH */
//GPIO(USB_P1_PWR_VBUS_EN,     B, 8,  GPIO_OUT_LOW)    /* DFP get power from VBUS by UFP,     active HIGH */
#endif

#if defined(CONFIG_BIZ_DUAL_CC) && defined(CONFIG_BIZ_HULK)
//GPIO(USB_P0_MCU_WAKE,        B, 12, GPIO_ANALOG)		/*Hulk use to detect the voltage from Port1*/
#endif

GPIO(USB_VBUS_DETECT,            A, 3,  GPIO_ANALOG)    /*Hulk PA3 USB_VBUS_DETECT , vbus voltage after mos4*/
GPIO(USB_C_RECEPTACLE_DETECT,    A, 6,  GPIO_ANALOG)    /*Hulk PA6 RECEPTACLE VBUS*/
GPIO(USB_C_VBUS_DETECT,          A, 15,  GPIO_ANALOG)   /*Hulk PA15 PLUG VBUS*/

/* Discarge signals */
GPIO(UP_Discharge,     B, 11, GPIO_OUT_LOW)		/*Hulk ,port 0 discharge , high enable*/
GPIO(VBUS_UP_CTRL1,    C, 15, GPIO_OUT_LOW)		/*Hulk ,port 0 vbus power control . keep LOW*/
GPIO(DS_Discharge,     B, 15, GPIO_OUT_LOW)   	/*Hulk ,port 1 discharge , high enable*/
GPIO(VBUS_DS_CTRL1,    C, 14, GPIO_OUT_LOW)   	/*Hulk ,port 1 vbus power control . default LOW*/

/* Misc signals */
GPIO(LED_CONTROL,      C, 13, GPIO_OUT_HIGH)	/*Hulk ,LED control , high enable*/



#if 0
/*
 * I2C pins should be configured as inputs until I2C module is
 * initialized. This will avoid driving the lines unintentionally.
 */
GPIO(SLAVE_I2C_SCL,          B, 6,  GPIO_INPUT)
GPIO(SLAVE_I2C_SDA,          B, 7,  GPIO_INPUT)
GPIO(MASTER_I2C_SCL,         B, 10, GPIO_INPUT)
GPIO(MASTER_I2C_SDA,         B, 11, GPIO_INPUT)
#endif

GPIO(USB_DM,                 A, 11, GPIO_ANALOG)		/*Hulk ,usb*/
GPIO(USB_DP,                 A, 12, GPIO_ANALOG)		/*Hulk ,usb*/

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

