/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
/* samus_pd board configuration */

#include "adc.h"
#include "adc_chip.h"
#include "common.h"
#include "ec_version.h"
#include "console.h"
#include "gpio.h"
#include "hooks.h"
#include "i2c.h"
#include "registers.h"
#include "task.h"
#include "usb.h"
#include "usb_bb.h"
#include "usb_pd.h"
#include "usb_pd_config.h"
#include "timer.h"
#include "util.h"

#define POWER_SIGNALS_DEBOUNCE_INTERVAL  (20*MSEC)
#define POWER_SIGNALS_DEBOUNCE2_INTERVAL (500*MSEC)

#define MAX_HPD_MSG_QUEUE   4
#define HPD_MSG_QUEUE_GAP   (4*MSEC)

static volatile uint64_t hpd_prev_ts;
static volatile int hpd_prev_level;
static volatile enum hpd_event hpd_reported_event = hpd_none;

void hpd_event(enum gpio_signal signal);


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
#if defined(CONFIG_USB_PD_ALT_MODE) && !defined(CONFIG_USB_PD_ALT_MODE_DFP)

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
	enum hpd_event ev = (gpio_get_level(GPIO_USB_P0_DP_HPD)) ? hpd_high : hpd_low;
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
#endif

void vbus0_evt(enum gpio_signal signal)
{
	ccprintf("VBUS %d, %d!\n", signal, gpio_get_level(signal));
	task_wake(TASK_ID_PD_P0);
}

#ifdef CONFIG_BIZ_DUAL_CC
void vbus1_evt(enum gpio_signal signal)
{
	ccprintf("VBUS %d, %d!\n", signal, gpio_get_level(signal));
	task_wake(TASK_ID_PD_P1);
}
#endif

#ifndef CONFIG_BIZ_EMU_HOST
#ifndef CONFIG_BIZ_HULK
int pwr_dc_in_detection=0;

static void pwr_in_debounce(void)
{
	int lvl = gpio_get_level(GPIO_MCU_PWR_DC_IN_DET);
	if (lvl != pwr_dc_in_detection)
		return;

	// whenever DC is in, we won't need Host's VBUS power
	pd_pwr_local_change(pwr_dc_in_detection);

	// we may get false HPD signals, get it being stablized
	hpd_reported_event = hpd_none;
	hook_call_deferred(hpd_lvl_deferred, POWER_SIGNALS_DEBOUNCE2_INTERVAL);

	// task_wake(TASK_ID_PD_P1);
}
DECLARE_DEFERRED(pwr_in_debounce);

void pwr_in_event(enum gpio_signal signal)
{
	pwr_dc_in_detection = gpio_get_level(GPIO_MCU_PWR_DC_IN_DET);
	ccprintf("DC-IN %d!\n", pwr_dc_in_detection);

	//hook_call_deferred(pwr_in_debounce, POWER_SIGNALS_DEBOUNCE_INTERVAL);
}
#endif
#endif

// must be after GPIO's signal definition
#include "gpio_list.h"


//==============================================================================
void board_config_pre_init(void)
{
	/* enable SYSCFG clock */
	STM32_RCC_APB2ENR |= 1 << 0;
	/*
	 * the DMA mapping is :
	 *  Chan 2 : TIM1_CH1  (C0 RX)
	 *  Chan 3 : SPI1_TX   (C1 TX)
	 *  Chan 4 : USART1_TX
	 *  Chan 5 : USART1_RX
	 *  Chan 6 : TIM3_CH1  (C1 RX)
	 *  Chan 7 : SPI2_TX   (C0 TX)
	 */

	/*
	 * Remap USART1 RX/TX DMA to match uart driver. Remap SPI2 RX/TX and
	 * TIM3_CH1 for unique DMA channels.
	 */
	STM32_SYSCFG_CFGR1 |= (1 << 9) | (1 << 10) | (1 << 24) | (1 << 30);
}

#ifdef CONFIG_SPI_FLASH
// from board/hoho
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

#ifdef CONFIG_BIZ_EMU_HOST
void pd_task_dummy(void)
{
	int port = TASK_ID_TO_PORT(task_get_current());
	if (port == 0) {
		while (1) task_wait_event(5000*MSEC);
	}
}
#endif

