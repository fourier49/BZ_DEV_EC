/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "adc.h"
#include "chip/stm32/registers.h"
#include "gpio.h"

/* USB Power delivery board configuration */

#ifndef __USB_PD_CONFIG_H
#define __USB_PD_CONFIG_H

/* Port and task configuration */
#ifdef CONFIG_BIZ_DUAL_CC
#define PD_PORT_COUNT 2
#define PORT_TO_TASK_ID(port) ((port) ? TASK_ID_PD_P1 : TASK_ID_PD_P0)
#define TASK_ID_TO_PORT(id)   ((id) == TASK_ID_PD_P0 ? 0 : 1)
#else
#define PD_PORT_COUNT 1
#define PORT_TO_TASK_ID(port) TASK_ID_PD_P0
#define TASK_ID_TO_PORT(id)   0
#endif

/* Timer selection for baseband PD communication */
#ifdef __BIZ_SPICLK_USE_PB9__
#define TIM_CLOCK_PD_TX_P0 17
#define TIM_CLOCK_PD_TX_P1 17
#else
#define TIM_CLOCK_PD_TX_P0 14
#define TIM_CLOCK_PD_TX_P1 14
#endif
#define TIM_CLOCK_PD_RX_P0 1
#define TIM_CLOCK_PD_RX_P1 3

#define TIM_CLOCK_PD_TX(p) ((p) ? TIM_CLOCK_PD_TX_P1 : TIM_CLOCK_PD_TX_P0)
#define TIM_CLOCK_PD_RX(p) ((p) ? TIM_CLOCK_PD_RX_P1 : TIM_CLOCK_PD_RX_P0)

/* Timer channel */
#define TIM_RX_CCR_P0 1
#define TIM_RX_CCR_P1 1
#define TIM_TX_CCR_P0 1
#define TIM_TX_CCR_P1 1

/* RX timer capture/compare register */
#define TIM_CCR_P0 (&STM32_TIM_CCRx(TIM_CLOCK_PD_RX_P0, TIM_RX_CCR_P0))
#define TIM_CCR_P1 (&STM32_TIM_CCRx(TIM_CLOCK_PD_RX_P1, TIM_RX_CCR_P1))
#define TIM_RX_CCR_REG(p) ((p) ? TIM_CCR_P1 : TIM_CCR_P0)

/* TX and RX timer register */
#define TIM_REG_TX_P0 (STM32_TIM_BASE(TIM_CLOCK_PD_TX_P0))
#define TIM_REG_RX_P0 (STM32_TIM_BASE(TIM_CLOCK_PD_RX_P0))
#define TIM_REG_TX_P1 (STM32_TIM_BASE(TIM_CLOCK_PD_TX_P1))
#define TIM_REG_RX_P1 (STM32_TIM_BASE(TIM_CLOCK_PD_RX_P1))
#define TIM_REG_TX(p) ((p) ? TIM_REG_TX_P1 : TIM_REG_TX_P0)
#define TIM_REG_RX(p) ((p) ? TIM_REG_RX_P1 : TIM_REG_RX_P0)

/* use the hardware accelerator for CRC */
#define CONFIG_HW_CRC

/* TX uses SPI1 on PB3-4 for port P0, SPI2 on PB 13-14 for port P1 */
#define SPI_REGS(p) ((p) ? STM32_SPI2_REGS : STM32_SPI1_REGS)
static inline void spi_enable_clock(int port)
{
	if (port == 1)
		STM32_RCC_APB1ENR |= STM32_RCC_PB1_SPI2;
	else
		STM32_RCC_APB2ENR |= STM32_RCC_PB2_SPI1;
}

/* DMA for transmit uses DMA CH7 for P1 and DMA_CH3 for P0 */
#define DMAC_SPI_TX(p) ((p) ? STM32_DMAC_CH7 : STM32_DMAC_CH3)

/* RX uses COMP1 and TIM1 CH1 on port C0 and COMP2 and TIM3_CH1 for port C1*/
#define CMP1OUTSEL STM32_COMP_CMP1OUTSEL_TIM1_IC1
#define CMP2OUTSEL STM32_COMP_CMP2OUTSEL_TIM3_IC1

