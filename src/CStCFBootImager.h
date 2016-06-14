/*
 * File: CStCFBootImager.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#if !defined(_CStCFBootImager_h_)
#define _CStCFBootImager_h_

#include "CStFatPartitionFormatter.h"
#include "stfwcomponent.h"
#include "CStBlockDevice.h"
#include "StExtraComponent.h"

#define PARTITION_BLOCK_ALIGNMENT 0x100 /*128K BYTE cylinder bound*/
#define PARTITION_BLOCK_ALIGNMENT_MASK (PARTITION_BLOCK_ALIGNMENT -1)

//! Sector numbers.
enum {
	kMBRSector = 0,			//!< Sector for the partition table.
	kFirmwarePartitionStartSector = PARTITION_BLOCK_ALIGNMENT,	//!< Logical Drive Table start sector.
	kLDTLengthTA4 = 4,			//!< Number of sectors consumed by the LDT for TA4.
	kLDTLengthTA3 = 1,		//!< LDT length for TA3 ROM.
	kLDTStartTA3 = 1,		//!< Location of LDT for TA3 ROM.
	
	//! First sector of the firmware image. This applies to TA3, TA4, and
	//! compatible mode.
	kImageStartSector = kFirmwarePartitionStartSector + kLDTLengthTA4
};

// sector numbers for i.MX51 layout
enum {
        kXLDRStart = 0x2,
        kEBOOTStart = 0x100,
        kNKStart = 0x500,
        kBootPartitionSize = 0x2FFFE,         // 94 MBs - 1024 bytes (sector 0 for MBR, sector 1 rsvd)
};

// sector numbers for i.MX53 layout
enum {
        kMX53EBOOTStart = 0x2,
        kMX53NKStart = 0x402,
        kMX53BootPartitionSize = 0x6FFFE,         // 94 MBs - 1024 bytes (sector 0 for MBR, sector 1 rsvd)
};


//! Firmware partition related constants.
enum {
	//! The system ID for our firmware partition. This value is not used for
	//! any standard filesystem and is ignored by all major operating systems.
	kSigmaTelFirmwarePartitionSystemID = 'S'
};

#define CB_SIGNATURE 0x00112233
#define MAX_DEVICE_INFO 10

typedef struct _DriveInfo_t
{
    uint32_t u32ChipNum; //!< Chip Select, ROM does not use it
    uint32_t u32DriveType; //!< Always system drive, ROM does not use it
    uint32_t u32Tag; //!< Drive Tag
    uint32_t u32FirstSectorNumber; //!< Start sector/block address of firmware.
    uint32_t u32SectorCount; //!< Not used by ROM
} DriveInfo_t;

typedef struct _ConfigBlock_t
{
    uint32_t u32Signature; //!< Signature 0x00112233
    uint32_t u32PrimaryBootTag; //!< Primary boot drive identified by this tag
    uint32_t u32SecondaryBootTag; //!< Secondary boot drive identified by this tag
    uint32_t u32NumCopies; //!< Num elements in aFWSizeLoc array
    DriveInfo_t aDriveInfo[MAX_DEVICE_INFO]; //!< Let array aDriveInfo be last in this data
    //!< structure to be able to add more drives in future
    //!< without changing ROM code
} ConfigBlock_t;

/*!
 * \brief Main controller class for the CF Imager.
 *
 * After constructing the object, you must set a number of values before the
 * writeImage() method can be called. These required methods are setDevice()
 * and setFirmware(). If you call writeImage() without first using these
 * methods, an assertion failure will be raised.
 *
 * If you wish to receive feedback about the progress of the disk update, use
 * the setListener() method to provide an instance of a
 * CStCFBootImager::Listener subclass. The listener object's methods will be
 * called during the writeImage() process.
 *
 * The device must already be opened when passed to setDevice(), and it
 * will not be closed at any time by this class.
 *
 * \todo The methods isFirmwarePartitionValid() and 
 * getFirmwarePartitionBlockCount() both share some similar code. It would be
 * good to refactor this into a common routine.
 */
class CStCFBootImager
{
public:

	/*!
	 * Exception raised when we cannot allocate room for the firmware.
	 */
	class no_firmware_room_error : public std::exception
	{
		virtual const char * what() throw() { return "no room for the firmware"; }
	};
    class no_extra_file_error : public std::exception
	{
		virtual const char * what() throw() { return "extra image file wrong"; }
	};
	class no_extra_room_error : public std::exception
	{
		virtual const char * what() throw() { return "no room for the extra"; }
	};
	//! List of possible ROM revisions. These versions are used to determine how
	//! the drive will be formatted.
	typedef enum {
		//! This ROM version is used to format the drive in a manner compatible with
		//! both TA3 and TA4 ROMs.
		kSTMP3600ROMVersion_All,
		kSTMP3600ROMVersion_TA3,    //!< TA3 only.
		kSTMP3600ROMVersion_TA4,    //!< TA4 only.

        kiMX51ROMVersion = 51,
        kiMX53ROMVersion = 53
	} imx_rom_version_t;

