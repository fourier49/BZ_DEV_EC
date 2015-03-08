/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "adc.h"
#include "charge_manager.h"
#include "charge_ramp.h"
#include "console.h"
#include "gpio.h"
#include "hooks.h"
#include "host_command.h"
#include "system.h"
#include "timer.h"
#include "usb_pd.h"
#include "usb_pd_config.h"
#include "util.h"

#define CPRINTS(format, args...) cprints(CC_USBCHARGE, format, ## args)

#define POWER(charge_port) ((charge_port.current) * (charge_port.voltage))

/* Timeout for delayed override power swap, allow for 500ms extra */
#define POWER_SWAP_TIMEOUT (PD_T_SRC_RECOVER_MAX + PD_T_SRC_TURN_ON + \
			    PD_T_SAFE_0V + 500 * MSEC)

/* Keep track of available charge for each charge port. */
static struct charge_port_info available_charge[CHARGE_SUPPLIER_COUNT]
					       [PD_PORT_COUNT];

/* Keep track of when the supplier on each port is registered. */
static timestamp_t registration_time[PD_PORT_COUNT];

/*
 * Charge ceiling for ports. This can be set to temporarily limit the charge
 * pulled from a port, without influencing the port selection logic.
 */
static int charge_ceil[PD_PORT_COUNT];

/* Dual-role capability of attached partner port */
static enum dualrole_capabilities dualrole_capability[PD_PORT_COUNT];

#ifdef CONFIG_USB_PD_LOGGING
/* Mark port as dirty when making changes, for later logging */
static int save_log[PD_PORT_COUNT];
#endif

/* Store current state of port enable / charge current. */
static int charge_port = CHARGE_PORT_NONE;
static int charge_current = CHARGE_CURRENT_UNINITIALIZED;
static int charge_current_uncapped = CHARGE_CURRENT_UNINITIALIZED;
static int charge_voltage;
static int charge_supplier = CHARGE_SUPPLIER_NONE;
static int override_port = OVERRIDE_OFF;

static int delayed_override_port = OVERRIDE_OFF;
static timestamp_t delayed_override_deadline;

enum charge_manager_change_type {
	CHANGE_CHARGE,
	CHANGE_DUALROLE,
};

/**
 * Initialize available charge. Run before board init, so board init can
 * initialize data, if needed.
 */
static void charge_manager_init(void)
{
	int i, j;

	for (i = 0; i < PD_PORT_COUNT; ++i) {
		for (j = 0; j < CHARGE_SUPPLIER_COUNT; ++j) {
			available_charge[j][i].current =
				CHARGE_CURRENT_UNINITIALIZED;
			available_charge[j][i].voltage =
				CHARGE_VOLTAGE_UNINITIALIZED;
		}
		charge_ceil[i] = CHARGE_CEIL_NONE;
		dualrole_capability[i] = CAP_UNKNOWN;
	}
}
DECLARE_HOOK(HOOK_INIT, charge_manager_init, HOOK_PRIO_DEFAULT-1);

/**
 * Returns 1 if all ports + suppliers have reported in with some initial charge,
 * 0 otherwise.
 */
static int charge_manager_is_seeded(void)
{
	/* Once we're seeded, we don't need to check again. */
	static int is_seeded;
	int i, j;

	if (is_seeded)
		return 1;

	for (i = 0; i < CHARGE_SUPPLIER_COUNT; ++i)
		for (j = 0; j < PD_PORT_COUNT; ++j)
			if (available_charge[i][j].current ==
			    CHARGE_CURRENT_UNINITIALIZED ||
			    available_charge[i][j].voltage ==
			    CHARGE_VOLTAGE_UNINITIALIZED)
				return 0;

	is_seeded = 1;
	return 1;
}

#ifndef TEST_CHARGE_MANAGER
/**
 * Fills passed power_info structure with current info about the passed port.
 */
static void charge_manager_fill_power_info(int port,
	struct ec_response_usb_pd_power_info *r)
{
	int sup = CHARGE_SUPPLIER_NONE;
	int i;

