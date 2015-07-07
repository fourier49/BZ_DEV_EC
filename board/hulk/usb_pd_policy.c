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
#include "system.h"
#include "task.h"
#include "timer.h"
#include "util.h"
#include "usb.h"
#include "usb_bb.h"
#include "usb_pd.h"
#include "usb_pd_config.h"

#define CPRINTF(format, args...) cprintf(CC_USBPD, format, ## args)
#define CPRINTS(format, args...) cprints(CC_USBPD, format, ## args)


//==============================================================================
/* Define typical operating power and max power */
#define OPERATING_POWER_MW 12000
#define MAX_POWER_MW       60000
#define MAX_CURRENT_MA     3000

/*
 * Do not request any voltage within this deadband region, where
 * we're not sure whether or not the boost or the bypass will be on.
 */
#define INPUT_VOLTAGE_DEADBAND_MIN 9700
#define INPUT_VOLTAGE_DEADBAND_MAX 11999

#define PDO_FIXED_FLAGS (PDO_FIXED_DUAL_ROLE | PDO_FIXED_EXTERNAL | PDO_FIXED_COMM_CAP)
//#define PDO_FIXED_FLAGS (PDO_FIXED_DUAL_ROLE | PDO_FIXED_DATA_SWAP)

const uint32_t pd_src_pdo[] = {
		PDO_FIXED(5000,  1500, PDO_FIXED_FLAGS),
		PDO_FIXED(20000, 2000, PDO_FIXED_FLAGS),
};
const int pd_src_pdo_cnt = ARRAY_SIZE(pd_src_pdo);

const uint32_t pd_snk_pdo[] = {
		PDO_FIXED(5000, 500, PDO_FIXED_FLAGS),
};
const int pd_snk_pdo_cnt = ARRAY_SIZE(pd_snk_pdo);

/* current and previous selected PDO entry */
static int volt_idx;
static int last_volt_idx;

/*charger conntected status flag used to decide if plug/unplug happen */
static int charger_con_status_chaged_flag = 0;
static int charger_pre_connect_status = 0;    //default will enter one time
static int charger_cur_connect_st = 0;

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

void set_output_voltage(int vidx)
{
#ifndef CONFIG_BIZ_EMU_HOST
	//extern int pwr_dc_in_detection;

	gpio_set_level(GPIO_USB_P0_PWR_5V_EN, 0);
	gpio_set_level(GPIO_USB_P0_PWR_20V_EN, 0);

	if (vidx == PDO_IDX_SRC_5V) {
		gpio_set_level(GPIO_USB_P0_PWR_5V_EN, 1);
		CPRINTF("SRC 5V\n");
	}
	else
	if (vidx == PDO_IDX_SRC_20V) {
		gpio_set_level(GPIO_USB_P0_PWR_20V_EN, 1);
		//gpio_set_level(GPIO_USB_P0_PWR_5V_EN, 1);
		CPRINTF("SRC 20V\n");
	}
	else
	if (vidx == PDO_IDX_SNK_VBUS) {
		//CPRINTF("SNK VBUS %d\n", pwr_dc_in_detection);
		CPRINTF("SNK VBUS %d\n");
	//	if (!pwr_dc_in_detection)
	//		gpio_set_level(GPIO_USB_P0_PWR_VBUS_EN, 1);
		;
	}
	else
		CPRINTF("no PWR\n");
#endif
}

void discharge_voltage(int target_vidx)
{
	// gpio_set_level(GPIO_USB_P0_PWR_DISCHARGE, 1);
}

void pd_transition_voltage(int idx)
{
	last_volt_idx = volt_idx;
	volt_idx = idx - 1;
	CPRINTF("Xsit Vol: %d %d\n", last_volt_idx, volt_idx);
	if (volt_idx < last_volt_idx) { /* down voltage transition */
#if 0
		/* Stop OCP monitoring */
		adc_disable_watchdog();

		discharge_volt_idx = volt_idx;
		/* from 20V : do an intermediate step at 12V */
		if (volt_idx == PDO_IDX_SRC_5V && last_volt_idx == PDO_IDX_SRC_20V)
			volt_idx = PDO_IDX_12V;
		discharge_voltage(voltages[volt_idx].ovp);
#else
		discharge_voltage(PDO_IDX_SRC_5V);//do nothing
		set_output_voltage(PDO_IDX_SRC_5V);
#endif
	} else if (volt_idx > last_volt_idx) { /* up voltage transition */
#if 0
		if (discharge_is_enabled()) {
			/* Make sure discharging is disabled */
			discharge_disable();
			/* Enable over-current monitoring */
			adc_enable_watchdog(ADC_CH_A_SENSE,
					    MAX_CURRENT_FAST, 0);
		}
#else
		set_output_voltage(PDO_IDX_SRC_20V);
#endif
	}
}

