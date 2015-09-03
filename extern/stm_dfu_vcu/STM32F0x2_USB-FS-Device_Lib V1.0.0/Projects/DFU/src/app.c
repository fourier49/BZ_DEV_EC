/**
  ******************************************************************************
  * @file    app.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    31-January-2014
  * @brief   This file provides all the Application firmware functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/ 
#include <stdio.h>
#include "usbd_dfu_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"

/* RW image main funtion link table in vedio chip update(VCU) defintion
// In vedio chip firmware, link table data structure is below:
struct VCU_Link_Table {
	uint64_t LinktlbMagicNum;  // Link table Head token
	uint32_t VCUsize;          // VCU FW size
	uint32_t VCUFWVer;         // VCU FW Version
};

const struct VCU_Link_Table  testlink __attribute__((at(VCU_IMAGE_START_ADDRESS+LINK_TABLE_OFFSET)))=
{
	0x1357246809ABCDEF,	// Link table Head token magic word
	0x0000B000,
	0x00000001
};
*/

struct VCU_Link_Table {
	uint64_t LinktlbMagicNum;  // Link table Head token
	uint32_t VCUsize;          // VCU FW size
	uint32_t VCUFWVer;         // VCU FW Version
};

#define IMAGE_START_ADDRESS			0x08000000
#define IMAGE_TOTAL_SIZE				0x20000			// 128K
#define VCU_IMAGE_START_ADDRESS	0x08005000	// RO size : 20K
#define DEFAULT_RW_JMP_ADDRESS  0x08010000  // if no VCU table, Default RW Jump address : 0x08010000, size 64K.


#define LINK_MAGIC_NUMBER 			0x1357246809ABCDEF
#define LINK_TABLE_OFFSET				0x1000	// Offset 4K 
#define LINK_VCU_MAX_SIZE				0xB000 // Maximum size : 44K
	
/* Set VCU Link table phsycal address*/
const struct VCU_Link_Table* vcu_linktlb = (struct VCU_Link_Table *)(VCU_IMAGE_START_ADDRESS+LINK_TABLE_OFFSET);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern int check_enter_dfu(void);
// Add check vcu enter and rw signature verify region from VCU link table
extern int check_enter_vcu(void);
extern int check_rw_signature(uint32_t startAdd,uint32_t size);
extern int check_verify_signature(void);


USB_CORE_HANDLE  USB_Device_dev ;
pFunction Jump_To_Application;
uint32_t JumpAddress;

/* check whether dfu app is launched
	0 = initialized value
	1 = enter app mode
	2 = enter dfu mode
*/
extern volatile int get_status_req;
extern volatile  uint8_t leave_dfu;
/* Private function prototypes -----------------------------------------------*/
#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* Private functions ---------------------------------------------------------*/
/**
  * @brief Configure the USART Device
  * @param  None
  * @retval None
  */
void USART1_Config(void)
{ 
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	/* Enable GPIO clock */
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

	/* Enable USART clock */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); 

	/* Connect PXx to USARTx_Tx */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_1);

	/* Connect PXx to USARTx_Rx */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_1);
  
	/* Configure USART Tx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* Configure USART Rx as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	/* USART configuration */
	USART_Init(USART1, &USART_InitStructure);

	/* Enable USART */
	USART_Cmd(USART1, ENABLE);
}

#ifdef SUPPORT_KRONITECH
/**
  * @brief  Due to usb dp/dm on the kronitech is connected to hub then pass to mcu.
  *			We need to init the hub and power in advance.
  * @param  None
  * @retval None
  */
void Kronitech_HW_Init(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;

  /* Enable the GPIO for USB3.0 HUB Standby */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

  /* Output high for USB3.0 HUB Standby*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  GPIOA->ODR =  GPIO_Pin_7;
}

void Kronitech_HW_DeInit(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
 
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
  /* Config as input mode as default reset state*/
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_Init(GPIOA, &GPIO_InitStructure);
  //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, DISABLE);

}
#endif

// Jmp application by different region of VCU link table
void Jump_to_main_application(uint32_t App_startAdd)
{
	 if (((*(__IO uint32_t*)App_startAdd) & 0x2FFE0000 ) == 0x20000000)
    { /* Jump to user application */
      
		JumpAddress = *(__IO uint32_t*) (App_startAdd + 4);
    Jump_To_Application = (pFunction) JumpAddress;
		printf("\r\nJump to APP... %x\r\n", (unsigned int)Jump_To_Application);
    /* Initialize user application's Stack Pointer */
    __set_MSP(*(__IO uint32_t*) App_startAdd);
    Jump_To_Application();
		}
}

/**
  * @brief  Program entry point
  * @param  None
  * @retval None
  */
