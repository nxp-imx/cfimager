/*
 * File: CStCFBootImager.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stdafx.h"
#include "CStCFBootImager.h"
#include "stsdisk.h"
#include <string.h>
#include "CStNTScsiBlockDevice.h"
#include "CStDiskGeometry.h"
#include <algorithm>

//! The maximum bytes to write at once. Currently 1 MB.
#define MAX_WRITE_CHUNK (1 * 1024 * 1024)

CStCFBootImager::CStCFBootImager()
:    m_romVersion(kSTMP3600ROMVersion_All),
    m_alwaysFormat(false),
    m_writeFAT32(true),
    m_device(NULL),
    m_firmware(NULL),
    m_WinCEVersion(kWinCE6),
    m_extraBlocks(0),
    m_blockSize(0),
    m_formatter(NULL),
    m_listener(NULL),
	m_bRaw(false),
	m_offset(0),
	m_skip(0)
{
}

CStCFBootImager::~CStCFBootImager()
{
    if (m_formatter)
    {
        delete m_formatter;
    }
}

void CStCFBootImager::setROMVersion(CStCFBootImager::imx_rom_version_t version)
{
    m_romVersion = version;
}
    
void CStCFBootImager::setAlwaysFormat(bool flag)
{
    m_alwaysFormat = flag;
}

void CStCFBootImager::setWriteFAT32(bool flag)
{
    m_writeFAT32 = flag;
}

//! Gets the block size and block count of \a device.
//!
void CStCFBootImager::setDevice(CStBlockDevice * device)
{
    m_device = device;
    
    // read some info from the device
    m_blockSize = m_device->getBlockSize();
    m_blockCount = m_device->getBlockCount();
}

void CStCFBootImager::setFirmware(CStFwComponent * firmware)
{
    m_firmware = firmware;
}

void CStCFBootImager::setExtraBlocks(unsigned blocks)
{
    m_extraBlocks = blocks;
}

void CStCFBootImager::setWinCEVersion(wince_version_t WinCEVersion)
{
    m_WinCEVersion = WinCEVersion;
}

void CStCFBootImager::setExtra(CStExtraComponent * extra)
{
    m_extra = extra;
}

void CStCFBootImager::setVolumeLabel(const std::string & label)
{
    m_volumeLabel = label;
}

//! This method is where the action happens. All other public methods are just
//! for setting things up to be used by this method. Here the device is
//! examined, formatted if necessary, and the firmware is written to disk.
//! 
//! If #m_alwaysFormat is false, the disk is checked to see if there is an MBR
//! and valid FAT and firmware partitions. If so, and if the firmware partition
//! is large enough to hold the new firmware, then the firmware partition is
//! filled with the new firmware.
//!
//! Otherwise (or if #m_alwaysFormat is true), the entire device is formatted.
//! A fresh FAT partition is layed to disk and the firmware written. Finally,
//! the MBR is written with two partition entries, for the FAT and firmware
//! partitions. Note that the caller may optionally choose to not have the FAT
//! partition actually written, but just have the space allocated. This lets the
//! user use the operating system's native formatter to prepare the partition.
void CStCFBootImager::writeImage()
{
    assert(m_device);
    assert(m_firmware);

    if (m_bImage || m_bRaw)
    {
        writeFirmware();
        return;
    }
    
    bool isAll=true;

    ReadPartitionFromDisk();
    PART_ENTRY * firmwarePartition = findFirmwarePartitionEntry(&m_DiskPartition);
    PART_ENTRY * extraPartition = &m_DiskPartition.Partitions[2];

    std::string firmware_file_name,extr_file_name;
    this->m_firmware->GetFirmwareFilename(firmware_file_name);
    this->m_extra->GetExtraFilename(extr_file_name);

    // is there a valid partition and MBR?
    if (!m_alwaysFormat && isFirmwarePartitionValid() && 
        !firmware_file_name.empty())
    {
        // now check if we have room to write new firmware
        uint32_t firmwareRegionBlocks = getFirmwarePartitionBlockCount();    // includes ldt
        if (firmwareRegionBlocks > 0)
        {

            if ((m_romVersion != kiMX51ROMVersion) && (m_romVersion != kiMX53ROMVersion))
            {
                // subtract blocks used by LDT. the firmware always starts at the
                // same position
                if (!m_bRedundantBoot)
                {
                    firmwareRegionBlocks -= kLDTLengthTA4;
                    if (m_romVersion == kSTMP3600ROMVersion_TA3)
                    {
                        firmwareRegionBlocks -= kLDTLengthTA3;
                    }
                }
            }

            uint64_t firmwareRegionBytes = firmwareRegionBlocks * m_blockSize;
            uint64_t newFirmwareBytes = m_firmware->GetSizeInBytes();
            if (m_bRedundantBoot)
            {
                // second firmware + a block for config block
                newFirmwareBytes = (newFirmwareBytes * 2) + (m_bBCBBoot ? 0 : m_blockSize);
            }
            if (firmwareRegionBytes >= newFirmwareBytes)
            {
                // firmware will fit without moving FAT partition
                if (m_listener)
                {
                    m_listener->setStageCount(2);
                }

                if (m_romVersion != kiMX51ROMVersion  && !m_bRedundantBoot && (m_romVersion != kiMX53ROMVersion))
                {
                    writeLDT(firmwarePartition->FirstSectorNumber+kLDTLengthTA4);
                    writeBCB(firmwarePartition->FirstSectorNumber,
                                firmwarePartition->FirstSectorNumber+kLDTLengthTA4);
                }
                writeFirmware(firmwarePartition->FirstSectorNumber+kLDTLengthTA4);
            }else
			{
				 throw std::runtime_error("no rom for firmware file");
				 printf("required 0x%x, only 0x%x is available", newFirmwareBytes, firmwareRegionBytes);
			}
            isAll = false;
        }else
        {
            // otherwise...
            throw no_firmware_room_error();
        }
    }
    
    if (!m_alwaysFormat && !extr_file_name.empty())
    {
        if(m_extra->GetSizeInSectors(m_blockSize) == 0)
        {
            throw std::runtime_error("wrong extra file");
        }
        if( m_extra->GetSizeInSectors(m_blockSize) > extraPartition->SectorCount )
            throw std::runtime_error("no rom for extra file");

        writeExtraData(extraPartition->FirstSectorNumber);
        isAll = false;
    }

    if(isAll)
    {    
        // must write everything
        if (m_listener)
        {
            m_listener->setStageCount(3 + m_writeFAT32 ? 1 : 0);
        }
        
        prepareFatFormatter();
        
        if (m_romVersion != kiMX51ROMVersion  && !m_bRedundantBoot && (m_romVersion != kiMX53ROMVersion))
        {
              writeLDT();
              writeBCB();
        }  

        if (m_bRedundantBoot)
        {
            writeConfigBlock();
        }

        writeFirmware();
        
        if (m_writeFAT32)
        {
            writeFatPartition();
        }

        if (!m_bBCBBoot)
        {
            writeMBR();
        }
    
        if (!extr_file_name.empty() && m_extra->GetSizeInBytes())
        {
            writeExtraData();
        }
    }
}

//! Search for the SigmaTel firmware partition entry in the MBR.
//!
//! \return A pointer to the firmware partition entry, or NULL if no matching
//!        entry was found.
PART_ENTRY * CStCFBootImager::findFirmwarePartitionEntry(PARTITION_TABLE * mbr)
{
    assert(mbr);
    
    unsigned entryIndex = 0;
    PART_ENTRY * entry = mbr->Partitions;
    for (; entryIndex < 4; ++entryIndex)
    {
        if (entry->FileSystem == kSigmaTelFirmwarePartitionSystemID)
        {
            // found our partition
            return entry;
        }
             else if (((m_romVersion == kiMX51ROMVersion) || (m_romVersion == kiMX53ROMVersion))&& entry->FileSystem == 0)
             {
                    return entry;  // for i.MX51,i.MX53, the file system ID for firmware partition has to be 0
             }
        
        entry = &mbr->Partitions[entryIndex];
    }
    
    // return NULL if no match was made
    return NULL;
}

bool CStCFBootImager::ReadPartitionFromDisk()
{
    assert(m_device);
    
    // read MBR
    CStByteArray mbrData(m_blockSize);
    m_device->readOneBlock(kMBRSector, mbrData);
    
    // check MBR signature
    PARTITION_TABLE * mbr = reinterpret_cast<PARTITION_TABLE *>(mbrData.m_p_t);
    memcpy(&m_DiskPartition,mbr,sizeof(PARTITION_TABLE));
    return true;

}
//! Perform signature test for the MBR, and search for and check the SigmaTel 
//! firmware partition entry. The firmware partition must begin at sector 1.
//!
bool CStCFBootImager::isFirmwarePartitionValid()
{
    assert(m_device);
    
    ReadPartitionFromDisk();
    
    if (m_DiskPartition.Signature != BOOT_SIGNATURE)
    {
        return false;
    }
    
    // that's the only check for TA3
    if (m_romVersion == kSTMP3600ROMVersion_TA3)
    {
        return true;
    }
    
    // look for our partition
    PART_ENTRY * firmwarePartition = findFirmwarePartitionEntry(&m_DiskPartition);
    if (!firmwarePartition)
    {
        return false;
    }
    
    // our partition must begin at sector 2 => removing this requirement so that it can start after FAT partition
    return true; //firmwarePartition->FirstSectorNumber == kFirmwarePartitionStartSector;
}

//! Returns the number of blocks available between the MBR and
//! the PBS of the first FAT partition. The caller
//! must remember to subtract the LDT length to get the number of blocks actually
//! available for firmware. Keep in mind that this method does not validate
//! any data in the firmware region.
//!
//! If there is not an MBR, zero will be returned. This is the same as if
//! there is an MBR but the firmware region is missing. So, use the
//! CStCFBootImager::isMBRPresent() method to first determine whether there
//! is an MBR.
uint32_t CStCFBootImager::getFirmwarePartitionBlockCount()
{
    assert(m_device);
    
    // read MBR
    CStByteArray mbrData(m_blockSize);
    m_device->readOneBlock(kMBRSector, mbrData);
    
    PARTITION_TABLE * mbr = reinterpret_cast<PARTITION_TABLE *>(mbrData.m_p_t);
    
    // check MBR signature
    if (mbr->Signature != BOOT_SIGNATURE)
    {
        return 0;
    }
    
    // for TA3-only we consider all space between the MBR and the FAT32
    // partition to be fair game. for TA4 and compatible mode, we read the
    // sector count from our firmware partition entry in the MBR.
    if (m_romVersion == kSTMP3600ROMVersion_TA3)
    {
        // examine first partition
        PART_ENTRY * firstPartition = &mbr->Partitions[0];
        uint8_t fileSystemTag = firstPartition->FileSystem;
        if (fileSystemTag != PART_SYSID_FAT12 && fileSystemTag != PART_SYSID_FAT16 && fileSystemTag != PART_SYSID_FAT32)
        {
            return 0;
        }
        return firstPartition->FirstSectorNumber - kFirmwarePartitionStartSector;
    }
    else
    {
        // look for our partition
        PART_ENTRY * firmwarePartition = findFirmwarePartitionEntry(mbr);
        if (!firmwarePartition)
        {
            return 0;
        }
        
        return firmwarePartition->SectorCount;
    }
}

//! Calculates the size of the firmware partition for the TA4 drive layout.
//!
void CStCFBootImager::getFirmwarePartitionSize(uint32_t * startBlock, uint32_t * blockCount)
{
    assert(m_firmware);
    
    uint32_t start, PartStart, PartCount;
    getExtraPartitionSize(&start,NULL);

    uint32_t firmwareBlocks = static_cast<uint32_t>(m_firmware->GetSizeInSectors(m_blockSize));

    if (m_bRedundantBoot)
    {
        // extra redundant copy + 1 block for config block
        firmwareBlocks = (firmwareBlocks * 2) + (m_bBCBBoot ? 0 : 1);
        PartStart = start - firmwareBlocks - m_extraBlocks ;
        PartCount = firmwareBlocks + m_extraBlocks ;
    }
    else
    {
        PartStart = start - firmwareBlocks - m_extraBlocks - kLDTLengthTA4;

        PartCount = firmwareBlocks + m_extraBlocks + kLDTLengthTA4;
    }

   
    if (PartStart&PARTITION_BLOCK_ALIGNMENT_MASK)
        PartStart &= ~PARTITION_BLOCK_ALIGNMENT_MASK;

    //For MX51,MX53, it's not a real partition, so we needn't to align the parttion block.
    if (m_romVersion == kiMX51ROMVersion)
    {
        PartStart = kXLDRStart;
        PartCount = kBootPartitionSize;
    }
        
    if (m_romVersion == kiMX53ROMVersion)
    {
        PartStart = kMX53EBOOTStart;
        PartCount = kMX53BootPartitionSize;
    }

    if (startBlock)
        *startBlock = PartStart;

    if (blockCount)
        *blockCount = PartCount;
}

//! Calculates the size of the firmware partition for the TA4 drive layout.
//!
void CStCFBootImager::getExtraPartitionSize(uint32_t * startBlock, uint32_t * blockCount)
{
    assert(m_firmware);
    
    uint32_t extraBlocks = static_cast<uint32_t>(m_extra->GetSizeInSectors(m_blockSize));

    if(startBlock)
    {
        *startBlock = m_blockCount  - extraBlocks ;

        if( *startBlock & PARTITION_BLOCK_ALIGNMENT_MASK)
            *startBlock &= ~PARTITION_BLOCK_ALIGNMENT_MASK;
    }

    if(blockCount)
        *blockCount = extraBlocks;

}

//! Calculates the size of the FAT32 partition.
//!
void CStCFBootImager::getFatPartitionSize(uint32_t * startBlock, uint32_t * blockCount)
{
    assert(m_firmware);
    
    uint32_t start, PartStart, PartCount;
    getFirmwarePartitionSize(&start, NULL);

    PartStart= kFirmwarePartitionStartSector;
    PartCount= start - kFirmwarePartitionStartSector;

    if (m_bBCBBoot)
    {
        // leave last sector for BCB
        PartCount -= 1;
        PartStart = 0;
    }

    if (m_romVersion == kiMX51ROMVersion)
    {
        PartStart = kXLDRStart + kBootPartitionSize;

        // is there room for the FAT partition on the card?
        if (m_blockCount  <= (kXLDRStart + kBootPartitionSize))
            PartCount = 0;     
        else
            PartCount = m_blockCount - PartStart - m_extra->GetSizeInSectors(m_blockSize);
    }

    if (m_romVersion == kiMX53ROMVersion)
    {
        PartStart = kMX53EBOOTStart + kMX53BootPartitionSize;

        // is there room for the FAT partition on the card?
        if (m_blockCount  <= (kMX53EBOOTStart + kMX53BootPartitionSize))
            PartCount = 0;     
        else
            PartCount = m_blockCount - PartStart - m_extra->GetSizeInSectors(m_blockSize);
     }

    if (startBlock)
        *startBlock = PartStart;

    if (blockCount)
        *blockCount = PartCount;
}

//! Writes the Logical Drive Table sectors. For TA3 or compatible mode, a table
//! is written in sector 1 that points to the start of the firmware. In all modes
//! four sectors for the TA4 LDT are written with all zeros starting at sector
//! #kFirmwarePartitionStartSector.
void CStCFBootImager::writeLDT(uint32_t start_sector)
{
    assert(m_device);

    uint32_t firmwareStartSector;
    getFirmwarePartitionSize(&firmwareStartSector,NULL);

    firmwareStartSector += kLDTLengthTA4;
         
    if( start_sector != 0)
        firmwareStartSector = start_sector;
           
    // inform listener
    if (m_listener)
    {
        m_listener->setStage(kWritingLDTStage);
    }
    
    if (m_romVersion != kSTMP3600ROMVersion_TA4)
    {
        // fill in LDT
        ldt_t driveTable;
        driveTable.m_signature[0] = 'S';
        driveTable.m_signature[1] = 'T';
        driveTable.m_signature[2] = 'M';
        driveTable.m_signature[3] = 'P';
        driveTable.m_version = 1;
            // start at the end of the card
        driveTable.m_bootSector = firmwareStartSector;// AT: kImageStartSector;
        driveTable.m_reserved = 0;
        
        CStByteArray ldtData(m_blockSize);
        ldtData.Write(&driveTable, sizeof(driveTable), 0);
        
        m_device->writeOneBlock(kLDTStartTA3, ldtData);
    }
    
    {
        // write empty LDT sectors
        CStByteArray ldtData(m_blockSize);
        unsigned count = 0;
        for (; count < kLDTLengthTA4; ++count)
        {
            m_device->writeOneBlock(firmwareStartSector - kLDTLengthTA4 + count, ldtData);
        }
    }
}

//! start_sector, firmwarestat adddress
void CStCFBootImager::writeBCB(uint32_t bcb_start_sector, uint32_t start_sector)
{
        ConfigBlock_t bcb;
       
        uint32_t firmwareStartSector;
        getFirmwarePartitionSize(&firmwareStartSector,NULL);

        if( bcb_start_sector == 0)
            bcb_start_sector = firmwareStartSector;
        
        firmwareStartSector+=kLDTLengthTA4;
        
        if( start_sector != 0)
            firmwareStartSector = start_sector;

        bcb.u32Signature = CB_SIGNATURE;
        bcb.u32NumCopies = 2;
        bcb.u32PrimaryBootTag = 0x1;
        bcb.u32SecondaryBootTag = 0x2;
        
        bcb.aDriveInfo[0].u32ChipNum = 0;
        bcb.aDriveInfo[0].u32DriveType = 0;
        bcb.aDriveInfo[0].u32Tag = 0x1;
        bcb.aDriveInfo[0].u32FirstSectorNumber = firmwareStartSector;
        bcb.aDriveInfo[0].u32SectorCount = m_firmware->GetSizeInSectors(m_blockSize);

        bcb.aDriveInfo[1].u32ChipNum = 0;
        bcb.aDriveInfo[1].u32DriveType = 0;
        bcb.aDriveInfo[1].u32Tag = 0x2;
        bcb.aDriveInfo[1].u32FirstSectorNumber = firmwareStartSector;
        bcb.aDriveInfo[1].u32SectorCount = m_firmware->GetSizeInSectors(m_blockSize);

        m_device->writeOneBlock(bcb_start_sector, &bcb);
}

//! Writes the firmware to the device starting at sector #kImageStartSector.
//! Assumes that enough room has already been allocated for the firmware. If
//! this is not the case, the firmware may be written over the FAT partition.
#define FLASH_HEADER_SIZE    0x800
#define DCD_BARKER_CODE     0xB17219E9
void CStCFBootImager::writeFirmware(uint32_t start_sector)
{    
    assert(m_firmware);
    assert(m_device);

    uint32_t firmwareStartSector;
    getFirmwarePartitionSize(&firmwareStartSector,NULL);

    if (m_bRedundantBoot)
    {
        if(!m_bBCBBoot)
        {
            firmwareStartSector ++; // leave first sector for config block
        }
        // We should not leave those extra 4 blocks at the beginning of firmware partition if redundant boot is true
    }
    else
    {
        firmwareStartSector += kLDTLengthTA4;
    }

    if(start_sector != 0)
        firmwareStartSector = start_sector;

    if (m_romVersion == kiMX51ROMVersion)
    {
        std::string fileName;
        m_firmware->GetFirmwareFilename(fileName);

        // explicit cast needed to resolve ambiguity
        std::transform(fileName.begin(), fileName.end(), fileName.begin(), (int(*)(int)) toupper);

        if (fileName.find("XLDR.NB0", 0) != string::npos)
        {
            firmwareStartSector = kXLDRStart;
            printf("Flashing xldr.nb0 at sector 0x%x\n", firmwareStartSector);
        }
        else if(fileName.find("EBOOT.NB0", 0) != string::npos)
        {
            //printf("To flash eboot.nb0.\n");
            //Here we have to decide where to flash eboot since there are two kinds of eboot: eboot with flash header and eboot
            //without flash header. If an eboot with flash header is used, then it should be flashed at secotr address 0x2 
            //to allow ROM code boot it correctly, or it should be flashed at secotr address 0x100.

            //There are two method to decide what kind of eboot to be used: 
            //Method1: we can check if CE7parameter is specified by users to decide the type of eboot since only CE7 can provide
            //eboot with flash header. 
            //Method2: parse eboot data and search a flag which indicates flash header exists in the eboot. 

            //Method 1
            /*if(m_WinCEVersion == kWinCE7)
            {
                firmwareStartSector = kXLDRStart;
            }
            else
            {
                firmwareStartSector = kEBOOTStart;
            }*/

            //Method 2
            firmwareStartSector = kEBOOTStart;

            CStByteArray * tempfirmwareData = m_firmware->GetData();
            uint32_t *pu32Buf = (uint32_t *)malloc(FLASH_HEADER_SIZE);
            tempfirmwareData->Read(pu32Buf, FLASH_HEADER_SIZE, 0);

            for(int i=0; i<FLASH_HEADER_SIZE/sizeof(uint32_t); i++)
            {
                if(pu32Buf[i] == DCD_BARKER_CODE)
                {
                    firmwareStartSector = kXLDRStart;
                    break;
                }
            }
            free(pu32Buf);
            pu32Buf = NULL;

            printf("Flashing eboot.nb0 at sector 0x%x\n", firmwareStartSector);                
        }
        else if(fileName.find("NK.NB0", 0) != string::npos)
        {
            firmwareStartSector = kNKStart;
            printf("Flashing NK.nb0 at sector 0x%x\n", firmwareStartSector);                  
        }
    }

    if (m_romVersion == kiMX53ROMVersion)
    {
        std::string fileName;
        m_firmware->GetFirmwareFilename(fileName);

        // explicit cast needed to resolve ambiguity
        std::transform(fileName.begin(), fileName.end(), fileName.begin(), (int(*)(int)) toupper);

        if(fileName.find("EBOOT.NB0", 0) != string::npos)
        {
            firmwareStartSector = kMX53EBOOTStart;
            printf("Flashing eboot.nb0 at sector 0x%x\n", firmwareStartSector);                
        }
        else if(fileName.find("NK.NB0", 0) != string::npos)
        {
            firmwareStartSector = kMX53NKStart;
            printf("Flashing NK.nb0 at sector 0x%x\n", firmwareStartSector);                  
        }
    }
	
	if (m_bRaw)
	{
		firmwareStartSector = m_offset / m_blockSize;
	}
    // inform listener
    if (m_listener)
    {
        m_listener->setStage(kWritingFirmwareStage);
    }

    uint64_t firmwareSize = m_firmware->GetSizeInBytes() - m_skip;
    CStByteArray * firmwareData = m_firmware->GetData();
	if (m_skip)
		if(firmwareData->Remove(0, m_skip))
			throw std::runtime_error("skip bigger than firmware size");;
    // write the full sectors in one go
    uint32_t firmwareFullSectors = static_cast<uint32_t>((firmwareSize) / m_blockSize);
    m_device->writeBlocks(firmwareStartSector, firmwareFullSectors, *firmwareData);

    // write the last partial sector by itself
    uint32_t leftover = static_cast<uint32_t>(firmwareSize - firmwareFullSectors * m_blockSize);
    if (leftover > 0)
    {
        CStByteArray lastSectorData(m_blockSize);
        firmwareData->Read(lastSectorData.m_p_t, leftover, static_cast<size_t>(firmwareSize - leftover));
        m_device->writeOneBlock(firmwareStartSector + firmwareFullSectors, lastSectorData.m_p_t);
    }

    if (m_bRedundantBoot)
    {
        firmwareStartSector += static_cast<uint32_t>(m_firmware->GetSizeInSectors(m_blockSize));
        // write the second firmware copy
        uint64_t firmwareSize = m_firmware->GetSizeInBytes();
        CStByteArray * firmwareData = m_firmware->GetData();

        // write the full sectors in one go
        uint32_t firmwareFullSectors = static_cast<uint32_t>(firmwareSize / m_blockSize);
        m_device->writeBlocks(firmwareStartSector, firmwareFullSectors, *firmwareData);

        // write the last partial sector by itself
        uint32_t leftover = static_cast<uint32_t>(firmwareSize - firmwareFullSectors * m_blockSize);
        if (leftover > 0)
        {
            CStByteArray lastSectorData(m_blockSize);
            firmwareData->Read(lastSectorData.m_p_t, leftover, static_cast<size_t>(firmwareSize - leftover));
            m_device->writeOneBlock(firmwareStartSector + firmwareFullSectors, lastSectorData.m_p_t);
        }
    }
}