	/* Determine supplier information to show. */
	if (port == charge_port)
		sup = charge_supplier;
	else
		/* Find highest priority supplier */
		for (i = 0; i < CHARGE_SUPPLIER_COUNT; ++i)
			if (available_charge[i][port].current > 0 &&
			    available_charge[i][port].voltage > 0 &&
			    (sup == CHARGE_SUPPLIER_NONE ||
			     supplier_priority[i] <
			     supplier_priority[sup] ||
			    (supplier_priority[i] ==
			     supplier_priority[sup] &&
			     POWER(available_charge[i][port]) >
			     POWER(available_charge[sup]
						   [port]))))
				sup = i;

	/* Fill in power role */
	if (charge_port == port)
		r->role = USB_PD_PORT_POWER_SINK;
	else if (pd_is_connected(port) && pd_get_role(port) == PD_ROLE_SOURCE)
		r->role = USB_PD_PORT_POWER_SOURCE;
	else if (sup != CHARGE_SUPPLIER_NONE)
		r->role = USB_PD_PORT_POWER_SINK_NOT_CHARGING;
	else
		r->role = USB_PD_PORT_POWER_DISCONNECTED;

	/* Is port partner dual-role capable */
	r->dualrole = (dualrole_capability[port] == CAP_DUALROLE);

	if (sup == CHARGE_SUPPLIER_NONE ||
	    r->role == USB_PD_PORT_POWER_SOURCE) {
		r->type = USB_CHG_TYPE_NONE;
		r->meas.voltage_max = 0;
		r->meas.voltage_now = r->role == USB_PD_PORT_POWER_SOURCE ? 5000
									  : 0;
		r->meas.current_max = 0;
		r->max_power = 0;
	} else {
#ifdef HAS_TASK_CHG_RAMP
		/* Read ramped current if active charging port */
		int use_ramp_current = (charge_port == port);
#else
		const int use_ramp_current = 0;
#endif

		switch (sup) {
		case CHARGE_SUPPLIER_PD:
			r->type = USB_CHG_TYPE_PD;
			break;
		case CHARGE_SUPPLIER_TYPEC:
			r->type = USB_CHG_TYPE_C;
			break;
		case CHARGE_SUPPLIER_PROPRIETARY:
			r->type = USB_CHG_TYPE_PROPRIETARY;
			break;
		case CHARGE_SUPPLIER_BC12_DCP:
			r->type = USB_CHG_TYPE_BC12_DCP;
			break;
		case CHARGE_SUPPLIER_BC12_CDP:
			r->type = USB_CHG_TYPE_BC12_CDP;
			break;
		case CHARGE_SUPPLIER_BC12_SDP:
			r->type = USB_CHG_TYPE_BC12_SDP;
			break;
		case CHARGE_SUPPLIER_VBUS:
			r->type = USB_CHG_TYPE_VBUS;
			break;
		default:
			r->type = USB_CHG_TYPE_OTHER;
		}
		r->meas.voltage_max = available_charge[sup][port].voltage;

		if (use_ramp_current) {
			r->meas.current_max = chg_ramp_get_current_limit();
			r->max_power =
				r->meas.current_max * r->meas.voltage_max;
		} else {
			r->meas.current_max =
				available_charge[sup][port].current;
			r->max_power = POWER(available_charge[sup][port]);
		}

		/*
		 * If we are sourcing power, or sinking but not charging, then
		 * VBUS must be 5V. If we are charging, then read VBUS ADC.
		 */
		if (r->role == USB_PD_PORT_POWER_SINK_NOT_CHARGING)
			r->meas.voltage_now = 5000;
		else
			r->meas.voltage_now = adc_read_channel(ADC_VBUS);
	}
}
#endif /* TEST_CHARGE_MANAGER */

#ifdef CONFIG_USB_PD_LOGGING
/**
 * Saves a power state log entry with the current info about the passed port.
 */
void charge_manager_save_log(int port)
{
	uint16_t flags = 0;
	struct ec_response_usb_pd_power_info pinfo;

	if (port < 0 || port >= PD_PORT_COUNT)
		return;

	save_log[port] = 0;
	charge_manager_fill_power_info(port, &pinfo);

	/* Flags are stored in the data field */
	if (port == override_port)
		flags |= CHARGE_FLAGS_OVERRIDE;
	if (port == delayed_override_port)
		flags |= CHARGE_FLAGS_DELAYED_OVERRIDE;
	flags |= pinfo.role | (pinfo.type << CHARGE_FLAGS_TYPE_SHIFT) |
		 (pinfo.dualrole ? CHARGE_FLAGS_DUAL_ROLE : 0);

	pd_log_event(PD_EVENT_MCU_CHARGE,
		     PD_LOG_PORT_SIZE(port, sizeof(pinfo.meas)),
		     flags, &pinfo.meas);
}
#endif /* CONFIG_USB_PD_LOGGING */

