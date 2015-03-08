/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_REGISTERS_H
#define __CROS_EC_REGISTERS_H

#include "common.h"
#include "gc_regdefs.h"
#include "util.h"

/* Replace masked bits with val << lsb */
#define REG_WRITE_MLV(reg, mask, lsb, val) reg = ((reg & ~mask) | ((val << lsb) & mask))

/* Revision generated from the register definitions */
#define GC_REVISION_STR STRINGIFY(GC_REVISION)
/* Revision registers */
#define GR_SWDP_BUILD_DATE  REG32(GC_SWDP0_BASE_ADDR + GC_SWDP_BUILD_DATE_OFFSET)
#define GR_SWDP_BUILD_TIME  REG32(GC_SWDP0_BASE_ADDR + GC_SWDP_BUILD_TIME_OFFSET)

/* Power Management Unit */
#define GR_PMU_REG(off)               REG32(GC_PMU_BASE_ADDR + (off))

#define GR_PMU_RESET                  GR_PMU_REG(GC_PMU_RESET_OFFSET)
#define GR_PMU_SETRST                 GR_PMU_REG(GC_PMU_SETRST_OFFSET)
#define GR_PMU_CLRRST                 GR_PMU_REG(GC_PMU_CLRRST_OFFSET)
#define GR_PMU_RSTSRC                 GR_PMU_REG(GC_PMU_RSTSRC_OFFSET)
#define GR_PMU_GLOBAL_RESET           GR_PMU_REG(GC_PMU_GLOBAL_RESET_OFFSET)
#define GR_PMU_SETDIS                 GR_PMU_REG(GC_PMU_SETDIS_OFFSET)
#define GR_PMU_CLRDIS                 GR_PMU_REG(GC_PMU_CLRDIS_OFFSET)
#define GR_PMU_STATDIS                GR_PMU_REG(GC_PMU_STATDIS_OFFSET)
#define GR_PMU_SETWIC                 GR_PMU_REG(GC_PMU_SETWIC_OFFSET)
#define GR_PMU_CLRWIC                 GR_PMU_REG(GC_PMU_CLRWIC_OFFSET)
#define GR_PMU_SYSVTOR                GR_PMU_REG(GC_PMU_SYSVTOR_OFFSET)
#define GR_PMU_EXCLUSIVE              GR_PMU_REG(GC_PMU_EXCLUSIVE_OFFSET)
#define GR_PMU_DAP_ID0                GR_PMU_REG(GC_PMU_DAP_ID0_OFFSET)
#define GR_PMU_DAP_EN                 GR_PMU_REG(GC_PMU_DAP_EN_OFFSET)
#define GR_PMU_DAP_LOCK               GR_PMU_REG(GC_PMU_DAP_LOCK_OFFSET)
#define GR_PMU_DAP_UNLOCK             GR_PMU_REG(GC_PMU_DAP_UNLOCK_OFFSET)
#define GR_PMU_NAP_EN                 GR_PMU_REG(GC_PMU_NAP_EN_OFFSET)
#define GR_PMU_VREF                   GR_PMU_REG(GC_PMU_VREF_OFFSET)
#define GR_PMU_VREFCMP                GR_PMU_REG(GC_PMU_VREFCMP_OFFSET)
#define GR_PMU_RBIAS                  GR_PMU_REG(GC_PMU_RBIAS_OFFSET)
#define GR_PMU_RBIASLO                GR_PMU_REG(GC_PMU_RBIASLO_OFFSET)
#define GR_PMU_RBIASHI                GR_PMU_REG(GC_PMU_RBIASHI_OFFSET)
#define GR_PMU_SETHOLDVREF            GR_PMU_REG(GC_PMU_SETHOLDVREF_OFFSET)
#define GR_PMU_CLRHOLDVREF            GR_PMU_REG(GC_PMU_CLRHOLDVREF_OFFSET)
#define GR_PMU_BAT_LVL_OK             GR_PMU_REG(GC_PMU_BAT_LVL_OK_OFFSET)
#define GR_PMU_B_REG_DIG_CTRL         GR_PMU_REG(GC_PMU_B_REG_DIG_CTRL_OFFSET)
#define GR_PMU_B_REG_DIG_LATCH_CTRL   GR_PMU_REG(GC_PMU_B_REG_DIG_LATCH_CTRL_OFFSET)
#define GR_PMU_EXITPD_HOLD_SET        GR_PMU_REG(GC_PMU_EXITPD_HOLD_SET_OFFSET)
#define GR_PMU_EXITPD_HOLD_CLR        GR_PMU_REG(GC_PMU_EXITPD_HOLD_CLR_OFFSET)
#define GR_PMU_EXITPD_MASK            GR_PMU_REG(GC_PMU_EXITPD_MASK_OFFSET)
#define GR_PMU_EXITPD_SRC             GR_PMU_REG(GC_PMU_EXITPD_SRC_OFFSET)
#define GR_PMU_EXITPD_MON             GR_PMU_REG(GC_PMU_EXITPD_MON_OFFSET)
#define GR_PMU_OSC_HOLD_SET           GR_PMU_REG(GC_PMU_OSC_HOLD_SET_OFFSET)
#define GR_PMU_OSC_HOLD_CLR           GR_PMU_REG(GC_PMU_OSC_HOLD_CLR_OFFSET)
#define GR_PMU_OSC_SELECT             GR_PMU_REG(GC_PMU_OSC_SELECT_OFFSET)
#define GR_PMU_OSC_SELECT_STAT        GR_PMU_REG(GC_PMU_OSC_SELECT_STAT_OFFSET)
#define GR_PMU_OSC_CTRL               GR_PMU_REG(GC_PMU_OSC_CTRL_OFFSET)
#define GR_PMU_MEMCLKSET              GR_PMU_REG(GC_PMU_MEMCLKSET_OFFSET)
#define GR_PMU_MEMCLKCLR              GR_PMU_REG(GC_PMU_MEMCLKCLR_OFFSET)
#define GR_PMU_PERICLKSET0            GR_PMU_REG(GC_PMU_PERICLKSET0_OFFSET)
#define GR_PMU_PERICLKCLR0            GR_PMU_REG(GC_PMU_PERICLKCLR0_OFFSET)
#define GR_PMU_PERICLKSET1            GR_PMU_REG(GC_PMU_PERICLKSET1_OFFSET)
#define GR_PMU_PERICLKCLR1            GR_PMU_REG(GC_PMU_PERICLKCLR1_OFFSET)
#define GR_PMU_PERIGATEONSLEEPSET0    GR_PMU_REG(GC_PMU_PERIGATEONSLEEPSET0_OFFSET)
#define GR_PMU_PERIGATEONSLEEPCLR0    GR_PMU_REG(GC_PMU_PERIGATEONSLEEPCLR0_OFFSET)
#define GR_PMU_PERIGATEONSLEEPSET1    GR_PMU_REG(GC_PMU_PERIGATEONSLEEPSET1_OFFSET)
#define GR_PMU_PERIGATEONSLEEPCLR1    GR_PMU_REG(GC_PMU_PERIGATEONSLEEPCLR1_OFFSET)
#define GR_PMU_CLK0                   GR_PMU_REG(GC_PMU_CLK0_OFFSET)
#define GR_PMU_CLK1                   GR_PMU_REG(GC_PMU_CLK1_OFFSET)
#define GR_PMU_RST0                   GR_PMU_REG(GC_PMU_RST0_OFFSET)
#define GR_PMU_RST1                   GR_PMU_REG(GC_PMU_RST1_OFFSET)
#define GR_PMU_PWRDN_SCRATCH_HOLD_SET GR_PMU_REG(GC_PMU_PWRDN_SCRATCH_HOLD_SET_OFFSET)
#define GR_PMU_PWRDN_SCRATCH_HOLD_CLR GR_PMU_REG(GC_PMU_PWRDN_SCRATCH_HOLD_CLR_OFFSET)
#define GR_PMU_PWRDN_SCRATCH0         GR_PMU_REG(GC_PMU_PWRDN_SCRATCH0_OFFSET)
#define GR_PMU_PWRDN_SCRATCH1         GR_PMU_REG(GC_PMU_PWRDN_SCRATCH1_OFFSET)
#define GR_PMU_PWRDN_SCRATCH2         GR_PMU_REG(GC_PMU_PWRDN_SCRATCH2_OFFSET)
#define GR_PMU_PWRDN_SCRATCH3         GR_PMU_REG(GC_PMU_PWRDN_SCRATCH3_OFFSET)