#ifndef CONFIG_BIZ_EMU_HOST
void check_pr_role(int port, int local_pwr)
{
	int pr_role;
	int flags = pd_get_flags(port, &pr_role);
	int partner_extpower = flags & PD_FLAGS_PARTNER_EXTPOWER;
    
	CPRINTF("check_pr_role:port:%d,local_pwr:%d,pr_role:%s,p-exp:%d\n",
			port,
			local_pwr,
			(pr_role==PD_ROLE_SOURCE)?"src":"snk",
			partner_extpower
	      );
    	
	if (local_pwr) {
		// with local power, but only up-charging to a unpowered Host
		if (!partner_extpower && pr_role == PD_ROLE_SINK) {
			CPRINTF("PRSW: SNK->SRC\n");
			pd_request_power_swap(port);
		}
	}
	else {
		// w/o local power, but request power from the Host
		if (pr_role == PD_ROLE_SOURCE) {
			CPRINTF("PRSW: SRC->SNK\n");
			pd_request_power_swap(port);
		}
	}
}

void pd_pwr_local_change(int pwr_in)
{
    //int pr_role = 0;
	CPRINTF("PWR DC %d 5V:%d 20V:%d chg:%d c1_con:%d cst:%d\n", pwr_in,
	                         		     gpio_get_level(GPIO_USB_P0_PWR_5V_EN),
						     gpio_get_level(GPIO_USB_P0_PWR_20V_EN),
						     charger_con_status_chaged_flag,
						     charger_pre_connect_status,
						     charger_cur_connect_st
			);
	if (pwr_in) {
		CPRINTF("SRC DC-in\n");
		 gpio_set_level(GPIO_VBUS_DS_CTRL1, 1);
	}
	else {
		if (gpio_get_level(GPIO_USB_P0_PWR_5V_EN)
		||  gpio_get_level(GPIO_USB_P0_PWR_20V_EN)) {
		
			// No DC, thus shutdown to source power
			CPRINTF("SRC no-DC\n");
			gpio_set_level(GPIO_VBUS_DS_CTRL1, 0);
			set_output_voltage(PDO_IDX_OFF);
		}
	}
	check_pr_role( 0, pwr_in );
}
#endif

int pd_set_power_supply_ready(int port)
{
	if(volt_idx== PDO_IDX_SRC_5V)
 	{
	/* provide VBUS */
#ifdef CONFIG_BIZ_EMU_HOST
	if (port == 1)  gpio_set_level(GPIO_USB_P1_PWR_5V_EN, 1);
#else
 	if (port == 0)  gpio_set_level(GPIO_USB_P0_PWR_5V_EN, 1);
#endif
	}
	/* notify host of power info change */
	//	pd_send_host_event(PD_EVENT_POWER_CHANGE);
    	CPRINTF("setpw_reset p:%d volt:%d\n",port,volt_idx);
	return EC_SUCCESS; /* we are ready */
}

void pd_power_supply_reset(int port)
{
	if(volt_idx== PDO_IDX_SRC_5V)
 	{
	/* Kill VBUS */
#ifdef CONFIG_BIZ_EMU_HOST
	if (port == 1)  gpio_set_level(GPIO_USB_P1_PWR_5V_EN, 0);
#else
	if (port == 0)  gpio_set_level(GPIO_USB_P0_PWR_5V_EN, 0);
#endif
	 }
	/* notify host of power info change */
    // pd_send_host_event(PD_EVENT_POWER_CHANGE);
	CPRINTF("setpw_reset p:%d\n",port);
}

void pd_set_input_current_limit(int port, uint32_t max_ma,
				uint32_t supply_voltage)
{
#if 0
	struct charge_port_info charge;
	charge.current = max_ma;
	charge.voltage = supply_voltage;
	charge_manager_update_charge(CHARGE_SUPPLIER_PD, port, &charge);

	/* notify host of power info change */
	pd_send_host_event(PD_EVENT_POWER_CHANGE);
#endif
}

