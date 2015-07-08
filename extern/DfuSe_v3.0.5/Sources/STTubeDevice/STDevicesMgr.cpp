#include "stdafx.h"
#include "usb100.h"
#include "STTubeDeviceErr30.h"
#include "STTubeDeviceTyp30.h"
#include "STDevice.h"
#include "STDevicesMgr.h"
#include "BizlinkUSBDevice.h"

//Bizlink
#define DEVICE_ID "VID_06C4"

#define STM_DEVICE_ID "VID_0483"
//#define HP_DEVICE_ID "VID_03F0"

CSTDevicesManager::CSTDevicesManager()
{
}

CSTDevicesManager::~CSTDevicesManager()
{
	int i;
	
	// Close all still-open device objects
	if (m_OpenDevices.GetSize())
	{
		CSTDevice *pDevice;

		for (i=0;i<m_OpenDevices.GetSize();i++)
		{
			pDevice=(CSTDevice*)(m_OpenDevices[i]);
			if (pDevice)
				delete pDevice;
		}
	}
}


DWORD CSTDevicesManager::IsBIZLINKDevice(HANDLE hDevice, PBYTE pResult)
{
	DWORD nRet = STDEVICE_MEMORY;
	CObject* pObj = (CObject *)hDevice;

	CRuntimeClass* prt = RUNTIME_CLASS(CBIZLinkDevice);

	if (hDevice == NULL)
	{
		*pResult = 0;
		return STDEVICE_BADPARAMETER;
	}

	if (m_OpenDevices.GetCount() == 0)
	{
		*pResult = 0;
		return STDEVICE_BADPARAMETER;
	}

	if (pObj->IsKindOf(prt))
	{
		*pResult = 1;
	}
	else
	{
		*pResult = 0;
	}
	
	return nRet;
}

DWORD CSTDevicesManager::Open(CString sSymbName, 
								LPHANDLE phDevice, 
								LPHANDLE phUnPlugEvent)
{
	CSTDevice *pDevice=NULL;
	CBIZLinkDevice *pBizDevice = NULL;
	DWORD nRet=STDEVICE_MEMORY;

	if (!phDevice)
		return STDEVICE_BADPARAMETER;

	// Tries to create an object
	//if ((sSymbName.Find(DEVICE_ID) > 0))
	if((sSymbName.Find(STM_DEVICE_ID) < 0))
	{
		if (m_OpenDevices.GetCount() == 0)
		{
			pBizDevice = new CBIZLinkDevice(sSymbName);

			if (!pBizDevice)
				return STDEVICE_BADPARAMETER;

			// Tries to open the device
			nRet = pBizDevice->Open(phUnPlugEvent);
			if (nRet != STDEVICE_NOERROR)
			{
				delete pBizDevice;
				pBizDevice = NULL;
				return nRet;
			}

			// OK our Bizlink device object was successfully created. Let's add it to our collection
			m_OpenDevices.Add(pBizDevice);
			*phDevice = (HANDLE)pBizDevice;
		}else
		{
			*phDevice = (HANDLE)m_OpenDevices[0];
		}
		
	}
	else
	{
	pDevice=new CSTDevice(sSymbName);

	if (!pDevice)
		return STDEVICE_BADPARAMETER;

	// Tries to open the device
	nRet=pDevice->Open(phUnPlugEvent);
	if (nRet!=STDEVICE_NOERROR)
	{
		delete pDevice;
		pDevice=NULL;
		return nRet;
	}

	// OK our STDevice object was successfully created. Let's add it to our collection
	m_OpenDevices.Add(pDevice);

	*phDevice=(HANDLE)pDevice;
	}

	return STDEVICE_NOERROR;
}

