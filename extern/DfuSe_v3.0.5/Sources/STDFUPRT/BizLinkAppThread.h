/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* Company            : STMicroelectronics
* Author             : MCD Application Team
* Description        : STMicroelectronics Device Firmware Upgrade Extension Demo
* Version            : V3.0.2
* Date               : 09-May-2011
********************************************************************************
* THE PRESENT SOFTWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
********************************************************************************
* FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE
* "MCD-ST Liberty SW License Agreement V2.pdf"
*******************************************************************************/

#ifndef _BIZLINK_APP_THREAD_H_
#define _BIZLINK_APP_THREAD_H_


class CBIZLINKAPPThread : public CDFUThread  {
public:
	CBIZLINKAPPThread(PDFUThreadContext pContext);
	virtual ~CBIZLINKAPPThread();
private:
	virtual	BOOL	RunThread();
	//issue request to enter DFU mode.
	BOOL			EnterDfuMode(PDFUThreadContext pContext);
	//issue request to set FW Signature check flag
	BOOL			SetCheckFWSigFlag(PDFUThreadContext pContext);
	//    CEvent				m_EventWaitPoll;
};
#endif