#define GR_PMU_FUSE_RD_RC_OSC_26MHZ   GR_PMU_REG(GC_PMU_FUSE_RD_RC_OSC_26MHZ_OFFSET)
#define GR_PMU_FUSE_RD_XTL_OSC_26MHZ  GR_PMU_REG(GC_PMU_FUSE_RD_XTL_OSC_26MHZ_OFFSET)

/* More than one UART */
BUILD_ASSERT(GC_UART1_BASE_ADDR - GC_UART0_BASE_ADDR == GC_UART2_BASE_ADDR - GC_UART1_BASE_ADDR);
#define X_UART_BASE_ADDR_SEP   (GC_UART1_BASE_ADDR - GC_UART0_BASE_ADDR)
static inline int x_uart_addr(int ch, int offset)
{
	return offset + GC_UART0_BASE_ADDR + X_UART_BASE_ADDR_SEP * ch;
}
#define X_UARTREG(ch, offset)         REG32(x_uart_addr(ch, offset))
#define GR_UART_RDATA(ch)             X_UARTREG(ch, GC_UART_RDATA_OFFSET)
#define GR_UART_WDATA(ch)             X_UARTREG(ch, GC_UART_WDATA_OFFSET)
#define GR_UART_NCO(ch)               X_UARTREG(ch, GC_UART_NCO_OFFSET)
#define GR_UART_CTRL(ch)              X_UARTREG(ch, GC_UART_CTRL_OFFSET)
#define GR_UART_ICTRL(ch)             X_UARTREG(ch, GC_UART_ICTRL_OFFSET)
#define GR_UART_STATE(ch)             X_UARTREG(ch, GC_UART_STATE_OFFSET)
#define GR_UART_STATECLR(ch)          X_UARTREG(ch, GC_UART_STATECLR_OFFSET)
#define GR_UART_ISTATE(ch)            X_UARTREG(ch, GC_UART_ISTATE_OFFSET)
#define GR_UART_ISTATECLR(ch)         X_UARTREG(ch, GC_UART_ISTATECLR_OFFSET)
#define GR_UART_FIFO(ch)              X_UARTREG(ch, GC_UART_FIFO_OFFSET)
#define GR_UART_RFIFO(ch)             X_UARTREG(ch, GC_UART_RFIFO_OFFSET)