/**
 * Perform cleanup operations on an override port, when switching to a
 * different port. This involves switching the port from sink to source,
 * if applicable.
 */
static void charge_manager_cleanup_override_port(int port)
{
	if (port < 0 || port >= PD_PORT_COUNT)
		return;

	if (dualrole_capability[port] == CAP_DUALROLE &&
	    pd_get_role(port) == PD_ROLE_SINK)
		pd_request_power_swap(port);
}

/**
 * Select the 'best' charge port, as defined by the supplier heirarchy and the
 * ability of the port to provide power.
 */
static void charge_manager_get_best_charge_port(int *new_port,
						int *new_supplier)
{
	int supplier = CHARGE_SUPPLIER_NONE;
	int port = CHARGE_PORT_NONE;
	int best_port_power = -1, candidate_port_power;
	int i, j;

	/* Skip port selection on OVERRIDE_DONT_CHARGE. */
	if (override_port != OVERRIDE_DONT_CHARGE) {
		/*
		 * Charge supplier selection logic:
		 * 1. Prefer higher priority supply.
		 * 2. Prefer higher power over lower in case priority is tied.
		 * 3. Prefer current charge port over new port in case (1)
		 *    and (2) are tied.
		 * available_charge can be changed at any time by other tasks,
		 * so make no assumptions about its consistency.
		 */
		for (i = 0; i < CHARGE_SUPPLIER_COUNT; ++i)
			for (j = 0; j < PD_PORT_COUNT; ++j) {
				/*
				 * Skip this supplier if there is no
				 * available charge.
				 */
				if (available_charge[i][j].current == 0 ||
				    available_charge[i][j].voltage == 0)
					continue;

				/*
				 * Don't select this port if we have a
				 * charge on another override port.
				 */
				if (override_port != OVERRIDE_OFF &&
				    override_port == port &&
				    override_port != j)
					continue;

				/*
				 * Don't charge from a dual-role port unless
				 * it is our override port.
				 */
				if (dualrole_capability[j] != CAP_DEDICATED &&
				    override_port != j)
					continue;

				candidate_port_power =
					POWER(available_charge[i][j]);

				/* Select if no supplier chosen yet. */
				if (supplier == CHARGE_SUPPLIER_NONE ||
				/* ..or if supplier priority is higher. */
				    supplier_priority[i] <
				    supplier_priority[supplier] ||
				/* ..or if this is our override port. */
				   (j == override_port &&
				    port != override_port) ||
				/* ..or if priority is tied and.. */
				   (supplier_priority[i] ==
				    supplier_priority[supplier] &&
				/* candidate port can supply more power or.. */
				   (candidate_port_power > best_port_power ||
				/*
				 * candidate port is the active port and can
				 * supply the same amount of power.
				 */
				   (candidate_port_power == best_port_power &&
				    charge_port == j)))) {
					supplier = i;
					port = j;
					best_port_power = candidate_port_power;
				}
			}

	}

	*new_port = port;
	*new_supplier = supplier;
}

/**
 * Charge manager refresh -- responsible for selecting the active charge port
 * and charge power. Called as a deferred task.
 */
