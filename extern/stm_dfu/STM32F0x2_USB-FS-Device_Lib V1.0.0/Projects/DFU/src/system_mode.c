#include <stdio.h>
#include "stm32f0xx.h"
#include "stm32072b_eval.h"


/* Private function prototypes -----------------------------------------------*/
#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

#ifndef IRAM_BEGIN
#define IRAM_BEGIN	0x20000000
#endif
	
#ifndef IRAM_SIZE	
#define IRAM_SIZE		0x00004000
#endif

#define SYSTEM_RESET_ENTER_DFU   (1<<14)
#define SYSTEM_SKIP_VERIFY_SIG	 (1<<15)
#define RESET_FLAG_HARD        	 (1 << 11)  /* Hard reset from software */
	
/* Round up to a multiple of 4 */
#define ROUNDUP4(x) (((x) + 3) & ~3)


enum bkpdata_index {
	BKPDATA_INDEX_SCRATCHPAD,	     /* General-purpose scratchpad */
	BKPDATA_INDEX_SAVED_RESET_FLAGS,     /* Saved reset flags */
	BKPDATA_INDEX_VBNV_CONTEXT0,
	BKPDATA_INDEX_VBNV_CONTEXT1,
	BKPDATA_INDEX_VBNV_CONTEXT2,
	BKPDATA_INDEX_VBNV_CONTEXT3,
	BKPDATA_INDEX_VBNV_CONTEXT4,
	BKPDATA_INDEX_VBNV_CONTEXT5,
	BKPDATA_INDEX_VBNV_CONTEXT6,
	BKPDATA_INDEX_VBNV_CONTEXT7,
#ifdef CONFIG_SOFTWARE_PANIC
	BKPDATA_INDEX_SAVED_PANIC_REASON,    /* Saved panic reason */
	BKPDATA_INDEX_SAVED_PANIC_INFO,      /* Saved panic data */
	BKPDATA_INDEX_SAVED_PANIC_EXCEPTION, /* Saved panic exception code */
#endif
	BKPDATA_INDEX_MAX
};


/* Data for an individual jump tag */
struct jump_tag {
	uint16_t tag;		/* Tag ID */
	uint8_t data_size;	/* Size of data which follows */
	uint8_t data_version;	/* Data version */

	/* Followed by data_size bytes of data */
};

/*
 * Data passed between the current image and the next one when jumping between
 * images.
 */
#define JUMP_DATA_MAGIC 0x706d754a  /* "Jump" */
#define JUMP_DATA_VERSION 3
#define JUMP_DATA_SIZE_V2 16  /* Size of version 2 jump data struct */
struct jump_data {
	/*
	 * Add new fields to the _start_ of the struct, since we copy it to the
	 * _end_ of RAM between images.  This way, the magic number will always
	 * be the last word in RAM regardless of how many fields are added.
	 */

	/* Fields from version 3 */
	uint8_t reserved0;    /* (used in proto1 to signal recovery mode) */
	int struct_size;      /* Size of struct jump_data */

	/* Fields from version 2 */
	int jump_tag_total;   /* Total size of all jump tags */

	/* Fields from version 1 */
	uint32_t reset_flags; /* Reset flags from the previous boot */
	int version;          /* Version (JUMP_DATA_VERSION) */
	int magic;            /* Magic number (JUMP_DATA_MAGIC).  If this
			       * doesn't match at pre-init time, assume no valid
			       * data from the previous image. */
};

/* Jump data (at end of RAM, or preceding panic data) */
//static struct jump_data *jdata;

#define STM32_BKP_ENTRIES 20


int check_enter_dfu(void);
int check_verify_signature(void);


/**
 * Read backup register at specified index.
 *
 * @return The value of the register or 0 if invalid index.
 */
static uint16_t bkpdata_read(enum bkpdata_index index)
{
	//if (index < 0 || index >= BKPDATA_INDEX_MAX)
	if (index >= BKPDATA_INDEX_MAX)
		return 0;

	if (index & 1)
		return RTC_ReadBackupRegister(index >> 1) >> 16;
	else
		return RTC_ReadBackupRegister(index >> 1) & 0xFFFF;
}
/**
	* @brief  write backup register at specified index.
  * @param  None
  * @retval None
*/
static void bkpdata_write(enum bkpdata_index index , uint16_t data)
{
	static uint32_t val = 0;
	
	if (index >= BKPDATA_INDEX_MAX)
		return ;

	val = RTC_ReadBackupRegister(index >> 1);
	
	if (index & 1) //high short
	{
			val = (val&0xFFFF)|(data<<16);
	}
	else{ //low short
			val = (val&0xFFFF0000)|data;
	}
	RTC_WriteBackupRegister(index >> 1 , val);
}
/**
	* @brief  Check if enter DFU mode.
  * @param  None
  * @retval None
*/

int check_enter_dfu(void)
{
	int ret = 0;
	uint16_t reset_flag = 0;
	reset_flag = bkpdata_read(BKPDATA_INDEX_SAVED_RESET_FLAGS);
	
  printf("check_enter_dfu... flag:0x%x \n\r",reset_flag);

	if(reset_flag & SYSTEM_RESET_ENTER_DFU)
	{
		ret = 1;
		if(reset_flag & RESET_FLAG_HARD)
		{
				reset_flag = reset_flag & ~(SYSTEM_RESET_ENTER_DFU|RESET_FLAG_HARD);
		}else
		{
				reset_flag = reset_flag & ~SYSTEM_RESET_ENTER_DFU;
		}
		//update reset flag in backup data register.
		bkpdata_write(BKPDATA_INDEX_SAVED_RESET_FLAGS,reset_flag);
	}
	return ret;
}

/**
	* @brief  Check if need to verify signature.
  * @param  None
  * @retval None
*/
int check_verify_signature(void)
{
	int ret = 1;
	uint16_t reset_flag = 0;
	reset_flag = bkpdata_read(BKPDATA_INDEX_SAVED_RESET_FLAGS);
  printf("check_verify_signature... flag:0x%x \n\r",reset_flag);
	if(reset_flag & SYSTEM_SKIP_VERIFY_SIG)
	{
		ret = 0;
	}
	return ret;
}

/**
	* @brief  Check if need to verify signature.
  * @param  None
  * @retval None
*/
int check_set_check_signature_flag(int bCheck)
{
	int ret = 1;
	uint16_t reset_flag = 0;
	reset_flag = bkpdata_read(BKPDATA_INDEX_SAVED_RESET_FLAGS);
  printf("check_set_check_signature_flag... flag:0x%x \n\r",reset_flag);
	if(bCheck)
	{
		reset_flag &= ~SYSTEM_SKIP_VERIFY_SIG;
	}else
	{
		reset_flag |= SYSTEM_SKIP_VERIFY_SIG;
	}
	bkpdata_write(BKPDATA_INDEX_SAVED_RESET_FLAGS,reset_flag);
	return ret;
}