/* GPIO port naming scheme left over from the LM4. Must maintain tradition! */
#define GPIO_0 0
#define GPIO_1 1
#define DUMMY_GPIO_BANK 0

/*
 * Our ARM core doesn't have GPIO alternate functions, but it does have a full
 * NxM crossbar called the pinmux, which connects internal peripherals
 * including GPIOs to external pins. We'll reuse the alternate function stuff
 * from other ECs to configure the pinmux. This requires some clever macros
 * that pack both a MUX selector offset (register address) and a MUX selector
 * value (which input to choose) into a single uint32_t.
 */

/* Flags to indicate the direction of the signal-to-pin connection */
#define DIO_INPUT 0x0001
#define DIO_OUTPUT 0x0002

/*
 * To store a pinmux DIO in the struct gpio_alt_func's mask field, we use:
 *
 *   bits 31-16: offset of the MUX selector register that drives this signal
 *   bits 15-0:  value to write to any pinmux selector to choose this source
 */
#define DIO(name) (uint32_t)(CONCAT3(GC_PINMUX_DIO, name, _SEL) |	\
			     (CONCAT3(GC_PINMUX_DIO, name, _SEL_OFFSET) << 16))
/* Extract the MUX selector register addres for the DIO */
#define DIO_SEL_REG(word) REG32(GC_PINMUX_BASE_ADDR +			\
				(((uint32_t)(word) >> 16) & 0xffff))