static void charge_manager_refresh(void)
{
	int new_supplier, new_port;
	int new_charge_current, new_charge_current_uncapped;
	int new_charge_voltage, i;
	int updated_new_port = CHARGE_PORT_NONE;
	int updated_old_port = CHARGE_PORT_NONE;

	/* Hunt for an acceptable charge port */
	while (1) {
		charge_manager_get_best_charge_port(&new_port, &new_supplier);

		/*
		 * If the port or supplier changed, make an attempt to switch to
		 * the port. We will re-set the active port on a supplier change
		 * to give the board-level function another chance to reject
		 * the port, for example, if the port has become a charge
		 * source.
		 */
		if ((new_port == charge_port &&
		    new_supplier == charge_supplier) ||
		    board_set_active_charge_port(new_port) == EC_SUCCESS)
			break;

		/* 'Dont charge' request must be accepted */
		ASSERT(new_port != CHARGE_PORT_NONE);

		/*
		 * Zero the available charge on the rejected port so that
		 * it is no longer chosen.
		 */
		for (i = 0; i < CHARGE_SUPPLIER_COUNT; ++i)
			available_charge[i][new_port].current = 0;
	}

	/*
	 * Clear override if it wasn't selected as the 'best' port -- it means
	 * that no charge is available on the port, or the port was rejected.
	 */
	if (override_port >= 0 &&
	    override_port != new_port) {
		charge_manager_cleanup_override_port(override_port);
		override_port = OVERRIDE_OFF;
	}

	if (new_supplier == CHARGE_SUPPLIER_NONE) {
		new_charge_current = 0;
		new_charge_current_uncapped = 0;
		new_charge_voltage = 0;
	} else {
		new_charge_current_uncapped =
			available_charge[new_supplier][new_port].current;
		/* Enforce port charge ceiling. */
		if (charge_ceil[new_port] != CHARGE_CEIL_NONE)
			new_charge_current = MIN(charge_ceil[new_port],
						 new_charge_current_uncapped);
		else
			new_charge_current = new_charge_current_uncapped;

		new_charge_voltage =
			available_charge[new_supplier][new_port].voltage;
	}

	/* Change the charge limit + charge port/supplier if modified. */
	if (new_port != charge_port || new_charge_current != charge_current ||
	    new_supplier != charge_supplier) {
#ifdef HAS_TASK_CHG_RAMP
		chg_ramp_charge_supplier_change(
				new_port, new_supplier, new_charge_current,
				registration_time[new_port]);
#else
		board_set_charge_limit(new_charge_current);
#endif
		CPRINTS("CL: p%d s%d i%d v%d", new_port, new_supplier,
			new_charge_current, new_charge_voltage);
	}

	/*
	 * Signal new power request only if the port changed, the voltage
	 * on the same port changed, or the actual uncapped current
	 * on the same port changed (don't consider ceil).
	 */
	if (new_port != CHARGE_PORT_NONE &&
	    (new_port != charge_port ||
	     new_charge_current_uncapped != charge_current_uncapped ||
	     new_charge_voltage != charge_voltage))
		updated_new_port = new_port;

	/* Signal new power request on old port if we're switching away. */
	if (charge_port != new_port && charge_port != CHARGE_PORT_NONE)
		updated_old_port = charge_port;

	/* Update globals to reflect current state. */
	charge_current = new_charge_current;
	charge_current_uncapped = new_charge_current_uncapped;
	charge_voltage = new_charge_voltage;
	charge_supplier = new_supplier;
	charge_port = new_port;

#ifdef CONFIG_USB_PD_LOGGING
	/*
	 * Write a log under the following conditions:
	 *  1. A port becomes active or
	 *  2. A port becomes inactive or
	 *  3. The active charge port power limit changes or
	 *  4. Any supplier change on an inactive port
	 */
	if (updated_new_port != CHARGE_PORT_NONE)
		save_log[updated_new_port] = 1;
	/* Don't log non-meaningful changes on charge port */
	else if (charge_port != CHARGE_PORT_NONE)
		save_log[charge_port] = 0;

	if (updated_old_port != CHARGE_PORT_NONE)
		save_log[updated_old_port] = 1;

	for (i = 0; i < PD_PORT_COUNT; ++i)
		if (save_log[i])
			charge_manager_save_log(i);
#endif

	/* New power requests must be set only after updating the globals. */
	if (updated_new_port != CHARGE_PORT_NONE)
		pd_set_new_power_request(updated_new_port);
	if (updated_old_port != CHARGE_PORT_NONE)
		pd_set_new_power_request(updated_old_port);
}
DECLARE_DEFERRED(charge_manager_refresh);

static void charge_manager_make_change(enum charge_manager_change_type change,
				       int supplier,
				       int port,
				       struct charge_port_info *charge)
{
	int i;
	int clear_override = 0;

