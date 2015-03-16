/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "atomic.h"
#include "charge_manager.h"
#include "common.h"
#include "console.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "registers.h"
#include "task.h"
#include "timer.h"
#include "util.h"
#include "usb_pd.h"

#define CPRINTF(format, args...) cprintf(CC_USBPD, format, ## args)
#define CPRINTS(format, args...) cprints(CC_USBPD, format, ## args)


//==============================================================================
/* Define typical operating power and max power */
#define OPERATING_POWER_MW 15000
#define MAX_POWER_MW       60000
#define MAX_CURRENT_MA     3000

/*
 * Do not request any voltage within this deadband region, where
 * we're not sure whether or not the boost or the bypass will be on.
 */
#define INPUT_VOLTAGE_DEADBAND_MIN 9700
#define INPUT_VOLTAGE_DEADBAND_MAX 11999

#define PDO_FIXED_FLAGS (PDO_FIXED_DUAL_ROLE | PDO_FIXED_DATA_SWAP)

const uint32_t pd_src_pdo[] = {
		PDO_FIXED(5000,   900, PDO_FIXED_FLAGS),
		PDO_FIXED(20000, 3000, PDO_FIXED_FLAGS),
};
const int pd_src_pdo_cnt = ARRAY_SIZE(pd_src_pdo);

const uint32_t pd_snk_pdo[] = {
		PDO_FIXED(5000, 500, PDO_FIXED_FLAGS),
		PDO_BATT(5000, 20000, 15000),
		PDO_VAR(5000, 20000, 3000),
};
const int pd_snk_pdo_cnt = ARRAY_SIZE(pd_snk_pdo);

/* Cap on the max voltage requested as a sink (in millivolts) */
static unsigned max_mv = -1; /* no cap */

int pd_choose_voltage_common(int cnt, uint32_t *src_caps, uint32_t *rdo,
			     uint32_t *curr_limit, uint32_t *supply_voltage,
			     int choose_min)
{
	int i;
	int sel_mv;
	int max_uw = 0;
	int max_ma;
	int max_i = -1;
	int max;
	uint32_t flags;

	/* Get max power */
	for (i = 0; i < cnt; i++) {
		int uw;
		int mv = ((src_caps[i] >> 10) & 0x3FF) * 50;
		if ((src_caps[i] & PDO_TYPE_MASK) == PDO_TYPE_BATTERY) {
			uw = 250000 * (src_caps[i] & 0x3FF);
		} else {
			int ma = (src_caps[i] & 0x3FF) * 10;
			uw = ma * mv;
		}
		if ((uw > max_uw) && (mv <= max_mv)) {
			max_i = i;
			max_uw = uw;
			sel_mv = mv;
		}
		/*
		 * Choose the first entry if seaching for minimum, which will
		 * always be vSafe5V.
		 */
		if (choose_min)
			break;
	}
	if (max_i < 0)
		return -EC_ERROR_UNKNOWN;

	/* build rdo for desired power */
	if ((src_caps[max_i] & PDO_TYPE_MASK) == PDO_TYPE_BATTERY) {
		int uw = 250000 * (src_caps[max_i] & 0x3FF);
		max = MIN(1000 * uw, MAX_POWER_MW);
		flags = (max < OPERATING_POWER_MW) ? RDO_CAP_MISMATCH : 0;
		max_ma = 1000 * max / sel_mv;
		*rdo = RDO_BATT(max_i + 1, max, max, flags);
		CPRINTF("Request [%d] %dV %dmW",
			max_i, sel_mv/1000, max);
	} else {
		int ma = 10 * (src_caps[max_i] & 0x3FF);
		/*
		 * If we're choosing the minimum charge mode, limit our current
		 * to what we can set with ilim PWM (500mA)
		 */
		max = MIN(ma, choose_min ? CONFIG_CHARGER_INPUT_CURRENT :
					   MAX_CURRENT_MA);
		flags = (max * sel_mv) < (1000 * OPERATING_POWER_MW) ?
				RDO_CAP_MISMATCH : 0;
		max_ma = max;
		*rdo = RDO_FIXED(max_i + 1, max, max, flags);
		CPRINTF("Request [%d] %dV %dmA",
			max_i, sel_mv/1000, max);
	}
	/* Mismatch bit set if less power offered than the operating power */
	if (flags & RDO_CAP_MISMATCH)
		CPRINTF(" Mismatch");
	CPRINTF("\n");

	*curr_limit = max_ma;
	*supply_voltage = sel_mv;
	return EC_SUCCESS;
}

