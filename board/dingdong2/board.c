/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* Dingdong dongle configuration */

#include "adc.h"
#include "adc_chip.h"
#include "common.h"
#include "ec_version.h"
#include "console.h"
#include "gpio.h"
#include "hooks.h"
#include "registers.h"
#include "usb.h"
#include "usb_bb.h"
#include "usb_pd.h"
#include "usb_pd_config.h"
#include "task.h"
#include "timer.h"
#include "util.h"

#define MAX_HPD_MSG_QUEUE   4
#define HPD_MSG_QUEUE_GAP   (4*MSEC)

static volatile uint64_t hpd_prev_ts;
static volatile int hpd_prev_level;
static volatile enum hpd_event hpd_reported_event = hpd_none;
static volatile int hpd_event_triggering;   // HPD event is triggering and not resolved yet

void hpd_event(enum gpio_signal signal);
#include "gpio_list.h"

#ifdef CONFIG_COMMON_RUNTIME
#define CPRINTF(format, args...) cprintf(CC_USBPD, format, ## args)
#define CPRINTS(format, args...) cprints(CC_USBPD, format, ## args)
#endif

/**
 * Hotplug detect deferred task
 *
 * Called after level change on hpd GPIO to evaluate (and debounce) what event
 * has occurred.  There are 3 events that occur on HPD:
 *    1. low  : downstream display sink is deattached
 *    2. high : downstream display sink is attached
 *    3. irq  : downstream display sink signalling an interrupt.
 *
 * The debounce times for these various events are:
 *  100MSEC : min pulse width of level value.
 *    2MSEC : min pulse width of IRQ low pulse.  Max is level debounce min.
 *  150USEC : below of this means glitch  --- BXU
 *
 * lvl(n-2) lvl(n-1)  lvl   prev_delta  now_delta event
 * ----------------------------------------------------
 * 1        0         1     <2ms        n/a       low glitch (ignore)
 * 1        0         1     >2ms        <100ms    irq
 * x        0         1     n/a         >100ms    high
 * 0        1         0     <100ms      n/a       high glitch (ignore)
 * x        1         0     n/a         >100ms    low
 *
 *************************************************************************
 * 3.15.2.1.1 HPD-to-PD Converter Message Queuing Rules
 * <1> If an IRQ_HPD is detected while a previous IRQ_HPD is in the queue,
 *     the converter shall transmit DisplayPort Status updates for both events.
 * <2> If an IRQ_HPD is detected while a previous HPD_High is in the queue,
 *     both shall be transmitted, either in one status update (recommended)
 *     or separate messages in the order detected.
 * <3> If HPD_Low is detected while any previous status updates are in the
 *     queue (including IRQ_HPD), the previous status updates may be discarded.
 * <4> If HPD_High is detected while a previous HPD_Low is in the queue,
 *     both shall be transmitted in the order detected.
 */
static volatile enum hpd_event hpd_msg_queue[MAX_HPD_MSG_QUEUE];
static volatile int hpd_qhead, hpd_qtail;

#define hpd_mque_depth()    ((hpd_qhead - hpd_qtail + MAX_HPD_MSG_QUEUE) % MAX_HPD_MSG_QUEUE)
#define hpd_mque_reset()    hpd_qhead = hpd_qtail

// n > 0: peek in forward direction, from tail
// n < 0: peek in reverse direction, from head
static enum hpd_event hpd_mque_peek(int n)
{
	int hpd_ptr;
	if (hpd_mque_depth() == 0)
		return hpd_none;

	if (n > 0)
		hpd_ptr = (hpd_qtail + n - 1 + MAX_HPD_MSG_QUEUE) % MAX_HPD_MSG_QUEUE;
	else if (n < 0)
		hpd_ptr = (hpd_qhead + n + 1 + MAX_HPD_MSG_QUEUE) % MAX_HPD_MSG_QUEUE;
	else
		hpd_ptr = hpd_qtail;

	return hpd_msg_queue[hpd_ptr];
}

static enum hpd_event hpd_mque_last(void)
{
	if (hpd_mque_depth() > 0)
		return hpd_mque_peek(-1);
	else
		return hpd_reported_event;
}

static enum hpd_event hpd_mque_get(void)
{
	enum hpd_event ev = hpd_msg_queue[hpd_qtail];
	if (hpd_qhead == hpd_qtail) {
		/* QUEUE empty */
		CPRINTF("HPD MQue.E\n");
		return hpd_none;
	}
	hpd_qtail = (hpd_qtail + 1) % MAX_HPD_MSG_QUEUE;
	return ev;
}