#define TIM_TX_CCR_IDX(p) ((p) ? TIM_TX_CCR_P1 : TIM_TX_CCR_P0)
#define TIM_RX_CCR_IDX(p) ((p) ? TIM_RX_CCR_P1 : TIM_RX_CCR_P0)
#define TIM_CCR_CS  1
#define EXTI_COMP_MASK(p) ((p) ? (1<<22) : (1 << 21))
#define IRQ_COMP STM32_IRQ_COMP
/* triggers packet detection on comparator falling edge */
#define EXTI_XTSR STM32_EXTI_FTSR

/* DMA for receive uses DMA_CH2 for C0 and DMA_CH6 for C1 */
#define DMAC_TIM_RX(p) ((p) ? STM32_DMAC_CH6 : STM32_DMAC_CH2)

/* the pins used for communication need to be hi-speed */
static inline void pd_set_pins_speed(int port)
{
	if (port == 1) {
		/* 40 MHz pin speed on SPI PB13/14  -->  10MHz */
		STM32_GPIO_OSPEEDR(GPIO_B) |= 0x14000000; // 0x3C000000;
		/* 40 Mhz pin speed on TX_EN (PA7) */
		STM32_GPIO_OSPEEDR(GPIO_A) |= 0x00004000; // 0x0000C000;
	} else {
		/* 40 MHz pin speed on SPI PB3/4 */
		STM32_GPIO_OSPEEDR(GPIO_B) |= 0x00000140; // 0x000003C0;
		/* 40 Mhz pin speed on TX_EN (PB0) */
		STM32_GPIO_OSPEEDR(GPIO_B) |= 0x00000001; // 0x00000003;
	}

#ifdef __BIZ_SPICLK_USE_PB9__
	/* 40 MHz pin speed on TIM17_CH1 (PB9) */
	STM32_GPIO_OSPEEDR(GPIO_B) |= 0x00040000; // 0x000C0000;
#else
	/* 40 MHz pin speed on TIM14_CH1 (PB1) */
	STM32_GPIO_OSPEEDR(GPIO_B) |= 0x00000004; // 0x0000000C;
#endif
}

/* Reset SPI peripheral used for TX */
static inline void pd_tx_spi_reset(int port)
{
	if (port == 1) {
		/* Reset SPI2 */
		STM32_RCC_APB1RSTR |= (1 << 14);
		STM32_RCC_APB1RSTR &= ~(1 << 14);
	} else {
		/* Reset SPI1 */
		STM32_RCC_APB2RSTR |= (1 << 12);
		STM32_RCC_APB2RSTR &= ~(1 << 12);
	}
}

/* Drive the CC line from the TX block */
static inline void pd_tx_enable(int port, int polarity)
{
	if (port == 0) {
#ifndef CONFIG_BIZ_EMU_HOST
		/* put SPI function on TX pin */
		/* PB4 is SPI1 MISO */
		gpio_set_alternate_function(GPIO_B, 0x0010, 0);

		/* set the low level reference */
		gpio_set_level(GPIO_USB_P0_CC_TX_EN, 1);
#endif
#ifdef CONFIG_BIZ_DUAL_CC
	} else {
		/* set the CC1/CC2 orientation */
		gpio_set_level(GPIO_USB_P1_CC_POLARITY, polarity);

		/* put SPI function on TX pin */
		/* PB14 is SPI2 MISO */
		gpio_set_alternate_function(GPIO_B, 0x4000, 0);

		/* set the low level reference */
		gpio_set_level(GPIO_USB_P1_CC_TX_EN, 1);
#endif
	}
}

