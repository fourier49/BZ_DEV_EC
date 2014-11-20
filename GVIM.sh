#!/bin/sh
#============================================================================================================

function files_list()
{
	XuCat=$1
	if [ "$XuCat" = "hcs" -o "$XuCat" = "wip" ]; then
#		vimCmd="-c 'set tabstop=4'"
#		tags="$tags,$wpath/__scratch__/_LEDProj01.tags"
		files="$files common/system.c common/main.c "
#		files="$files common/host_command.c common/host_command_master.c common/host_event_commands.c common/host_command_pd.c "
#		files="$files common/usb_port_power_dumb.c common/usb_port_power_smart.c "
		files="$files include/usb_bb.h include/usb_pd.h "
#		files="$files Makefile Makefile.toolchain Makefile.rules README.fmap "

#		files="$files board/host/usb_pd_config.c board/host/usb_pd_config.h board/host/usb_pd_policy.c chip/host/usb_pd_phy.c "
#		files="$files test/usb_pd.c test/host_usb_pd_phy.c test/usb_pd.tasklist test/usb_pd_test_util.h "

		files="$files board/hoho/board.c board/hoho/board.h board/hoho/gpio.inc board/hoho/ec.tasklist "
		files="$files board/hoho/usb_pd_policy.c board/hoho/usb_pd_config.h "
		files="$files common/usb_pd_policy.c common/usb_pd_protocol.c "
		files="$files chip/stm32/usb_pd_phy.c "

#		files="$files board/dingdong/usb_pd_policy.c board/dingdong/usb_pd_config.h "

#		files="$files board/samus_pd/board.c board/samus_pd/board.h board/samus_pd/gpio.inc board/samus_pd/ec.tasklist "
#		files="$files board/samus_pd/usb_pd_policy.c board/samus_pd/usb_pd_config.h "

		files="$files board/kronitech/board.c board/kronitech/board.h board/kronitech/gpio.inc board/kronitech/ec.tasklist "
		files="$files board/kronitech/usb_pd_policy.c board/kronitech/usb_pd_config.h "


	elif [ -f "$XuCat" ]; then
		# this might be a SINGLE file, instead of a group of files
		files="$files  "
		files="$files $XuCat "
	fi
}

wpath=`pwd`
#tags="$wpath/__scratch__/nRF51-common.tags"
#SRC_FOLDERS="myproj/ nrf51422/ nrf51822/"
#SRC_FOLDERS="myproj/"

return
#============================================================================================================
TODO::

-----------------------------------------------------------------------------------------------------
http://dev.chromium.org/chromium-os/ec-development
--------------------------------------------------
The EC software is written in C and currently supports two different ARM Cortex based controllers.
Intel based designs, such as the Chromebook Pixel use the TI Stellaris LM4F (Cortex M4) while the
Samsung Chromebook (XE303C12) and HP Chromebook 11 use an ST-Micro STM32F100 (Cortex M3).
Some STM32L variants are also supported.


bash command to find the CHIP used by each variants of board
	find -name \*.mk -print -exec grep CHIP {} \;

-----------------------------------------------------------------------------------------------------
Tasks typically have a top-level loop with a call to task_wait_event() or usleep()
The list of tasks for a board is specified in ec.tasklist in the board/$BOARD/ sub-directory.

The console “taskinfo” command will print run-time stats on each task:
> taskinfo
Task Ready Name         Events      Time (s)  StkUsed
   0 R << idle >>       00000000   32.975554  196/256
   1 R HOOKS            00000000    0.007835  192/488
   2   VBOOTHASH        00000000    0.042818  392/488
   3   POWERLED         00000000    0.000096  120/256
   4   CHARGER          00000000    0.029050  392/488
   5   CHIPSET          00000000    0.017558  400/488
   6   HOSTCMD          00000000    0.379277  328/488
   7 R CONSOLE          00000000    0.042050  348/640
   8   KEYSCAN          00000000    0.002988  292/488

-----------------------------------------------------------------------------------------------------
Hooks allow you to register a function to be run when specific events occur; such as the host suspending or external power being applied:
	DECLARE_HOOK(HOOK_AC_CHANGE, ac_change_callback, HOOK_PRIO_DEFAULT);

There are also hooks for running functions periodically: HOOK_TICK (fires every HOOK_TICK_INVERVAL mSec) and HOOK_SECOND.

-----------------------------------------------------------------------------------------------------
Deferred functions allow you to call a function after a delay specified in uSec without blocking. Deferred functions run in the HOOKS task.

Here is an example of an interrupt handler. Delaying the deferred call by 30 mSec also allows the interrupt to be debounced.
//--------------------------------------------------------------------
static int debounced_gpio_state;