int pd_choose_voltage_min(int cnt, uint32_t *src_caps, uint32_t *rdo,
			  uint32_t *curr_limit, uint32_t *supply_voltage)
{
	return pd_choose_voltage_common(cnt, src_caps, rdo, curr_limit,
					supply_voltage, 1);
}

int pd_choose_voltage(int cnt, uint32_t *src_caps, uint32_t *rdo,
		      uint32_t *curr_limit, uint32_t *supply_voltage)
{
	return pd_choose_voltage_common(cnt, src_caps, rdo, curr_limit,
					supply_voltage, 0);
}

int pd_is_valid_input_voltage(int mv)
{
        /* Allow any voltage not in the boost bypass deadband */
        return  (mv < INPUT_VOLTAGE_DEADBAND_MIN) ||
                (mv > INPUT_VOLTAGE_DEADBAND_MAX);
}

int pd_check_requested_voltage(uint32_t rdo)
{
	int max_ma = rdo & 0x3FF;
	int op_ma = (rdo >> 10) & 0x3FF;
	int idx = rdo >> 28;
	uint32_t pdo;
	uint32_t pdo_ma;

	if (!idx || idx > pd_src_pdo_cnt)
		return EC_ERROR_INVAL; /* Invalid index */

	/* check current ... */
	pdo = pd_src_pdo[idx - 1];
	pdo_ma = (pdo & 0x3ff);

	CPRINTF("Requested %d V %d mA (for %d/%d mA)\n",
		 ((pdo >> 10) & 0x3ff) * 50, (pdo & 0x3ff) * 10,
		 ((rdo >> 10) & 0x3ff) * 10, (rdo & 0x3ff) * 10);

	if (op_ma > pdo_ma)
		return EC_ERROR_INVAL; /* too much op current */
	if (max_ma > pdo_ma)
		return EC_ERROR_INVAL; /* too much max current */

	return EC_SUCCESS;
}

void pd_transition_voltage(int idx)
{
	/* No-operation: we are always 5V */
}

int pd_set_power_supply_ready(int port)
{
	/* provide VBUS */
#ifdef CONFIG_BIZ_EMU_HOST
	gpio_set_level(port ? GPIO_USB_P1_PWR_5V_EN : GPIO_USB_P0_PWR_5V_EN, 1);
#else
	if (port == 0)  gpio_set_level(GPIO_USB_P0_PWR_5V_EN, 1);
#endif

	/* notify host of power info change */
//	pd_send_host_event(PD_EVENT_POWER_CHANGE);

	return EC_SUCCESS; /* we are ready */
}

void pd_power_supply_reset(int port)
{
	/* Kill VBUS */
#ifdef CONFIG_BIZ_EMU_HOST
	gpio_set_level(port ? GPIO_USB_P1_PWR_5V_EN : GPIO_USB_P0_PWR_5V_EN, 0);
#else
	if (port == 0)  gpio_set_level(GPIO_USB_P0_PWR_5V_EN, 0);
#endif

	/* notify host of power info change */
//	pd_send_host_event(PD_EVENT_POWER_CHANGE);
}

void pd_set_input_current_limit(int port, uint32_t max_ma,
				uint32_t supply_voltage)
{
	/* No battery, nothing to do */
	return;
}

void typec_set_input_current_limit(int port, uint32_t max_ma,
				   uint32_t supply_voltage)
{
}

int pd_board_checks(void)
{
	return EC_SUCCESS;
}

int pd_check_power_swap(int port)
{
	/* TODO: use battery level to decide to accept/reject power swap */
	/* Always allow power swap */
	return 1;
}

int pd_check_data_swap(int port, int data_role)
{
	/* Allow data swap if we are a UFP, otherwise don't allow */
	return (data_role == PD_ROLE_UFP) ? 1 : 0;
}

void pd_execute_data_swap(int port, int data_role)
{
	/* TODO: when switching to UFP need to open D+/D- switches */
}

void pd_new_contract(int port, int pr_role, int dr_role,
		     int partner_pr_swap, int partner_dr_swap)
{
#ifdef CONFIG_BIZ_EMU_HOST
	/* If UFP, try to switch to DFP */
	if (partner_dr_swap && dr_role == PD_ROLE_UFP)
		pd_request_data_swap(port);
#endif
}

void pd_check_pr_role(int port, int pr_role, int partner_pr_swap)
{
        /* If sink, and dual role toggling is on, then switch to source */
        if (partner_pr_swap && pr_role == PD_ROLE_SINK &&
            pd_get_dual_role() == PD_DRP_TOGGLE_ON)
                pd_request_power_swap(port);
}

void pd_check_dr_role(int port, int dr_role, int partner_dr_swap)
{
        /* If UFP, try to switch to DFP */
        if (partner_dr_swap && dr_role == PD_ROLE_UFP)
                pd_request_data_swap(port);
}

