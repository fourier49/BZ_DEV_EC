/* -*- mode:c -*-
 *
 * Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
 
/*
* Following is the changed GPIO list between VGA and RJ45/HDMI/DP	
*RJ45  	
*1.USB_P1_CC_TX_EN	PA7->PB10
*2.USB_P1_CC_POLARITY	PA8->PB12
*3.USB_P0_DP_HPD	not used
*4.USB_P0_SBU_ENABLE	not used

*HDMI
*1.USB_P1_CC_TX_EN	PA7 -> PB10 
*2.USB_P1_CC_POLARITY	PA8 -> PB12
*3.USB_P0_PWR_5V_EN	PB6 -> PA7
*4.USB_P0_PWROLE_SRC	PB7 -> PA8
*5.I2C_CSCL		PB8 -> PB6
*6.I2C_CSDA		PB9 -> PB7
*7.MCDP_RESET_L		PB8(new)
*8.USB_P0_SBU_ENABLE	PB10 -> PB9

*DP
*1.PWR_RST 		PA7(new)
*2.PWRONZ  		PA8(new)
*3.USB_P0_DP_SS_LANE	PB8
*4.USB_P0_SBU_ENABLE	PB10->PB9
*5.USB_P1_CC_TX_EN	PA7->PB10
*6.USB_P1_CC_POLARITY	PA8->PB12
*7.LANE_SWITCH		PF0(new)
*/

#ifdef CONFIG_BIZ_HULK_V2_0

#if CONFIG_BIZ_HULK_V2_0_HW_TYPE ==  CONFIG_BIZ_HULK_V2_0_TYPE_VGA
#include "gpio_vga.inc"
#elif CONFIG_BIZ_HULK_V2_0_HW_TYPE ==  CONFIG_BIZ_HULK_V2_0_TYPE_RJ45
#include "gpio_rj45.inc"
#elif CONFIG_BIZ_HULK_V2_0_HW_TYPE ==  CONFIG_BIZ_HULK_V2_0_TYPE_HDMI
#include "gpio_hdmi.inc"
#elif CONFIG_BIZ_HULK_V2_0_HW_TYPE ==  CONFIG_BIZ_HULK_V2_0_TYPE_DP
#include "gpio_dp.inc"
#else
#error "Please set  CONFIG_BIZ_HULK_V2_0_HW_TYPE in biz_board.conf"
#endif

#else
#include "gpio_vga.inc"
#endif