int main(void)
{
	int start_app = 1; //default will start application.
	// Add VCU and RW region check
	uint32_t 	VCU_size = 0;
	uint32_t 	verify_startAdd = VCU_IMAGE_START_ADDRESS;
	uint32_t 	verify_size = IMAGE_START_ADDRESS + IMAGE_TOTAL_SIZE - VCU_IMAGE_START_ADDRESS;
	
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f072.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f0xx.c file
      */  
  
  ///////////////////////////////////////////////////
  /* USART configuration */
  USART1_Config();
  printf("boot start... %s %s vid:0x%x pid:0x%x\n\r", __DATE__, __TIME__ , USBD_VID , USBD_PID);
  ///////////////////////////////////////////////////
	
#if 0
    /* Configure "DFU enter" button */
  STM_EVAL_PBInit(BUTTON_SEL, Mode_GPIO);
  
  // original switch between dfu/app mode via reset/user button, removed

  /* Check if the TAMPER push-button on STM32-EVAL Board is pressed */
  if (STM_EVAL_PBGetState(BUTTON_SEL) == 0x00)
  { /* Test if user code is programmed starting from address 0x8003000 */
    if (((*(__IO uint32_t*)APP_DEFAULT_ADD) & 0x2FFE0000 ) == 0x20000000)
    { /* Jump to user application */
      
      JumpAddress = *(__IO uint32_t*) (APP_DEFAULT_ADD + 4);
      Jump_To_Application = (pFunction) JumpAddress;
      /* Initialize user application's Stack Pointer */
      __set_MSP(*(__IO uint32_t*) APP_DEFAULT_ADD);
      Jump_To_Application();
    }
  } /* Otherwise enters DFU mode to allow user to program his application */
#endif
  
  /* The Application layer has only to call USBD_Init to 
  initialize the USB low level driver, the USB device library, the USB clock 
  ,pins and interrupt service routine (BSP) to start the Library*/
	// Avoid reinitial on RW, Initial USB should be Only in DFU mode
  /*USBD_Init(&USB_Device_dev,
            &USR_desc, 
            &DFU_cb, 
            &USR_cb);*/
  
  /* Setup SysTick Timer for 10 msec interrupts 
  This interrupt is used to display the current state of the DFU core */
#if 0 // removed, it causes continuous reset after jumping to user application (happened on hoho/ec.RW.bin)
  if (SysTick_Config(SystemCoreClock / 100))
  { 
    /* Capture error */ 
    while (1) {
		}
  }
#endif

	// VCU link table check :
	printf("LinktlbMagicNum = %llX, VCUsize = 0x%08X\r\n",vcu_linktlb->LinktlbMagicNum,vcu_linktlb->VCUsize);
	if( (vcu_linktlb->LinktlbMagicNum==LINK_MAGIC_NUMBER)&&(vcu_linktlb->VCUsize>LINK_TABLE_OFFSET)/*&&(vcu_linktlb->VCUsize<=LINK_VCU_MAX_SIZE)*/ )
	{
		printf("Link table found!\r\n");
		VCU_size = vcu_linktlb->VCUsize;
	}
	
	// Add backup domain initial here because Origin backup domain initial is in USBD_Init(change to DFU mode after dfu&vcu judgement)
	USBD_USR_Init();

  if (check_enter_dfu() == 0)
  {
		if ( check_enter_vcu()&& VCU_size ) // enter vcu and vcu table valid
		{
			verify_size = VCU_size;
			printf("Enter VCU\r\n");
		}
		else																// otherwise, enter RW
		{
			if(VCU_size==0)	// No VCU table, Jump to default RW address
			{
				verify_startAdd = DEFAULT_RW_JMP_ADDRESS;
				verify_size = IMAGE_START_ADDRESS + IMAGE_TOTAL_SIZE - DEFAULT_RW_JMP_ADDRESS;
			}
			else
			{
				verify_startAdd += VCU_size;
				verify_size -= VCU_size;
			}
			printf("Enter RW\r\n");
		}
		
		if(check_verify_signature())
		{
				//verify signature before jump to application.
				start_app = check_rw_signature(verify_startAdd,verify_size);
		}
		//if start to application
		if(start_app)
		{
			/*disable and clear usb interrupt*/
			USBD_DeInit(&USB_Device_dev); 
			USB_BSP_mDelay(100);
			/* Jump to user application */
			Jump_to_main_application(verify_startAdd);
		}
  }
	
	#ifdef SUPPORT_KRONITECH
		//enable Hub
		Kronitech_HW_Init();
	#endif

  // stay at boot loader
  printf("\r\nenter dfu mode\r\n");
	
	//Initial USB should be Only in DFU mode to avoid reinitial USB by RO and RW
	USBD_Init(&USB_Device_dev,
            &USR_desc, 
            &DFU_cb, 
            &USR_cb);
  
  while (!leave_dfu)
	{
		 USB_BSP_mDelay(10);
	}
	printf("\r\nleave dfu mode\r\n");
  USBD_DeInit(&USB_Device_dev); 
	USB_BSP_mDelay(100);
	/* reboot */
	NVIC_SystemReset();
} 


/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the USART */
  USART_SendData(USART1, (uint8_t) ch);

  /* Loop until transmit data register is empty */
  while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
  {}

  return ch;
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  assert_failed
  *         Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  File: pointer to the source file name
  * @param  Line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  
  /* Infinite loop */
  while (1)
  {}
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