void typec_set_input_current_limit(int port, uint32_t max_ma,
				   uint32_t supply_voltage)
{
#if 0
	struct charge_port_info charge;
	charge.current = max_ma;
	charge.voltage = supply_voltage;
	charge_manager_update_charge(CHARGE_SUPPLIER_TYPEC, port, &charge);

	/* notify host of power info change */
	pd_send_host_event(PD_EVENT_POWER_CHANGE);
#endif
}

int pd_board_checks(void)
{
	return EC_SUCCESS;
}

int pd_check_power_swap(int port)
{
	/* TODO: use battery level to decide to accept/reject power swap */
	/*
	 * Allow power swap as long as we are acting as a dual role device,
	 * otherwise assume our role is fixed (not in S0 or console command
	 * to fix our role).
	 */
	CPRINTF("ch_pw_swap:p%d ret:%d \n",port,(pd_get_dual_role(port) == PD_DRP_TOGGLE_ON ? 1 : 0));
	return pd_get_dual_role(port) == PD_DRP_TOGGLE_ON ? 1 : 0;
}

int pd_check_data_swap(int port, int data_role)
{
	/* Allow data swap if we are a UFP, otherwise don't allow */
	return (data_role == PD_ROLE_UFP) ? 1 : 0;
}

int pd_check_vconn_swap(int port)
{
#if 0
	/* in S5, do not allow vconn swap since pp5000 rail is off */
	return gpio_get_level(GPIO_PCH_SLP_S5_L);
#endif
	return 0;
}

void pd_execute_data_swap(int port, int data_role)
{

}

void pd_check_pr_role(int port, int pr_role, int flags)
{
	/*
	 * If partner is dual-role power and dualrole toggling is on, consider
	 * if a power swap is necessary.
	 */
#if 0 
// TODO: we temporarily disable this checking for auto PR-SWAP
//       instead, use console command for the moment. Will need to get it AUTO later...
#ifdef CONFIG_BIZ_EMU_HOST
#else

	extern int pwr_dc_in_detection;

	if (!pwr_dc_in_detection) return;

	if ((flags & PD_FLAGS_PARTNER_DR_POWER) &&
	    pd_get_dual_role() == PD_DRP_TOGGLE_ON) {
		/*
		 * If we are a sink and partner is not externally powered, then
		 * swap to become a source. If we are source and partner is
		 * externally powered, swap to become a sink.
		 */
		int partner_extpower = flags & PD_FLAGS_PARTNER_EXTPOWER;
		if ((!partner_extpower && pr_role == PD_ROLE_SINK) ||
		     (partner_extpower && pr_role == PD_ROLE_SOURCE))
			pd_request_power_swap(port);
	}
#endif
#endif
	CPRINTF("pd_check_pr_role:%d\n",port);
}

void pd_check_dr_role(int port, int dr_role, int flags)
{
#ifdef CONFIG_BIZ_EMU_HOST
	/* If UFP, try to switch to DFP */
	if ((flags & PD_FLAGS_PARTNER_DR_DATA) && dr_role == PD_ROLE_UFP)
		pd_request_data_swap(port);
#else
	/* If DFP, try to switch to UFP */
	if ((flags & PD_FLAGS_PARTNER_DR_DATA) && dr_role == PD_ROLE_DFP)
		pd_request_data_swap(port);
#endif
}

                      
void pd_check_charger_deferred(void)
{
	uint32_t delay = 20*MSEC;
	charger_cur_connect_st =  pd_is_connected(1)&pd_get_cc_state(1);
	charger_con_status_chaged_flag = charger_pre_connect_status^charger_cur_connect_st;
    charger_pre_connect_status = charger_cur_connect_st ;
	if( charger_con_status_chaged_flag )
	{
		pd_pwr_local_change(charger_pre_connect_status );
		CPRINTF("[hook]check charger\n");
    }
	if(0 != hook_call_deferred(pd_check_charger_deferred, delay))//20ms
		CPRINTF("warning\n");
}
DECLARE_DEFERRED(pd_check_charger_deferred);

static int command_charger(int argc, char **argv)
{
ccprintf("changed:%d,connected:%d,cc:%d \n",charger_con_status_chaged_flag,charger_cur_connect_st,pd_get_cc_state(1));
	return EC_SUCCESS;
}
DECLARE_CONSOLE_COMMAND(chg, command_charger,
			"<port>",
			"charger info",
			NULL);

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

