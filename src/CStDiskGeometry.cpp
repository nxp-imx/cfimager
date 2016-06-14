/*
 * File: CStDiskGeometry.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stdafx.h"
#include "CStDiskGeometry.h"
#include "string.h"
#include <stdexcept>

//! \exception std::runtime_error Raised if the CHS geometry cannot be computed for
//!		the disk given the \a diskSectors value.
CStDiskGeometry::CStDiskGeometry(uint32_t diskSectors)
:	m_diskSectors(diskSectors),
	m_isExactCHSSolution(false),
	m_wastedDiskSectors(0),
	m_adjustedDiskSectors(0)
{
	memset(&m_diskGeometry, 0, sizeof(m_diskGeometry));
	if (!computeDiskGeometry())
	{
		throw std::runtime_error("invalid disk parameters");
	}
}

bool CStDiskGeometry::sectorToCHS(uint32_t sector, CHS * chs) const
{
	uint32_t tempSector = 0;
	chs->Cylinder = 0;
	chs->Head = 0;
	chs->Sector = 1;

	for (;;)
	{
		// convert our CHS form back to a sector
		tempSector = (chs->Cylinder * m_diskGeometry.Head * m_diskGeometry.Sector);
		tempSector += (chs->Head * m_diskGeometry.Sector) + chs->Sector;
		
		// if it's a match, exit the loop
		if (tempSector == sector)
		{
			break;
		}
		
		chs->Sector++;
		if (chs->Sector > m_diskGeometry.Sector)
		{
			chs->Sector = 1;
			chs->Head++;
			if (chs->Head == m_diskGeometry.Head)
			{
				chs->Head = 0;
				chs->Cylinder++;
				if (chs->Cylinder == m_diskGeometry.Cylinder)
				{
					return false;
				}
			}
		}
	}
	
	return true;
}

uint32_t CStDiskGeometry::chsToSector(const CHS * chs) const
{
	return (chs->Cylinder * m_diskGeometry.Head * m_diskGeometry.Sector) + (chs->Head * m_diskGeometry.Sector) + chs->Sector;
}

void CStDiskGeometry::packCHS(const CHS * unpacked, CHS_PACKED * packed) const
{
	packed->Cylinder = (UCHAR)(unpacked->Cylinder & 0x00FF);
	packed->Head = (UCHAR)unpacked->Head;
	packed->Sector = unpacked->Sector | ((UCHAR)((unpacked->Cylinder & 0x0300) >> 2));
}

void CStDiskGeometry::unpackCHS(const CHS_PACKED * packed, CHS * unpacked) const
{
}

//! This method finds the maximum CHS values for a disk, given the total
//! number of sectors the disk has available.
//!
//! \result Returns true if the contents of \a m_diskGeometry are valid.
bool CStDiskGeometry::computeDiskGeometry()
{
	ULONG ulSize, ulWastedSectors;
	UCHAR ucSectors, ucSectors2, ucOptimalSectors=0;
	USHORT usHeads, usHeads2, usOptimalHeads=0, usCylinders, usCylinders2, usOptimalCylinders=0;
	
	m_isExactCHSSolution = false;
	
	// Number of bits available for CHS:
	//
	// Standard      Cylinders   Heads   Sectors   Total
	// --------------------------------------------------
	//  IDE/ATA        16          4        8       28
	//  Int13/MBR      10          8        6       24
	//  Combination    10          4        6       20
	//
	// In decimal we get
	//
	// Standard      Cylinders   Heads   Sectors            Total
	// ----------------------------------------------------------------
	//  IDE/ATA        65536      16       256       268435456 =  128GB
	//  Int13/MBR      1024       256       63*       16515072 = 8064MB
	//  Combination    1024       16        63         1032192 =  504MB
	//
	// * There is no sector "0" in CHS (there is in LBA, though)
	//
	// All drives with more than 16,515,072 sectors will get bogus CHS
	//	parameters.

	if (m_diskSectors >= (ULONG)16515072)
	{
		// Create bogus, non-zero parameters.  Params are non-zero because
		//	some 3rd party media readers may fail to recognize the media.
		m_diskGeometry.Cylinder	= 1;
		m_diskGeometry.Head		= 1;
		m_diskGeometry.Sector	= 16;

		return true;
	}

	usCylinders = 1;
	usHeads = 1;
	ucSectors = 1;
	ulWastedSectors = 0x7FFFFFFF;
	
	for (;;)
	{
		ulSize = (ULONG)usCylinders * (ULONG)usHeads * (ULONG)ucSectors;
		if (ulSize < m_diskSectors)
		{
			// Not enough
			ucSectors++;
			if (ucSectors > MAX_SECTORS)
			{
				ucSectors = 1;
				usHeads++;
				if (usHeads > MAX_HEADS)
				{
					usHeads = 1;
					usCylinders++;
					if (usCylinders > MAX_CYLINDERS)
					{
						break;
					}
				}
			}
		}
		else
		{
			if (ulSize == m_diskSectors)
			{
				// Found an exact solution so it's time to stop
				m_isExactCHSSolution = true;
				usOptimalCylinders = usCylinders;
				usOptimalHeads = usHeads;
				ucOptimalSectors = ucSectors;
				break;
			}
			else
			{
				// Found a solution.  We're over by some amount so we need
				//	to back up
				usCylinders2 = usCylinders;
				usHeads2 = usHeads;
				ucSectors2 = ucSectors;

				ucSectors2--;
				if (ucSectors2 == 0)
				{
					ucSectors2 = MAX_SECTORS;
					usHeads2--;
					if (usHeads2 == 0)
					{
						usHeads2 = MAX_HEADS;
						usCylinders2--;
						if (usCylinders2 == 0)
						{
//							printf("ERROR - bad CHS solution\r\n");
							return false;
						}
					}
				}
				
				// Only keep it if it's optimal
				if ((m_diskSectors - ((ULONG)usCylinders2 * (ULONG)usHeads2 * (ULONG)ucSectors2)) < ulWastedSectors)
				{
					ulWastedSectors = m_diskSectors - ((ULONG)usCylinders2 * (ULONG)usHeads2 * (ULONG)ucSectors2);
					usOptimalCylinders = usCylinders2;
					usOptimalHeads = usHeads2;
					ucOptimalSectors = ucSectors2;
				}

				// Keep searching
				ucSectors++;
				if (ucSectors > MAX_SECTORS)
				{
					ucSectors = 1;
					usHeads++;
					if (usHeads > MAX_HEADS)
					{
						usHeads = 1;
						usCylinders++;
						if (usCylinders > MAX_CYLINDERS)
						{
							break;
						}
					}
				}
			}
		}
	}

	m_diskGeometry.Cylinder = usOptimalCylinders;
	m_diskGeometry.Head = usOptimalHeads;
	m_diskGeometry.Sector = ucOptimalSectors;

	m_wastedDiskSectors = m_diskSectors - ((ULONG)usOptimalCylinders * (ULONG)usOptimalHeads * (ULONG)ucOptimalSectors);
	m_adjustedDiskSectors = m_diskSectors - m_wastedDiskSectors;

	return true;
}

