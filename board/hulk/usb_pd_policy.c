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

/*
 * Do not request any voltage within this deadband region, where
 * we're not sure whether or not the boost or the bypass will be on.
 */
#define INPUT_VOLTAGE_DEADBAND_MIN 9700
#define INPUT_VOLTAGE_DEADBAND_MAX 11999

//Default is definition for no cpower connected.
#define PDO_FIXED_FLAGS (PDO_FIXED_DUAL_ROLE | PDO_FIXED_COMM_CAP)


const uint32_t pd_src_pdo[] = {
		PDO_FIXED(5000,  900, PDO_FIXED_FLAGS),
};
const int pd_src_pdo_cnt = ARRAY_SIZE(pd_src_pdo);

#ifdef CONFIG_USB_PD_DYNAMIC_SRC_CAP

#define CPOWER_PDO_FIXED_FLAGS (PDO_FIXED_DUAL_ROLE| PDO_FIXED_EXTERNAL | PDO_FIXED_COMM_CAP)

uint32_t cpower_pd_src_cnt = 0;
uint32_t cpower_pd_src_pdo[5] = {
	PDO_FIXED(5000,  900, CPOWER_PDO_FIXED_FLAGS),
	PDO_FIXED(5000,  900, CPOWER_PDO_FIXED_FLAGS),
	PDO_FIXED(5000,  900, CPOWER_PDO_FIXED_FLAGS),
	PDO_FIXED(5000,  900, CPOWER_PDO_FIXED_FLAGS),
	PDO_FIXED(5000,  900, CPOWER_PDO_FIXED_FLAGS),
};
#endif

const uint32_t pd_snk_pdo[] = {
	PDO_FIXED(5000, 500, PDO_FIXED_FLAGS),
};
const int pd_snk_pdo_cnt = ARRAY_SIZE(pd_snk_pdo);

#ifdef CONFIG_USB_PD_DYNAMIC_SRC_CAP
const uint32_t cpower_pd_snk_pdo[] = {
	PDO_FIXED(5000, 1500, CPOWER_PDO_FIXED_FLAGS),
};
const int cpower_pd_snk_pdo_cnt = ARRAY_SIZE(cpower_pd_snk_pdo);
#endif

/* current and previous selected PDO entry */
static int volt_idx;
static int last_volt_idx;

/*charger conntected status flag used to decide if plug/unplug happen */
static int cpower_con_status_chaged_flag = 0;
static int cpower_pre_connect_status = 0;    //default will enter one time
static int cpower_cur_connect_st = 0;
static int LedCnt = 0;

#ifdef CONFIG_USB_PD_DYNAMIC_SRC_CAP
int pd_get_source_pdo(const uint32_t **src_pdo)
{
	int cnt = 0;
	if(pd_is_connected(1) == 0){ 
		*src_pdo = pd_src_pdo;
		cnt = pd_src_pdo_cnt;
		CPRINTF("get_src_pdo,-cpower cnt:%d\n",cnt);
	}else{ //if port 1 is connected and power nego is done
		*src_pdo = cpower_pd_src_pdo;
		cnt = cpower_pd_src_cnt;
		CPRINTF("get_src_pdo,+cpower cnt:%d\n",cnt);
	}
	return cnt;
}

int pd_get_snk_pdo(int port , const uint32_t **snk_pdo)
{
	int cnt = 0;
	if((pd_is_connected(1) == 0)||(port==1)){ 
		*snk_pdo = pd_snk_pdo;
		cnt = pd_snk_pdo_cnt;
		CPRINTF("get_snk_pdo,-cpower cnt:%d\n",cnt);
	}else{ //if port 1 is connected and power nego is done
		*snk_pdo = cpower_pd_snk_pdo;
		cnt = cpower_pd_snk_pdo_cnt;
		CPRINTF("get_snk_pdo,+cpower cnt:%d\n",cnt);
	}
	return cnt;
}