/* Put the TX driver in Hi-Z state */
static inline void pd_tx_disable(int port, int polarity)
{
	if (port == 0) {
#ifndef CONFIG_BIZ_EMU_HOST
		/* output low on SPI TX to disable the FET */
		/* PB4 is SPI1 MISO */
		STM32_GPIO_MODER(GPIO_B) = (STM32_GPIO_MODER(GPIO_B)
					   & ~(3 << (2*4)))
					   |  (1 << (2*4));

		/* put the low level reference in Hi-Z */
		gpio_set_level(GPIO_USB_P0_CC_TX_EN, 0);
#endif
#ifdef CONFIG_BIZ_DUAL_CC
	} else {
		/* output low on SPI TX to disable the FET */
		/* PB14 is SPI2 MISO */
		STM32_GPIO_MODER(GPIO_B) = (STM32_GPIO_MODER(GPIO_B)
					   & ~(3 << (2*14)))
					   |  (1 << (2*14));

		/* put the low level reference in Hi-Z */
		gpio_set_level(GPIO_USB_P1_CC_TX_EN, 0);
#endif
	}
}

/* we know the plug polarity, do the right configuration */
static inline void pd_select_polarity(int port, int polarity)
{
	uint32_t val = STM32_COMP_CSR;

	/* Use window mode so that COMP1 and COMP2 share non-inverting input */
	val |= STM32_COMP_CMP1EN | STM32_COMP_CMP2EN | STM32_COMP_WNDWEN;

	if (port == 0) {
		/* use the right comparator inverted input for COMP1 */
		STM32_COMP_CSR = (val & ~STM32_COMP_CMP1INSEL_MASK) |
				 (polarity ? STM32_COMP_CMP1INSEL_INM4   // pa4
					   : STM32_COMP_CMP1INSEL_INM6); // pa0
	} else {
		/* use the right comparator inverted input for COMP2 */
		STM32_COMP_CSR = (val & ~STM32_COMP_CMP2INSEL_MASK) |
				 (polarity ? STM32_COMP_CMP2INSEL_INM5   // pa5
					   : STM32_COMP_CMP2INSEL_INM6); // pa2
	}
}

/* Initialize pins used for TX and put them in Hi-Z */
static inline void pd_tx_init(void)
{
	gpio_config_module(MODULE_USB_PD, 1);
}

static inline void pd_set_host_mode(int port, int enable)
{
	if (port == 0) {
		if (enable) {
			gpio_set_level(GPIO_USB_P0_PWROLE_SRC, 1);
			/* We never charging in power source mode */
//			gpio_set_level(GPIO_USB_P0_CHARGE_EN_L, 1);

			/* High-Z is used for host mode. */
//			gpio_set_level(GPIO_USB_P0_CC1_ODL, 1);
//			gpio_set_level(GPIO_USB_P0_CC2_ODL, 1);
		} else {
			gpio_set_level(GPIO_USB_P0_PWROLE_SRC, 0);
			/* Kill VBUS power supply */
			gpio_set_level(GPIO_USB_P0_PWR_5V_EN, 0);
			gpio_set_level(GPIO_USB_P0_PWR_20V_EN, 0);

			/* Pull low for device mode. */
//			gpio_set_level(GPIO_USB_P0_CC1_ODL, 0);
//			gpio_set_level(GPIO_USB_P0_CC2_ODL, 0);

			/* Let charge_manager decide to enable the port */
		}
	}
}

/**
 * Initialize various GPIOs and interfaces to safe state at start of pd_task.
 *
 * These include:
 *   VBUS, charge path based on power role.
 *   Physical layer CC transmit.
 *   VCONNs disabled.
 *
 * @param port        USB-C port number
 * @param power_role  Power role of device
 */
static inline void pd_config_init(int port, uint8_t power_role)
{
	/*
	 * Set CC pull resistors, and charge_en and vbus_en GPIOs to match
	 * the initial role.
	 */
	pd_set_host_mode(port, power_role);

	/* Initialize TX pins and put them in Hi-Z */
	pd_tx_init();
#ifdef CONFIG_USBC_SS_MUX
	/* Reset mux ... for NONE polarity doesn't matter */
	board_set_usb_mux(port, TYPEC_MUX_NONE, USB_SWITCH_DISCONNECT, 0);
#endif

#ifdef CONFIG_BIZ_EMU_HOST
	if (port == 1) {
		gpio_set_level(GPIO_USB_P1_CC1_VCONN_EN, 0);
		gpio_set_level(GPIO_USB_P1_CC2_VCONN_EN, 0);
		gpio_set_level(GPIO_USB_P1_DP_HPD, 0);
	}
#endif
}