/* Extract the control register address for this MUX */
#define DIO_CTL_REG(word) REG32(GC_PINMUX_BASE_ADDR + 0x4 +		\
				(((uint32_t)(word) >> 16) & 0xffff))
/* Extract the selector value to choose this DIO */
#define DIO_FUNC(word) ((uint32_t)(word) & 0xffff)

/*
 * The struct gpio_alt_func's port field will either contain an enum
 * gpio_signal from gpio_list[], or an internal peripheral function. If bit 31
 * is clear, then bits 30-0 are the gpio_signal. If bit 31 is set, the
 * peripheral function is packed like the DIO, above.
 *
 * gpio:
 *   bit 31:     0
 *   bit 30-0:   enum gpio_signal
 *
 * peripheral:
 *   bit 31:     1
 *   bits 30-16: offset of the MUX selector register that drives this signal
 *   bits 15-0:  value to write to any pinmux selector to choose this source
*/

/* Which is it? */
#define FIELD_IS_FUNC(port) (0x80000000 & (port))
/* Encode a pinmux identifier (both a MUX and a signal name) */
#define GPIO_FUNC(name) (uint32_t)(CONCAT3(GC_PINMUX_, name, _SEL) |	\
				   (CONCAT3(GC_PINMUX_, name, _SEL_OFFSET) << 16) | \
		0x80000000)
/* Extract the MUX selector register address to drive this signal */
#define PERIPH_SEL_REG(word) REG32(GC_PINMUX_BASE_ADDR +	\
				   (((uint32_t)(word) >> 16) & 0x7fff))
/* Extract the control register address for this MUX */
#define PERIPH_CTL_REG(word) REG32(GC_PINMUX_BASE_ADDR + 0x4 +	\
				 (((uint32_t)(word) >> 16) & 0x7fff))
/* Extract the selector value to choose this input source */
#define PERIPH_FUNC(word) ((uint32_t)(word) & 0xffff)
/* Extract the GPIO signal */
#define FIELD_GET_GPIO(word) ((uint32_t)(word) & 0x7fffffff)


/* Map a GPIO <port,bitnum> to a selector value or register */
#define GET_GPIO_FUNC(port, bitnum) \
	(GC_PINMUX_GPIO0_GPIO0_SEL + 16 * port + bitnum)

#define GET_GPIO_SEL_REG(port, bitnum) \
	REG32(GC_PINMUX_BASE_ADDR + \
	       GC_PINMUX_GPIO0_GPIO0_SEL_OFFSET + 64 * port + 4 * bitnum)

/* Constants for setting MUX control bits (same bits for all DIO pins) */
#define DIO_CTL_IE_LSB  GC_PINMUX_DIOA0_CTL_IE_LSB
#define DIO_CTL_IE_MASK GC_PINMUX_DIOA0_CTL_IE_MASK
#define DIO_CTL_PD_LSB  GC_PINMUX_DIOA0_CTL_PD_LSB
#define DIO_CTL_PD_MASK GC_PINMUX_DIOA0_CTL_PD_MASK
#define DIO_CTL_PU_LSB  GC_PINMUX_DIOA0_CTL_PU_LSB
#define DIO_CTL_PU_MASK GC_PINMUX_DIOA0_CTL_PU_MASK