DWORD CSTDevicesManager::Close(HANDLE hDevice)
{
	
	DWORD nRet=STDEVICE_NOERROR;
	int i;
	CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
	CSTDevice * pDevice = (CSTDevice *)hDevice;
	BYTE isBizlink = 0;
	

	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			if (isBizlink)
			{
				pBizDevice->Close();
				delete pBizDevice;
				pBizDevice = NULL;
			}
			else
			{
			pDevice->Close();
			delete pDevice;
			pDevice=NULL;
			}

			m_OpenDevices.RemoveAt(i);
			nRet=STDEVICE_NOERROR;
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::OpenPipes(HANDLE hDevice)
{
	DWORD nRet=STDEVICE_NOERROR;
	int i;


	BYTE isBizlink = 0;

	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->OpenPipes();
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->OpenPipes();
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::ClosePipes(HANDLE hDevice)
{
	DWORD nRet=STDEVICE_NOERROR;
	int i;

	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->ClosePipes();

			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->ClosePipes();
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->ClosePipes();
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::GetStringDescriptor( HANDLE hDevice, 
						   UINT nIndex, 
						   CString &sString)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;

	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->GetStringDescriptor(nIndex, sString);

			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->GetStringDescriptor(nIndex, sString);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->GetStringDescriptor(nIndex, sString);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::GetDeviceDescriptor( HANDLE hDevice, 
						   PUSB_DEVICE_DESCRIPTOR pDesc)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;


	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->GetDeviceDescriptor(pDesc);
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->GetDeviceDescriptor(pDesc);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->GetDeviceDescriptor(pDesc);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::GetNbOfConfigurations(HANDLE hDevice, PUINT pNbOfConfigs)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);


	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->GetNbOfConfigurations(pNbOfConfigs);
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->GetNbOfConfigurations(pNbOfConfigs);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->GetNbOfConfigurations(pNbOfConfigs);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::GetConfigurationDescriptor( HANDLE hDevice, 
								  UINT nConfigIdx, 
								  PUSB_CONFIGURATION_DESCRIPTOR pDesc)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->GetConfigurationDescriptor(nConfigIdx, pDesc);
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->GetConfigurationDescriptor(nConfigIdx, pDesc);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->GetConfigurationDescriptor(nConfigIdx, pDesc);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::GetNbOfInterfaces(HANDLE hDevice, 
						UINT nConfigIdx, 
						PUINT pNbOfInterfaces)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->GetNbOfInterfaces(nConfigIdx, pNbOfInterfaces);
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->GetNbOfInterfaces(nConfigIdx, pNbOfInterfaces);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->GetNbOfInterfaces(nConfigIdx, pNbOfInterfaces);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::GetNbOfAlternates(HANDLE hDevice, 
						UINT nConfigIdx, 
						UINT nInterfaceIdx, 
						PUINT pNbOfAltSets)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;

	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->GetNbOfAlternates(nConfigIdx, nInterfaceIdx, pNbOfAltSets);

			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->GetNbOfAlternates(nConfigIdx, nInterfaceIdx, pNbOfAltSets);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->GetNbOfAlternates(nConfigIdx, nInterfaceIdx, pNbOfAltSets);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::GetInterfaceDescriptor( HANDLE hDevice, 
							  UINT nConfigIdx, 
							  UINT nInterfaceIdx, 
							  UINT nAltSetIdx, 
							  PUSB_INTERFACE_DESCRIPTOR pDesc)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;

	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->GetInterfaceDescriptor(nConfigIdx, nInterfaceIdx, nAltSetIdx, pDesc);
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->GetInterfaceDescriptor(nConfigIdx, nInterfaceIdx, nAltSetIdx, pDesc);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->GetInterfaceDescriptor(nConfigIdx, nInterfaceIdx, nAltSetIdx, pDesc);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::GetNbOfEndPoints( HANDLE hDevice, 
						UINT nConfigIdx, 
						UINT nInterfaceIdx, 
						UINT nAltSetIdx, 
						PUINT pNbOfEndPoints)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);


	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->GetNbOfEndPoints(nConfigIdx, nInterfaceIdx, nAltSetIdx, pNbOfEndPoints);
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->GetNbOfEndPoints(nConfigIdx, nInterfaceIdx, nAltSetIdx, pNbOfEndPoints);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->GetNbOfEndPoints(nConfigIdx, nInterfaceIdx, nAltSetIdx, pNbOfEndPoints);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::GetEndPointDescriptor( HANDLE hDevice, 
							 UINT nConfigIdx, 
							 UINT nInterfaceIdx, 
							 UINT nAltSetIdx, 
							 UINT nEndPointIdx, 
							 PUSB_ENDPOINT_DESCRIPTOR pDesc)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;

	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->GetEndPointDescriptor(nConfigIdx, nInterfaceIdx, nAltSetIdx, nEndPointIdx, pDesc);

			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->GetEndPointDescriptor(nConfigIdx, nInterfaceIdx, nAltSetIdx, nEndPointIdx, pDesc);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->GetEndPointDescriptor(nConfigIdx, nInterfaceIdx, nAltSetIdx, nEndPointIdx, pDesc);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::GetNbOfDescriptors(HANDLE hDevice, 
											BYTE nLevel,
											BYTE nType,
											UINT nConfigIdx, 
											UINT nInterfaceIdx, 
											UINT nAltSetIdx, 
											UINT nEndPointIdx, 
											PUINT pNbOfDescriptors)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;

	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->GetNbOfDescriptors(nLevel, nType, nConfigIdx, nInterfaceIdx, nAltSetIdx, nEndPointIdx, pNbOfDescriptors);
			
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->GetNbOfDescriptors(nLevel, nType, nConfigIdx, nInterfaceIdx, nAltSetIdx, nEndPointIdx, pNbOfDescriptors);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
				nRet = pDevice->GetNbOfDescriptors(nLevel, nType, nConfigIdx, nInterfaceIdx, nAltSetIdx, nEndPointIdx, pNbOfDescriptors);
			}

			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::GetDescriptor(HANDLE hDevice, 
										BYTE nLevel,
										BYTE nType,
										UINT nConfigIdx, 
										UINT nInterfaceIdx, 
										UINT nAltSetIdx, 
										UINT nEndPointIdx, 
										UINT nIdx,
										PBYTE pDesc,
										UINT nDescSize)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);


	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->GetDescriptor(nLevel, nType, nConfigIdx, nInterfaceIdx, nAltSetIdx, nEndPointIdx, nIdx, pDesc, nDescSize);
			
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->GetDescriptor(nLevel, nType, nConfigIdx, nInterfaceIdx, nAltSetIdx, nEndPointIdx, nIdx, pDesc, nDescSize);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
				nRet = pDevice->GetDescriptor(nLevel, nType, nConfigIdx, nInterfaceIdx, nAltSetIdx, nEndPointIdx, nIdx, pDesc, nDescSize);
			}

			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::SelectCurrentConfiguration( HANDLE hDevice, 
								  UINT nConfigIdx, 
								  UINT nInterfaceIdx, 
								  UINT nAltSetIdx)

