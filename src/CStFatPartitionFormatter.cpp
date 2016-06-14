/*
 * File: CStFatPartitionFormatter.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stdafx.h"
#include "CStFatPartitionFormatter.h"
#include <stdexcept>
#include <string.h>
#include "stsdisk.h"
#include "st_error.h"

CStFatPartitionFormatter::CStFatPartitionFormatter(bool imx51Layout)
:	m_device(NULL),
	m_onlyFAT16(false)
{
    m_fatPartitionNumber = imx51Layout;
}

CStFatPartitionFormatter::~CStFatPartitionFormatter()
{
}

void CStFatPartitionFormatter::setDevice(CStBlockDevice * device)
{
	m_device = device;
}

void CStFatPartitionFormatter::setStartBlock(uint32_t startBlock)
{
	m_startBlock = startBlock;
}

void CStFatPartitionFormatter::setBlockCount(uint32_t blockCount)
{
	m_blockCount = blockCount;
}

void CStFatPartitionFormatter::writePartition()
{
	assert(m_device);
	
	// make sure partition doesn't extend past end of device
	uint32_t deviceBlockCount = m_device->getBlockCount();
	if (m_startBlock + m_blockCount > deviceBlockCount)
	{
		m_blockCount = deviceBlockCount - m_startBlock;
	}
	
	uint32_t sectorSize = m_device->getBlockSize();
	CStSDisk sdisk(deviceBlockCount, m_blockCount, m_startBlock, sectorSize, (UCHAR *)m_volumeLabel.c_str(), m_volumeLabel.size(), m_onlyFAT16, m_fatPartitionNumber);
	THROW_IF_ST_ERROR(sdisk.GetLastError());
	
	uint32_t currentSector = m_startBlock;
	
	// write PBS
	writeAndVerify(sdisk.GetPartitionBootSector(), sizeof(BOOT_SECTOR), currentSector++);
	
	// write other PBSes for FAT32
	if (sdisk.GetFileSystem() == FAT_32)
	{
		// PBS2
		writeAndVerify(sdisk.GetPartitionBootSector2(), sizeof(BOOT_SECTOR2), currentSector++);
		
		// PBS3
		writeAndVerify(sdisk.GetPartitionBootSector3(), sizeof(BOOT_SECTOR3), currentSector++);
		
		// 3 empty sectors
		eraseBlocks(currentSector, 3);
		currentSector += 3;
		
		// duplicates of the PBS sectors
		writeAndVerify(sdisk.GetPartitionBootSector(), sizeof(BOOT_SECTOR), currentSector++);
		writeAndVerify(sdisk.GetPartitionBootSector2(), sizeof(BOOT_SECTOR2), currentSector++);
		writeAndVerify(sdisk.GetPartitionBootSector3(), sizeof(BOOT_SECTOR3), currentSector++);
		
		// erase remaining reserved sectors
		eraseBlocks(currentSector, sdisk.GetPartitionBootSector()->BPB_RsvdSecCnt - 9);
		currentSector += sdisk.GetPartitionBootSector()->BPB_RsvdSecCnt - 9;
	}
	
	currentSector += formatFatArea(sdisk, currentSector);
	currentSector += formatDirectoryStructure(sdisk.GetNumDirectoryEntriesInSectors(), currentSector);
}

uint32_t CStFatPartitionFormatter::formatFatArea(CStSDisk & sdisk, uint32_t startBlock)
{
	BOOT_SECTOR * pbs = sdisk.GetPartitionBootSector();
	uint32_t sectorCount;
	if (sdisk.GetFileSystem() == FAT_32 )
	{
		sectorCount = pbs->Fat32.BPB_FATSz32;
	}
	else
	{
		sectorCount = pbs->BPB_FATSz16;
	}
	
	uint32_t sectorSize = m_device->getBlockSize();
	unsigned fat = 0;

	for (; fat < pbs->BPB_NumFATs; ++fat)
	{
		unsigned sector = 0;
		for (; sector < sectorCount; ++sector)
		{
			CStByteArray sectorData(sectorSize);
			
			if (sector == 0)
			{
				sdisk.GetFirstFatSector(&sectorData);
			}
			
			writeAndVerify(sectorData, startBlock + fat * sectorCount + sector);
		}
	}

	return sectorCount * pbs->BPB_NumFATs;
}

uint32_t CStFatPartitionFormatter::formatDirectoryStructure(uint32_t entries, uint32_t startBlock)
{
	uint32_t sectorSize = m_device->getBlockSize();
	unsigned sector = 0;
	for (; sector < entries; ++sector)
	{
		CStByteArray sectorData(sectorSize);
		if (sector == 0 )
		{
			sectorData.Write(m_volumeLabel.c_str(), m_volumeLabel.size(), 0);
			sectorData.SetAt(ROOT_DIR_ATTRIB_OFFSET, (UCHAR)ATTR_VOLUME_ID);
		}
		
		writeAndVerify(sectorData, startBlock + sector);
	}
	
	return entries;
}

void CStFatPartitionFormatter::setPartitionEntry(PART_ENTRY * entry)
{
	assert(m_device);
	
	// make sure partition doesn't extend past end of device
	uint32_t deviceBlockCount = m_device->getBlockCount();
	if (m_startBlock + m_blockCount > deviceBlockCount)
	{
		m_blockCount = deviceBlockCount - m_startBlock;
	}
	
	uint32_t sectorSize = m_device->getBlockSize();
	CStSDisk sdisk(deviceBlockCount, m_blockCount, m_startBlock, sectorSize, (UCHAR *)m_volumeLabel.c_str(), m_volumeLabel.size(), m_onlyFAT16, m_fatPartitionNumber);
	THROW_IF_ST_ERROR(sdisk.GetLastError());
	
	PARTITION_TABLE * table = sdisk.GetMasterBootRecord();
	memcpy(entry, &table->Partitions[m_fatPartitionNumber], sizeof(PART_ENTRY));
}

void CStFatPartitionFormatter::writeAndVerify(const CStByteArray & blocks, uint32_t startBlock)
{
	uint32_t sectorSize = m_device->getBlockSize();
	uint32_t blockCount = static_cast<uint32_t>(blocks.GetCount() / sectorSize);
	
	// write
	m_device->writeBlocks(startBlock, blockCount, blocks);
	
	// read back
	CStByteArray verifyBlocks(blockCount * sectorSize);
	m_device->readBlocks(startBlock, blockCount, verifyBlocks);
	
	// verify
	if (!(verifyBlocks == blocks))
	{
		throw std::runtime_error("read back verify failure");
	}
}

void CStFatPartitionFormatter::writeAndVerify(const void * data, uint32_t length, uint32_t startBlock)
{
	uint32_t sectorSize = m_device->getBlockSize();
	uint32_t blockCount = (length - 1) / sectorSize + 1;
	
	CStByteArray blocks(blockCount * sectorSize);
	blocks.Write(data, length, 0);
	
	writeAndVerify(blocks, startBlock);
}

void CStFatPartitionFormatter::eraseBlocks(uint32_t startBlock, uint32_t blockCount)
{
	uint32_t sectorSize = m_device->getBlockSize();
	CStByteArray zeroData(sectorSize);
	
	while (blockCount--)
	{
		writeAndVerify(zeroData, startBlock++);
	}
}


