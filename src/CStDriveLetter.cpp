/*
 * File: CStDriveLetter.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stdafx.h"
#include "CStDriveLetter.h"
#include <ntddscsi.h>

CStDriveLetter::CStDriveLetter()
:	CStDeviceReference(),
	m_driveLetter(0)
{
}

CStDriveLetter::CStDriveLetter(char letter)
:	CStDeviceReference(),
	m_driveLetter(letter)
{
}

CStDriveLetter::CStDriveLetter(wchar_t letter)
:	CStDeviceReference(),
	m_driveLetter(static_cast<char>(letter))
{
}

CStDriveLetter::~CStDriveLetter()
{
}

//! \note This doesn't really belong here, but it's fine for the time being.
//!
bool CStDriveLetter::isRemovable() const
{
	char drivePath[16];
	sprintf_s(drivePath, 16, "%c:\\", m_driveLetter);
	
	return ::GetDriveType(drivePath) == DRIVE_REMOVABLE;
}
