/*
 * File: sterror.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stheader.h"
#include "sterror.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
TCHAR* g_ERROR_STR[]=
{
	_T("Success"),
	_T("No Memory"),
	_T("Invalid pos in array"),
	_T("Invalid request"),
	_T("Invalid Driver Type"),
	_T("Funciton not supported"),
	_T("Data incomplete"),
	_T("Array uninitialized"),
	_T("Invalid media info request"),
	_T("Invalid driver info request"),
	_T("Put data size exceeds array size"),
	_T("Failed to open file"),
	_T("Failed to read file data"),
	_T("Failed to write file data"),
	_T("Failed to find device in recover mode"),
	_T("Failed to create event object"),
	_T("Device Time out"),
	_T("Failed to open register key"),
	_T("Failed to find driver letter in registery"),
	_T("Failed to load WNASPI32"),
	_T("Failed to get function ptr in WNASPI32 DLL"),
	_T("State of WNASPI32 not initialized"),
	_T("Failed to locate scsi device"),
	_T("Failed to send scsi command"),
	_T("Invalid device handle"),
	_T("Failed device io control"),
	_T("Device state uninitalized"),
	_T("Unsupported operating system"),
	_T("Sterr failed to load string"),
	_T("Null drive object"),
	_T("Failed to find drive number"),							
	_T("Failed to lock the drive"),							
	_T("Failed to unlock the drive"),							
	_T("Bad chs solution"),									
	_T("Unable to calculate chs"),								
	_T("Unable to pack chs"),									
	_T("Failed to read sector"),								
	_T("Failed to write sector"),								
	_T("Unknown error"),										
	_T("Invalid file system request"),							
	_T("Failed to dismount the drive"),						
	_T("Failed to load icon"),									
	_T("Missing cmdline parameter filename"),					
	_T("Media state uninitialized"),							
	_T("Failed to load setupapi lib"),							
	_T("Missing api in setupapi lib"),							
	_T("Failed to load cfgmgr32 lib"),							
	_T("Missing api in cfgmgr32 lib"),							
	_T("Failed to get device info set"),						
	_T("Failed get device registry property"),					
	_T("Error in cfgmgr32 api"),								
	_T("Failed to create mutex object"),						
	_T("Failed to bring the running application to foreground"),
	_T("Failed to locate scsi device on start"),				
	_T("Failed to locate scsi device on show versions"),		
	_T("Failed to get drive map"),								
	_T("Invalid disk info"),									
	_T("Failed read back verify test"),						
	_T("No administrator"),									
	_T("Failed to delete settings dot dat file"),
};

CStError::CStError()
{
	m_err_in_obj_name = "";
	m_last_error = STERR_NONE;
	m_system_last_error = 0;
	m_drive_index = 0xFF;
	m_more_error_information = L"";
}

CStError::~CStError()
{
}

void CStError::SaveStatus(CStBase* _p_base)
{
	m_err_in_obj_name = _p_base->GetObjName();
	m_last_error = _p_base->GetLastError();
	m_system_last_error = _p_base->GetSystemLastError();
}

void CStError::SaveStatus(CStBase* _p_base, UCHAR _drive_index)
{
	m_drive_index = _drive_index;
	SaveStatus(_p_base);
}

void CStError::SaveStatus(CStBase* _p_base, wstring _more_information)
{
	m_more_error_information = _more_information;
	SaveStatus(_p_base);
}

void CStError::SaveStatus(CStBase* _p_base, UCHAR _drive_index, wstring _more_information)
{
	SaveStatus(_p_base, _more_information);
	SaveStatus(_p_base, _drive_index);
}
void CStError::SaveStatus(ST_ERROR _last_error, long _system_last_error)
{
	m_last_error = _last_error;
	m_system_last_error = _system_last_error;
}