/* ----------------- Vendor Defined Messages ------------------ */

#ifdef CONFIG_USB_PD_ALT_MODE_DFP
//==============================================================================
#ifdef __NEVER_USED__
static void ___CONFIG_DFP__(void) {}
#endif

const struct svdm_response svdm_rsp = {
	.identity = NULL,
	.svids = NULL,
	.modes = NULL,
};

static void svdm_safe_dp_mode(int port)
{
	/* make DP interface safe until configure */
	board_set_usb_mux(port, TYPEC_MUX_NONE, pd_get_polarity(port));
}

static int svdm_enter_dp_mode(int port, uint32_t mode_caps)
{
	/* Only enter mode if device is DFP_D capable */
	if (mode_caps & MODE_DP_SNK) {
		svdm_safe_dp_mode(port);
		return 0;
	}

	return -1;
}

static int dp_on;

static int svdm_dp_status(int port, uint32_t *payload)
{
	payload[0] = VDO(USB_SID_DISPLAYPORT, 1,
			 CMD_DP_STATUS | VDO_OPOS(pd_alt_mode(port, USB_SID_DISPLAYPORT)));
	payload[1] = VDO_DP_STATUS(0, /* HPD IRQ  ... not applicable */
				   0, /* HPD level ... not applicable */
				   0, /* exit DP? ... no */
				   0, /* usb mode? ... no */
				   0, /* multi-function ... no */
				   dp_on,
				   0, /* power low? ... no */
				   dp_on);
	return 2;
};

static int svdm_dp_config(int port, uint32_t *payload)
{
	board_set_usb_mux(port, TYPEC_MUX_DP, pd_get_polarity(port));
	dp_on = 1;
	payload[0] = VDO(USB_SID_DISPLAYPORT, 1,
			 CMD_DP_CONFIG | VDO_OPOS(pd_alt_mode(port, USB_SID_DISPLAYPORT)));
	payload[1] = VDO_DP_CFG(MODE_DP_PIN_C, /* sink pins */
				MODE_DP_PIN_C, /* src pins */
				1,             /* DPv1.3 signaling */
				2);            /* UFP connected */
	return 2;
};

static void hpd0_irq_deferred(void)
{
#if 0
	gpio_set_level(GPIO_USB_P0_DP_HPD, 1);
#endif
}

static void hpd1_irq_deferred(void)
{
	gpio_set_level(GPIO_USB_P1_DP_HPD, 1);
}

DECLARE_DEFERRED(hpd0_irq_deferred);
DECLARE_DEFERRED(hpd1_irq_deferred);

//#define PORT_TO_HPD(port) ((port) ? GPIO_USB_P1_DP_HPD : GPIO_USB_P0_DP_HPD)
#define PORT_TO_HPD(port)    GPIO_USB_P1_DP_HPD

static int svdm_dp_attention(int port, uint32_t *payload)
{
	int cur_lvl;
	int lvl = PD_VDO_HPD_LVL(payload[1]);
	int irq = PD_VDO_HPD_IRQ(payload[1]);
	enum gpio_signal hpd = PORT_TO_HPD(port);
	cur_lvl = gpio_get_level(hpd);
	if (irq & cur_lvl) {
		gpio_set_level(hpd, 0);
		/* 250 usecs is minimum, 2msec is max */
		if (port)
			hook_call_deferred(hpd1_irq_deferred, 300);
		else
			hook_call_deferred(hpd0_irq_deferred, 300);
	} else if (irq & !cur_lvl) {
		CPRINTF("PE ERR: IRQ_HPD w/ HPD_LOW\n");
		return 0; /* nak */
	} else {
		gpio_set_level(hpd, lvl);
	}
	/* ack */
	return 1;
}

static void svdm_exit_dp_mode(int port)
{
	svdm_safe_dp_mode(port);
	gpio_set_level(PORT_TO_HPD(port), 0);
}

static int svdm_enter_gfu_mode(int port, uint32_t mode_caps)
{
	/* Always enter GFU mode */
	return 0;
}

static void svdm_exit_gfu_mode(int port)
{
}

static int svdm_gfu_status(int port, uint32_t *payload)
{
	/*
	 * This is called after enter mode is successful, send unstructured
	 * VDM to read info.
	 */
	pd_send_vdm(port, USB_VID_GOOGLE, VDO_CMD_READ_INFO, NULL, 0);
	return 0;
}

static int svdm_gfu_config(int port, uint32_t *payload)
{
	return 0;
}

static int svdm_gfu_attention(int port, uint32_t *payload)
{
	return 0;
}

