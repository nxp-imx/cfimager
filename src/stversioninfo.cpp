/*
 * File: stversioninfo.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stversioninfo.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStVersionInfo::CStVersionInfo(string _base):CStBase(_base)
{
	SetHigh(0);
	SetMid(0);
	SetLow(0);
}

CStVersionInfo::CStVersionInfo(const CStVersionInfo& _ver)
{
	*this = _ver;
}

CStVersionInfo& CStVersionInfo::operator=(const CStVersionInfo& _ver)
{
	SetHigh(_ver.GetHigh());
	SetMid(_ver.GetMid());
	SetLow(_ver.GetLow());

	m_last_error		= _ver.m_last_error;
	m_system_last_error = _ver.m_system_last_error;
	m_obj_name			= _ver.m_obj_name;

	return *this;
}

CStVersionInfo::~CStVersionInfo()
{

}

uint16_t CStVersionInfo::GetHigh() const
{
	return m_high;
}

uint16_t CStVersionInfo::GetMid() const
{
	return m_mid;
}

uint16_t CStVersionInfo::GetLow() const
{
	return m_low;
}

void CStVersionInfo::SetHigh(uint16_t _high)
{
	m_high = _high;
}

void CStVersionInfo::SetMid(uint16_t _mid)
{
	m_mid = _mid;
}

void CStVersionInfo::SetLow(uint16_t _low)
{
	m_low = _low;
}

bool CStVersionInfo::operator != (CStVersionInfo& _ver)
{
	return !(*this == _ver);
}

bool CStVersionInfo::operator == (CStVersionInfo& _ver)
{
	if( (_ver.GetHigh() == GetHigh()) && (_ver.GetMid() == GetMid()) &&
		(_ver.GetLow() == GetLow()) )
	{
		return true;
	}
	return false;
}

wstring CStVersionInfo::GetVersionString()
{
#if !defined(TARGET_OS_MAC)
	wchar_t ver[MAX_PATH];
	swprintf_s(ver, MAX_PATH, L"%03d.%03d.%03d", GetHigh(), GetMid(), GetLow());
#else
	char ver[256];   //! \todo Figure out a clean way to handle the max path length.
	snprintf(ver, sizeof(ver)-sizeof(char), "%03d.%03d.%03d", GetHigh(), GetMid(), GetLow());
#endif
	return ver;
}

CStVersionInfoPtrArray::CStVersionInfoPtrArray(size_t _size, string _name):
	CStArray<CStVersionInfo*>(_size, _name)
{
	CStVersionInfo* ver;
	for(size_t index=0; index<_size; index ++)
	{
		ver = new CStVersionInfo;
		SetAt(index, ver);
	}
}

CStVersionInfoPtrArray::~CStVersionInfoPtrArray()
{
	CStVersionInfo* ver;
	for(size_t index=0; index<GetCount(); index ++)
	{
		ver = *GetAt(index);
		delete ver;
		SetAt(index, NULL);
	}
}