static void some_interrupt_deferred(void)
{
 int gpio_state = gpio_get_level(GPIO_SOME_SIGNAL);

 if (gpio_state == debounced_gpio_state)
 return;

 debounced_gpio_state = gpio_state; 
 dispense_sandwich(); /* Or some other useful action. */
}
/* A function must be explicitly declared as being deferrable. */ 
DECLARE_DEFERRED(some_interrupt_deferred); 

void some_interrupt(enum gpio_signal signal)
{
    hook_call_deferred(some_interrupt_deferred, 30 * MSEC);
}


-----------------------------------------------------------------------------------------------------
build Chromium EC procedure
---------------------------------
1. download "full" chromium source code
cd ${SOURCE_REPO}
repo init -u https://chromium.googlesource.com/chromiumos/manifest.git
repo sync


2. instal depot_tools
./chromium/tools/depot_tools/cros_sdk
cd ~/trunk/src/platform/ec
make -j BOARD=list

binary image is under build/[XXX], ec.bin

3. delete build directory before running emerge
make clobber

4. reference
install cros_sdk
http://www.boyunjian.com/do/article/snapshot.do?uid=9018504810107928784


-----------------------------------------------------------------------------------------------------
# to analyze the binary data of AHC_V4L Video Buffer (from ADS-ICE memory dump)
hexdump -f /tmp/.bshyu.hexdump2 /tmp/ads-V4Lmem-0x4A00000.bin | awk '{ if ($0 ~ "55 aa 00 ff") { show=NR+3; printf "\n"; } if (NR < show) print $0; }' | less

-----------------------------------------------------------------------------------------------------
Chromium contribution
-----------------------
https://chromium-review.googlesource.com/


-----------------------------------------------
The only different thing about this is the refs/for/master branch. This is a magic branch that creates reviews that target the master branch. For every branch Gerrit tracks there is a magic refs/for/<branch_name> that you push to to create reviews.
-----------------------------------------------
git commit
git push origin HEAD:refs/for/master



-----------------------
git --no-pager diff --no-ext-diff --no-color -w xxx/yyy/zzz.c
git diff --no-ext-diff xxx/yyy/zzz.c

