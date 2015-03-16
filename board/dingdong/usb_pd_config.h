/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* USB Power delivery board configuration */

#ifndef __USB_PD_CONFIG_H
#define __USB_PD_CONFIG_H

/* Port and task configuration */
#define PD_PORT_COUNT 1
#define PORT_TO_TASK_ID(port) TASK_ID_PD
#define TASK_ID_TO_PORT(id)   0

/* Timer selection for baseband PD communication */
#define TIM_CLOCK_PD_TX_C0 17
#define TIM_CLOCK_PD_RX_C0 1

#define TIM_CLOCK_PD_TX(p) TIM_CLOCK_PD_TX_C0
#define TIM_CLOCK_PD_RX(p) TIM_CLOCK_PD_RX_C0

/* Timer channel */
#define TIM_RX_CCR_C0 1
#define TIM_TX_CCR_C0 1

/* RX timer capture/compare register */
#define TIM_CCR_C0 (&STM32_TIM_CCRx(TIM_CLOCK_PD_RX_C0, TIM_RX_CCR_C0))
#define TIM_RX_CCR_REG(p) TIM_CCR_C0

/* TX and RX timer register */
#define TIM_REG_TX_C0 (STM32_TIM_BASE(TIM_CLOCK_PD_TX_C0))
#define TIM_REG_RX_C0 (STM32_TIM_BASE(TIM_CLOCK_PD_RX_C0))
#define TIM_REG_TX(p) TIM_REG_TX_C0
#define TIM_REG_RX(p) TIM_REG_RX_C0

/* use the hardware accelerator for CRC */
#define CONFIG_HW_CRC

/* TX is using SPI1 on PB3-4 */
#define SPI_REGS(p) STM32_SPI1_REGS

static inline void spi_enable_clock(int port)
{
	STM32_RCC_APB2ENR |= STM32_RCC_PB2_SPI1;
}

/* SPI1_TX no remap needed */
#define DMAC_SPI_TX(p) STM32_DMAC_CH3

/* RX is using COMP1 triggering TIM1 CH1 */
#define CMP1OUTSEL STM32_COMP_CMP1OUTSEL_TIM1_IC1
#define CMP2OUTSEL 0

#define TIM_TX_CCR_IDX(p) TIM_TX_CCR_C0
#define TIM_RX_CCR_IDX(p) TIM_RX_CCR_C0
#define TIM_CCR_CS  1
#define EXTI_COMP_MASK(p) (1 << 21)
#define IRQ_COMP STM32_IRQ_COMP
/* triggers packet detection on comparator falling edge */
#define EXTI_XTSR STM32_EXTI_FTSR

/* TIM1_CH1 no remap needed */
#define DMAC_TIM_RX(p) STM32_DMAC_CH2

/* the pins used for communication need to be hi-speed */
static inline void pd_set_pins_speed(int port)
{
	/* 40 Mhz pin speed on TX_EN (PA15) */
	STM32_GPIO_OSPEEDR(GPIO_A) |= 0xC0000000;
	/* 40 MHz pin speed on SPI CLK/MOSI (PB3/4) TIM17_CH1 (PB9) */
	STM32_GPIO_OSPEEDR(GPIO_B) |= 0x000C03C0;
}

/* Reset SPI peripheral used for TX */
static inline void pd_tx_spi_reset(int port)
{
	/* Reset SPI1 */
	STM32_RCC_APB2RSTR |= (1 << 12);
	STM32_RCC_APB2RSTR &= ~(1 << 12);
}

/* Drive the CC line from the TX block */
static inline void pd_tx_enable(int port, int polarity)
{
	/* PB4 is SPI1_MISO */
	gpio_set_alternate_function(GPIO_B, 0x0010, 0);

	gpio_set_level(GPIO_PD_CC1_TX_EN, 1);
}

/* Put the TX driver in Hi-Z state */
static inline void pd_tx_disable(int port, int polarity)
{
	/* output low on SPI TX (PB4) to disable the FET */
	STM32_GPIO_MODER(GPIO_B) = (STM32_GPIO_MODER(GPIO_B)
				   & ~(3 << (2*4)))
				   |  (1 << (2*4));
	/* put the low level reference in Hi-Z */
	gpio_set_level(GPIO_PD_CC1_TX_EN, 0);
}

static inline void pd_select_polarity(int port, int polarity)
{
	/*
	 * use the right comparator : CC1 -> PA1 (COMP1 INP)
	 * use VrefInt / 2 as INM (about 600mV)
	 */
	STM32_COMP_CSR = (STM32_COMP_CSR & ~STM32_COMP_CMP1INSEL_MASK)
		| STM32_COMP_CMP1EN | STM32_COMP_CMP1INSEL_VREF12;
}

/* Initialize pins used for TX and put them in Hi-Z */
static inline void pd_tx_init(void)
{
	gpio_config_module(MODULE_USB_PD, 1);
}

