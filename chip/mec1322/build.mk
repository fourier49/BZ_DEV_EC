# -*- makefile -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# MEC1322 chip specific files build
#

# MEC1322 SoC has a Cortex-M4 ARM core
CORE:=cortex-m
# Allow the full Cortex-M4 instruction set
CFLAGS_CPU+=-march=armv7e-m -mcpu=cortex-m4

# Required chip modules
chip-y=clock.o gpio.o hwtimer.o system.o uart.o jtag.o
chip-$(CONFIG_ADC)+=adc.o
chip-$(CONFIG_FANS)+=fan.o
chip-$(CONFIG_FLASH)+=flash.o
chip-$(CONFIG_I2C)+=i2c.o
chip-$(CONFIG_LPC)+=lpc.o
chip-$(CONFIG_PWM)+=pwm.o
chip-$(CONFIG_WATCHDOG)+=watchdog.o
chip-$(HAS_TASK_KEYSCAN)+=keyboard_raw.o
chip-$(CONFIG_DMA)+=dma.o
chip-$(CONFIG_SPI)+=spi.o

# location of the scripts and keys used to pack the SPI flash image
SCRIPTDIR:=./chip/${CHIP}/util

# Allow SPI size to be overridden by board specific size, default to 256KB.
CHIP_SPI_SIZE_KB?=256

# Commands to convert $^ to $@.tmp
cmd_obj_to_bin = $(OBJCOPY) --gap-fill=0xff -O binary $< $@.tmp1 ; \
		 ${SCRIPTDIR}/pack_ec.py -o $@.tmp -i $@.tmp1 \
		--loader_file $(mec1322-lfw-flat) \
		--payload_key ${SCRIPTDIR}/rsakey_sign_payload.pem \
		--header_key ${SCRIPTDIR}/rsakey_sign_header.pem \
		--spi_size ${CHIP_SPI_SIZE_KB} ; rm -f $@.tmp1

mec1322-lfw = chip/mec1322/lfw/ec_lfw
mec1322-lfw-flat = $(out)/$(mec1322-lfw)-lfw.flat

# build these specifically for lfw with -lfw suffix
objs_lfw = $(patsubst %, $(out)/%-lfw.o, \
		$(addprefix common/, util gpio) \
		$(addprefix chip/$(CHIP)/, spi dma gpio clock hwtimer) \
		core/$(CORE)/cpu $(mec1322-lfw))

# reuse version.o (and its dependencies) from main board
objs_lfw += $(out)/common/version.o

dirs-y+=chip/$(CHIP)/lfw

# objs with -lfw suffix are to include lfw's gpio
$(out)/%-lfw.o: private CC+=-Iboard/$(BOARD)/lfw
$(out)/%-lfw.o: %.c
	$(call quiet,c_to_o,CC     )

# let lfw's elf link only with selected objects
$(out)/%-lfw.elf: private objs = $(objs_lfw)
$(out)/%-lfw.elf: %.ld $(objs_lfw)
	$(call quiet,elf,LD     )

# final image needs lfw loader
$(out)/$(PROJECT).bin: $(mec1322-lfw-flat)
