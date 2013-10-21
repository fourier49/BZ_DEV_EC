/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* X86 chipset power control module for Chrome EC */

#include "chipset.h"
#include "chipset_x86_common.h"
#include "common.h"
#include "console.h"
#include "gpio.h"
#include "hooks.h"
#include "lid_switch.h"
#include "registers.h"
#include "system.h"
#include "task.h"
#include "timer.h"
#include "util.h"
#include "wireless.h"

/* Console output macros */
#define CPUTS(outstr) cputs(CC_CHIPSET, outstr)
#define CPRINTF(format, args...) cprintf(CC_CHIPSET, format, ## args)

/* Input state flags */
#define IN_PGOOD_PP1050           X86_SIGNAL_MASK(X86_PGOOD_PP1050)
#define IN_PGOOD_PP1200           X86_SIGNAL_MASK(X86_PGOOD_PP1200)
#define IN_PGOOD_PP1800           X86_SIGNAL_MASK(X86_PGOOD_PP1800)
#define IN_PGOOD_VCORE            X86_SIGNAL_MASK(X86_PGOOD_VCORE)

#define IN_PCH_SLP_S0_DEASSERTED  X86_SIGNAL_MASK(X86_PCH_SLP_S0_L_DEASSERTED)
#define IN_PCH_SLP_S3_DEASSERTED  X86_SIGNAL_MASK(X86_PCH_SLP_S3_L_DEASSERTED)
#define IN_PCH_SLP_S5_DEASSERTED  X86_SIGNAL_MASK(X86_PCH_SLP_S5_L_DEASSERTED)
#define IN_PCH_SLP_SUS_DEASSERTED X86_SIGNAL_MASK(X86_PCH_SLP_SUS_L_DEASSERTED)


/* All non-core power rails */
#define IN_PGOOD_ALL_NONCORE (IN_PGOOD_PP1050)
/* All core power rails */
#define IN_PGOOD_ALL_CORE    (IN_PGOOD_VCORE)
/* Rails required for S3 */
#define IN_PGOOD_S3          (IN_PGOOD_PP1200 | IN_PGOOD_PP1800)
/* Rails required for S0 */
#define IN_PGOOD_S0          (IN_PGOOD_ALL_NONCORE)
/* Rails used to detect if PP5000 is up. 1.8V PGOOD is not
 * a reliable signal to use here with an internal pullup. */
#define IN_PGOOD_PP5000	     (IN_PGOOD_PP1050 | IN_PGOOD_PP1200)


/* All PM_SLP signals from PCH deasserted */
#define IN_ALL_PM_SLP_DEASSERTED (IN_PCH_SLP_S3_DEASSERTED | \
				  IN_PCH_SLP_S5_DEASSERTED | \
				  IN_PCH_SLP_SUS_DEASSERTED)

/* All inputs in the right state for S0 */
#define IN_ALL_S0 (IN_PGOOD_ALL_NONCORE | IN_PGOOD_ALL_CORE | \
		   IN_ALL_PM_SLP_DEASSERTED)

static int throttle_cpu;      /* Throttle CPU? */

void chipset_force_shutdown(void)
{
	CPRINTF("[%T %s()]\n", __func__);

	/*
	 * Force x86 off. This condition will reset once the state machine
	 * transitions to G3.
	 */
	gpio_set_level(GPIO_PCH_DPWROK, 0);
}

void chipset_reset(int cold_reset)
{
	CPRINTF("[%T %s(%d)]\n", __func__, cold_reset);
	if (cold_reset) {
		/*
		 * Drop and restore PWROK.  This causes the PCH to reboot,
		 * regardless of its after-G3 setting.  This type of reboot
		 * causes the PCH to assert PLTRST#, SLP_S3#, and SLP_S5#, so
		 * we actually drop power to the rest of the system (hence, a
		 * "cold" reboot).
		 */

		/* Ignore if PWROK is already low */
		if (gpio_get_level(GPIO_PCH_PWROK) == 0)
			return;

		/* PWROK must deassert for at least 3 RTC clocks = 91 us */
		gpio_set_level(GPIO_PCH_PWROK, 0);
		udelay(100);
		gpio_set_level(GPIO_PCH_PWROK, 1);

	} else {
		/*
		 * Send a RCIN# pulse to the PCH.  This just causes it to
		 * assert INIT# to the CPU without dropping power or asserting
		 * PLTRST# to reset the rest of the system.
		 */

		/*
		 * Pulse must be at least 16 PCI clocks long = 500 ns.
		 */
		gpio_set_level(GPIO_PCH_RCIN_L, 0);
		udelay(10);
		gpio_set_level(GPIO_PCH_RCIN_L, 1);
	}
}