{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;

	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->SelectCurrentConfiguration(nConfigIdx, nInterfaceIdx, nAltSetIdx);

			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->SelectCurrentConfiguration(nConfigIdx, nInterfaceIdx, nAltSetIdx);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->SelectCurrentConfiguration(nConfigIdx, nInterfaceIdx, nAltSetIdx);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::SetDefaultTimeOut(HANDLE hDevice, DWORD nTimeOut)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;

	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->SetDefaultTimeOut(nTimeOut);

			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->SetDefaultTimeOut(nTimeOut);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->SetDefaultTimeOut(nTimeOut);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::SetMaxNumInterruptInputBuffer(HANDLE hDevice, WORD nMaxNumInputBuffer)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);


	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->SetMaxNumInterruptInputBuffer(nMaxNumInputBuffer);

			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->SetMaxNumInterruptInputBuffer(nMaxNumInputBuffer);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
				nRet = pDevice->SetMaxNumInterruptInputBuffer(nMaxNumInputBuffer);
			}

			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::GetMaxNumInterruptInputBuffer(HANDLE hDevice, PWORD pMaxNumInputBuffer)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->GetMaxNumInterruptInputBuffer(pMaxNumInputBuffer);
			
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->GetMaxNumInterruptInputBuffer(pMaxNumInputBuffer);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
				nRet = pDevice->GetMaxNumInterruptInputBuffer(pMaxNumInputBuffer);
			}

			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::SetSuspendModeBehaviour(HANDLE hDevice, BOOL Allow)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->SetSuspendModeBehaviour(Allow);

			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->SetSuspendModeBehaviour(Allow);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->SetSuspendModeBehaviour(Allow);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::EndPointControl( HANDLE hDevice, 
					   UINT nEndPointIdx, 
					   UINT nOperation)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->EndPointControl(nEndPointIdx, nOperation);

			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->EndPointControl(nEndPointIdx, nOperation);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
				nRet = pDevice->EndPointControl(nEndPointIdx, nOperation);
			}

			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::Reset(HANDLE hDevice)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->Reset();
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->Reset();
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->Reset();
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::ControlPipeRequest(HANDLE hDevice, PCNTRPIPE_RQ pRequest, PBYTE pData)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->ControlPipeRequest(pRequest, pData);
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->ControlPipeRequest(pRequest, pData);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->ControlPipeRequest(pRequest, pData);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::Read(  HANDLE hDevice, 
							    UINT nEndPointIdx,
								PBYTE pBuffer, 
								PUINT pSize, 
								DWORD nTimeOut)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->Read(nEndPointIdx, pBuffer, pSize, nTimeOut);
			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->Read(nEndPointIdx, pBuffer, pSize, nTimeOut);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->Read(nEndPointIdx, pBuffer, pSize, nTimeOut);
			}
			break;
		}
	}

	return nRet;
}

DWORD CSTDevicesManager::Write(HANDLE hDevice, 
							    UINT nEndPointIdx,
								PBYTE pBuffer, 
								PUINT pSize, 
								DWORD nTimeOut)
{
	int i;
	DWORD nRet=STDEVICE_BADPARAMETER;
	BYTE isBizlink = 0;
	IsBIZLINKDevice(hDevice, &isBizlink);

	for (i=0;i<m_OpenDevices.GetSize();i++)
	{
		if (hDevice==(HANDLE)m_OpenDevices[i])
		{
			//CSTDevice *pDevice=(CSTDevice *)hDevice;

			//nRet=pDevice->Write(nEndPointIdx, pBuffer, pSize, nTimeOut);

			if (isBizlink)
			{
				CBIZLinkDevice * pBizDevice = (CBIZLinkDevice *)hDevice;
				nRet = pBizDevice->Write(nEndPointIdx, pBuffer, pSize, nTimeOut);
			}
			else{
			CSTDevice *pDevice=(CSTDevice *)hDevice;
			nRet=pDevice->Write(nEndPointIdx, pBuffer, pSize, nTimeOut);
			}
			break;
		}
	}

	return nRet;
}