extern enum pd_dual_role_states drp_states[PD_PORT_COUNT];
/* Initialize board. */
static void board_init(void)
{
#if defined(CONFIG_USB_PD_ALT_MODE) && !defined(CONFIG_USB_PD_ALT_MODE_DFP)
	timestamp_t now;

	now = get_time();
	hpd_prev_level = gpio_get_level(GPIO_USB_P0_DP_HPD);
	hpd_prev_ts = now.val;
	gpio_enable_interrupt(GPIO_USB_P0_DP_HPD);
#endif

#ifdef CONFIG_SPI_FLASH
	board_init_spi2();
#endif

#if 0
	/*
	 * Enable CC lines after all GPIO have been initialized. Note, it is
	 * important that this is enabled after the CC_ODL lines are set low
	 * to specify device mode.
	 */
	gpio_set_level(GPIO_USB_C_CC_EN, 1);
#endif

#ifdef CONFIG_BIZ_EMU_HOST
	drp_states[0] = PD_DRP_TOGGLE_ON;
	drp_states[1] = PD_DRP_TOGGLE_ON;
#else
	drp_states[0] = PD_DRP_TOGGLE_ON;
	drp_states[1] = PD_DRP_TOGGLE_OFF;
	/* Enable interrupts on VBUS transitions. */
#ifndef CONFIG_BIZ_HULK
	gpio_enable_interrupt(GPIO_USB_P0_VBUS_WAKE);
	gpio_enable_interrupt(GPIO_MCU_PWR_DC_IN_DET);
	pwr_in_event(GPIO_MCU_PWR_DC_IN_DET);
#endif

#ifdef CONFIG_BIZ_HULK
	ccprintf("[hook]pd_check_charger_deferred %d\n",POWER_SIGNALS_DEBOUNCE_INTERVAL);
	hook_call_deferred(pd_check_charger_deferred, POWER_SIGNALS_DEBOUNCE_INTERVAL);
#endif

#endif
}
DECLARE_HOOK(HOOK_INIT, board_init, HOOK_PRIO_DEFAULT);


//==============================================================================
/* ADC channels */
const struct adc_t adc_channels[] = {
	/* USB PD CC lines sensing. Converted to mV (3300mV/4096). */
	[ADC_P0_CC_PD]  = {"P0_CC_PD",  3300, 4096, 0, STM32_AIN(0)},
	[ADC_P1_CC1_PD] = {"P1_CC1_PD", 3300, 4096, 0, STM32_AIN(2)},
	[ADC_P1_CC2_PD] = {"P1_CC2_PD", 3300, 4096, 0, STM32_AIN(5)},
      [ADC_P1_VBUS_DT] = {"P1_VBUS_DETECT",  3300, 4096, 0, STM32_AIN(6)},   //P1 vbus detect	
	[ADC_P0_VBUS_DT] = {"P0_VBUS_DETECT",  3300, 4096, 0, STM32_AIN(15)},   //P0 vbus detect	
	[ADC_P1_20VBUS_DT] = {"USB_VBUS_DETECT",  3300, 4096, 0, STM32_AIN(3)},   //P1 20V vbus detect
	/* Vbus sensing. Converted to mV, full ADC is equivalent to 25.774V. */
	[ADC_BOOSTIN] = {"V_BOOSTIN",  25774, 4096, 0, STM32_AIN(11)},
	
	};
BUILD_ASSERT(ARRAY_SIZE(adc_channels) == ADC_CH_COUNT);

/* I2C ports */
const struct i2c_port_t i2c_ports[] = {
#if 0
	{"master", I2C_PORT_MASTER, 100,
		GPIO_MASTER_I2C_SCL, GPIO_MASTER_I2C_SDA},
	{"slave",  I2C_PORT_SLAVE, 100,
		GPIO_SLAVE_I2C_SCL, GPIO_SLAVE_I2C_SDA},
#endif
};
const unsigned int i2c_ports_used = ARRAY_SIZE(i2c_ports);


//==============================================================================
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


//==============================================================================
struct usb_port_mux
{
	enum gpio_signal dp_mode_l;
	enum gpio_signal dp_2_4_lanes;
};

const struct usb_port_mux usb_muxes[] = {
	{
#ifndef CONFIG_BIZ_EMU_HOST
#ifndef CONFIG_BIZ_HULK
		.dp_mode_l    = GPIO_USB_P0_SBU_ENABLE,
		.dp_2_4_lanes = GPIO_USB_P0_DP_SS_LANE,
#endif
#endif
	},
#ifdef CONFIG_BIZ_EMU_HOST
	{
		.dp_mode_l    = GPIO_USB_P1_SBU_ENABLE,
		.dp_2_4_lanes = GPIO_USB_P1_DP_SS_LANE,
	},
#endif
};
//BUILD_ASSERT(ARRAY_SIZE(usb_muxes) == PD_PORT_COUNT);