void chipset_throttle_cpu(int throttle)
{
	/* FIXME CPRINTF("[%T %s(%d)]\n", __func__, throttle);*/
}

enum x86_state x86_chipset_init(void)
{
	/*
	 * If we're switching between images without rebooting, see if the x86
	 * is already powered on; if so, leave it there instead of cycling
	 * through G3.
	 */
	if (system_jumped_to_this_image()) {
		if ((x86_get_signals() & IN_ALL_S0) == IN_ALL_S0) {
			CPRINTF("[%T x86 already in S0]\n");
			return X86_S0;
		} else {
			/* Force all signals to their G3 states */
			CPRINTF("[%T x86 forcing G3]\n");
			gpio_set_level(GPIO_PCH_PWROK, 0);
			gpio_set_level(GPIO_SYS_PWROK, 0);
			gpio_set_level(GPIO_PP1050_EN, 0);
			gpio_set_level(GPIO_PP1200_EN, 0);
			gpio_set_level(GPIO_PP1800_EN, 0);
			gpio_set_level(GPIO_PP3300_DSW_GATED_EN, 0);
			gpio_set_level(GPIO_PP5000_USB_EN, 0);
			gpio_set_level(GPIO_PP5000_EN, 0);
			gpio_set_level(GPIO_PCH_DPWROK, 0);
			wireless_enable(0);
		}
	}

	return X86_G3;
}

