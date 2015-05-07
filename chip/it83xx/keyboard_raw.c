/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.h"
#include "keyboard_raw.h"
#include "keyboard_scan.h"
#include "registers.h"
#include "task.h"
#include "irq_chip.h"

/*
 * Initialize the raw keyboard interface.
 */
void keyboard_raw_init(void)
{
	/* Ensure top-level interrupt is disabled */
	keyboard_raw_enable_interrupt(0);

	/*
	 * bit2, Setting 1 enables the internal pull-up of the KSO[15:0] pins.
	 * To pull up KSO[17:16], set the GPCR registers of their
	 * corresponding GPIO ports.
	 * bit0, Setting 1 enables the open-drain mode of the KSO[17:0] pins.
	 */
	IT83XX_KBS_KSOCTRL = 0x05;

	/* bit2, 1 enables the internal pull-up of the KSI[7:0] pins. */
	IT83XX_KBS_KSICTRL = 0x04;

	/* KSO[7:0] pins low. */
	IT83XX_KBS_KSOL = 0x00;

	/* KSO[15:8] pins low. */
	IT83XX_KBS_KSOH1 = 0x00;

#ifdef CONFIG_KEYBOARD_KSI_WUC_INT
	/* KSI[0-7] falling-edge triggered is selected */
	IT83XX_WUC_WUEMR3 = 0xFF;

	/* W/C */
	IT83XX_WUC_WUESR3 = 0xFF;

	task_clear_pending_irq(IT83XX_IRQ_WKINTC);

	/* Enable WUC for KSI[0-7] */
	IT83XX_WUC_WUENR3 = 0xFF;
#else
	task_clear_pending_irq(IT83XX_IRQ_KB_MATRIX);
#endif

	keyboard_raw_enable_interrupt(1);
}

/*
 * Finish initialization after task scheduling has started.
 */
void keyboard_raw_task_start(void)
{
#ifdef CONFIG_KEYBOARD_KSI_WUC_INT
	IT83XX_WUC_WUESR3 = 0xFF;
	task_clear_pending_irq(IT83XX_IRQ_WKINTC);
	task_enable_irq(IT83XX_IRQ_WKINTC);
#else
	task_clear_pending_irq(IT83XX_IRQ_KB_MATRIX);
	task_enable_irq(IT83XX_IRQ_KB_MATRIX);
#endif
}

/*
 * Drive the specified column low.
 */
test_mockable void keyboard_raw_drive_column(int col)
{
	int mask;

	/* Tri-state all outputs */
	if (col == KEYBOARD_COLUMN_NONE)
		mask = 0xffff;
	/* Assert all outputs */
	else if (col == KEYBOARD_COLUMN_ALL)
		mask = 0;
	/* Assert a single output */
	else
		mask = 0xffff ^ (1 << col);

	IT83XX_KBS_KSOL = mask & 0xff;
	IT83XX_KBS_KSOH1 = (mask >> 8) & 0xff;
}

/*
 * Read raw row state.
 * Bits are 1 if signal is present, 0 if not present.
 */
test_mockable int keyboard_raw_read_rows(void)
{
	/* Bits are active-low, so invert returned levels */
	return IT83XX_KBS_KSI ^ 0xff;
}

/*
 * Enable or disable keyboard matrix scan interrupts.
 */
void keyboard_raw_enable_interrupt(int enable)
{
	if (enable) {
#ifdef CONFIG_KEYBOARD_KSI_WUC_INT
		IT83XX_WUC_WUESR3 = 0xFF;
		task_clear_pending_irq(IT83XX_IRQ_WKINTC);
		task_enable_irq(IT83XX_IRQ_WKINTC);
#else
		task_clear_pending_irq(IT83XX_IRQ_KB_MATRIX);
		task_enable_irq(IT83XX_IRQ_KB_MATRIX);
#endif
	} else {
#ifdef CONFIG_KEYBOARD_KSI_WUC_INT
		task_disable_irq(IT83XX_IRQ_WKINTC);
#else
		task_disable_irq(IT83XX_IRQ_KB_MATRIX);
#endif
	}
}

/*
 * Interrupt handler for keyboard matrix scan interrupt.
 */
void keyboard_raw_interrupt(void)
{
#ifdef CONFIG_KEYBOARD_KSI_WUC_INT
	task_disable_irq(IT83XX_IRQ_WKINTC);
#else
	task_disable_irq(IT83XX_IRQ_KB_MATRIX);
#endif

	/* Wake the scan task */
	task_wake(TASK_ID_KEYSCAN);
}