/* Registers controlling the ARM core GPIOs */
#define GR_GPIO_REG(n, off)         REG16(GC_GPIO0_BASE_ADDR + (n) * 0x10000 + (off))
#define GR_GPIO_DATAIN(n)           GR_GPIO_REG(n, GC_GPIO_DATAIN_OFFSET)
#define GR_GPIO_DOUT(n)             GR_GPIO_REG(n, GC_GPIO_DOUT_OFFSET)
#define GR_GPIO_SETDOUTEN(n)        GR_GPIO_REG(n, GC_GPIO_SETDOUTEN_OFFSET)
#define GR_GPIO_CLRDOUTEN(n)        GR_GPIO_REG(n, GC_GPIO_CLRDOUTEN_OFFSET)
#define GR_GPIO_SETINTEN(n)         GR_GPIO_REG(n, GC_GPIO_SETINTEN_OFFSET)
#define GR_GPIO_CLRINTEN(n)         GR_GPIO_REG(n, GC_GPIO_CLRINTEN_OFFSET)
#define GR_GPIO_SETINTTYPE(n)       GR_GPIO_REG(n, GC_GPIO_SETINTTYPE_OFFSET)
#define GR_GPIO_CLRINTTYPE(n)       GR_GPIO_REG(n, GC_GPIO_CLRINTTYPE_OFFSET)
#define GR_GPIO_SETINTPOL(n)        GR_GPIO_REG(n, GC_GPIO_SETINTPOL_OFFSET)
#define GR_GPIO_CLRINTPOL(n)        GR_GPIO_REG(n, GC_GPIO_CLRINTPOL_OFFSET)
#define GR_GPIO_CLRINTSTAT(n)       GR_GPIO_REG(n, GC_GPIO_CLRINTSTAT_OFFSET)

#define GR_GPIO_MASKLOWBYTE(n, mask)  GR_GPIO_REG(n, GC_GPIO_MASKLOWBYTE_400_OFFSET + (mask) * 4)
#define GR_GPIO_MASKHIGHBYTE(n, mask) GR_GPIO_REG(n, GC_GPIO_MASKHIGHBYTE_800_OFFSET + (mask) * 4)

/*
 * High-speed timers. Two modules with two timers each; four timers total.
 */
#define X_TIMEHS_BASE_ADDR_SEP    (GC_TIMEHS1_BASE_ADDR - GC_TIMEHS0_BASE_ADDR)
#define X_TIMEHSX_TIMER_OFS_SEP   (GC_TIMEHS_TIMER2LOAD_OFFSET - GC_TIMEHS_TIMER1LOAD_OFFSET)
/* NOTE: module is 0-1, timer is 1-2 */
static inline int x_timehs_addr(unsigned int module, unsigned int timer,
				int offset)
{
	return GC_TIMEHS0_BASE_ADDR + X_TIMEHS_BASE_ADDR_SEP * module
		+ GC_TIMEHS_TIMER1LOAD_OFFSET + X_TIMEHSX_TIMER_OFS_SEP * (timer - 1)
		+ offset;
}
/* Per-timer registers */
#define X_TIMEHSREG(m, t, ofs)        REG32(x_timehs_addr(m, t, ofs))
#define GR_TIMEHS_LOAD(m, t)          X_TIMEHSREG(m, t, GC_TIMEHS_TIMER1LOAD_OFFSET)
#define GR_TIMEHS_VALUE(m, t)         X_TIMEHSREG(m, t, GC_TIMEHS_TIMER1VALUE_OFFSET)
#define GR_TIMEHS_CONTROL(m, t)       X_TIMEHSREG(m, t, GC_TIMEHS_TIMER1CONTROL_OFFSET)
#define GR_TIMEHS_INTCLR(m, t)        X_TIMEHSREG(m, t, GC_TIMEHS_TIMER1INTCLR_OFFSET)
#define GR_TIMEHS_RIS(m, t)           X_TIMEHSREG(m, t, GC_TIMEHS_TIMER1RIS_OFFSET)
#define GR_TIMEHS_MIS(m, t)           X_TIMEHSREG(m, t, GC_TIMEHS_TIMER1MIS_OFFSET)
#define GR_TIMEHS_BGLOAD(m, t)        X_TIMEHSREG(m, t, GC_TIMEHS_TIMER1BGLOAD_OFFSET)

