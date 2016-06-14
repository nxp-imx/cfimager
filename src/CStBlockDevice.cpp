/*
 * File: CStBlockDevice.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stdafx.h"
#include "CStBlockDevice.h"
#include <stdexcept>

//! The \a deviceRef is not owned by the CStBlockDevice, and will not be
//! deleted by the destructor. It is up to the caller to dispose of the
//! device reference object at the appropriate time.
//!
//! \exception std::runtime_error is raised if \a deviceRef is NULL.
CStBlockDevice::CStBlockDevice(CStDeviceReference * deviceRef)
:	m_deviceRef(deviceRef)
{
	if (!m_deviceRef)
	{
		throw std::runtime_error("invalid device reference");
	}
}

// Will close the device if it was left open.
//
CStBlockDevice::~CStBlockDevice()
{
	//if (isOpen())
	//{
	//	close();
	//}
}

//! \exception std::range_error Thrown if \a data is not large enough to hold
//!		the number of blocks to be read from the device.
void CStBlockDevice::readBlocks(uint32_t firstBlock, uint32_t blockCount, CStByteArray & data)
{
	if (data.GetCount() < blockCount * getBlockSize())
	{
		throw std::range_error("data is too small");
	}
	
	readBlocks(firstBlock, blockCount, data.m_p_t);
}

//! \exception std::range_error Thrown if \a data is not large enough to hold
//!     the number of blocks to be written to the device.
void CStBlockDevice::writeBlocks(uint32_t firstBlock, uint32_t blockCount, const CStByteArray & data)
{
    if (data.GetCount() < blockCount * getBlockSize())
    {
        throw std::range_error("data is too small");
    }
    if(blockCount < 0x100000/getBlockSize())
    {
        //If data size is smaller than 1Mbytes, write it directly.
        writeBlocks(firstBlock, blockCount, data.m_p_t);
    }
    else
    {
        uint32_t i,percent,oldpercent,iPerBlockCount;
        percent =0;
        oldpercent =0;
        iPerBlockCount=0x100000/getBlockSize();  //   1M bytes per write
        for(i = 0;i < blockCount;i += iPerBlockCount)
        {
            if(blockCount-i>=iPerBlockCount)
            {
                writeBlocks(firstBlock+i, iPerBlockCount, data.m_p_t+i*getBlockSize());
                percent = (i+iPerBlockCount)*100 /blockCount;
                if(percent != oldpercent)
                {
                    printf("write %d%%\r", percent);
                    oldpercent =percent;
                }
            }
            else
            {
                writeBlocks(firstBlock+i, blockCount-i, data.m_p_t+i*getBlockSize());
                printf("write 100%%\r");
            }    
        }
    }
    
}

void CStBlockDevice::readOneBlock(uint32_t block, void * buffer)
{
	readBlocks(block, 1, buffer);
}

void CStBlockDevice::writeOneBlock(uint32_t block, const void * buffer)
{
	writeBlocks(block, 1, buffer);
}

void CStBlockDevice::readOneBlock(uint32_t block, CStByteArray & data)
{
	readBlocks(block, 1, data);
}

void CStBlockDevice::writeOneBlock(uint32_t block, const CStByteArray & data)
{
	writeBlocks(block, 1, data);
}

