/*
 * File: CStFatPartitionFormatter.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#if !defined(_CStFatPartitionFormatter_h_)
#define _CStFatPartitionFormatter_h_

#include "stdafx.h"
#include "CStBlockDevice.h"
#include "stsdisk.h"

/*!
 * \brief
 *
 * \note The terms "block" and "sector" are used interchangeably throughout
 * the source, and they shouldn't be. Stupid.
 */
class CStFatPartitionFormatter
{
public:
    CStFatPartitionFormatter(bool imx51Layout);
    virtual ~CStFatPartitionFormatter();
    
    virtual void setDevice(CStBlockDevice * device);
    virtual void setVolumeLabel(const std::string & label) { m_volumeLabel = label; }
    virtual void setFAT16Only(bool flag) { m_onlyFAT16 = flag; }
    
    virtual void setStartBlock(uint32_t startBlock);
    virtual void setBlockCount(uint32_t blockCount);
    
    virtual void writePartition();
    
    virtual void setPartitionEntry(PART_ENTRY * entry);

protected:
	CStBlockDevice * m_device;
	std::string m_volumeLabel;
	bool m_onlyFAT16;
	uint32_t m_startBlock;
	uint32_t m_blockCount;
      ULONG      m_fatPartitionNumber;
	
	uint32_t formatFatArea(CStSDisk & sdisk, uint32_t startBlock);
	uint32_t formatDirectoryStructure(uint32_t entries, uint32_t startBlock);

	void writeAndVerify(const CStByteArray & blocks, uint32_t startBlock);
	void writeAndVerify(const void * data, uint32_t length, uint32_t startBlock);
	void eraseBlocks(uint32_t startBlock, uint32_t blockCount);
};

#endif // _CStFatPartitionFormatter_h_
