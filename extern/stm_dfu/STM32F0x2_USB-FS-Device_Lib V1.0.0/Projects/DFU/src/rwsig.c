/* Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/*
 * Implementation of the RW firmware signature verification and jump.
 */

//#include "console.h"
//#include "ec_commands.h"

#include "rsa.h"
#include "sha256.h"
//#include "shared_mem.h"
//#include "system.h"
//#include "usb_pd.h"
//#include "util.h"


/* Private function prototypes -----------------------------------------------*/
#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

	
/* Console output macros */
#define CPRINTF(format, args...) printf(format, ## args)
#define CPRINTS(format, args...) printf(format, ## args)


/* Insert the RSA public key definition */
const struct rsa_public_key pkey __attribute__((section(".rsa_pubkey.__at_0x0800F000")))=
#include "gen_pub_key.h"


 //defined in /chip/stm/config-stm32f07x.h	
#define CONFIG_FLASH_BASE       0x08000000
#define CONFIG_FLASH_PHYSICAL_SIZE (128 * 1024)
#define CONFIG_FLASH_SIZE       CONFIG_FLASH_PHYSICAL_SIZE
#define CONFIG_FLASH_BANK_SIZE  0x1000
#define CONFIG_FLASH_ERASE_SIZE 0x0800  /* erase bank size */
#define CONFIG_FLASH_WRITE_SIZE 0x0002  /* minimum write size */
#define CONFIG_FLASH_BASE       0x08000000  
#define CONFIG_FW_IMAGE_SIZE    (CONFIG_FLASH_SIZE / 2)
#define CONFIG_FW_RO_OFF        0
#define CONFIG_FW_RO_SIZE       (CONFIG_FW_IMAGE_SIZE - CONFIG_FW_PSTATE_SIZE)
#define CONFIG_FW_RW_OFF        (CONFIG_FW_RO_OFF + CONFIG_FW_IMAGE_SIZE)
#define CONFIG_FW_RW_SIZE       (CONFIG_FLASH_SIZE - CONFIG_FW_IMAGE_SIZE)

	
/* The RSA signature is stored at the end of the RW firmware */
static const void *rw_sig = (void *) (CONFIG_FLASH_BASE + CONFIG_FW_RW_OFF + CONFIG_FW_RW_SIZE - RSANUMBYTES);

/* RW firmware reset vector */
static uint32_t * const rw_rst =
	(uint32_t *)(CONFIG_FLASH_BASE+CONFIG_FW_RW_OFF+4);

int check_rw_signature(void);

uint8_t g_u8_rsa_workbuf[3 * RSANUMBYTES];

int check_rw_signature(void)
{
	struct sha256_ctx ctx;
	int good;
	uint8_t *hash;
	uint32_t *rsa_workbuf = (uint32_t*)g_u8_rsa_workbuf;

	/* Check if we have a RW firmware flashed */
	if (*rw_rst == 0xffffffff)
		return 0;

	CPRINTF("Verifying RW image...");
	/* SHA-256 Hash of the RW firmware */
	SHA256_init(&ctx);
	SHA256_update(&ctx, (void *)(CONFIG_FLASH_BASE + CONFIG_FW_RW_OFF ),
		      CONFIG_FW_RW_SIZE - RSANUMBYTES);
	hash = SHA256_final(&ctx);

	good = rsa_verify(&pkey, (void *)rw_sig, (void *)hash, rsa_workbuf);
	if (good) {
		CPRINTF("RW image verified");
	} else {
		CPRINTS("RSA verify FAILED");
		//pd_log_event(PD_EVENT_ACC_RW_FAIL, 0, 0, NULL);
	}
	return good;
}