	/* Determine if this is a change which can affect charge status */
	switch (change) {
	case CHANGE_CHARGE:
		/* Ignore changes where charge is identical */
		if (available_charge[supplier][port].current ==
		    charge->current &&
		    available_charge[supplier][port].voltage ==
		    charge->voltage)
			return;
		if (charge->current > 0 &&
		    available_charge[supplier][port].current == 0)
			clear_override = 1;
#ifdef CONFIG_USB_PD_LOGGING
		save_log[port] = 1;
#endif
		break;
	case CHANGE_DUALROLE:
		/*
		 * Ignore all except for transition to non-dualrole,
		 * which may occur some time after we see a charge
		 */
		if (dualrole_capability[port] != CAP_DEDICATED)
			return;
		/* Clear override only if a charge is present on the port */
		for (i = 0; i < CHARGE_SUPPLIER_COUNT; ++i)
			if (available_charge[i][port].current > 0) {
				clear_override = 1;
				break;
			}
		/*
		 * If there is no charge present on the port, the dualrole
		 * change is meaningless to charge_manager.
		 */
		if (!clear_override)
			return;
		break;
	}

	/* Remove override when a dedicated charger is plugged */
	if (clear_override && override_port != port &&
	    dualrole_capability[port] == CAP_DEDICATED) {
		charge_manager_cleanup_override_port(override_port);
		override_port = OVERRIDE_OFF;
		if (delayed_override_port != OVERRIDE_OFF) {
			charge_manager_cleanup_override_port(
				delayed_override_port);
			delayed_override_port = OVERRIDE_OFF;
			hook_call_deferred(
				board_charge_manager_override_timeout,
				-1);
		}
	}

	if (change == CHANGE_CHARGE) {
		available_charge[supplier][port].current = charge->current;
		available_charge[supplier][port].voltage = charge->voltage;
		registration_time[port] = get_time();

		/*
		 * If we have a charge on our delayed override port within
		 * the deadline, make it our override port.
		*/
		if (port == delayed_override_port && charge->current > 0 &&
		    pd_get_role(delayed_override_port) == PD_ROLE_SINK &&
		    get_time().val < delayed_override_deadline.val)
			charge_manager_set_override(port);
	}

	/*
	 * Don't call charge_manager_refresh unless all ports +
	 * suppliers have reported in. We don't want to make changes
	 * to our charge port until we are certain we know what is
	 * attached.
	 */
	if (charge_manager_is_seeded())
		hook_call_deferred(charge_manager_refresh, 0);
}

/**
 * Update available charge for a given port / supplier.
 *
 * @param supplier		Charge supplier to update.
 * @param port			Charge port to update.
 * @param charge		Charge port current / voltage.
 */
void charge_manager_update_charge(int supplier,
				  int port,
				  struct charge_port_info *charge)
{
	ASSERT(supplier >= 0 && supplier < CHARGE_SUPPLIER_COUNT);
	ASSERT(port >= 0 && port < PD_PORT_COUNT);
	ASSERT(charge != NULL);

	charge_manager_make_change(CHANGE_CHARGE, supplier, port, charge);
}

/**
 * Notify charge_manager of a partner dualrole capability change.
 *
 * @param port			Charge port which changed.
 * @param cap			New port capability.
 */
void charge_manager_update_dualrole(int port, enum dualrole_capabilities cap)
{
	ASSERT(port >= 0 && port < PD_PORT_COUNT);

	/*
	 * We have no way of determining the charger dualrole capability in
	 * locked RO, so just assume we always have a dedicated charger.
	 */
	if (system_get_image_copy() == SYSTEM_IMAGE_RO && system_is_locked())
		cap = CAP_DEDICATED;

	/* Ignore when capability is unchanged */
	if (cap != dualrole_capability[port]) {
		dualrole_capability[port] = cap;
		charge_manager_make_change(CHANGE_DUALROLE, 0, port, NULL);
	}
}

/**
 * Update charge ceiling for a given port.
 *
 * @param port			Charge port to update.
 * @param ceil			Charge ceiling (mA).
 */
void charge_manager_set_ceil(int port, int ceil)
{
	ASSERT(port >= 0 && port < PD_PORT_COUNT);

	if (charge_ceil[port] != ceil) {
		charge_ceil[port] = ceil;
		if (port == charge_port && charge_manager_is_seeded())
				hook_call_deferred(charge_manager_refresh, 0);
	}
}