int pd_custom_vdm(int port, int cnt, uint32_t *payload,
		  uint32_t **rpayload)
{
#if 0
	int cmd = PD_VDO_CMD(payload[0]);
	uint16_t dev_id = 0;
	int is_rw, is_latest;

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
			is_rw = VDO_INFO_IS_RW(payload[6]);
			is_latest = pd_dev_store_rw_hash(port,
							 dev_id,
							 payload + 1,
							 is_rw ?
							 SYSTEM_IMAGE_RW :
							 SYSTEM_IMAGE_RO);

			/*
			 * Send update host event unless our RW hash is
			 * already known to be the latest update RW.
			 */
			if (!is_rw || !is_latest)
				pd_send_host_event(PD_EVENT_UPDATE_DEVICE);

			CPRINTF("DevId:%d.%d SW:%d RW:%d\n",
				HW_DEV_ID_MAJ(dev_id),
				HW_DEV_ID_MIN(dev_id),
				VDO_INFO_SW_DBG_VER(payload[6]),
				is_rw);
		} else if (cnt == 6) {
			/* really old devices don't have last byte */
			pd_dev_store_rw_hash(port, dev_id, payload + 1,
					     SYSTEM_IMAGE_UNKNOWN);
		}
		break;
	case VDO_CMD_CURRENT:
		CPRINTF("Current: %dmA\n", payload[1]);
		break;
	case VDO_CMD_FLIP:
		board_flip_usb_mux(port);
		break;
	case VDO_CMD_GET_LOG:
		pd_log_recv_vdm(port, cnt, payload);
		break;
	}
#endif

	return 0;
}

static int dp_flags[PD_PORT_COUNT];
/* DP Status VDM as returned by UFP */
static uint32_t dp_status[PD_PORT_COUNT];

static void svdm_safe_dp_mode(int port)
{
	/* make DP interface safe until configure */
	board_set_usb_mux(port, TYPEC_MUX_NONE, USB_SWITCH_CONNECT, 0);
	dp_flags[port] = 0;
	dp_status[port] = 0;
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

static int svdm_dp_status(int port, uint32_t *payload)
{
	int opos = pd_alt_mode(port, USB_SID_DISPLAYPORT);
	payload[0] = VDO(USB_SID_DISPLAYPORT, 1,
			 CMD_DP_STATUS | VDO_OPOS(opos));
	payload[1] = VDO_DP_STATUS(0, /* HPD IRQ  ... not applicable */
				   0, /* HPD level ... not applicable */
				   0, /* exit DP? ... no */
				   0, /* usb mode? ... no */
				   0, /* multi-function ... no */
				   (!!(dp_flags[port] & DP_FLAGS_DP_ON)),
				   0, /* power low? ... no */
				   DP_STS_CONN_DFPD);
	return 2;
};

static int svdm_dp_config(int port, uint32_t *payload)
{
	int opos = pd_alt_mode(port, USB_SID_DISPLAYPORT);
	int mf_pref = PD_VDO_DPSTS_MF_PREF(dp_status[port]);
	int pin_mode = pd_dfp_dp_get_pin_mode(port, dp_status[port]);

	if (!pin_mode)
		return 0;

	board_set_usb_mux(port, mf_pref ? TYPEC_MUX_DOCK : TYPEC_MUX_DP,
			  USB_SWITCH_CONNECT, pd_get_polarity(port));

	payload[0] = VDO(USB_SID_DISPLAYPORT, 1,
			 CMD_DP_CONFIG | VDO_OPOS(opos));
	payload[1] = VDO_DP_CFG(pin_mode,      /* UFP_U as UFP_D */
				0,             /* UFP_U as DFP_D */
				1,             /* DPv1.3 signaling */
				2);            /* UFP_U connected as UFP_D */
	return 2;
};

static void svdm_dp_post_config(int port)
{
	dp_flags[port] |= DP_FLAGS_DP_ON;
	if (!(dp_flags[port] & DP_FLAGS_HPD_HI_PENDING))
		return;

	if (port)
		gpio_set_level(GPIO_USB_P1_DP_HPD, 1);
#if 0
	else
		gpio_set_level(GPIO_USB_P0_DP_HPD, 1);
#endif
}

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

#define PORT_TO_HPD_IRQ_DEFERRED(port) ((port) ? hpd1_irq_deferred : \
					hpd0_irq_deferred)

//#define PORT_TO_HPD(port) ((port) ? GPIO_USB_P1_DP_HPD : GPIO_USB_P0_DP_HPD)
#define PORT_TO_HPD(port)    GPIO_USB_P1_DP_HPD