void board_set_usb_mux(int port, enum typec_mux mux, enum usb_switch usb, int polarity)
{
	const struct usb_port_mux *usb_mux = usb_muxes + port;

	/* reset everything */
	gpio_set_level(usb_mux->dp_mode_l,    0);   // default: disable DP AUX
	gpio_set_level(usb_mux->dp_2_4_lanes, 0);   // default: 2 lanes mode

#if 0
	/* Set D+/D- switch to appropriate level */
	board_set_usb_switches(port, usb);
#endif

	if (mux == TYPEC_MUX_NONE)
		/* everything is already disabled, we can return */
		return;

#if 0
	if (mux == TYPEC_MUX_USB || mux == TYPEC_MUX_DOCK) {
		/* USB 3.0 uses 2 superspeed lanes */
		gpio_set_level(polarity ? usb_mux->ss2_dp_mode :
					  usb_mux->dp_2_4_lanes, 0);
	}

	if (mux == TYPEC_MUX_DP || mux == TYPEC_MUX_DOCK) {
		/* DP uses available superspeed lanes (x2 or x4) */
		gpio_set_level(usb_mux->dp_polarity, polarity);
		gpio_set_level(usb_mux->dp_mode_l, 0);
	}
	/* switch on superspeed lanes */
	gpio_set_level(usb_mux->ss1_en_l, 0);
	gpio_set_level(usb_mux->ss2_en_l, 0);
#else
	if (mux == TYPEC_MUX_USB) {
		gpio_set_level(usb_mux->dp_mode_l, 1);  // enable U3 SS lanes
	}
	if (mux == TYPEC_MUX_DP || mux == TYPEC_MUX_DOCK) {
		gpio_set_level(usb_mux->dp_mode_l, 1);  // enable DP AUX
	}
	if (mux == TYPEC_MUX_DP) {
		gpio_set_level(usb_mux->dp_2_4_lanes, 1);  // 4 lanes
	}
#endif
}

int board_get_usb_mux(int port, const char **dp_str, const char **usb_str)
{
#if 0
	const struct usb_port_mux *usb_mux = usb_muxes + port;
	int has_ss, has_usb, has_dp;
	const char *dp, *usb;

	has_ss = !gpio_get_level(usb_mux->ss1_en_l);
	has_usb = !gpio_get_level(usb_mux->dp_2_4_lanes) ||
		  !gpio_get_level(usb_mux->ss2_dp_mode);
	has_dp = !gpio_get_level(usb_mux->dp_mode_l);
	dp = gpio_get_level(usb_mux->dp_polarity) ?
			"DP2" : "DP1";
	usb = gpio_get_level(usb_mux->dp_2_4_lanes) ?
			"USB2" : "USB1";

	*dp_str = has_dp ? dp : NULL;
	*usb_str = has_usb ? usb : NULL;

	return has_ss;
#else
	const struct usb_port_mux *usb_mux = usb_muxes + port;
	int has_usb, has_dp;

	has_usb = !gpio_get_level(usb_mux->dp_2_4_lanes);
	has_dp  = gpio_get_level(usb_mux->dp_mode_l);

	if (has_usb) {
		*usb_str = "USB3";
		*dp_str = has_dp ? "2DP" : NULL;
	}
	else {
		*usb_str = "USB2";
		*dp_str = has_dp ? "4DP" : NULL;
	}

	return 1;
#endif
}

void board_flip_usb_mux(int port)
{
	const struct usb_port_mux *usb_mux = usb_muxes + port;
	static int toggle=0;
#if 0
	int usb_polarity;

	/* Flip DP polarity */
	gpio_set_level(usb_mux->dp_polarity,
		       !gpio_get_level(usb_mux->dp_polarity));

	/* Flip USB polarity if enabled */
	if (gpio_get_level(usb_mux->dp_2_4_lanes) &&
	    gpio_get_level(usb_mux->ss2_dp_mode))
		return;
	usb_polarity = gpio_get_level(usb_mux->dp_2_4_lanes);

	/*
	 * Disable both sides first so that we don't enable both at the
	 * same time accidentally.
	 */
	gpio_set_level(usb_mux->dp_2_4_lanes, 1);
	gpio_set_level(usb_mux->ss2_dp_mode, 1);

	gpio_set_level(usb_mux->dp_2_4_lanes, !usb_polarity);
	gpio_set_level(usb_mux->ss2_dp_mode, usb_polarity);
#else
	gpio_set_level(usb_mux->dp_2_4_lanes, toggle);
	toggle = 1 - toggle;
#endif
}