enum x86_state x86_handle_state(enum x86_state state)
{
	switch (state) {
	case X86_G3:
		break;

	case X86_S5:
		if (gpio_get_level(GPIO_PCH_SLP_S5_L) == 1)
			return X86_S5S3; /* Power up to next state */

		break;

	case X86_S3:
		/*
		 * If lid is closed; hold touchscreen in reset to cut
		 * power usage.  If lid is open, take touchscreen out
		 * of reset so it can wake the processor.
		 */
		gpio_set_level(GPIO_TOUCHSCREEN_RESET_L, lid_is_open());

		/* Check for state transitions */
		if (!x86_has_signals(IN_PGOOD_S3)) {
			/* Required rail went away */
			chipset_force_shutdown();
			return X86_S3S5;
		} else if (gpio_get_level(GPIO_PCH_SLP_S3_L) == 1) {
			/* Power up to next state */
			return X86_S3S0;
		} else if (gpio_get_level(GPIO_PCH_SLP_S5_L) == 0) {
			/* Power down to next state */
			return X86_S3S5;
		}
		break;

	case X86_S0:
		if (!x86_has_signals(IN_PGOOD_S0)) {
			/* Required rail went away */
			chipset_force_shutdown();
			return X86_S0S3;
		} else if (gpio_get_level(GPIO_PCH_SLP_S3_L) == 0) {
			/* Power down to next state */
			return X86_S0S3;
		}
		break;

	case X86_G3S5:
		/*
		 * Wait 10ms after +3VALW good, since that powers VccDSW and
		 * VccSUS.
		 */
		msleep(10);

		/* Enable PP5000 (5V) rail as 1.05V and 1.2V rails need 5V
		 * rail to regulate properly. */
		gpio_set_level(GPIO_PP5000_EN, 1);

		/* Wait for PP1050/PP1200 PGOOD to go LOW to
		 * indicate that PP5000 is stable */
		while ((x86_get_signals() & IN_PGOOD_PP5000) != 0) {
			if (task_wait_event(SECOND) == TASK_EVENT_TIMER) {
				CPRINTF("[%T timeout waiting for PP5000\n");
				chipset_force_shutdown();
				return X86_G3;
			}
		}

		/* Turn on 3.3V DSW rail. */
		gpio_set_level(GPIO_PP3300_DSW_GATED_EN, 1);

		/* Assert DPWROK */
		gpio_set_level(GPIO_PCH_DPWROK, 1);

		/* Enable PP1050 rail. */
		gpio_set_level(GPIO_PP1050_EN, 1);

		/* Wait for 1.05V to come up and CPU to notice */
		if (x86_wait_signals(IN_PGOOD_PP1050 |
				     IN_PCH_SLP_SUS_DEASSERTED)) {
			chipset_force_shutdown();
			return X86_G3;
		}

		/* Wait 5ms for SUSCLK to stabilize */
		msleep(5);
		return X86_S5;

	case X86_S5S3:
		/* Turn on power to RAM */
		gpio_set_level(GPIO_PP1800_EN, 1);
		gpio_set_level(GPIO_PP1200_EN, 1);
		if (x86_wait_signals(IN_PGOOD_S3)) {
			chipset_force_shutdown();
			return X86_S5;
		}

		/*
		 * Take lightbar out of reset, now that +5VALW is
		 * available and we won't leak +3VALW through the reset
		 * line.
		 */
		gpio_set_level(GPIO_LIGHTBAR_RESET_L, 1);

		/*
		 * Enable touchpad power so it can wake the system from
		 * suspend.
		 */
		gpio_set_level(GPIO_ENABLE_TOUCHPAD, 1);

		/* Turn on USB power rail. */
		gpio_set_level(GPIO_PP5000_USB_EN, 1);

		/* Call hooks now that rails are up */
		hook_notify(HOOK_CHIPSET_STARTUP);
		return X86_S3;

	case X86_S3S0:
		/* Wait 20ms before allowing VCCST_PGOOD to rise. */
		msleep(20);

		/* Enable wireless, WLAN power first */
		wireless_enable(EC_WIRELESS_SWITCH_WLAN_POWER);
		msleep(1);
		wireless_enable(EC_WIRELESS_SWITCH_ALL);

		/*
		 * Make sure touchscreen is out if reset (even if the
		 * lid is still closed); it may have been turned off if
		 * the lid was closed in S3.
		 */
		gpio_set_level(GPIO_TOUCHSCREEN_RESET_L, 1);

		/* Wait for non-core power rails good */
		if (x86_wait_signals(IN_PGOOD_S0)) {
			chipset_force_shutdown();
			wireless_enable(0);
			return X86_S3;
		}

		/* Call hooks now that rails are up */
		hook_notify(HOOK_CHIPSET_RESUME);

		/* Wait 99ms after all voltages good */
		msleep(99);

		/*
		 * Throttle CPU if necessary.  This should only be asserted
		 * when +VCCP is powered (it is by now).
		 */
		gpio_set_level(GPIO_CPU_PROCHOT, throttle_cpu);

		/* Set PCH_PWROK */
		gpio_set_level(GPIO_PCH_PWROK, 1);
		gpio_set_level(GPIO_SYS_PWROK, 1);
		return X86_S0;

	case X86_S0S3:
		/* Call hooks before we remove power rails */
		hook_notify(HOOK_CHIPSET_SUSPEND);

		/* Clear PCH_PWROK */
		gpio_set_level(GPIO_SYS_PWROK, 0);
		gpio_set_level(GPIO_PCH_PWROK, 0);

		/* Wait 40ns */
		udelay(1);

		/* Disable wireless */
		wireless_enable(0);

		/*
		 * Deassert prochot since CPU is off and we're about to drop
		 * +VCCP.
		 */
		gpio_set_level(GPIO_CPU_PROCHOT, 0);

		return X86_S3;

	case X86_S3S5:
		/* Call hooks before we remove power rails */
		hook_notify(HOOK_CHIPSET_SHUTDOWN);

		/* Disable peripheral power */
		gpio_set_level(GPIO_ENABLE_TOUCHPAD, 0);
		gpio_set_level(GPIO_PP5000_USB_EN, 0);

		/* Turn off power to RAM */
		gpio_set_level(GPIO_PP1800_EN, 0);
		gpio_set_level(GPIO_PP1200_EN, 0);

		/*
		 * Put touchscreen and lightbar in reset, so we won't
		 * leak +3VALW through the reset line to chips powered
		 * by +5VALW.
		 *
		 * (Note that we're no longer powering down +5VALW due
		 * to crosbug.com/p/16600, but to minimize side effects
		 * of that change we'll still reset these components in
		 * S5.)
		 */
		gpio_set_level(GPIO_TOUCHSCREEN_RESET_L, 0);
		gpio_set_level(GPIO_LIGHTBAR_RESET_L, 0);

		return X86_S5;

	case X86_S5G3:
		/* Deassert DPWROK */
		gpio_set_level(GPIO_PCH_DPWROK, 0);

		/* Turn off power rails enabled in S5 */
		gpio_set_level(GPIO_PP1050_EN, 0);
		gpio_set_level(GPIO_PP3300_DSW_GATED_EN, 0);
		gpio_set_level(GPIO_PP5000_EN, 0);
		return X86_G3;
	}

	return state;
}