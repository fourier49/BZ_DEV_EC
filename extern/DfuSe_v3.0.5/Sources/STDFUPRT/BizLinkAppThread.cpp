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

#include "stdafx.h"
#include "STThread.h"
#include "../STDFU/STDFU.h"
#include "STDFUPRTINC.h"
#include "../STDFUFILES/STDFUFILESINC.h"
#include "DFUThread.h"
#include "BizLinkAppThread.h"
#include "image.h"

CBIZLINKAPPThread::CBIZLINKAPPThread(PDFUThreadContext pContext) : CDFUThread(pContext)
{

	if ((m_DfuDesc.bmAttributes & ATTR_ST_CAN_ACCELERATE) && (pContext->Operation != OPERATION_REBOOT))
	{
		pContext->wTransferSize = 4096;

		if (pContext->hImage)
		{
			CImage *pImage = (CImage*)pContext->hImage;
			DFUIMAGEELEMENT Element;
			DWORD i, NbEl = pImage->GetNbElements();
			m_NumBlockMax = 0;
			for (i = 0; i<NbEl; i++)
			{
				memset(&Element, 0, sizeof(Element));
				if (pImage->GetImageElement(i, &Element)) // To get datasize
				{
					m_NumBlockMax += (Element.dwDataLength / pContext->wTransferSize);
					if (Element.dwDataLength % pContext->wTransferSize != 0)
						m_NumBlockMax++;
				}
			}
		}

		m_Context.wTransferSize = pContext->wTransferSize;
	}
	else{
		pContext->ErrorCode = STDFU_NOERROR;
		m_Context.ErrorCode = STDFU_NOERROR;
		m_Context.RebootFlag = pContext->RebootFlag;
	}
}

CBIZLINKAPPThread::~CBIZLINKAPPThread()
{
}





BOOL CBIZLINKAPPThread::RunThread()
{
	DFUThreadContext Context;
	BOOL bRet = TRUE;
	BOOL bReDownload = FALSE;
	BOOL NeedsToChangeElement;
	DWORD dwPercentCalc;
	INT32 Length;

	// UploadState Machine
	m_PollTime = 0;

	GetCurrentContext(&Context);

	m_Retry = NB_TRIALS;
	Context.LastDFUStatus.bState = 0xFE;
	Context.CurrentRequest = STDFU_RQ_OPEN;
	SetCurrentContext(&Context);
	// We start our state machine. Let's open the handle
	if (STDFU_Open(Context.szDevLink, &Context.hDevice) != STDFU_NOERROR)
	{
		Context.ErrorCode = STDFU_OPENDRIVERERROR;
		bRet = FALSE; // Stops here
		SetCurrentContext(&Context);
	} 
	else
	{
		CString str_cmd;

		if (Context.Operation == OPERATION_REBOOT)
		{
			str_cmd = "bzcmd ";
			if (Context.RebootFlag & REBOOT_FLAG_SET_DFU)
			{
				//SendData[13] |= REBOOT_FLAG_SET_DFU;
				str_cmd += "dfu_on ";
			}
			
			if (Context.RebootFlag & REBOOT_FLAG_SET_SIGVERIFY)
			{
				str_cmd += "scheck_off";
			}
			else if (Context.RebootFlag & REBOOT_FLAG_CLEAR_SIGVERIFY)
			{
				str_cmd += "scheck_on";
			}
			str_cmd += "\n";
			char sz[128];
			//sprintf(sz, "%S", str_cmd.GetBuffer());
			//sprintf(sz, "%S", (LPCSTR) str_cmd);
			strcpy(sz, (LPCTSTR)str_cmd);
			
			STDFU_AUX_WritePipe(&Context.hDevice, UsbConsoleAppWritePipe, (PBYTE)sz, strlen(sz), &Length);
			//EnsureIdleMode(&Context);
			Context.Percent = 100;
			SetCurrentContext(&Context);
			STDFU_AUX_FreeWinUSB(&Context.hDevice);
			bRet = FALSE;
		}
#if(0)
		else if (Context.Operation == OPERATION_UPBLDRR)
		{

				Context.LastDFUStatus.bState = 0xFE;
				Context.CurrentRequest = STDFU_RQ_OPEN;
				SetCurrentContext(&Context);
				// We start our state machine. Let's open the handle
				if (STDFU_Open(Context.szDevLink, &Context.hDevice) != STDFU_NOERROR)
				{
					Context.ErrorCode = STDFU_OPENDRIVERERROR;
					bRet = FALSE; // Stops here
					SetCurrentContext(&Context);
				}
				else
				{
					if (STDFU_AUX_SetSigCheckFlag(&Context.hDevice,m_DfuInterfaceIdx,0) != STDFU_NOERROR)
					{
						Context.ErrorCode = STDFUPRT_UNEXPECTEDERROR;
						SetCurrentContext(&Context);
						STDFU_Abort(&Context.hDevice); // Reset State machine
					}
					bRet = FALSE; // Stops here
				}
		}
#endif

	}
	return bRet;
}


