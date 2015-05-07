/* Copyright 2015 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "common.h"
#include "console.h"
#include "flash.h"
#include "host_command.h"
#include "shared_mem.h"
#include "spi.h"
#include "spi_flash.h"
#include "system.h"
#include "util.h"
#include "watchdog.h"

#define PAGE_SIZE 256

/**
 * Read from physical flash.
 *
 * @param offset        Flash offset to write.
 * @param size          Number of bytes to write.
 * @param data          Destination buffer for data.  Must be 32-bit aligned.
 */
int flash_physical_read(int offset, int size, char *data)
{
	int ret;

	offset += CONFIG_FLASH_BASE_SPI;

	/* Fail if offset, size, and data aren't at least word-aligned */
	if ((offset | size | (uint32_t)(uintptr_t)data) & 3)
		return EC_ERROR_INVAL;

	spi_enable(1);
	ret = spi_flash_read((uint8_t *)data, offset, size);
	spi_enable(0);
	return ret;
}

/**
 * Write to physical flash.
 *
 * Offset and size must be a multiple of CONFIG_FLASH_WRITE_SIZE.
 *
 * @param offset        Flash offset to write.
 * @param size          Number of bytes to write.
 * @param data          Data to write to flash.  Must be 32-bit aligned.
 */
int flash_physical_write(int offset, int size, const char *data)
{
	int ret, i, write_size;

	offset += CONFIG_FLASH_BASE_SPI;

	/* Fail if offset, size, and data aren't at least word-aligned */
	if ((offset | size | (uint32_t)(uintptr_t)data) & 3)
		return EC_ERROR_INVAL;

	spi_enable(1);
	for (i = 0; i < size; i += write_size) {
		write_size = MIN(size, SPI_FLASH_MAX_WRITE_SIZE);
		ret = spi_flash_write(offset + i,
				      write_size,
				      (uint8_t *)data + i);
		if (ret != EC_SUCCESS)
			break;
		/* BUG: Multi-page writes fail if no delay */
		msleep(1);
	}
	spi_enable(0);
	return ret;
}

/**
 * Erase physical flash.
 *
 * Offset and size must be a multiple of CONFIG_FLASH_ERASE_SIZE.
 *
 * @param offset        Flash offset to erase.
 * @param size          Number of bytes to erase.
 */
int flash_physical_erase(int offset, int size)
{
	int ret;

	offset += CONFIG_FLASH_BASE_SPI;
	spi_enable(1);
	ret = spi_flash_erase(offset, size);
	spi_enable(0);
	return ret;
}

/**
 * Read physical write protect setting for a flash bank.
 *
 * @param bank    Bank index to check.
 * @return        non-zero if bank is protected until reboot.
 */
int flash_physical_get_protect(int bank)
{
	uint32_t addr = bank * CONFIG_FLASH_BANK_SIZE;
	int ret;

	spi_enable(1);
	ret = spi_flash_check_protect(addr, CONFIG_FLASH_BANK_SIZE);
	spi_enable(0);
	return ret;
}

/**
 * Protect flash now.
 *
 * @param all      Protect all (=1) or just read-only and pstate (=0).
 * @return         non-zero if error.
 */
int flash_physical_protect_now(int all)
{
	int offset, size, ret;

	if (all) {
		offset = 0;
		size = CONFIG_FLASH_PHYSICAL_SIZE;
	} else {
		offset = CONFIG_FW_RO_OFF;
		size = CONFIG_FW_RO_SIZE;
	}

	spi_enable(1);
	ret = spi_flash_set_protect(offset, size);
	spi_enable(0);
	return ret;
}

/**
 * Return flash protect state flags from the physical layer.
 *
 * This should only be called by flash_get_protect().
 *
 * Uses the EC_FLASH_PROTECT_* flags from ec_commands.h
 */
uint32_t flash_physical_get_protect_flags(void)
{
	uint32_t flags = 0;

	spi_enable(1);
	if (spi_flash_check_protect(CONFIG_FW_RO_OFF, CONFIG_FW_RO_SIZE)) {
		flags |= EC_FLASH_PROTECT_RO_AT_BOOT | EC_FLASH_PROTECT_RO_NOW;
		if (spi_flash_check_protect(CONFIG_FW_RW_OFF,
					    CONFIG_FW_RW_SIZE))
			flags |= EC_FLASH_PROTECT_ALL_NOW;
	}
	spi_enable(0);
	return flags;
}

/**
 * Return the valid flash protect flags.
 *
 * @return   A combination of EC_FLASH_PROTECT_* flags from ec_commands.h
 */
uint32_t flash_physical_get_valid_flags(void)
{
	return EC_FLASH_PROTECT_RO_AT_BOOT |
	       EC_FLASH_PROTECT_RO_NOW |
	       EC_FLASH_PROTECT_ALL_NOW;
}

/**
 * Return the writable flash protect flags.
 *
 * @param    cur_flags The current flash protect flags.
 * @return   A combination of EC_FLASH_PROTECT_* flags from ec_commands.h
 */
uint32_t flash_physical_get_writable_flags(uint32_t cur_flags)
{
	uint32_t ret = 0;
	enum spi_flash_wp wp_status = spi_flash_check_wp();

	if (wp_status == SPI_WP_NONE || (wp_status == SPI_WP_HARDWARE &&
	   !(cur_flags & EC_FLASH_PROTECT_GPIO_ASSERTED)))
		ret = EC_FLASH_PROTECT_RO_AT_BOOT | EC_FLASH_PROTECT_RO_NOW |
		      EC_FLASH_PROTECT_ALL_NOW;

	return ret;
}

/**
 * Enable write protect for the specified range.
 *
 * Once write protect is enabled, it will STAY enabled until the system is
 * hard-rebooted with the hardware write protect pin deasserted.  If the write
 * protect pin is deasserted, the protect setting is ignored, and the entire
 * flash will be writable.
 *
 * @param range         The range to protect.
 * @return              EC_SUCCESS, or nonzero if error.
 */
int flash_physical_protect_at_boot(enum flash_wp_range range)
{
	int offset, size, ret;

	switch (range) {
	case FLASH_WP_NONE:
		offset = size = 0;
		break;
	case FLASH_WP_RO:
		offset = CONFIG_FW_RO_OFF;
		size = CONFIG_FW_RO_SIZE;
		break;
	case FLASH_WP_ALL:
		offset = 0;
		size = CONFIG_FLASH_PHYSICAL_SIZE;
		break;
	}

	spi_enable(1);
	ret = spi_flash_set_protect(offset, size);
	spi_enable(0);
	return ret;
}

/**
 * Initialize the module.
 *
 * Applies at-boot protection settings if necessary.
 */
int flash_pre_init(void)
{
	return EC_SUCCESS;
}