static void hpd_mque_put(enum hpd_event ev)
{
	hpd_msg_queue[hpd_qhead] = ev;
	hpd_qhead = (hpd_qhead + 1) % MAX_HPD_MSG_QUEUE;
	if (hpd_qhead == hpd_qtail) {
		/* QUEUE full */
		CPRINTF("HPD MQue.F: %d\n", ev);
		hpd_qtail = (hpd_qtail + 1) % MAX_HPD_MSG_QUEUE;
	}
}

static void hpd_mque_launch_deferred(void);
static void hpd_mque_launch(enum hpd_event ev)
{
	int depth = hpd_mque_depth();
	hpd_mque_put(ev);
	if (depth == 0)
		hook_call_deferred(hpd_mque_launch_deferred, 0);
}

static void hpd_mque_launch_deferred(void)
{
	hpd_reported_event = hpd_mque_get();
	pd_send_hpd(0, hpd_reported_event);
	if (hpd_mque_depth() > 0)
		hook_call_deferred(hpd_mque_launch_deferred, HPD_MSG_QUEUE_GAP);
}
DECLARE_DEFERRED(hpd_mque_launch_deferred);

static void hpd_lvl_deferred(void)
{
	enum hpd_event ev = (gpio_get_level(GPIO_DP_HPD)) ? hpd_high : hpd_low;
	if (ev != hpd_reported_event) {
		hpd_reported_event = ev;
		pd_send_hpd(0, hpd_reported_event);
	}
}
DECLARE_DEFERRED(hpd_lvl_deferred);

void hpd_event_trigger_clear(void)
{
	hpd_event_triggering = 0;
}
DECLARE_DEFERRED(hpd_event_trigger_clear);

void hpd_event(enum gpio_signal signal)
{
	timestamp_t now = get_time();
	int level = gpio_get_level(signal);
	uint64_t cur_delta = now.val - hpd_prev_ts;
	static unsigned glitch_count = 0;

	/* store current time */
	hpd_prev_ts = now.val;

	/* All previous hpd level events need to be re-triggered */
	hook_call_deferred(hpd_lvl_deferred, -1);

	if (hpd_prev_level == level) {
		CPRINTF("ERR:HPD_EVT lev=%d\n", level);
		glitch_count++;
		return;
	}
	hpd_prev_level = level;

	// Type-C end must be DFP_D (or BOTH) for this HPD event message
	if (!(tce_conn_status & DP_STS_CONN_DFPD))  // not DFP_D or BOTH
		return;

	if (level) { // GLITCH: < 250us
		// -------+  +-----++-
		//        |  |     ||     HIGH, IRQ, HIGH, GLITCH
		//        +--+     ++ 
		// -------+  +--++-
		//        |  |  ||        HIGH, IRQ, glitch, GLITCH
		//        +--+  ++ 
		// -------+  +-----+  +-
		//        |  |     |  |   HIGH, IRQ, HIGH, IRQ
		//        +--+     +--+ 
		// -------+  +--+  +-
		//        |  |  |  |      HIGH, IRQ, glitch, IRQ
		//        +--+  +--+ 
		// ---+      +--+  +-
		//    |      |  |  |      HIGH, LOW, glitch, IRQ
		//    +------+  +--+ 
		//
		// -------+      +-
		//        |      |        HIGH, LOW
		//        +------+ 
		if (cur_delta <= HPD_DEBOUNCE_GLITCH) {
			glitch_count++;
			hook_call_deferred(hpd_lvl_deferred, HPD_DEBOUNCE_LVL);
			return;
		}
		else
		if (cur_delta <= HPD_DEBOUNCE_IRQ) {
			// %SPEC%: <1> and <2>
			// ensure previous status is HIGH
			enum hpd_event pre_event = hpd_mque_last();
			if (pre_event == hpd_low) {
				CPRINTF("ERR:HPD_EVT GLITCHs-IRQ:%d\n", glitch_count);

				// reset and inject HIGH into queue
				hpd_mque_reset();
				hpd_mque_put(hpd_high);
			}
			else
			if (hpd_mque_depth() > 1) {
				// only keep 1 event in queue
				hpd_mque_reset();
				hpd_mque_put(pre_event);
			}

			hpd_mque_launch(hpd_irq);
			glitch_count = 0;
			return;
		}
		else {
			// We will honor it as a LEVEL-LOW, even it may be smaller than 100ms
			// %SPEC%: <4>
			hpd_mque_reset();
			hpd_mque_launch( hpd_low );

			hook_call_deferred(hpd_lvl_deferred, HPD_DEBOUNCE_LVL);
			glitch_count = 0;
			return;
		}
	}
	else { // GLITCH: < 2ms
		// ---+  +--+
		//    |  |  |             HIGH, IRQ, GLITCH
		//    +--+  +-
		// ---+      +--+
		//    |      |  |         HIGH, LOW, GLITCH
		//    +------+  +-
		// ---+      +------+
		//    |      |      |     HIGH, LOW, HIGH
		//    +------+      +-

		if (cur_delta <= HPD_DEBOUNCE_IRQ)
			glitch_count++;
		else {
			// We will honor it as a LEVEL-HIGH, even it may be smaller than 100ms
			// %SPEC%: <4>
			hpd_mque_launch( hpd_high );
			glitch_count = 0;
		}
		hook_call_deferred(hpd_lvl_deferred, HPD_DEBOUNCE_LVL);
		return;
	}

	hpd_event_triggering = 1;
	hook_call_deferred(hpd_event_trigger_clear, 300 * MSEC);
}