//! Sets up the #m_formatter object. This is separate from writeFatPartition()
//! because the formatter object is used to calculate the partition entry in
//! the MBR for the FAT32 partition, even if the FAT32 partition is not
//! written to disk.
void CStCFBootImager::prepareFatFormatter()
{
    assert(m_device);

    m_formatter = new CStFatPartitionFormatter((m_romVersion == kiMX51ROMVersion)
                                                ||(m_romVersion == kiMX53ROMVersion));
    m_formatter->setDevice(m_device);
    m_formatter->setVolumeLabel("untitled");
    
    uint32_t startBlock;
    uint32_t blockCount;
    getFatPartitionSize(&startBlock, &blockCount);
    
    m_formatter->setStartBlock(startBlock);
    m_formatter->setBlockCount(blockCount);
}

//! Calculates and fills in the partition entry \a entry so that it describes
//! the firmware partition. The partition entry's file system tag is set to
//! #kSigmaTelFirmwarePartitionSystemID, and the partition is made non-bootable.
void CStCFBootImager::setFirmwarePartitionEntry(PART_ENTRY * entry)
{
    // get the size and extent of our firmware partition
    uint32_t startBlock;
    uint32_t blockCount;
    getFirmwarePartitionSize(&startBlock, &blockCount);
    
    // and calculate our firmware partition entry. we use the second entry, even
    // though our partition is logically before the first entry (the FAT
    // partition) on the disk.
    entry->BootDescriptor = 0;    // non-bootable
    entry->FirstSectorNumber = startBlock;
    entry->SectorCount = blockCount;

      if ((m_romVersion == kiMX51ROMVersion)||(m_romVersion == kiMX53ROMVersion))
          entry->FileSystem = 0;
      else
          entry->FileSystem = kSigmaTelFirmwarePartitionSystemID;
    
    // compute start CHS
    CHS chs;
    CHS_PACKED packed;
    CStDiskGeometry geometry(m_blockCount);
    geometry.sectorToCHS(entry->FirstSectorNumber, &chs);
    geometry.packCHS(&chs, &packed);
    entry->StartCHSPacked = packed;
    
    // compute end CHS
    uint32_t lastFirmwarePartitionSector = entry->FirstSectorNumber + entry->SectorCount;
    geometry.sectorToCHS(lastFirmwarePartitionSector, &chs);
    geometry.packCHS(&chs, &packed);
    entry->EndCHSPacked = packed;
}

