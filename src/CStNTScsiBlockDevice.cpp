/*
 * File: CStNTScsiBlockDevice.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stdafx.h"
#include "CStNTScsiBlockDevice.h"
#include <stdexcept>
#include "CStDriveLetter.h"
#include "st_error.h"
#include "StScsi_Nt.h"

//! The only supported subclass, currently at least, of CStDeviceReference that
//! can be passed in \a deviceRef is CStDriveLetter.
CStNTScsiBlockDevice::CStNTScsiBlockDevice(CStDeviceReference * deviceRef)
:	CStScsiBlockDevice(deviceRef)
{
}

CStNTScsiBlockDevice::~CStNTScsiBlockDevice()
{
}

//! Lets the superclass manage the creation of the CStScsi instance. Then
//! it calls CStScsi_Nt:OpenPhysicalDrive(). Like the superclass'
//! implementation, the drive is locked when the method returns.
void CStNTScsiBlockDevice::openPhysicalDrive()
{
	DISK_GEOMETRY geometry;
	CStScsiBlockDevice::open();
	
	CStScsi_Nt * scsi = ntScsi();
	if (scsi)
	{
		/* Dismount volumn */
		THROW_IF_ST_ERROR( scsi->Dismount(true));
		THROW_IF_ST_ERROR( scsi->OpenPhysicalDrive());
		/* lock physical device */
		THROW_IF_ST_ERROR( scsi->Lock(false,false));
		/* Dismount physical device */
		THROW_IF_ST_ERROR( scsi->Dismount(false));
		
		THROW_IF_ST_ERROR( scsi->ReadGeometry(&geometry));
		m_blockSize = geometry.BytesPerSector;
		m_blockCount = geometry.Cylinders.QuadPart*geometry.SectorsPerTrack*geometry.TracksPerCylinder;
	}

}

void CStNTScsiBlockDevice::allocatePartition(uint32_t hiddenSectors, uint32_t partitionSectors)
{
	DISK_GEOMETRY geometry;
	
	CStScsi_Nt * scsi = ntScsi();
	if (scsi)
	{
		THROW_IF_ST_ERROR( scsi->ReadGeometry(&geometry));
		THROW_IF_ST_ERROR( scsi->FormatPartition(&geometry, hiddenSectors, partitionSectors));
	}
}

void CStNTScsiBlockDevice::unmount()
{
	CStScsi_Nt * scsi = ntScsi();
	if (scsi)
	{
		scsi->Dismount(true);
	}
}

CStScsi * CStNTScsiBlockDevice::createConcreteScsi()
{
	const CStDriveLetter * driveLetter = dynamic_cast<const CStDriveLetter *>(deviceReference());
	if (!driveLetter)
	{
		throw std::runtime_error("invalid device reference type");
	}
	
	return new CStScsi_Nt(driveLetter->getDriveLetter());
}

CStScsi_Nt * CStNTScsiBlockDevice::ntScsi()
{
	if (m_scsi)
	{
		CStScsi_Nt * scsi = dynamic_cast<CStScsi_Nt *>(m_scsi);
		if (scsi)
		{
			return scsi;
		}
	}
	
	return NULL;
}