	typedef enum {
        kWinCE7 ,
        kWinCE6 
	} wince_version_t;

	//! \brief Imaging stages.
	//!
	//! These stages are not necessarily in the order that they will occur.
	typedef enum {
		kWritingLDTStage,			//!< Writing the LDT.
		kWritingFirmwareStage,		//!< Writing the firmware.
		kWritingMBRStage,			//!< Writing the MBR.
		kWritingFatPartitionStage,	//!< Writing the FAT partition.
        kWritingExtraData           //!< Writing extra data
	} imaging_stage_t;

	/*!
	 * \brief Status listener mix-in for CStCFBootImager.
	 *
	 * \todo Add finer grained progress reports so the progress of writing the
	 *		firmware and FAT partitions can be reported to the user.
	 */
	class Listener
	{
	public:
		virtual void setStageCount(int count)=0;
		virtual void setStage(imaging_stage_t stage)=0;
	};

public:
    CStCFBootImager();
    virtual ~CStCFBootImager();
    
	void setROMVersion(imx_rom_version_t version);
    void setAlwaysFormat(bool flag);
    void setWriteFAT32(bool flag);
    void setDevice(CStBlockDevice * device);
    void setFirmware(CStFwComponent * firmware);
    void setExtra(CStExtraComponent * extra);
    void setExtraBlocks(unsigned blocks);
    void setWinCEVersion(wince_version_t WinCEVersion);
    void setVolumeLabel(const std::string & label);
    void Image(bool bImage);
    void RedundantBoot(bool bRedundantBoot);
    void BCBBoot(bool bBCBBoot);

    void setListener(Listener * listener) { m_listener = listener; }
    
	//! \brief The guts of the class.
    virtual void writeImage();
    virtual void writeBCB(uint32_t bcb_start_sector=0 , uint32_t start_sector=0);

	virtual bool isFirmwarePartitionValid();
    virtual bool ReadPartitionFromDisk();
	virtual uint32_t getFirmwarePartitionBlockCount();
	bool m_bRaw;
	__int64 m_offset;
	__int64 m_skip;

protected:
	virtual PART_ENTRY * findFirmwarePartitionEntry(PARTITION_TABLE * mbr);
	virtual void getFirmwarePartitionSize(uint32_t * startBlock, uint32_t * blockCount);
    virtual void getExtraPartitionSize(uint32_t * startBlock, uint32_t * blockCount);
	virtual void getFatPartitionSize(uint32_t * startBlock, uint32_t * blockCount);
    virtual void setFirmwarePartitionEntry(PART_ENTRY * entry);
    virtual void setExtraPartitionEntry(PART_ENTRY * entry);

	virtual void prepareFatFormatter();
	
	virtual void writeLDT(uint32_t start_sector=0);
	virtual void writeFirmware(uint32_t start_sector=0);
    virtual void writeExtraData(uint32_t start_sector=0);
	virtual void writeMBR();
	virtual void writeFatPartition();
    virtual void writeConfigBlock();
 
    PARTITION_TABLE m_DiskPartition;

protected:
	imx_rom_version_t m_romVersion;	//!< Version of ROM for which the drive will be formatted.
	bool m_alwaysFormat;	//!< If false, can skip formatting if there's room.
	bool m_writeFAT32;		//!< Always format as FAT32?
	CStBlockDevice * m_device;		//!< The device we're writing to.
	CStFwComponent * m_firmware;	//!< Firmware to write to disk.
    CStExtraComponent * m_extra;    //!< Write extra data to 3rd partition
    wince_version_t  m_WinCEVersion;    //!< wince version which decides CE7 or CE6 image?
	unsigned m_extraBlocks;	//! Extra blocks to reserve in the firmware partition.
	uint32_t m_blockSize;	//!< Bytes per block for the device.
	uint32_t m_blockCount;	//!< Total number of blocks of device.
	std::string m_volumeLabel;	//!< New volume name used for formatting FAT partition.
	CStFatPartitionFormatter * m_formatter;	//!< The FAT partition formatter instance.
	Listener * m_listener;	//!< An object to receive progress updates.
    bool m_bImage;          //!< Readymade image to burn 
    bool m_bRedundantBoot;  //!< Program Config block at first sector of firmware partition, two copies of firmware inside firmware partition
    bool m_bBCBBoot;        //!< Program config block at last sector of device instead of an MBR.
protected:
	
#pragma pack(push,1)

	//! \brief Logical drive table.
	//!
	//! The first four words of the LDT are defined, and the rest of the
	//! LDT sector is zeroed.
	typedef struct {
		uint8_t m_signature[4];
		uint32_t m_version;
		uint32_t m_bootSector;
		uint32_t m_reserved;
	} ldt_t;
	
#pragma pack(pop)

};

#endif // _CStCFBootImager_h_