void CStCFBootImager::setExtraPartitionEntry(PART_ENTRY * entry)
{
    // get the size and extent of our firmware partition
    uint32_t startBlock;
    uint32_t blockCount;
    getExtraPartitionSize(&startBlock, &blockCount);
    
    // and calculate our firmware partition entry. we use the second entry, even
    // though our partition is logically before the first entry (the FAT
    // partition) on the disk.
    entry->BootDescriptor = 0;    // non-bootable
    entry->FirstSectorNumber = startBlock;
    entry->SectorCount = blockCount;

    entry->FileSystem = 0x10; 

    // compute start CHS
    CHS chs;
    CHS_PACKED packed;
    CStDiskGeometry geometry(m_blockCount);
    geometry.sectorToCHS(entry->FirstSectorNumber, &chs);
    geometry.packCHS(&chs, &packed);
    entry->StartCHSPacked = packed;
    
    // compute end CHS
    uint32_t lastFirmwarePartitionSector = entry->FirstSectorNumber + entry->SectorCount;
    geometry.sectorToCHS(lastFirmwarePartitionSector, &chs);
    geometry.packCHS(&chs, &packed);
    entry->EndCHSPacked = packed;
}

//! Fills in the config block and writes it at first sector of firmware partition
void CStCFBootImager::writeConfigBlock()
{
    // get the size and extent of our firmware partition
    uint32_t startBlock;
    uint32_t blockCount;
    
    assert(m_device);
    
    getFirmwarePartitionSize(&startBlock, &blockCount);

    ConfigBlock_t cb;

    cb.u32Signature = 0x00112233;
    cb.u32PrimaryBootTag = 0x50;  
    cb.u32SecondaryBootTag = 0x60;
    cb.u32NumCopies=2;           

    cb.aDriveInfo[0].u32ChipNum = 0;
    cb.aDriveInfo[0].u32DriveType = 0;
    cb.aDriveInfo[0].u32Tag = cb.u32PrimaryBootTag;
    cb.aDriveInfo[0].u32FirstSectorNumber = startBlock + (m_bBCBBoot ? 0 : 1);

    cb.aDriveInfo[1].u32ChipNum = 0;
    cb.aDriveInfo[1].u32DriveType = 0;
    cb.aDriveInfo[1].u32Tag = cb.u32SecondaryBootTag;
    cb.aDriveInfo[1].u32FirstSectorNumber = cb.aDriveInfo[0].u32FirstSectorNumber + static_cast<uint32_t>(m_firmware->GetSizeInSectors(m_blockSize));

    // write it
    CStByteArray cbSectorData(m_blockSize);
    cbSectorData.Write(&cb, sizeof(cb), 0);

    if(m_bBCBBoot)
    {
        // if its a bcb boot then config block should be present on the last sector
        startBlock = m_blockCount-1;
    }

    m_device->writeOneBlock(startBlock, cbSectorData);
}


