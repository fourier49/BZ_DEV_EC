/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_CONFIG_CHIP_H
#define __CROS_EC_CONFIG_CHIP_H

#include "core/cortex-m/config_core.h"

/* Number of IRQ vectors on the NVIC */
#define CONFIG_IRQ_COUNT 108

/* Describe the RAM layout */
#define CONFIG_RAM_BASE         0x10000
#define CONFIG_RAM_SIZE         0x8000

/* Flash chip specifics */
/* TODO(crosbug.com/p/33815): These are probably wrong. Don't use them yet. */
#define CONFIG_FLASH_BANK_SIZE      0x00000800  /* protect bank size */
#define CONFIG_FLASH_ERASE_SIZE     0x00000400  /* erase bank size */
#define CONFIG_FLASH_WRITE_SIZE     0x00000004  /* minimum write size */

/* Describe the flash layout */
#define CONFIG_FLASH_BASE       0x40000
/* TODO(wfrichar): Lying about this, so image signing works.
 * I'll file a bug once this CL goes in. */
/* #define CONFIG_FLASH_PHYSICAL_SIZE (512 * 1024) */
#define CONFIG_FLASH_PHYSICAL_SIZE (256 * 1024)

/* Size of one firmware image in flash */
#define CONFIG_FW_IMAGE_SIZE    (128 * 1024)
/* Compute the rest of the flash params from these */
#include "config_std_flash.h"

/* Interval between HOOK_TICK notifications */
#define HOOK_TICK_INTERVAL_MS 500
#define HOOK_TICK_INTERVAL    (HOOK_TICK_INTERVAL_MS * MSEC)

/* System stack size */
#define CONFIG_STACK_SIZE 1024

/* Idle task stack size */
#define IDLE_TASK_STACK_SIZE 256

/* Default task stack size */
#define TASK_STACK_SIZE 488

/* Larger task stack size, for hook task */
#define LARGER_TASK_STACK_SIZE 640

/* Maximum number of deferrable functions */
#define DEFERRABLE_MAX_COUNT 8

/* USB : TODO FIXME */
#define CONFIG_USB_RAM_ACCESS_TYPE uint16_t
/* No dedicated USB RAM */
#define CONFIG_USB_RAM_BASE 0xdead0000
#define CONFIG_USB_RAM_ACCESS_SIZE 0
#define CONFIG_USB_RAM_SIZE 0

#endif /* __CROS_EC_CONFIG_CHIP_H */