static inline void pd_set_host_mode(int port, int enable) {}

static inline void pd_config_init(int port, uint8_t power_role)
{
	/* Initialize TX pins and put them in Hi-Z */
	pd_tx_init();
}

static inline int pd_adc_read(int port, int cc)
{
	/* only one CC line, assume other one is always low */
	return (cc == 0) ? adc_read_channel(ADC_CH_CC1_PD) : 0;
}

static inline int pd_snk_is_vbus_provided(int port)
{
	return 1;
}

#define DFPD_SBUOFF_AUX_N_L    3160
#define DFPD_SBUOFF_AUX_N_H    3210
#define DFPD_SBUOFF_AUX_P_H    50
#define DFPD_SBUEN_AUX_N_L     1740
#define DFPD_SBUEN_AUX_N_H     1780
#define DFPD_SBUEN_AUX_P_H     50

#define UFPD_SBUOFF_AUX_P_L    1040
#define UFPD_SBUOFF_AUX_P_H    1090
#define UFPD_SBUOFF_AUX_N_H    50
#define UFPD_SBUEN_AUX_P_L     160
#define UFPD_SBUEN_AUX_P_H     230
#define UFPD_SBUEN_AUX_N_L     2760
#define UFPD_SBUEN_AUX_N_H     2820

#define DP_DIR_NORMAL          1
#define DP_DIR_REVERSE         -1
#define DP_DIR_DISCONNECTED    0
/**
 * Detect normal or reverse direction of type-C to DP cable
 *
 * Detect the cable direction by resistive voltage measurement
 * on AUX_N & AUX_P based on DisplayPort connection.
 *
 * Normal  direction: connected to UFP_D (DP Monitor, Type-C NB)
 *                    Spec fig 3-2
 * Reverse direction: connected to DFP_D (DP NB, Type-C Monitor)
 *                    Spec fig 3-3
 *
 * @param port   USB-C port number
 * @return +1    if UFP_D connected, normal  direction
 *         -1    if DFP_D connected, reverse direction
 *         0     if disconnected
 */

static inline int dp_cable_direction(int port)
{
	int aux_n = adc_read_channel(ADC_CH_AUX_N);
	int aux_p = adc_read_channel(ADC_CH_AUX_P);

	if (gpio_get_level(GPIO_PD_SBU_ENABLE)) {
		// SBUEN,  SBU isolation switches are closed
		//-------------------------------------------
		if (aux_p >= UFPD_SBUEN_AUX_P_L && aux_p <= UFPD_SBUEN_AUX_P_H
		&&  aux_n >= UFPD_SBUEN_AUX_N_L && aux_n <= UFPD_SBUEN_AUX_N_H)
			return DP_DIR_NORMAL;

		if (aux_n >= DFPD_SBUEN_AUX_N_L && aux_n <= DFPD_SBUEN_AUX_N_H
		&&  aux_p <= DFPD_SBUEN_AUX_P_H)
			return DP_DIR_REVERSE;
	}
	else {
		// SBUOFF, SBU isolation switches are opened
		//-------------------------------------------
		if (aux_p >= UFPD_SBUOFF_AUX_P_L && aux_p <= UFPD_SBUOFF_AUX_P_H
		&&  aux_n <= UFPD_SBUOFF_AUX_N_H)
			return DP_DIR_NORMAL;

		if (aux_n >= DFPD_SBUOFF_AUX_N_L && aux_n <= DFPD_SBUOFF_AUX_N_H
		&&  aux_p <= DFPD_SBUOFF_AUX_P_H)
			return DP_DIR_REVERSE;
	}

	// Spec p.64, Some displays do not provide this pull-up resistor (1M on AUX_CH_P),
	// but assert HPD without DisplayPort DFP_D detection."
	if ((gpio_list[GPIO_DP_HPD].flags & GPIO_INPUT) && gpio_get_level(GPIO_DP_HPD))
		return DP_DIR_NORMAL;

	return DP_DIR_DISCONNECTED;
}

/* 3.0A DFP : no-connect voltage is 2.45V */
#define PD_SRC_VNC 2450 /* mV */

/* UFP-side : threshold for DFP connection detection */
#define PD_SNK_VA   250 /* mV */

/* we are acting only as a sink */
#define PD_DEFAULT_STATE PD_STATE_SNK_DISCONNECTED

/* we are never a source : don't care about power supply */
#define PD_POWER_SUPPLY_TURN_ON_DELAY  0 /* us */
#define PD_POWER_SUPPLY_TURN_OFF_DELAY 0 /* us */

/* Define typical operating power and max power */
#define PD_OPERATING_POWER_MW 1000
#define PD_MAX_POWER_MW       1500
#define PD_MAX_CURRENT_MA     300
#define PD_MAX_VOLTAGE_MV     5000

#endif /* __USB_PD_CONFIG_H */