static int svdm_dp_attention(int port, uint32_t *payload)
{
	int cur_lvl;
	int lvl = PD_VDO_DPSTS_HPD_LVL(payload[1]);
	int irq = PD_VDO_DPSTS_HPD_IRQ(payload[1]);
	enum gpio_signal hpd = PORT_TO_HPD(port);
	cur_lvl = gpio_get_level(hpd);

	dp_status[port] = payload[1];

	/* Its initial DP status message prior to config */
	if (!(dp_flags[port] & DP_FLAGS_DP_ON)) {
		if (lvl)
			dp_flags[port] |= DP_FLAGS_HPD_HI_PENDING;
		return 1;
	}
	if (irq & cur_lvl) {
		gpio_set_level(hpd, 0);
		hook_call_deferred(PORT_TO_HPD_IRQ_DEFERRED(port),
				   HPD_DEBOUNCE_IRQ);
	} else if (irq & !cur_lvl) {
		CPRINTF("ERR:HPD:IRQ&LOW\n");
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
		.post_config = &svdm_dp_post_config,
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
				 CONFIG_USB_VID);

const uint32_t vdo_product = VDO_PRODUCT(CONFIG_USB_PID, CONFIG_USB_BCD_DEV);

const uint32_t vdo_ama = VDO_AMA(CONFIG_USB_PD_IDENTITY_HW_VERS,
				 CONFIG_USB_PD_IDENTITY_SW_VERS,
				 0, 0, 0, 0, /* SS[TR][12] */
				 0, /* Vconn power */
				 0, /* Vconn power required */
				 1, /* Vbus power required */
				 AMA_USBSS_BBONLY /* USB SS support */);

/* Holds valid object position (opos) for entered mode */
static int alt_mode[PD_AMODE_COUNT];


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
	payload[1] = VDO_SVID(USB_SID_DISPLAYPORT, USB_VID_GOOGLE);
	payload[2] = 0;
	return 3;
}

#define MODE_CNT 1
#define OPOS_DP 1
#define OPOS_GFU 1

const uint32_t vdo_dp_modes[MODE_CNT] =  {
	VDO_MODE_DP(0,		   /* UFP pin cfg supported : none */
		    MODE_DP_PIN_C  /* DFP pin cfg supported */
		  | MODE_DP_PIN_D,
		    0,		   /* with usb2.0 signalling in AMode */
		    CABLE_PLUG,	   /* its a plug */
		    MODE_DP_V13,   /* DPv1.3 Support, no Gen2 */
		    MODE_DP_SNK)   /* Its a sink only */
};

const uint32_t vdo_goog_modes[1] =  {
	VDO_MODE_GOOGLE(MODE_GOOGLE_FU)
};

static int svdm_response_modes(int port, uint32_t *payload)
{
	if (PD_VDO_VID(payload[0]) == USB_SID_DISPLAYPORT) {
		memcpy(payload + 1, vdo_dp_modes, sizeof(vdo_dp_modes));
		return ARRAY_SIZE(vdo_dp_modes) + 1;
	} else if (PD_VDO_VID(payload[0]) == USB_VID_GOOGLE) {
		memcpy(payload + 1, vdo_goog_modes, sizeof(vdo_goog_modes));
		return ARRAY_SIZE(vdo_goog_modes) + 1;
	} else {
		return 0; /* nak */
	}
}

