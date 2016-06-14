/*
 * File: CStScsiBlockDevice.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stdafx.h"
#include "CStScsiBlockDevice.h"
#include <stdexcept>
#include "st_error.h"
#include "stddiapi.h"

CStScsiBlockDevice::CStScsiBlockDevice(CStDeviceReference * deviceRef)
:	CStBlockDevice(deviceRef),
	m_blockSize(0),
	m_blockCount(0),
	m_isRemovable(false)
{
}

CStScsiBlockDevice::~CStScsiBlockDevice()
{
	if (isOpen())
	{
		close();
	}
}

//! Locks the drive when it is successfully opened.
//!
void CStScsiBlockDevice::open()
{
	// create scsi interface instance
	m_scsi = createConcreteScsi();
	if (!m_scsi)
	{
		throw std::runtime_error("failed to create scsi interface");
	}
	
	THROW_IF_ST_ERROR( m_scsi->Initialize());
	THROW_IF_ST_ERROR( m_scsi->Open());
	THROW_IF_ST_ERROR( m_scsi->Lock(false));

	//Below method is not a universal one. It works for a USB SD card reader,however,  
	//it fails if a PCIe based SD card reader, i.e. a SD card reader which is embedded into a PC is used.
	/*
	// inquiry
	INQUIRYDATA inquiryData;
	performInquiry(&inquiryData);
	
	m_isRemovable = inquiryData.RemovableMedia;
	
	// convert vendor string
	int i;
	m_vendorID = "";
	for (i=0; inquiryData.VendorId[i] != 0 && i < sizeof(inquiryData.VendorId); ++i)
	{
		m_vendorID += inquiryData.VendorId[i];
	}
	
	// convert product string
	m_productID = "";
	for (i=0; inquiryData.ProductId[i] != 0 && i < sizeof(inquiryData.ProductId); ++i)
	{
		m_productID += inquiryData.ProductId[i];
	}
	
	// read capacity
	READ_CAPACITY_DATA readCapacityData;
	performReadCapacity(&readCapacityData);
	
	m_blockSize = readCapacityData.BytesPerBlock;
	m_blockCount = readCapacityData.LogicalBlockAddress + 1;
	*/
}

void CStScsiBlockDevice::close()
{
	if (!isOpen())
	{
		return;
	}
	
	if (m_scsi->Unlock(false) != STERR_NONE)
	{
	    printf("Failed to unlock drive, closing anyway!\n");
	}
	THROW_IF_ST_ERROR( m_scsi->Close());
	
	delete m_scsi;
	m_scsi = NULL;
}

bool CStScsiBlockDevice::isOpen()
{
	return m_scsi != NULL;
}

void CStScsiBlockDevice::readBlocks(uint32_t firstBlock, uint32_t blockCount, void * buffer)
{
	assert(m_scsi);
	
	uint32_t dataSize = m_blockSize * blockCount;
	CStByteArray sectors(dataSize);
	THROW_IF_ST_ERROR( m_scsi->ReadSector(&sectors, blockCount, firstBlock, m_blockSize));
	THROW_IF_ST_ERROR( sectors.Read(buffer, dataSize, 0));
}

void CStScsiBlockDevice::writeBlocks(uint32_t firstBlock, uint32_t blockCount, const void * data)
{
	assert(m_scsi);
	
	uint32_t dataSize = m_blockSize * blockCount;
	CStByteArray sectors(dataSize);
	THROW_IF_ST_ERROR( sectors.Write(data, dataSize, 0));
	THROW_IF_ST_ERROR( m_scsi->WriteSector(&sectors, blockCount, firstBlock, m_blockSize));
}

void CStScsiBlockDevice::performInquiry(INQUIRYDATA * inquiryData)
{
	assert(m_scsi);
	
	CStScsiInquiry inquiryCommand;
	THROW_IF_ST_ERROR( m_scsi->SendDdiApiCommand(&inquiryCommand));
	inquiryCommand.GetInquiryData(inquiryData);
}

void CStScsiBlockDevice::performReadCapacity(READ_CAPACITY_DATA * readCapacity)
{
	assert(m_scsi);
	
	CStReadCapacity readCapacityCommand;
	THROW_IF_ST_ERROR( m_scsi->SendDdiApiCommand(&readCapacityCommand));
	THROW_IF_ST_ERROR( readCapacityCommand.GetCapacity(readCapacity));
}

std::string CStScsiBlockDevice::convertInquiryString(unsigned char inquiryString[], unsigned length)
{
	std::string result;
	unsigned i;
	unsigned trailingSpaces = 0;
	
	// count trailing spaces
	for (i=length - 1; i >= 0; --i)
	{
		if (inquiryString[i] == ' ')
		{
			trailingSpaces++;
		}
		else
		{
			break;
		}
	}
	
	// convert string
	for (i=0; inquiryString[i] != 0 && i < length - trailingSpaces; ++i)
	{
		result += inquiryString[i];
	}
	
	return result;
}
