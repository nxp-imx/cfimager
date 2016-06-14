/*
 * File: CStNTScsiBlockDevice.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#if !defined(_CStNTScsiBlockDevice_h_)
#define _CStNTScsiBlockDevice_h_

#include "CStScsiBlockDevice.h"
#include "stscsi_nt.h"

/*!
 * The concrete implementation of CStBlockDevice for Windows NT/XP. Even
 * though the device reference is a drive letter (via CStDriveLetter), the
 * physical drive will always be opened instead of the logical one. This
 * needs to be changed in the future, probably by adding another device
 * reference class.
 */
class CStNTScsiBlockDevice : public CStScsiBlockDevice
{
public:
	//! \brief Constructor.
    CStNTScsiBlockDevice(CStDeviceReference * deviceRef);
    virtual ~CStNTScsiBlockDevice();
    
    virtual void openPhysicalDrive();
    
    virtual void allocatePartition(uint32_t hiddenSectors, uint32_t partitionSectors);
    
    virtual void unmount();
    
protected:
	virtual CStScsi * createConcreteScsi();
	
	CStScsi_Nt * ntScsi();
};

#endif // _CStNTScsiBlockDevice_h_
