/*
 * File: ddildl_defs.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#ifndef _DDILDL_DEFS_H
#define _DDILDL_DEFS_H

#define MAX_MEDIA_TABLE_ENTRIES 10

///////////////////////////////////////////////////////////////////////////////
// Typedefs
///////////////////////////////////////////////////////////////////////////////

typedef enum {
  kMediaStateUnknown,
  kMediaStateErased,
  kMediaStateAllocated
} MEDIA_STATE, * P_MEDIA_STATE;

typedef enum {
    kMediaTypeNand = 0,
    kMediaTypeMMC = 1,
    kMediaTypeHDD = 2,
    kMediaTypeRAM = 3
} PHYSICAL_MEDIA_TYPE, * P_PHYSICAL_MEDIA_TYPE;

typedef enum {
    kMediaInfoNumberOfDrives = 0,
    kMediaInfoSizeInBytes = 1,
    kMediaInfoAllocationUnitSizeInBytes = 2,
    kMediaInfoIsInitialized = 3,
    kMediaInfoMediaState = 4,
    kMediaInfoIsWriteProtected = 5,
    kMediaInfoPhysicalMediaType = 6,
    kMediaInfoSizeOfSerialNumberInBytes = 7,
    kMediaInfoSerialNumber = 8,
    kMediaInfoIsSystemMedia = 9,
    kMediaInfoIsMediaPresent = 10
} LOGICAL_MEDIA_INFO, * P_LOGICAL_MEDIA_INFO;

#ifdef MFG_TOOL
  
typedef enum {
    SerialNoInfoSizeOfSerialNumberInBytes = 0,
    SerialNoInfoSerialNumber = 1
} SERIAL_NO_INFO, * P_SERIAL_NO_INFO;
#endif

typedef enum {
    kDriveInfoSectorSizeInBytes = 0,
    kDriveInfoEraseSizeInBytes = 1,
    kDriveInfoSizeInBytes = 2,
    kDriveInfoSizeInMegaBytes = 3,
    kDriveInfoSizeInSectors = 4,
    kDriveInfoType = 5,
    kDriveInfoTag = 6,
    kDriveInfoComponentVersion = 7,
    kDriveInfoProjectVersion = 8,
    kDriveInfoIsWriteProtected = 9,
    kDriveInfoSizeOfSerialNumberInBytes = 10,
    kDriveInfoSerialNumber = 11,
    kDriveInfoMediaPresent = 12,
    kDriveInfoMediaChange = 13,
    kDriveInfoSectorAllocation = 14,
    kDriveInfoSizeOfRawSerialNumberInBytes=15, // this & next line inserted nov07 '07 to be like main ddildl_defs.h
    kDriveInfoRawSerialNumber=16
} LOGICAL_DRIVE_INFO, * P_LOGICAL_DRIVE_INFO;

#define DRIVE_TAG_STMPSYS_S         0x00        //!< player drive tag.
#define DRIVE_TAG_HOSTLINK_S        0x01        //!< UsbMscMtp drive tag, old name was DRIVE_TAG_USBMSC_S. 
#define DRIVE_TAG_RESOURCE_BIN      0x02        //!< player resource drive tag.
#define DRIVE_TAG_EXTRA_S           0x03        //!< the host has 0x03 reserved for an extra system drive.
#define DRIVE_TAG_RESOURCE1_BIN     0x04        //!< the host has 0x04 reserved for an extra system drive.
#define DRIVE_TAG_OTGHOST_S         0x05        //!< the host has 0x05 reserved for OTG drive.
#define DRIVE_TAG_HOSTRSC_BIN       0x06        //!< UsbMscMtp resource drive tag, old name was DRIVE_TAG_MTP_BIN.
#define DRIVE_TAG_DATA              0x0A        //!< data drive tag.
#define DRIVE_TAG_DATA_HIDDEN       0x0B        //!< hidden data drive tag, old name was DRIVE_TAG_HIDDEN
#define DRIVE_TAG_BOOTMANAGER_S     0x50        //!< boot manager drive tag
#define DRIVE_TAG_UPDATER_S         0xFF        //!< the host has 0xFF reserved for usbmsc.sb file used in recovery mode operation only.

// Do not use this enum... use the defs above.  We need to use defs so customers
//  may extend the system drives without DDI source code.
/*
typedef enum {
    ResourceBinDriveTag = 0x00,
    BootManagerDriveTag = 0x50,
    StmpSysDriveTag = 0x01,
    UsbMscDriveTag = 0x02,
    DataDriveTag = 0x0A
} LOGICAL_DRIVE_TAG, * P_LOGICAL_DRIVE_TAG;
*/

typedef enum {
    kDriveTypeData = 0,
    kDriveTypeSystem = 1,
    kDriveTypeHidden = 2,
    kDriveTypeUnknown = 3
} LOGICAL_DRIVE_TYPE, * P_LOGICAL_DRIVE_TYPE;

typedef struct {
    WORD wDriveNumber;          // In reference to the entire system
    LOGICAL_DRIVE_TYPE Type;
    WORD wTag;
    DWORD dwSizeInBytes;
    BOOL bRequired;
} MEDIA_ALLOCATION_TABLE_ENTRY, * P_MEDIA_ALLOCATION_TABLE_ENTRY;

typedef struct {
    WORD wNumEntries;
    MEDIA_ALLOCATION_TABLE_ENTRY Entry[MAX_MEDIA_TABLE_ENTRIES];
} MEDIA_ALLOCATION_TABLE, * P_MEDIA_ALLOCATION_TABLE;

 
typedef union {

    struct {
        WORD MinorL      : 8;
        WORD MinorH      : 8;
        WORD MiddleL     : 8;
        WORD MiddleH     : 8;
        WORD MajorL      : 8;
        WORD MajorH      : 8;
        } PARTIAL_VERSION;
        
    DWORD   Version;        

} SYSTEM_VERSION;


#endif // #ifndef _DDILDL_DEFS_H