//! Fills in the partition table and writes the MBR to sector 0. The FAT32
//! partition is always given partition table entry 0. For TA4 or compatible
//! mode, entry 1 is set up for the firmware partition.
void CStCFBootImager::writeMBR()
{
    assert(m_device);
    assert(m_formatter);
    
    // inform listener
    if (m_listener)
    {
        m_listener->setStage(kWritingMBRStage);
    }
    
    // construct the MBR
    PARTITION_TABLE mbr;
    memset(&mbr, 0, sizeof(mbr));
    mbr.Signature = BOOT_SIGNATURE;
    
    // fill in partition entries: on MX51,MX53 the FAT partition is partition#1, while on MX233 the FAT partition is partition#0
    if ((m_romVersion == kiMX51ROMVersion)||(m_romVersion == kiMX53ROMVersion))
          m_formatter->setPartitionEntry(&mbr.Partitions[1]);
      else
          m_formatter->setPartitionEntry(&mbr.Partitions[0]);
    
    if (m_romVersion != kSTMP3600ROMVersion_TA3)
    {
          if ((m_romVersion == kiMX51ROMVersion)||(m_romVersion == kiMX53ROMVersion))
                setFirmwarePartitionEntry(&mbr.Partitions[0]);
            else
                setFirmwarePartitionEntry(&mbr.Partitions[1]);
    }

    if(m_extra->GetSizeInBytes())
        setExtraPartitionEntry(&mbr.Partitions[2]);
    
    // special case for WinNT
    CStNTScsiBlockDevice * ntDevice = dynamic_cast<CStNTScsiBlockDevice *>(m_device);
    if (ntDevice)
    {
        uint32_t startBlock;
        uint32_t blockCount;
        getFatPartitionSize(&startBlock, &blockCount);
        ntDevice->allocatePartition(startBlock, blockCount);
    }
    
    // write it
    CStByteArray mbrSectorData(m_blockSize);
    mbrSectorData.Write(&mbr, sizeof(mbr), 0);
    m_device->writeOneBlock(kMBRSector, mbrSectorData);
}