#define DPE_CONN_MONITOR_INTERVAL   100 * MSEC
#define DPE_CONN_UNKNOWN_RESOLVING  12

extern void pd_attention_dp_status(int port);

/* periodic monitor the DP AUX, for UFP_D / DFP_D detection */
static void dp_aux_gpio_monitor(void)
{
	int port = 0;
	int new_sts = dpe_plug_conn_status(port);
	static int unknown_resolving_cnt = 0;
	static int sbu = -1;

	if (hpd_event_triggering) {
		// skip AUX monitoring while HPD event
		if (new_sts != DPE_PLUG_UNKNOWN) {
			if (new_sts != dpe_conn_status)
				ccprints("DPE:%d->%d hpd:%d EN:%d AUX_N/P:%d/%d", dpe_conn_status, new_sts,
					gpio_get_level(GPIO_DP_HPD), gpio_get_level(GPIO_PD_SBU_ENABLE),
					adc_read_channel(ADC_CH_AUX_N), adc_read_channel(ADC_CH_AUX_P));
			dpe_conn_status = new_sts;
		}
		goto L_exit;
	}

	if (new_sts == DPE_PLUG_UNKNOWN) {
		int aux_n=-1, aux_p=-1;

		if (++unknown_resolving_cnt <= DPE_CONN_UNKNOWN_RESOLVING) {
			if (unknown_resolving_cnt == DPE_CONN_UNKNOWN_RESOLVING-1) {
				// break the SBU switch to do correct ADC measurement of AUX_N/P
				sbu = gpio_get_level(GPIO_PD_SBU_ENABLE);
				gpio_set_level(GPIO_PD_SBU_ENABLE, 0);
				goto L_exit;
			}
			else
			if (unknown_resolving_cnt != DPE_CONN_UNKNOWN_RESOLVING) {
				goto L_exit;
			}

			new_sts = dpe_plug_conn_status(port);
			aux_n = adc_read_channel(ADC_CH_AUX_N);
			aux_p = adc_read_channel(ADC_CH_AUX_P);
		}

		// Still in UNKNOWN state?
		if (new_sts == DPE_PLUG_UNKNOWN) {
			new_sts = DPE_PLUG_NONE;

			if (new_sts != dpe_conn_status)
				ccprintf("dpe:%d hpd:%d en:%d aux_n/p:%d/%d\n", dpe_conn_status,
					gpio_get_level(GPIO_DP_HPD), gpio_get_level(GPIO_PD_SBU_ENABLE),
					aux_n, aux_p);
		}
	}
	unknown_resolving_cnt = 0;
	if (sbu >= 0) {
		gpio_set_level(GPIO_PD_SBU_ENABLE, sbu);
		sbu = -1;
	}

	if (new_sts == dpe_conn_status)
		goto L_exit;

	ccprints("DPE:%d->%d HPD:%d EN:%d AUX_N/P:%d/%d", dpe_conn_status, new_sts,
		gpio_get_level(GPIO_DP_HPD), gpio_get_level(GPIO_PD_SBU_ENABLE),
		adc_read_channel(ADC_CH_AUX_N), adc_read_channel(ADC_CH_AUX_P));

	dpe_conn_status = new_sts;
	switch (dpe_conn_status) {
	case DPE_PLUG_DFPD:
		gpio_set_flags( GPIO_DP_HPD, GPIO_OUT_LOW );
		break;
	case DPE_PLUG_UFPD:
	default:
		gpio_set_flags( GPIO_DP_HPD, GPIO_INT_BOTH );
		break;
	}

	usleep(10);
	pd_attention_dp_status(port);

L_exit:
	hook_call_deferred(dp_aux_gpio_monitor, DPE_CONN_MONITOR_INTERVAL);
}