const struct svdm_amode_fx supported_modes[] = {
	{
		.svid = USB_SID_DISPLAYPORT,
		.enter = &svdm_enter_dp_mode,
		.status = &svdm_dp_status,
		.config = &svdm_dp_config,
		.attention = &svdm_dp_attention,
		.exit = &svdm_exit_dp_mode,
	},
	{
		.svid = USB_VID_GOOGLE,
		.enter = &svdm_enter_gfu_mode,
		.status = &svdm_gfu_status,
		.config = &svdm_gfu_config,
		.attention = &svdm_gfu_attention,
		.exit = &svdm_exit_gfu_mode,
	}
};
const int supported_modes_cnt = ARRAY_SIZE(supported_modes);

#else  // CONFIG_USB_PD_ALT_MODE_DFP
//==============================================================================
#ifdef __NEVER_USED__
static void ___CONFIG_UFP__(void) {}
#endif

const uint32_t vdo_idh = VDO_IDH(0, /* data caps as USB host */
				 1, /* data caps as USB device */
				 IDH_PTYPE_AMA, /* Alternate mode */
				 1, /* supports alt modes */
				 USB_VID_BIZLINK);

const uint32_t vdo_product = VDO_PRODUCT(CONFIG_USB_PID, CONFIG_USB_BCD_DEV);

const uint32_t vdo_ama = VDO_AMA(CONFIG_USB_PD_IDENTITY_HW_VERS,
				 CONFIG_USB_PD_IDENTITY_SW_VERS,
				 0, 0, 0, 0, /* SS[TR][12] */
				 0, /* Vconn power */
				 0, /* Vconn power required */
				 1, /* Vbus power required */
				 AMA_USBSS_BBONLY /* USB SS support */);

/* Whether alternate mode has been entered or not */
static int alt_mode;

static int svdm_response_identity(int port, uint32_t *payload)
{
	payload[VDO_I(IDH)] = vdo_idh;
	/* TODO(tbroch): Do we plan to obtain TID (test ID) for hoho */
	payload[VDO_I(CSTAT)] = VDO_CSTAT(0);
	payload[VDO_I(PRODUCT)] = vdo_product;
	payload[VDO_I(AMA)] = vdo_ama;
	return VDO_I(AMA) + 1;
}

static int svdm_response_svids(int port, uint32_t *payload)
{
	payload[1] = VDO_SVID(USB_SID_DISPLAYPORT, 0);
	return 2;
}

/*
 * Will only ever be a single mode for this UFP_D device as it has no real USB
 * support making it only PIN_C configureable
 */
#define MODE_CNT 1
#define OPOS 1

const uint32_t vdo_dp_mode[MODE_CNT] =  {
	VDO_MODE_DP(MODE_DP_PIN_C  /* UFP pin cfg supported */
		  | MODE_DP_PIN_D,
		    0,		   /* DFP pin cfg supported : none */
		    0,		   /* with usb2.0 signalling in AMode */
		    CABLE_PLUG,	   /* its a plug */
		    MODE_DP_V13,   /* DPv1.3 Support, no Gen2 */
		    MODE_DP_SNK)   /* Its a sink only */
};

static int svdm_response_modes(int port, uint32_t *payload)
{
	if (PD_VDO_VID(payload[0]) != USB_SID_DISPLAYPORT)
		return 0; /* nak */

	memcpy(payload + 1, vdo_dp_mode, sizeof(vdo_dp_mode));
	return MODE_CNT + 1;
}

static int dp_status(int port, uint32_t *payload)
{
	int opos = PD_VDO_OPOS(payload[0]);
	int hpd = gpio_get_level(GPIO_USB_P0_DP_HPD);
	if (opos != OPOS)
		return 0; /* nak */

	payload[1] = VDO_DP_STATUS(0,                /* IRQ_HPD */
				   (hpd == 1),       /* HPD_HI|LOW */
				   0,		     /* request exit DP */
				   0,		     /* request exit USB */
				   0,		     /* MF pref */
				   gpio_get_level(GPIO_USB_P0_SBU_ENABLE),
				   0,		     /* power low */
				   0x2);
	return 2;
}

extern void hpd_irq_deferred(void);
extern void hpd_lvl_deferred(void);
static void dp_switch_4L_2L(void)
{
	// enable DP AUX
	gpio_set_level(GPIO_USB_P0_SBU_ENABLE, 1);

	// send RESET pulse to external peripherals
	gpio_set_level(GPIO_MCU_PWR_STDBY_LAN,  1);
	gpio_set_level(GPIO_MCU_PWR_STDBY_HUB,  1);

	usleep(300);
	gpio_set_level(GPIO_MCU_CHIPS_RESET_EN, 1);

	usleep(300);
	hpd_irq_deferred();

	usleep(300);
	hpd_lvl_deferred();
}
DECLARE_DEFERRED(dp_switch_4L_2L);