int pd_handle_cpower_capability(int port, int cnt, uint32_t *src_caps)
{
	int i = 0;
	int numa = 0;
	uint32_t ma, mv;
	uint32_t uw = 0;
	uint32_t pdo = 0;
	int max_ma;
	int max_uw = 0;

	if(port == 0 )//when receive src cap from port-0,we don't need to save and process it.
		return 0;

	cpower_pd_src_cnt = 1;//at least 1
	for (i = 0; i < cnt; i++) {
		pdo= *(src_caps+i);		 
		mv = ((pdo >> 10) & 0x3FF) * 50;//unit is 50 mv
		if ((pdo & PDO_TYPE_MASK) == PDO_TYPE_BATTERY) {
			uw = 250000 * (pdo & 0x3FF);
			max_ma = 1000 * MIN(1000 * uw, PD_MAX_POWER_MW) / mv;
		} else {
			max_ma = 10 * (pdo & 0x3FF); //unit is 10 ma
			max_ma = MIN(max_ma, PD_MAX_POWER_MW * 1000 / mv);
		}
		ma = MIN(max_ma, PD_MAX_CURRENT_MA);
		CPRINTF("ma:%d max-ma%d \n", (10 * (pdo & 0x3FF)),max_ma);
		uw = ma*mv;

		if(uw > (PD_MAX_POWER_MW *1000))
			uw -= (PD_MAX_POWER_MW *1000);

		if(max_uw  <  uw) {
			max_uw = uw;
			numa = uw/mv;
			cpower_pd_src_pdo[1] = PDO_FIXED(mv,numa, PDO_FIXED_FLAGS);
			CPRINTF("adapter_cap idx:%d mv:%d uw:%d new-ma%d \n", i, mv,uw,numa);
		}
	}
	if(cnt > 1)//if receive src pdo.
		cpower_pd_src_cnt = 2;

	return cpower_pd_src_cnt;
}
#endif

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

#ifdef CONFIG_USB_PD_DYNAMIC_SRC_CAP
	const uint32_t *src_pdo;
	const int src_pdo_cnt = pd_get_source_pdo(&src_pdo);
#else
	const uint32_t *src_pdo = pd_src_pdo;
	const int src_pdo_cnt = pd_src_pdo_cnt;
#endif

	if(pd_snk_is_vbus_provided(1) == 0) {
		//this case means when c-power is removed and 
		//before completing the pr-swap from src->snk
		//we have receive request voltage.
		if(idx != 1)
			return EC_ERROR_INVAL; /* Invalid index */
	}

	if (!idx || idx > src_pdo_cnt)
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
	gpio_set_level(GPIO_USB_P0_PWR_5V_EN, 0);
	gpio_set_level(GPIO_USB_P0_PWR_20V_EN, 0);

	if (vidx == PDO_IDX_SRC_5V) {
		gpio_set_level(GPIO_USB_P0_PWR_5V_EN, 1);
		CPRINTF("SRC 5V\n");
	}
	else
	if (vidx == PDO_IDX_SRC_20V) {
		gpio_set_level(GPIO_USB_P0_PWR_20V_EN, 1);
		CPRINTF("SRC 20V\n");
	}
	else
	if (vidx == PDO_IDX_SNK_VBUS) {
		CPRINTF("SNK VBUS %d\n");
	}
	else
		CPRINTF("no PWR\n");
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

void check_pr_role(int port, int local_pwr)
{
	int pr_role;
	int flags = pd_get_flags(port, &pr_role);
	int partner_extpower = flags & PD_FLAGS_PARTNER_EXTPOWER;
    
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
	if (pwr_in) {
		  CPRINTF("SRC DC-in\n");
		  gpio_set_level(GPIO_VBUS_DS_CTRL1, 1);
	}
	else {
		if (gpio_get_level(GPIO_USB_P0_PWR_5V_EN)
		||  gpio_get_level(GPIO_USB_P0_PWR_20V_EN)) {
		
			// No DC, thus shutdown to source power
			gpio_set_level(GPIO_VBUS_DS_CTRL1, 0);
			CPRINTF("SRC no-DC\n");
			set_output_voltage(PDO_IDX_OFF);
			gpio_set_level(GPIO_VBUS_UP_CTRL1, 1);
		}
	}
	check_pr_role( 0, pwr_in );
}