/**
 * Select an 'override port', a port which is always the preferred charge port.
 * Returns EC_SUCCESS on success, ec_error_list status on failure.
 *
 * @param port			Charge port to select as override, or
 *				OVERRIDE_OFF to select no override port,
 *				or OVERRIDE_DONT_CHARGE to specifc that no
 *				charge port should be selected.
 */
int charge_manager_set_override(int port)
{
	int retval = EC_SUCCESS;

	ASSERT(port >= OVERRIDE_DONT_CHARGE && port < PD_PORT_COUNT);

	CPRINTS("Charge Override: %d", port);

	/* Supersede any pending delayed overrides. */
	if (delayed_override_port != OVERRIDE_OFF) {
		if (delayed_override_port != port)
			charge_manager_cleanup_override_port(
				delayed_override_port);

		delayed_override_port = OVERRIDE_OFF;
		hook_call_deferred(
			board_charge_manager_override_timeout, -1);
	}

	/* Set the override port if it's a sink. */
	if (port < 0 || pd_get_role(port) == PD_ROLE_SINK) {
		if (override_port != port) {
			charge_manager_cleanup_override_port(override_port);
			override_port = port;
			if (charge_manager_is_seeded())
				hook_call_deferred(charge_manager_refresh, 0);
		}
	}
	/*
	 * If the attached device is capable of being a sink, request a
	 * power swap and set the delayed override for swap completion.
	 */
	else if (pd_get_role(port) != PD_ROLE_SINK &&
		 dualrole_capability[port] == CAP_DUALROLE) {
		delayed_override_deadline.val = get_time().val +
						POWER_SWAP_TIMEOUT;
		delayed_override_port = port;
		hook_call_deferred(
			board_charge_manager_override_timeout,
			POWER_SWAP_TIMEOUT);
		pd_request_power_swap(port);
	/* Can't charge from requested port -- return error. */
	} else
		retval = EC_ERROR_INVAL;

	return retval;
}

/**
 * Get the override port. OVERRIDE_OFF if no override port.
 * OVERRIDE_DONT_CHARGE if override is set for no port.
 *
 * @return override port
 */
int charge_manager_get_override(void)
{
	return override_port;
}

int charge_manager_get_active_charge_port(void)
{
	return charge_port;
}

#ifndef TEST_CHARGE_MANAGER
static int hc_pd_power_info(struct host_cmd_handler_args *args)
{
	const struct ec_params_usb_pd_power_info *p = args->params;
	struct ec_response_usb_pd_power_info *r = args->response;
	int port = p->port;

	/* If host is asking for the charging port, set port appropriately */
	if (port == PD_POWER_CHARGING_PORT)
		port = charge_port;

	charge_manager_fill_power_info(port, r);

	args->response_size = sizeof(*r);
	return EC_RES_SUCCESS;
}
DECLARE_HOST_COMMAND(EC_CMD_USB_PD_POWER_INFO,
		     hc_pd_power_info,
		     EC_VER_MASK(0));
#endif /* TEST_CHARGE_MANAGER */

static int hc_charge_port_override(struct host_cmd_handler_args *args)
{
	const struct ec_params_charge_port_override *p = args->params;
	const int16_t override_port = p->override_port;

	if (override_port < OVERRIDE_DONT_CHARGE ||
	    override_port >= PD_PORT_COUNT)
		return EC_RES_INVALID_PARAM;

	return charge_manager_set_override(override_port);
}
DECLARE_HOST_COMMAND(EC_CMD_PD_CHARGE_PORT_OVERRIDE,
		     hc_charge_port_override,
		     EC_VER_MASK(0));

static int command_charge_port_override(int argc, char **argv)
{
	int port = OVERRIDE_OFF;
	int ret = EC_SUCCESS;
	char *e;

	if (argc >= 2) {
		port = strtoi(argv[1], &e, 0);
		if (*e || port < OVERRIDE_DONT_CHARGE || port >= PD_PORT_COUNT)
			return EC_ERROR_PARAM1;
		ret = charge_manager_set_override(port);
	}

	ccprintf("Override: %d\n", (argc >= 2 && ret == EC_SUCCESS) ?
					port : override_port);
	return ret;
}
DECLARE_CONSOLE_COMMAND(chgoverride, command_charge_port_override,
	"[port | -1 | -2]",
	"Force charging from a given port (-1 = off, -2 = disable charging)",
	NULL);
