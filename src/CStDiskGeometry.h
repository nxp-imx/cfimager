/*
 * File: CStDiskGeometry.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#if !defined(_CStDiskGeometry_h_)
#define _CStDiskGeometry_h_

#include "stdafx.h"
#include "stsdisk.h"

/*!
 * \brief Sector/CHS disk geometry converter.
 *
 * This class can perform calculations and conversions on CHS
 * (i.e. Cylinder-Header-Sector) values. The object must be properly initialized
 * by passing the total number of sectors for the disk to the constructor, or
 * the sector-conversion methods will not return valid results.
 */
class CStDiskGeometry
{
public:
	//! \brief Constructor. Pass the total number of sectors for the disk.
	CStDiskGeometry(uint32_t diskSectors);
	
	//! \name Conversions
	//@{
	
	//! \brief Convert a logical sector number into a CHS tuple.
	bool sectorToCHS(uint32_t sector, CHS * chs) const;
	
	//! \brief Convert from a CHS tuple to a sector number.
	uint32_t chsToSector(const CHS * chs) const;
	
	//! \brief Pack a CHS tuple.
	void packCHS(const CHS * unpacked, CHS_PACKED * packed) const;
	
	//! \brief Unpack a packed CHS value.
	void unpackCHS(const CHS_PACKED * packed, CHS * unpacked) const;
	
	//@}
	
	//! \name Accessors
	//@{
	
	//! \brief Returns the total number of sectors for the disk.
	inline uint32_t getDiskSectors() const { return m_diskSectors; }
	
	//! \brief Returns the CHS geometry computed for the disk.
	inline void getDiskGeometry(CHS * geometry) const { memcpy(geometry, &m_diskGeometry, sizeof(CHS)); }
	
	//! \brief Returns true if the disk geometry matches exactly.
	inline bool isExactCHSSolution() const { return m_isExactCHSSolution; }
	
	//! \brief Returns the number of wasted sectors at the end of the disk.
	inline uint32_t getWastedDiskSectors() const { return m_wastedDiskSectors; }
	
	//! \brief Returns the number of available sectors using the CHS geometry.
	inline uint32_t getAdjustedDiskSectors() const { return m_adjustedDiskSectors; }
	
	//@}

protected:
	uint32_t m_diskSectors;
	CHS m_diskGeometry;
	bool m_isExactCHSSolution;
	uint32_t m_wastedDiskSectors;
	uint32_t m_adjustedDiskSectors;
	
	//! \brief Find CHS for the disk.
	bool computeDiskGeometry();
};

#endif // _CStDiskGeometry_h_