int pd_set_power_supply_ready(int port)
{
	if(volt_idx== PDO_IDX_SRC_5V) {
		/* provide VBUS */
		if (port == 0)  gpio_set_level(GPIO_USB_P0_PWR_5V_EN, 1);
	}
	/* notify host of power info change */
	//	pd_send_host_event(PD_EVENT_POWER_CHANGE);
	CPRINTF("setpw_ready p:%d volt:%d\n",port,volt_idx);
	return EC_SUCCESS; /* we are ready */
}

void pd_power_supply_reset(int port)
{
	if(volt_idx== PDO_IDX_SRC_5V){
		/* Kill VBUS */
		if (port == 0)  gpio_set_level(GPIO_USB_P0_PWR_5V_EN, 0);
	 }
	/* notify host of power info change */
	//	pd_send_host_event(PD_EVENT_POWER_CHANGE);
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
	int pr_role;
	
	pd_get_flags(port, &pr_role);

	/* TODO: use battery level to decide to accept/reject power swap */
	/*
	 * Allow power swap as long as we are acting as a dual role device,
	 * otherwise assume our role is fixed (not in S0 or console command
	 * to fix our role).
	 */

	//we won't accept change from sink->src if no c-power connected
	if((pd_snk_is_vbus_provided(1) == 1) && (pd_get_dual_role(port) == PD_DRP_TOGGLE_ON)){
		CPRINTF("C%d ch_pw_swap - accept\n",port);
		return 1;
	}else if((pd_get_dual_role(port) == PD_DRP_TOGGLE_ON ) && (pr_role==PD_ROLE_SOURCE)) { //c-power is not connected
		//only allow change from src->snk.
		CPRINTF("C%d ch_pw_swap - accept\n",port);
		return 1;
	}
	CPRINTF("C%d ch_pw_swap - reject\n",port);
	return 0;
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
#if 0 
// TODO: we temporarily disable this checking for auto PR-SWAP
//       instead, use console command for the moment. Will need to get it AUTO later...

	/*
	 * If partner is dual-role power and dualrole toggling is on, consider
	 * if a power swap is necessary.
	 */
	if ((flags & PD_FLAGS_PARTNER_DR_POWER) &&

		pd_get_dual_role(port) == PD_DRP_TOGGLE_ON) {
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
	CPRINTF("pd_check_pr_role:%d\n",port);
}

void pd_check_dr_role(int port, int dr_role, int flags)
{
	/* If UFP, try to switch to DFP */
	if ((flags & PD_FLAGS_PARTNER_DR_DATA) && dr_role == PD_ROLE_UFP)
		pd_request_data_swap(port);
}

static int vol_check_retry_times = 0;
static int enable_power = 0;
void pd_check_cpower_power_nego_done_deferred(void)
{
	int delay = 300*MSEC;
	int mv =  adc_read_channel(ADC_P1_VBUS_DT);
     
	CPRINTS("[%d]mv:%d,enable:%d\n",vol_check_retry_times,mv,enable_power);

	//Fixme: use real target voltage
	if( mv > 1000) {
		if(enable_power == 0) {
			delay = 50*MSEC;
			enable_power = 1;
			gpio_set_level(GPIO_VBUS_UP_CTRL1, 0);
			if(0 != hook_call_deferred(pd_check_cpower_power_nego_done_deferred, delay))
				CPRINTF("[hook fail] call check charger pwr nego done -r\n");
		}
		else 
		if(enable_power == 1) {
			enable_power = 0;
			pd_pwr_local_change(1);
		}
	}
	else{
		if(vol_check_retry_times-- > 0){
			if(0 != hook_call_deferred( pd_check_cpower_power_nego_done_deferred, delay))
				CPRINTF("[hook fail] call check charger pwr nego done -r\n");
		}
	}	
}

DECLARE_DEFERRED(pd_check_cpower_power_nego_done_deferred);

//static int vsafe_check_retry_times = 0;
void pd_cpower_unplung_deferred(void)
{
	pd_pwr_local_change(0);
}
DECLARE_DEFERRED(pd_cpower_unplung_deferred);
                      
void pd_check_cpower_deferred(void)
{
	uint32_t delay = 100*MSEC;
	cpower_cur_connect_st =  pd_snk_is_vbus_provided(1);
	cpower_con_status_chaged_flag = cpower_pre_connect_status^cpower_cur_connect_st;
	cpower_pre_connect_status = cpower_cur_connect_st ;
	if( cpower_con_status_chaged_flag )	{
		if(cpower_cur_connect_st){
			CPRINTF("charger connected\n");
			vol_check_retry_times= 10;
			if(0 != hook_call_deferred(pd_check_cpower_power_nego_done_deferred, 800*MSEC))
				CPRINTF("[hook fail] call check charger pwr nego done\n");
		}else{
			CPRINTF("charger disconnected\n");
#ifdef CONFIG_USB_PD_DYNAMIC_SRC_CAP	
			cpower_pd_src_cnt = 0;
			cpower_pd_src_pdo[0] = PDO_FIXED(5000,900,PDO_FIXED_FLAGS);
#endif				
			gpio_set_level(GPIO_VBUS_DS_CTRL1, 0);
			if(0 != hook_call_deferred(pd_cpower_unplung_deferred, 1000*MSEC))
				CPRINTF("[hook fail]pd_cpower_unplung_deferred\n");
		}
	}
	else{
		if(PD_ROLE_SOURCE == pd_get_role(0)){
			//control LED breeze.
			if (volt_idx == PDO_IDX_SRC_5V ){
				if((LedCnt++)%20==0)
					gpio_set_level(GPIO_LED_CONTROL, !gpio_get_level(GPIO_LED_CONTROL));
			}else{
				if((LedCnt++)%10==0)
					gpio_set_level(GPIO_LED_CONTROL, !gpio_get_level(GPIO_LED_CONTROL));
			}
		}else{
			gpio_set_level(GPIO_LED_CONTROL, 1);
		}
	}

	if(0 != hook_call_deferred(pd_check_cpower_deferred, delay))
		CPRINTF("[hook fail] call check charger \n");
}
DECLARE_DEFERRED(pd_check_cpower_deferred);

/* ----------------- Vendor Defined Messages ------------------ */



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

#ifndef CONFIG_BIZ_HULK
	// send RESET pulse to external peripherals
	gpio_set_level(GPIO_MCU_PWR_STDBY_LAN,  1);
	gpio_set_level(GPIO_MCU_PWR_STDBY_HUB,  1);

	usleep(300);
	gpio_set_level(GPIO_MCU_CHIPS_RESET_EN, 1);
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

#ifndef CONFIG_BIZ_HULK
	gpio_set_level(GPIO_MCU_CHIPS_RESET_EN, 0);

	// send RESET pulse to external peripherals
	gpio_set_level(GPIO_MCU_PWR_STDBY_LAN,  0);
	gpio_set_level(GPIO_MCU_PWR_STDBY_HUB,  0);

	// DP 4 lanes / 2 lanes switch
	if (PD_VDO_MODE_DP_SNKP(payload[1]) & MODE_DP_PIN_C)
		gpio_set_level(GPIO_USB_P0_DP_SS_LANE, 1);  // 4 lanes
	else
	if (PD_VDO_MODE_DP_SNKP(payload[1]) & MODE_DP_PIN_D)
		gpio_set_level(GPIO_USB_P0_DP_SS_LANE, 0);  // 2 lanes
#endif
	hook_call_deferred(dp_switch_4L_2L, 50 * MSEC);
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


//==============================================================================