//! Uses the #m_formatter object to initialize the FAT32 partition.
//!
void CStCFBootImager::writeFatPartition()
{
    assert(m_device);
    assert(m_formatter);
    
    // inform listener
    if (m_listener)
    {
        m_listener->setStage(kWritingFatPartitionStage);
    }
    
    m_formatter->writePartition();
}

void CStCFBootImager::writeExtraData(uint32_t start_sector)
{
    assert(m_device);
    assert(m_extra);
    if (m_listener)
    {
        m_listener->setStage(kWritingExtraData);
    }

    uint32_t startBlock;
    uint32_t blockCount;
    getExtraPartitionSize(&startBlock, &blockCount);
    if( start_sector != 0 )
        startBlock = start_sector;
    
    m_extra->WriteToDisk(startBlock,this->m_device);
    
}
//! This will tell imager if the ready-made image is present to burn.
//!
void CStCFBootImager::Image(bool bImage)
{
    m_bImage = bImage;
}

//! This will tell imager if redundant boot is supported.
//!
void CStCFBootImager::RedundantBoot(bool bRedundantBoot)
{
    m_bRedundantBoot = bRedundantBoot;
}

//! This will tell imager if BCB Boot is supported.
//!
void CStCFBootImager::BCBBoot(bool bBCBBoot)
{
    m_bBCBBoot = bBCBBoot;
}