DECLARE_DEFERRED(dp_aux_gpio_monitor);

/* Initialize board. */
void board_config_pre_init(void)
{
	/* enable SYSCFG clock */
	STM32_RCC_APB2ENR |= 1 << 0;
	/* Remap USART DMA to match the USART driver */
	STM32_SYSCFG_CFGR1 |= (1 << 9) | (1 << 10);/* Remap USART1 RX/TX DMA */
}

/* Initialize board. */
static void board_init(void)
{
	timestamp_t now = get_time();
	hpd_prev_level = gpio_get_level(GPIO_DP_HPD);
	hpd_prev_ts = now.val;
	gpio_enable_interrupt(GPIO_DP_HPD);

	hook_call_deferred(dp_aux_gpio_monitor, DPE_CONN_MONITOR_INTERVAL);

	gpio_set_level(GPIO_STM_READY, 1); /* factory test only */
}

DECLARE_HOOK(HOOK_INIT, board_init, HOOK_PRIO_DEFAULT);

/* ADC channels */
const struct adc_t adc_channels[] = {
	/* USB PD CC lines sensing. Converted to mV (3300mV/4096). */
	[ADC_CH_CC1_PD] = {"USB_C_CC1_PD", 3300, 4096, 0, STM32_AIN(1)},
	[ADC_CH_AUX_N]  = {"DP_AUX_N", 3300, 4096, 0, STM32_AIN(5)},
	[ADC_CH_AUX_P]  = {"DP_AUX_P", 3300, 4096, 0, STM32_AIN(6)},
};
BUILD_ASSERT(ARRAY_SIZE(adc_channels) == ADC_CH_COUNT);
const void * const usb_strings[] = {
	[USB_STR_DESC] = usb_string_desc,
	[USB_STR_VENDOR] = USB_STRING_DESC(CONFIG_USB_VENDOR_STR),
	[USB_STR_PRODUCT] = USB_STRING_DESC(CONFIG_USB_PRODUCT_STR),
	[USB_STR_VERSION] = USB_STRING_DESC(CROS_EC_VERSION32),
	[USB_STR_BB_URL] = USB_STRING_DESC(CONFIG_USB_BB_URL),
	[USB_STR_CONSOLE_NAME] = USB_STRING_DESC("Shell"),
};
BUILD_ASSERT(ARRAY_SIZE(usb_strings) == USB_STR_COUNT);

/**
 * USB configuration
 * Any type-C device with alternate mode capabilities must have the following
 * set of descriptors.
 *
 * 1. Standard Device
 * 2. BOS
 *    2a. Container ID
 *    2b. Billboard Caps
 */
struct my_bos {
	struct usb_bos_hdr_descriptor bos;
	struct usb_contid_caps_descriptor contid_caps;
	struct usb_bb_caps_base_descriptor bb_caps;
	struct usb_bb_caps_svid_descriptor bb_caps_svids[1];
};

static struct my_bos bos_desc = {
	.bos = {
		.bLength = USB_DT_BOS_SIZE,
		.bDescriptorType = USB_DT_BOS,
		.wTotalLength = (USB_DT_BOS_SIZE + USB_DT_CONTID_SIZE +
				 USB_BB_CAPS_BASE_SIZE +
				 USB_BB_CAPS_SVID_SIZE * 1),
		.bNumDeviceCaps = 2,  /* contid + bb_caps */
	},
	.contid_caps =	{
		.bLength = USB_DT_CONTID_SIZE,
		.bDescriptorType = USB_DT_DEVICE_CAPABILITY,
		.bDevCapabilityType = USB_DC_DTYPE_CONTID,
		.bReserved = 0,
		.ContainerID = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},
	.bb_caps = {
		.bLength = (USB_BB_CAPS_BASE_SIZE + USB_BB_CAPS_SVID_SIZE * 1),
		.bDescriptorType = USB_DT_DEVICE_CAPABILITY,
		.bDevCapabilityType = USB_DC_DTYPE_BILLBOARD,
		.iAdditionalInfoURL = USB_STR_BB_URL,
		.bNumberOfAlternateModes = 1,
		.bPreferredAlternateMode = 1,
		.VconnPower = 0,
		.bmConfigured = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.bReserved = 0,
	},
	.bb_caps_svids = {
		{
			.wSVID = USB_SID_DISPLAYPORT,
			.bAlternateMode = 1,
			.iAlternateModeString = USB_STR_BB_URL, /* TODO(crosbug.com/p/32687) */
		},
	},
};

const struct bos_context bos_ctx = {
	.descp = (void *)&bos_desc,
	.size = sizeof(struct my_bos),
};