static inline int pd_adc_read(int port, int cc)
{
	if (port == 0)
#ifdef CONFIG_BIZ_EMU_HOST
		return 3000;
#else
		return cc ? 0 : adc_read_channel(ADC_P0_CC_PD);
#endif
	else
		return adc_read_channel(cc ? ADC_P1_CC2_PD : ADC_P1_CC1_PD);
}

static inline void pd_set_vconn(int port, int polarity, int enable)
{
	/* Set VCONN on the opposite CC line from the polarity */
#ifdef CONFIG_BIZ_EMU_HOST
	if (enable) {
		gpio_set_level(polarity ? GPIO_USB_P1_CC1_VCONN_EN :
					  GPIO_USB_P1_CC2_VCONN_EN, 1);
	}
	else {
		gpio_set_level(GPIO_USB_P1_CC1_VCONN_EN, 0);
		gpio_set_level(GPIO_USB_P1_CC2_VCONN_EN, 0);
	}
#endif
}

static inline int pd_snk_is_vbus_provided(int port)
{
	
	enum pd_states state = pd_get_state(port);

	if (state == PD_STATE_SNK_DISCONNECTED)
		return 0;

    if((port==1)&&(adc_read_channel(ADC_P1_VBUS_DT)<300))
		return 0;

	//FIXME: for port 0, we should add check  with ADC
	return 1;
}



/* Voltage indexes for the PDOs */
enum volt_idx {
	PDO_IDX_SRC_5V  = 0,
	PDO_IDX_SRC_20V = 1,
	PDO_IDX_COUNT,
	PDO_IDX_SNK_VBUS,
	PDO_IDX_OFF,
};

void set_output_voltage(int vidx);
void discharge_voltage(int target_vidx);
void pd_pwr_local_change(int pwr_in);

void pd_check_cpower_deferred(void);


/* start as a sink in case we have no other power supply/battery */
#ifdef CONFIG_BIZ_EMU_HOST
#define PD_DEFAULT_STATE PD_STATE_SRC_DISCONNECTED

/* PD_SRC_VNC: Standard-current DFP : no-connect voltage is 1.55V */
#ifdef CONFIG_USB_PD_ADVPWR_1A5
#define PD_SRC_RD_THRESHOLD  400  /* mV */
#define PD_SRC_VNC           1600 /* mV */
#endif
#ifdef CONFIG_USB_PD_ADVPWR_3A
#define PD_SRC_RD_THRESHOLD  800  /* mV */
#define PD_SRC_VNC           2600 /* mV */
#endif
#ifdef CONFIG_USB_PD_ADVPWR_DEFAULT
#define PD_SRC_RD_THRESHOLD  200  /* mV */
#define PD_SRC_VNC           1600 /* mV */
#endif

/* UFP-side : threshold for DFP connection detection */
#define PD_SNK_VA            200 /* mV */

#else
#define PD_DEFAULT_STATE PD_STATE_SNK_DISCONNECTED

/* 3.0A DFP : no-connect voltage is 2.45V */
#define PD_SRC_VNC           2450 /* mV */
#define PD_SRC_RD_THRESHOLD  200  /* mV */

/* UFP-side : threshold for DFP connection detection */
#define PD_SNK_VA            250 /* mV */
#endif

/*
 * delay to turn on the power supply max is ~16ms.
 * delay to turn off the power supply max is about ~180ms.
 */
#define PD_POWER_SUPPLY_TURN_ON_DELAY  30000  /* us */
#define PD_POWER_SUPPLY_TURN_OFF_DELAY 250000 /* us */

/* Define typical operating power and max power */
#define PD_OPERATING_POWER_MW 1000
//this value is used to calculated the real wanted current.
//in pd_build_requet() and pd_process_source_cap().
//calculated the needed current and send request to power src.
#define PD_MAX_POWER_MW       60000 
#define PD_MAX_CURRENT_MA     3000
#define PD_MAX_VOLTAGE_MV     20000

#endif /* __USB_PD_CONFIG_H */