static int dp_config(int port, uint32_t *payload)
{
	if (port != 0) return 0;

	if (!PD_DP_CFG_DPON(payload[1])) {
		gpio_set_level(GPIO_USB_P0_SBU_ENABLE, 0);  // disable DP AUX
		gpio_set_level(GPIO_USB_P0_DP_SS_LANE, 0);  // 2 lanes
		return 1;
	}

	gpio_set_level(GPIO_MCU_CHIPS_RESET_EN, 0);

	// send RESET pulse to external peripherals
	gpio_set_level(GPIO_MCU_PWR_STDBY_LAN,  0);
	gpio_set_level(GPIO_MCU_PWR_STDBY_HUB,  0);

	// DP 4 lanes / 2 lanes switch
	if (PD_VDO_AMA_SNK_PINS(payload[1]) & MODE_DP_PIN_C)
		gpio_set_level(GPIO_USB_P0_DP_SS_LANE, 1);  // 4 lanes
	else
	if (PD_VDO_AMA_SNK_PINS(payload[1]) & MODE_DP_PIN_D)
		gpio_set_level(GPIO_USB_P0_DP_SS_LANE, 0);  // 2 lanes

	hook_call_deferred(dp_switch_4L_2L, 20 * MSEC);
	return 1;
}

static int svdm_enter_mode(int port, uint32_t *payload)
{
	/* SID & mode request is valid */
	if ((PD_VDO_VID(payload[0]) != USB_SID_DISPLAYPORT) ||
	    (PD_VDO_OPOS(payload[0]) != OPOS))
		return 0; /* will generate a NAK */

	/* TODO(tbroch) Enumerate USB BB here with updated mode choice */
	alt_mode = OPOS;
	return 1;
}

int pd_alt_mode(int port, uint16_t svid)
{
	return alt_mode;
}

static int svdm_exit_mode(int port, uint32_t *payload)
{
	gpio_set_level(GPIO_USB_P0_SBU_ENABLE, 0);
	alt_mode = 0;
	return 1; /* Must return ACK */
}

static struct amode_fx dp_fx = {
	.status = &dp_status,
	.config = &dp_config,
};

const struct svdm_response svdm_rsp = {
	.identity = &svdm_response_identity,
	.svids = &svdm_response_svids,
	.modes = &svdm_response_modes,
	.enter_mode = &svdm_enter_mode,
	.amode = &dp_fx,
	.exit_mode = &svdm_exit_mode,
};


#endif // CONFIG_USB_PD_ALT_MODE_DFP


//==============================================================================
int pd_custom_vdm(int port, int cnt, uint32_t *payload,
			 uint32_t **rpayload)
{
	int cmd = PD_VDO_CMD(payload[0]);
	uint16_t dev_id = 0;
	CPRINTF("VDM/%d [%d] %08x\n", cnt, cmd, payload[0]);

	/* make sure we have some payload */
	if (cnt == 0)
		return 0;

	switch (cmd) {
	case VDO_CMD_VERSION:
		/* guarantee last byte of payload is null character */
		*(payload + cnt - 1) = 0;
		CPRINTF("version: %s\n", (char *)(payload+1));
		break;
	case VDO_CMD_READ_INFO:
	case VDO_CMD_SEND_INFO:
		/* copy hash */
		if (cnt == 7) {
			dev_id = VDO_INFO_HW_DEV_ID(payload[6]);
//			pd_dev_store_rw_hash(port, dev_id, payload + 1);

			CPRINTF("Dev:0x%04x SW:%d RW:%d\n", dev_id,
				VDO_INFO_SW_DBG_VER(payload[6]),
				VDO_INFO_IS_RW(payload[6]));
		} else if (cnt == 6) {
			/* really old devices don't have last byte */
//			pd_dev_store_rw_hash(port, dev_id, payload + 1);
		}
		break;
	case VDO_CMD_CURRENT:
		CPRINTF("Current: %dmA\n", payload[1]);
		break;
	case VDO_CMD_FLIP:
		board_flip_usb_mux(port);
		break;
	}

	return 0;
}

int pd_vdm(int port, int cnt, uint32_t *payload, uint32_t **rpayload)
{
	if (PD_VDO_SVDM(payload[0]))
		return pd_svdm(port, cnt, payload, rpayload);
	else
		return pd_custom_vdm(port, cnt, payload, rpayload);
}


