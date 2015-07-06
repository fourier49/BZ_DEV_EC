/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* Hoho dongle configuration */

#include "adc.h"
#include "adc_chip.h"
#include "common.h"
#include "ec_commands.h"
#include "ec_version.h"
#include "console.h"
#include "gpio.h"
#include "hooks.h"
#include "mcdp28x0.h"
#include "registers.h"
#include "task.h"
#include "usb.h"
#include "usb_bb.h"
#include "usb_pd.h"
#include "timer.h"
#include "util.h"

#define MAX_HPD_MSG_QUEUE   4
#define HPD_MSG_QUEUE_GAP   (4*MSEC)

static volatile uint64_t hpd_prev_ts;
static volatile int hpd_prev_level;
static volatile enum hpd_event hpd_reported_event = hpd_none;

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
}

/* Initialize board. */
void board_config_pre_init(void)
{
	/* enable SYSCFG clock */
	STM32_RCC_APB2ENR |= 1 << 0;
	/* Remap USART DMA to match the USART driver */
	STM32_SYSCFG_CFGR1 |= (1 << 9) | (1 << 10);/* Remap USART1 RX/TX DMA */
}

#ifdef CONFIG_SPI_FLASH

static void board_init_spi2(void)
{
	/* Remap SPI2 to DMA channels 6 and 7 */
	STM32_SYSCFG_CFGR1 |= (1 << 24);

	/* Set pin NSS to general purpose output mode (01b). */
	/* Set pins SCK, MISO, and MOSI to alternate function (10b). */
	STM32_GPIO_MODER(GPIO_B) &= ~0xff000000;
	STM32_GPIO_MODER(GPIO_B) |= 0xa9000000;

	/* Set all four pins to alternate function 0 */
	STM32_GPIO_AFRH(GPIO_B) &= ~(0xffff0000);

	/* Set all four pins to output push-pull */
	STM32_GPIO_OTYPER(GPIO_B) &= ~(0xf000);

	/* Set pullup on NSS */
	STM32_GPIO_PUPDR(GPIO_B) |= 0x1000000;

	/* Set all four pins to high speed */
	STM32_GPIO_OSPEEDR(GPIO_B) |= 0xff000000;

	/* Reset SPI2 */
	STM32_RCC_APB1RSTR |= (1 << 14);
	STM32_RCC_APB1RSTR &= ~(1 << 14);

	/* Enable clocks to SPI2 module */
	STM32_RCC_APB1ENR |= STM32_RCC_PB1_SPI2;
}
#endif /* CONFIG_SPI_FLASH */

static void factory_validation_deferred(void)
{
	struct mcdp_info info;

	mcdp_enable();

	/* test mcdp via serial to validate function */
	if (!mcdp_get_info(&info) && (MCDP_FAMILY(info.family) == 0xe) &&
	    (MCDP_CHIPID(info.chipid) == 0x1)) {
		gpio_set_level(GPIO_MCDP_READY, 1);
		pd_log_event(PD_EVENT_VIDEO_CODEC,
			     PD_LOG_PORT_SIZE(0, sizeof(info)),
			     0, &info);
	}

	mcdp_disable();
}
DECLARE_DEFERRED(factory_validation_deferred);

/* Initialize board. */
static void board_init(void)
{
	timestamp_t now;
#ifdef CONFIG_SPI_FLASH
	board_init_spi2();
#endif
	now = get_time();
	hpd_prev_level = gpio_get_level(GPIO_DP_HPD);
	hpd_prev_ts = now.val;
	gpio_enable_interrupt(GPIO_DP_HPD);

	gpio_set_level(GPIO_STM_READY, 1); /* factory test only */
	/* Delay needed to allow HDMI MCU to boot. */
	hook_call_deferred(factory_validation_deferred, 200*MSEC);
}

DECLARE_HOOK(HOOK_INIT, board_init, HOOK_PRIO_DEFAULT);

/* ADC channels */
const struct adc_t adc_channels[] = {
	/* USB PD CC lines sensing. Converted to mV (3300mV/4096). */
	[ADC_CH_CC1_PD] = {"USB_C_CC1_PD", 3300, 4096, 0, STM32_AIN(1)},
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