-----------------------------------------------------------------------------------------------------
# Creation of CTAGS cross-reference dictionary
ctags -R --exclude=*extra/*

-----------------------------------------------------------------------------------------------------
"$HOME/Documents/08-Standards/USB3.1_081114/USB Type-C/USB Type-C Specification Release 1.0.pdf"
"$HOME/Documents/08-Standards/USB3.1_081114/USB Power Delivery/USB_PD_R2_0 V1.0 - 20140807.pdf"
"$HOME/Documents/08-Standards/VESA/spec/BXu-DP_Alt_Mode_on_USB_Type-C_v1.0.pdf"
"$HOME/Documents/10-RD/general/2014 Q4 RD Quarterly meeting/Type-C Intro/BizLink USB Type-C Introduction, Oct 17 2014.pdf"
"$HOME/Documents/10-RD/Type-C primer/solutions/STM/STM32F0xx RM.pdf"
"$HOME/Documents/10-RD/Type-C primer/solutions/STM/STM32F072xx DS.pdf"
"$HOME/Documents/10-RD/Type-C primer/solutions/Google/810-10116-02_20141007_hoho_SCH_0.pdf"

evince "$HOME/Documents/08-Standards/USB3.1_081114/USB Type-C/USB Type-C Specification Release 1.0.pdf" "$HOME/Documents/08-Standards/USB3.1_081114/USB Power Delivery/USB_PD_R2_0 V1.0 - 20140807.pdf" "$HOME/Documents/08-Standards/VESA/spec/BXu-DP_Alt_Mode_on_USB_Type-C_v1.0.pdf" "$HOME/Documents/10-RD/general/2014 Q4 RD Quarterly meeting/Type-C Intro/BizLink USB Type-C Introduction, Oct 17 2014.pdf" "$HOME/Documents/10-RD/Type-C primer/solutions/STM/STM32F0xx RM.pdf" "$HOME/Documents/10-RD/Type-C primer/solutions/STM/STM32F072xx DS.pdf" "$HOME/Documents/10-RD/Type-C primer/solutions/Google/810-10116-02_20141007_hoho_SCH_0.pdf"  &



#============================================================================================================
COMMIT-QUEUE.ini
LICENSE
Makefile
Makefile.rules
Makefile.toolchain
OWNERS
PRESUBMIT.cfg
README
README.fmap

./include/gpio.wrap

----------------------------------------------------------------------------------------
bshyu@BXuPortegeR30:~/work/BZL/chromium-ec$ find \( -name \*.[ch] -o -name \*.mk -o -name ec.tasklist \) | sort
./common/acpi.c
./common/adc.c
./common/als.c
./common/ap_hang_detect.c
./common/backlight_lid.c
./common/battery.c
./common/build.mk
./common/button.c
./common/capsense.c
./common/charge_manager.c
./common/charger.c
./common/charge_state_v1.c
./common/charge_state_v2.c
./common/chipset.c
./common/clz.c
./common/console.c
./common/console_output.c
./common/crc8.c
./common/crc.c
./common/eoption.c
./common/extpower_falco.c
./common/extpower_gpio.c
./common/extpower_snow.c
./common/extpower_spring.c
./common/fan.c
./common/flash.c
./common/fmap.c
./common/gpio.c
./common/hooks.c
./common/host_command.c
./common/host_command_master.c
./common/host_command_pd.c
./common/host_event_commands.c
./common/i2c_arbitration.c
./common/i2c.c
./common/i2c_wedge.c
./common/inductive_charging.c
./common/in_stream.c
./common/keyboard_8042.c
./common/keyboard_mkbp.c
./common/keyboard_scan.c
./common/keyboard_test.c
./common/lb_common.c
./common/led_common.c
./common/lid_angle.c
./common/lid_switch.c
./common/lightbar.c
./common/main.c
./common/math_util.c
./common/memory_commands.c
./common/mkbp_event.c
./common/motion_sense.c
./common/onewire.c
./common/out_stream.c
./common/panic_output.c
./common/pmu_tps65090.c
./common/pmu_tps65090_charger.c
./common/pmu_tps65090_powerinfo.c
./common/port80.c
./common/power_button.c
./common/power_button_x86.c
./common/printf.c
./common/pstore_commands.c
./common/pwm.c
./common/pwm_kblight.c
./common/queue.c
./common/rsa.c
./common/sha1.c
./common/sha256.c
./common/shared_mem.c
./common/smbus.c
./common/spi_flash.c
./common/switch.c
./common/system.c
./common/temp_sensor.c
./common/test_util.c
./common/thermal.c
./common/throttle_ap.c
./common/timer.c
./common/uart_buffering.c
./common/usb_pd_policy.c
./common/usb_pd_protocol.c
./common/usb_port_power_dumb.c
./common/usb_port_power_smart.c
./common/util.c
./common/vboot_hash.c
./common/version.c
./common/wireless.c

./core/cortex-m0/atomic.h
./core/cortex-m0/build.mk
./core/cortex-m0/config_core.h
./core/cortex-m0/cpu.c
./core/cortex-m0/cpu.h
./core/cortex-m0/irq_handler.h
./core/cortex-m0/panic.c
./core/cortex-m0/task.c
./core/cortex-m0/watchdog.c

./core/cortex-m/atomic.h
./core/cortex-m/build.mk
./core/cortex-m/config_core.h
./core/cortex-m/cpu.c
./core/cortex-m/cpu.h
./core/cortex-m/include/math.h
./core/cortex-m/include/mpu.h
./core/cortex-m/irq_handler.h
./core/cortex-m/mpu.c
./core/cortex-m/panic.c
./core/cortex-m/task.c
./core/cortex-m/watchdog.c

./core/host/atomic.h
./core/host/build.mk
./core/host/cpu.h
./core/host/disabled.c
./core/host/host_task.h
./core/host/irq_handler.h
./core/host/main.c
./core/host/panic.c
./core/host/stack_trace.c
./core/host/task.c
./core/host/timer.c

./core/nds32/atomic.h
./core/nds32/build.mk
./core/nds32/config_core.h
./core/nds32/cpu.c
./core/nds32/cpu.h
./core/nds32/irq_chip.h
./core/nds32/irq_handler.h
./core/nds32/panic.c
./core/nds32/task.c

./driver/accelgyro_lsm6ds0.c
./driver/accelgyro_lsm6ds0.h
./driver/accel_kxcj9.c
./driver/accel_kxcj9.h
./driver/als_isl29035.c
./driver/als_isl29035.h
./driver/battery/bq20z453.c
./driver/battery/bq27541.c
./driver/battery/link.c
./driver/battery/samus.c
./driver/battery/sb_fw_update.c
./driver/battery/sb_fw_update.h
./driver/battery/smart.c
./driver/build.mk
./driver/charger/bq24192.c
./driver/charger/bq24192.h
./driver/charger/bq24707a.c
./driver/charger/bq24707a.h
./driver/charger/bq24715.c
./driver/charger/bq24715.h
./driver/charger/bq24725.c
./driver/charger/bq24725.h
./driver/charger/bq24735.c
./driver/charger/bq24735.h
./driver/charger/bq24738.c
./driver/charger/bq24738.h
./driver/charger/bq24773.c
./driver/charger/bq24773.h
./driver/ina2xx.c
./driver/ina2xx.h
./driver/ioexpander_pca9534.c
./driver/ioexpander_pca9534.h
./driver/led/ds2413.c
./driver/led/lp5562.c
./driver/led/lp5562.h
./driver/pi3usb9281.h
./driver/regulator_ir357x.c
./driver/temp_sensor/g781.c
./driver/temp_sensor/g781.h
./driver/temp_sensor/tmp006.c
./driver/temp_sensor/tmp006.h
./driver/temp_sensor/tmp432.c
./driver/temp_sensor/tmp432.h
./driver/tsu6721.h
./driver/usb_switch_pi3usb9281.c
./driver/usb_switch_tsu6721.c

./extra/lightbar/input.c
./extra/lightbar/main.c
./extra/lightbar/simulation.h
./extra/lightbar/windows.c
./extra/usb_gpio/usb_gpio.c

./include/accelgyro.h
./include/acpi.h
./include/adc.h
./include/als.h
./include/ap_hang_detect.h
./include/backlight.h
./include/battery.h
./include/battery_smart.h
./include/board_config.h
./include/button.h
./include/capsense.h
./include/charge_manager.h
./include/charger.h
./include/charge_state.h
./include/charge_state_v1.h
./include/charge_state_v2.h
./include/chipset.h
./include/clock.h
./include/common.h
./include/compile_time_macros.h
./include/config.h
./include/console.h
./include/crc8.h
./include/crc.h
./include/dma.h
./include/dptf.h
./include/ec_commands.h
./include/eeprom.h
./include/eoption.h
./include/extpower_falco.h
./include/extpower.h
./include/extpower_spring.h
./include/fan.h
./include/flash.h
./include/gesture.h
./include/gpio.h
./include/gpio_list.h
./include/gpio_signal.h
./include/hooks.h
./include/host_command.h
./include/hwtimer.h
./include/i2c_arbitration.h
./include/i2c.h
./include/i8042_protocol.h
./include/inductive_charging.h
./include/in_stream.h
./include/jtag.h
./include/keyboard_8042.h
./include/keyboard_config.h
./include/keyboard_mkbp.h
./include/keyboard_protocol.h
./include/keyboard_raw.h
./include/keyboard_scan.h
./include/keyboard_test.h
./include/lb_common.h
./include/led_common.h
./include/lid_angle.h
./include/lid_switch.h
./include/lightbar.h
./include/lightbar_msg_list.h
./include/lightbar_opcode_list.h
./include/link_defs.h
./include/lpc.h
./include/math_util.h
./include/memory_commands.h
./include/mkbp_event.h
./include/module_id.h
./include/motion_sense.h
./include/onewire.h
./include/out_stream.h
./include/panic.h
./include/peci.h
./include/pmu_tpschrome.h
./include/port80.h
./include/power_button.h
./include/power.h
./include/power_led.h
./include/printf.h
./include/pwm.h
./include/queue.h
./include/rsa.h
./include/sha1.h
./include/sha256.h
./include/shared_mem.h
./include/smbus.h
./include/spi_flash.h
./include/spi.h
./include/stack_trace.h
./include/switch.h
./include/system.h
./include/task.h
./include/task_id.h
./include/temp_sensor_chip.h
./include/temp_sensor.h
./include/test_util.h
./include/thermal.h
./include/throttle_ap.h
./include/timer.h
./include/uart.h
./include/usb_bb.h
./include/usb_charge.h
./include/usb_console.h
./include/usb.h
./include/usb_hid.h
./include/usb_ms.h
./include/usb_ms_scsi.h
./include/usb_pd.h
./include/util.h
./include/vboot_hash.h
./include/version.h
./include/watchdog.h
./include/wireless.h

./power/baytrail.c
./power/build.mk
./power/common.c
./power/gaia.c
./power/haswell.c
./power/ivybridge.c
./power/rockchip.c
./power/tegra.c

./test/adapter.c
./test/adapter_externs.h
./test/battery_get_params_smart.c
./test/bklight_lid.c
./test/bklight_passthru.c
./test/build.mk
./test/button.c
./test/console_edit.c
./test/extpwr_gpio.c
./test/flash.c
./test/hooks.c
./test/host_command.c
./test/inductive_charging.c
./test/interrupt.c
./test/kb_8042.c
./test/kb_mkbp.c
./test/kb_scan.c
./test/led_spring.c
./test/led_spring_impl.c
./test/lid_sw.c
./test/lightbar.c
./test/math_util.c
./test/motion_sense.c
./test/mutex.c
./test/pingpong.c
./test/power_button.c
./test/powerdemo.c
./test/powerdemo.h
./test/queue.c
./test/sbs_charging.c
./test/sbs_charging_v2.c
./test/stress.c
./test/system.c
./test/test_config.h
./test/thermal.c
./test/thermal_falco.c
./test/thermal_falco_externs.h
./test/timer_calib.c
./test/timer_dos.c
./test/usb_pd.c
./test/usb_pd_test_util.h
./test/utils.c

./util/build.mk
./util/burn_my_ec.c
./util/comm-dev.c
./util/comm-host.c
./util/comm-host.h
./util/comm-i2c.c
./util/comm-lpc.c
./util/comm-mec1322.c
./util/cros_ec_dev.h
./util/ec_flash.c
./util/ec_flash.h
./util/ec_sb_firmware_update.c
./util/ec_sb_firmware_update.h
./util/ectool.c
./util/ectool.h
./util/ectool_keyscan.c
./util/ec_uartd.c
./util/iteflash.c
./util/lbcc.c
./util/lbplay.c
./util/lock/build.mk
./util/lock/csem.c
./util/lock/csem.h
./util/lock/gec_lock.c
./util/lock/gec_lock.h
./util/lock/ipc_lock.c
./util/lock/ipc_lock.h
./util/lock/locks.h
./util/misc_util.c
./util/misc_util.h
./util/stm32mon.c


-------------------------------------------------------------------------
./board/dingdong/board.c
./board/dingdong/board.h
./board/dingdong/build.mk
./board/dingdong/ec.tasklist
./board/dingdong/usb_pd_config.h
./board/dingdong/usb_pd_policy.c
./board/hoho/board.c
./board/hoho/board.h
./board/hoho/build.mk
./board/hoho/ec.tasklist
./board/hoho/usb_pd_config.h
./board/hoho/usb_pd_policy.c
./board/samus_pd/board.c
./board/samus_pd/board.h
./board/samus_pd/build.mk
./board/samus_pd/ec.tasklist
./board/samus_pd/usb_pd_config.h
./board/samus_pd/usb_pd_policy.c
./board/discovery-stm32f072/board.c
./board/discovery-stm32f072/board.h
./board/discovery-stm32f072/build.mk
./board/discovery-stm32f072/echo.c
./board/discovery-stm32f072/ec.tasklist
./board/discovery-stm32f072/spi.c

./board/host/battery.c
./board/host/board.c
./board/host/board.h
./board/host/build.mk
./board/host/charger.c
./board/host/chipset.c
./board/host/ec.tasklist
./board/host/fan.c
./board/host/usb_pd_config.c
./board/host/usb_pd_config.h
./board/host/usb_pd_policy.c


-------------------------------------------------------------------------
./chip/stm32/adc_chip.h
./chip/stm32/adc-stm32f0.c
./chip/stm32/adc-stm32f3.c
./chip/stm32/adc-stm32f.c
./chip/stm32/adc-stm32l.c
./chip/stm32/build.mk
./chip/stm32/clock-stm32f0.c
./chip/stm32/clock-stm32f3.c
./chip/stm32/clock-stm32f.c
./chip/stm32/clock-stm32l.c
./chip/stm32/config_chip.h
./chip/stm32/config-stm32f03x.h
./chip/stm32/config-stm32f07x.h
./chip/stm32/config-stm32f100.h
./chip/stm32/config-stm32f10x.h
./chip/stm32/config-stm32f373.h
./chip/stm32/config-stm32l100.h
./chip/stm32/config-stm32l15x.h
./chip/stm32/config-stm32ts60.h
./chip/stm32/crc_hw.h
./chip/stm32/dma.c
./chip/stm32/flash-f.c
./chip/stm32/flash-stm32f0.c
./chip/stm32/flash-stm32f3.c
./chip/stm32/flash-stm32f.c
./chip/stm32/flash-stm32l.c
./chip/stm32/gpio.c
./chip/stm32/gpio-f0-l.c
./chip/stm32/gpio-stm32f0.c
./chip/stm32/gpio-stm32f3.c
./chip/stm32/gpio-stm32f.c
./chip/stm32/gpio-stm32l.c
./chip/stm32/hwtimer32.c
./chip/stm32/hwtimer.c
./chip/stm32/i2c.c
./chip/stm32/i2c-stm32f0.c
./chip/stm32/i2c-stm32f3.c
./chip/stm32/i2c-stm32f.c
./chip/stm32/i2c-stm32l.c
./chip/stm32/jtag-stm32f0.c
./chip/stm32/jtag-stm32f3.c
./chip/stm32/jtag-stm32f.c
./chip/stm32/jtag-stm32l.c
./chip/stm32/keyboard_raw.c
./chip/stm32/power_led.c
./chip/stm32/pwm.c
./chip/stm32/pwm_chip.h
./chip/stm32/registers.h
./chip/stm32/spi.c
./chip/stm32/spi_master.c
./chip/stm32/system.c
./chip/stm32/uart.c
./chip/stm32/usart.c
./chip/stm32/usart.h
./chip/stm32/usart-stm32f0.c
./chip/stm32/usart-stm32f0.h
./chip/stm32/usart-stm32f3.c
./chip/stm32/usart-stm32f.c
./chip/stm32/usart-stm32f.h
./chip/stm32/usart-stm32l.c
./chip/stm32/usart-stm32l.h
./chip/stm32/usb.c
./chip/stm32/usb_console.c
./chip/stm32/usb_gpio.c
./chip/stm32/usb_gpio.h
./chip/stm32/usb_hid.c
./chip/stm32/usb_ms.c
./chip/stm32/usb_ms_scsi.c
./chip/stm32/usb_pd_phy.c
./chip/stm32/usb_spi.c
./chip/stm32/usb_spi.h
./chip/stm32/usb-stream.c
./chip/stm32/usb-stream.h
./chip/stm32/watchdog.c

./chip/host/build.mk
./chip/host/clock.c
./chip/host/config_chip.h
./chip/host/flash.c
./chip/host/gpio.c
./chip/host/host_test.h
./chip/host/i2c.c
./chip/host/keyboard_raw.c
./chip/host/lpc.c
./chip/host/persistence.c
./chip/host/persistence.h
./chip/host/reboot.c
./chip/host/reboot.h
./chip/host/registers.h
./chip/host/system.c
./chip/host/uart.c
./chip/host/usb_pd_phy.c


-------------------------------------------------------------------------
./board/auron/battery.c
./board/auron/board.c
./board/auron/board.h
./board/auron/build.mk
./board/auron/ec.tasklist
./board/auron/led.c
./board/bds/board.c
./board/bds/board.h
./board/bds/build.mk
./board/bds/ec.tasklist
./board/big/battery.c
./board/big/board.c
./board/big/board.h
./board/big/build.mk
./board/big/ec.tasklist
./board/big/led.c
./board/discovery/board.c
./board/discovery/board.h
./board/discovery/build.mk
./board/discovery/ec.tasklist
./board/falco/battery.c
./board/falco/board.c
./board/falco/board.h
./board/falco/build.mk
./board/falco/ec.tasklist
./board/falco/led.c
./board/falco/panel.c
./board/firefly/board.c
./board/firefly/board.h
./board/firefly/build.mk
./board/firefly/ec.tasklist
./board/firefly/usb_pd_config.h
./board/firefly/usb_pd_policy.c
./board/fruitpie/board.c
./board/fruitpie/board.h
./board/fruitpie/build.mk
./board/fruitpie/ec.tasklist
./board/fruitpie/usb_pd_config.h
./board/fruitpie/usb_pd_policy.c
./board/hadoken/board.c
./board/hadoken/board.h
./board/hadoken/build.mk
./board/hadoken/ec.tasklist
./board/it8380dev/board.c
./board/it8380dev/board.h
./board/it8380dev/build.mk
./board/it8380dev/ec.tasklist
./board/jerry/battery.c
./board/jerry/board.c
./board/jerry/board.h
./board/jerry/build.mk
./board/jerry/ec.tasklist
./board/jerry/led.c
./board/keyborg/board.c
./board/keyborg/board.h
./board/keyborg/build.mk
./board/keyborg/debug.c
./board/keyborg/debug.h
./board/keyborg/ec.tasklist
./board/keyborg/encode.h
./board/keyborg/encode_raw.c
./board/keyborg/encode_segment.c
./board/keyborg/hardware.c
./board/keyborg/master_slave.c
./board/keyborg/master_slave.h
./board/keyborg/runtime.c
./board/keyborg/spi_comm.c
./board/keyborg/spi_comm.h
./board/keyborg/touch_scan.c
./board/keyborg/touch_scan.h
./board/link/board.c
./board/link/board.h
./board/link/build.mk
./board/link/ec.tasklist
./board/mccroskey/board.c
./board/mccroskey/board.h
./board/mccroskey/build.mk
./board/mccroskey/ec.tasklist
./board/mec1322_evb/board.c
./board/mec1322_evb/board.h
./board/mec1322_evb/build.mk
./board/mec1322_evb/ec.tasklist
./board/nyan/battery.c
./board/nyan/board.c
./board/nyan/board.h
./board/nyan/build.mk
./board/nyan/ec.tasklist
./board/peppy/battery.c
./board/peppy/board.c
./board/peppy/board.h
./board/peppy/build.mk
./board/peppy/ec.tasklist
./board/peppy/led.c
./board/pinky/battery.c
./board/pinky/board.c
./board/pinky/board.h
./board/pinky/build.mk
./board/pinky/ec.tasklist
./board/pinky/led.c
./board/pit/board.c
./board/pit/board.h
./board/pit/build.mk
./board/pit/ec.tasklist
./board/plankton/board.c
./board/plankton/board.h
./board/plankton/build.mk
./board/plankton/ec.tasklist
./board/plankton/usb_pd_config.h
./board/plankton/usb_pd_policy.c
./board/rambi/battery.c
./board/rambi/board.c
./board/rambi/board.h
./board/rambi/build.mk
./board/rambi/ec.tasklist
./board/rambi/led.c
./board/ryu/battery.c
./board/ryu/board.c
./board/ryu/board.h
./board/ryu/build.mk
./board/ryu/ec.tasklist
./board/ryu_p2/battery.c
./board/ryu_p2/board.c
./board/ryu_p2/board.h
./board/ryu_p2/build.mk
./board/ryu_p2/ec.tasklist
./board/ryu_p2/usb_pd_config.h
./board/ryu_p2/usb_pd_policy.c
./board/ryu_sh/board.c
./board/ryu_sh/board.h
./board/ryu_sh/build.mk
./board/ryu_sh/ec.tasklist
./board/ryu/usb_pd_config.h
./board/ryu/usb_pd_policy.c
./board/samus/board.c
./board/samus/board.h
./board/samus/build.mk
./board/samus/ec.tasklist
./board/samus/extpower.c
./board/samus/gesture.c
./board/samus/panel.c
./board/samus/power_sequence.c
./board/snow/board.c
./board/snow/board.h
./board/snow/build.mk
./board/snow/ec.tasklist
./board/spring/battery.c
./board/spring/board.c
./board/spring/board.h
./board/spring/build.mk
./board/spring/ec.tasklist
./board/spring/led.c
./board/squawks/battery.c
./board/squawks/board.c
./board/squawks/board.h
./board/squawks/build.mk
./board/squawks/ec.tasklist
./board/squawks/led.c
./board/twinkie/board.c
./board/twinkie/board.h
./board/twinkie/build.mk
./board/twinkie/ec.tasklist
./board/twinkie/sniffer.c
./board/twinkie/usb_pd_config.h
./board/twinkie/usb_pd_policy.c
./board/zinger/board.c
./board/zinger/board.h
./board/zinger/build.mk
./board/zinger/debug.c
./board/zinger/debug.h
./board/zinger/ec.tasklist
./board/zinger/hardware.c
./board/zinger/runtime.c
./board/zinger/usb_pd_config.h
./board/zinger/usb_pd_policy.c


-------------------------------------------------------------------------
./chip/it83xx/build.mk
./chip/it83xx/clock.c
./chip/it83xx/config_chip.h
./chip/it83xx/gpio.c
./chip/it83xx/hwtimer.c
./chip/it83xx/irq.c
./chip/it83xx/jtag.c
./chip/it83xx/registers.h
./chip/it83xx/system.c
./chip/it83xx/uart.c
./chip/it83xx/watchdog.c
./chip/lm4/adc.c
./chip/lm4/adc_chip.h
./chip/lm4/build.mk
./chip/lm4/chip_temp_sensor.c
./chip/lm4/clock.c
./chip/lm4/config_chip.h
./chip/lm4/eeprom.c
./chip/lm4/fan.c
./chip/lm4/flash.c
./chip/lm4/gpio.c
./chip/lm4/hwtimer.c
./chip/lm4/i2c.c
./chip/lm4/jtag.c
./chip/lm4/keyboard_raw.c
./chip/lm4/lpc.c
./chip/lm4/peci.c
./chip/lm4/pwm.c
./chip/lm4/pwm_chip.h
./chip/lm4/registers.h
./chip/lm4/spi.c
./chip/lm4/system.c
./chip/lm4/uart.c
./chip/lm4/watchdog.c
./chip/mec1322/adc.c
./chip/mec1322/adc_chip.h
./chip/mec1322/build.mk
./chip/mec1322/clock.c
./chip/mec1322/config_chip.h
./chip/mec1322/dma.c
./chip/mec1322/fan.c
./chip/mec1322/gpio.c
./chip/mec1322/hwtimer.c
./chip/mec1322/i2c.c
./chip/mec1322/jtag.c
./chip/mec1322/keyboard_raw.c
./chip/mec1322/lpc.c
./chip/mec1322/pwm.c
./chip/mec1322/pwm_chip.h
./chip/mec1322/registers.h
./chip/mec1322/spi.c
./chip/mec1322/system.c
./chip/mec1322/uart.c
./chip/mec1322/watchdog.c
./chip/nrf51/build.mk
./chip/nrf51/clock.c
./chip/nrf51/config_chip.h
./chip/nrf51/gpio.c
./chip/nrf51/hwtimer.c
./chip/nrf51/jtag.c
./chip/nrf51/registers.h
./chip/nrf51/system.c
./chip/nrf51/uart.c
./chip/nrf51/watchdog.c



-------------------------------------------------------------------------
bshyu@BXuPortegeR30:~/work/BZL/chromium-ec$ for f in `find -name usb_pd_policy.c`; do d=`dirname $f`; echo -e "\n--------------------------------------\n$f"; grep 'the IC is' $d/build.mk; grep '#define CONFIG_USBC_VCONN' $d/board.h; done

--------------------------------------
./common/usb_pd_policy.c
grep: ./common/board.h: No such file or directory

--------------------------------------
./board/dingdong/usb_pd_policy.c
# the IC is STmicro STM32F072B

--------------------------------------
./board/host/usb_pd_policy.c

--------------------------------------
./board/zinger/usb_pd_policy.c
# the IC is STmicro STM32F031F6

--------------------------------------
./board/plankton/usb_pd_policy.c
# the IC is STmicro STM32F072CBU6

--------------------------------------
./board/twinkie/usb_pd_policy.c
# the IC is STmicro STM32F072B

--------------------------------------
./board/fruitpie/usb_pd_policy.c
# the IC is STmicro STM32F072B
#define CONFIG_USBC_VCONN

--------------------------------------
./board/firefly/usb_pd_policy.c
# the IC is STmicro STM32F072CBU6

--------------------------------------
./board/ryu/usb_pd_policy.c
# the IC is STmicro STM32F072VBH6
#define CONFIG_USBC_VCONN

--------------------------------------
./board/hoho/usb_pd_policy.c
# the IC is STmicro STM32F072B

--------------------------------------
./board/samus_pd/usb_pd_policy.c
# the IC is STmicro STM32F072VBH6
#define CONFIG_USBC_VCONN



#============================================================================================================
Command line for media player:
vlc --longhelp --advanced | less

url=rtsp://192.168.11.8/liveRTSP/v1
ncache=4000
vlcOPTS="--file-logging --logfile ~/temp/vlc-log.`date '+%Y%m%d'`.txt"
vlcOPTS="$vlcOPTS --network-caching $ncache "
vlcOPTS="$vlcOPTS --verbose 2 "
vlcOPTS="$vlcOPTS --clock-jitter=0 "
vlcOPTS="$vlcOPTS --no-drop-late-frames "
vlc $vlcOPTS $url

url=http://192.168.11.8/cgi-bin/liveMJPEG
vlcOPTS="--file-logging --logfile ~/temp/vlc-log.`date '+%Y%m%d'`.txt"
vlcOPTS="$vlcOPTS --verbose 2 "
vlc $vlcOPTS $url


openRTSP -b 500000 -B 500000 rtsp://192.168.11.8/liveRTSP/v1


--------------------------------------------------------------------------
cd ~/work/wifi/packages/vlc/vlc-2.0.8
./GVIM

#============================================================================================================
my cheat sheet
#--------------------------------------------
SS=""; RR=""; FOLDERS="./"
find $FOLDERS \( -name \*.[ch] -o -name \*.inc \) -exec sed -ie "s,$SS,$RR,g" {} \;  ; find $FOLDERS \( -name \*.[ch]e -o -name \*.ince \) -print -exec rm {} \;

#--------------------------------------------
ctags -R board/hoho chip/stm32 common core/cortex-m0 driver include power test util
ctags -R board/hoho chip/stm32 common core/cortex-m0 driver include power test util board/samus_pd

./chromium/tools/depot_tools/cros_sdk
cd ~/trunk/src/platform/ec
make clobber
make -j BOARD=hoho
make tests -j BOARD=hoho
make buildall

evince "$HOME/Documents/08-Standards/USB3.1_081114/USB Type-C/USB Type-C Specification Release 1.0.pdf" "$HOME/Documents/08-Standards/USB3.1_081114/USB Power Delivery/USB_PD_R2_0 V1.0 - 20140807.pdf" "$HOME/Documents/08-Standards/VESA/spec/BXu-DP_Alt_Mode_on_USB_Type-C_v1.0.pdf" "$HOME/Documents/10-RD/general/2014 Q4 RD Quarterly meeting/Type-C Intro/BizLink USB Type-C Introduction, Oct 17 2014.pdf" "$HOME/Documents/10-RD/Type-C primer/solutions/STM/STM32F0xx RM.pdf" "$HOME/Documents/10-RD/Type-C primer/solutions/STM/STM32F072xx DS.pdf" "$HOME/Documents/10-RD/Type-C primer/solutions/Google/810-10116-02_20141007_hoho_SCH_0.pdf"  &