/* Watchdog */
#define GR_WDOG_REG(off)              REG32(GC_WATCHDOG0_BASE_ADDR + (off))
#define GR_WATCHDOG_LOAD              GR_WDOG_REG(GC_WATCHDOG_WDOGLOAD_OFFSET)
#define GR_WATCHDOG_VALUE             GR_WDOG_REG(GC_WATCHDOG_WDOGVALUE_OFFSET)
#define GR_WATCHDOG_CTL               GR_WDOG_REG(GC_WATCHDOG_WDOGCONTROL_OFFSET)
#define GR_WATCHDOG_ICR               GR_WDOG_REG(GC_WATCHDOG_WDOGINTCLR_OFFSET)
#define GR_WATCHDOG_RIS               GR_WDOG_REG(GC_WATCHDOG_WDOGRIS_OFFSET)
#define GR_WATCHDOG_LOCK              GR_WDOG_REG(GC_WATCHDOG_WDOGLOCK_OFFSET)
#define GR_WATCHDOG_ITCR              GR_WDOG_REG(GC_WATCHDOG_WDOGITCR_OFFSET)
#define GR_WATCHDOG_ITOP              GR_WDOG_REG(GC_WATCHDOG_WDOGITOP_OFFSET)

/* Oscillator */
#define GR_XO_OSC_CLKOUT              REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_CLKOUT_OFFSET)
#define GR_XO_OSC_ADC_CAL_FREQ2X      REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_ADC_CAL_FREQ2X_OFFSET)
#define GR_XO_OSC_ADC_CAL_FREQ2X_STAT REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_ADC_CAL_FREQ2X_STAT_OFFSET)
#define GR_XO_OSC_24_48B_SEL          REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_24_48B_SEL_OFFSET)
#define GR_XO_OSC_TEST                REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_TEST_OFFSET)
#define GR_XO_OSC_RC_CAL_RSTB         REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_RC_CAL_RSTB_OFFSET)
#define GR_XO_OSC_RC_CAL_LOAD         REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_RC_CAL_LOAD_OFFSET)
#define GR_XO_OSC_RC_CAL_START        REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_RC_CAL_START_OFFSET)
#define GR_XO_OSC_RC_CAL_DONE         REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_RC_CAL_DONE_OFFSET)
#define GR_XO_OSC_RC_CAL_COUNT        REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_RC_CAL_COUNT_OFFSET)
#define GR_XO_OSC_RC                  REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_RC_OFFSET)
#define GR_XO_OSC_RC_STATUS           REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_RC_STATUS_OFFSET)
#define GR_XO_OSC_XTL_TRIMD           REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_XTL_TRIMD_OFFSET)
#define GR_XO_OSC_XTL_TRIMG           REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_XTL_TRIMG_OFFSET)
#define GR_XO_OSC_XTL_CTRL            REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_XTL_CTRL_OFFSET)
#define GR_XO_OSC_XTL_RC_FLTR         REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_XTL_RC_FLTR_OFFSET)
#define GR_XO_OSC_XTL_OVRD            REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_XTL_OVRD_OFFSET)
#define GR_XO_OSC_XTL_OVRD_HOLDB      REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_XTL_OVRD_HOLDB_OFFSET)
#define GR_XO_OSC_XTL_TRIM            REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_XTL_TRIM_OFFSET)
#define GR_XO_OSC_XTL_TRIM_STAT       REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_XTL_TRIM_STAT_OFFSET)
#define GR_XO_OSC_XTL_FSM_EN          REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_XTL_FSM_EN_OFFSET)
#define GR_XO_OSC_XTL_FSM             REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_XTL_FSM_OFFSET)
#define GR_XO_OSC_XTL_FSM_CFG         REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_XTL_FSM_CFG_OFFSET)
#define GR_XO_OSC_SETHOLD             REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_SETHOLD_OFFSET)
#define GR_XO_OSC_CLRHOLD             REG32(GC_XO0_BASE_ADDR + GC_XO_OSC_CLRHOLD_OFFSET)

#endif	/* __CROS_EC_REGISTERS_H */