static int dp_status(int port, uint32_t *payload)
{
	int opos = PD_VDO_OPOS(payload[0]);
	int hpd = gpio_get_level(GPIO_USB_P0_DP_HPD);
	if (opos != OPOS_DP)
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


static void dp_switch_4L_2L(void)
{
	enum hpd_event ev;

#if 0
	// send RESET pulse to external peripherals
	gpio_set_level(GPIO_MCU_PWR_STDBY_LAN,  1);
	gpio_set_level(GPIO_MCU_PWR_STDBY_HUB,  1);
#endif
#if 0
	usleep(300);
	gpio_set_level(GPIO_MCU_CHIPS_RESET_EN, 1);
	usleep(300);
#endif
	ev = (gpio_get_level(GPIO_USB_P0_DP_HPD)) ? hpd_high : hpd_low;
	pd_send_hpd(0, ev);
}
DECLARE_DEFERRED(dp_switch_4L_2L);

static int dp_config(int port, uint32_t *payload)
{
	if (port != 0) return 0;

	if (!PD_DP_CFG_DPON(payload[1])) {
		gpio_set_level(GPIO_USB_P0_SBU_ENABLE, 0);  // disable DP AUX
#ifndef CONFIG_BIZ_HULK
		gpio_set_level(GPIO_USB_P0_DP_SS_LANE, 0);  // 2 lanes
#endif
		return 1;
	}

	// enable DP AUX
	gpio_set_level(GPIO_USB_P0_SBU_ENABLE, 1);

#if 0
	// send RESET pulse to external peripherals
	//gpio_set_level(GPIO_MCU_CHIPS_RESET_EN, 0);
	gpio_set_level(GPIO_MCU_PWR_STDBY_LAN,  0);
	gpio_set_level(GPIO_MCU_PWR_STDBY_HUB,  0);
#endif

	// DP 4 lanes / 2 lanes switch
#ifndef CONFIG_BIZ_HULK
	if (PD_VDO_MODE_DP_SRCP(payload[1]) & MODE_DP_PIN_C)
		gpio_set_level(GPIO_USB_P0_DP_SS_LANE, 1);  // 4 lanes
	else
	if (PD_VDO_MODE_DP_SRCP(payload[1]) & MODE_DP_PIN_D)
		gpio_set_level(GPIO_USB_P0_DP_SS_LANE, 0);  // 2 lanes
#endif
	CPRINTS("DP_CFG:%x", payload[1]);
	hook_call_deferred(dp_switch_4L_2L, 50 * MSEC);
	//hook_call_deferred(dp_switch_4L_2L, 300);
	return 1;
}

static int svdm_enter_mode(int port, uint32_t *payload)
{
	int rv = 0; /* will generate a NAK */

	/* SID & mode request is valid */
	if ((PD_VDO_VID(payload[0]) == USB_SID_DISPLAYPORT) &&
	    (PD_VDO_OPOS(payload[0]) == OPOS_DP)) {
		alt_mode[PD_AMODE_DISPLAYPORT] = OPOS_DP;
		rv = 1;
		pd_log_event(PD_EVENT_VIDEO_DP_MODE, 0, 1, NULL);
	} else if ((PD_VDO_VID(payload[0]) == USB_VID_GOOGLE) &&
		   (PD_VDO_OPOS(payload[0]) == OPOS_GFU)) {
		alt_mode[PD_AMODE_GOOGLE] = OPOS_GFU;
		rv = 1;
	}

#ifndef CONFIG_USB_CONSOLE
//Beccause we need to establish usb connection when console needed. 
	if (rv)
		/*
		 * If we failed initial mode entry we'll have enumerated the USB
		 * Billboard class.  If so we should disconnect.
		 */
		usb_disconnect();
#endif
	return rv;
}

int pd_alt_mode(int port, uint16_t svid)
{
	if (svid == USB_SID_DISPLAYPORT)
		return alt_mode[PD_AMODE_DISPLAYPORT];
	else if (svid == USB_VID_GOOGLE)
		return alt_mode[PD_AMODE_GOOGLE];
	return 0;
}

static int svdm_exit_mode(int port, uint32_t *payload)
{
	if (PD_VDO_VID(payload[0]) == USB_SID_DISPLAYPORT) {
		gpio_set_level(GPIO_PD_SBU_ENABLE, 0);
		alt_mode[PD_AMODE_DISPLAYPORT] = 0;
		pd_log_event(PD_EVENT_VIDEO_DP_MODE, 0, 0, NULL);
	} else if (PD_VDO_VID(payload[0]) == USB_VID_GOOGLE) {
		alt_mode[PD_AMODE_GOOGLE] = 0;
	} else {
		CPRINTF("Unknown exit mode req:0x%08x\n", payload[0]);
	}

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

int pd_custom_vdm(int port, int cnt, uint32_t *payload,
		  uint32_t **rpayload)
{
	int rsize;

	if (PD_VDO_VID(payload[0]) != USB_VID_GOOGLE ||
	    !alt_mode[PD_AMODE_GOOGLE])
		return 0;

	*rpayload = payload;

#if 0
	rsize = pd_custom_flash_vdm(port, cnt, payload);
	if (!rsize) {
		int cmd = PD_VDO_CMD(payload[0]);
		switch (cmd) {
		case VDO_CMD_GET_LOG:
			rsize = pd_vdm_get_log_entry(payload);
			break;
		default:
			/* Unknown : do not answer */
			return 0;
		}
	}
#endif

	/* respond (positively) to the request */
	payload[0] |= VDO_SRC_RESPONDER;

	return rsize;
}

#endif // CONFIG_USB_PD_ALT_MODE_DFP

//==============================================================================
