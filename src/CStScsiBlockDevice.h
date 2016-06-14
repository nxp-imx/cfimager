/*
 * File: CStScsiBlockDevice.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#if !defined(_CStScsiBlockDevice_h_)
#define _CStScsiBlockDevice_h_

#include "CStBlockDevice.h"
#include "stscsi.h"

#if 0
#pragma pack (push, 1)

typedef struct _INQUIRYDATA {
    UCHAR DeviceType : 5;
    UCHAR DeviceTypeQualifier : 3;
    UCHAR DeviceTypeModifier : 7;
    UCHAR RemovableMedia : 1;
    UCHAR Versions;
    UCHAR ResponseDataFormat : 4;
    UCHAR HiSupport : 1;
    UCHAR NormACA : 1;
    UCHAR ReservedBit : 1;
    UCHAR AERC : 1;
    UCHAR AdditionalLength;
    UCHAR Reserved[2];
    UCHAR SoftReset : 1;
    UCHAR CommandQueue : 1;
    UCHAR Reserved2 : 1;
    UCHAR LinkedCommands : 1;
    UCHAR Synchronous : 1;
    UCHAR Wide16Bit : 1;
    UCHAR Wide32Bit : 1;
    UCHAR RelativeAddressing : 1;
    UCHAR VendorId[8];
    UCHAR ProductId[16];
    UCHAR ProductRevisionLevel[4];
    UCHAR VendorSpecific[20];
    UCHAR Reserved3[40];
} INQUIRYDATA, *PINQUIRYDATA;

#pragma pack (pop)
#endif

/*!
 *
 */
class CStScsiBlockDevice : public CStBlockDevice
{
public:
    CStScsiBlockDevice(CStDeviceReference * deviceRef);
    virtual ~CStScsiBlockDevice();
    
    //! \name Device information
    //@{
    virtual unsigned getBlockSize() const { return m_blockSize; }
    virtual uint32_t getBlockCount() const { return m_blockCount; }
    virtual std::string getVendorID() const { return m_vendorID; }
    virtual std::string getProductID() const { return m_productID; }
    virtual bool isMediaRemovable() const { return m_isRemovable; }
    //@}
    
    //! \name Open/close
    //@{
    virtual void open();
    virtual void close();
    virtual bool isOpen();
    //@}
    
    //! \name I/O
    //@{
    virtual void readBlocks(uint32_t firstBlock, uint32_t blockCount, void * buffer);
    virtual void writeBlocks(uint32_t firstBlock, uint32_t blockCount, const void * data);
    //@}

protected:
	CStScsi * m_scsi;		//!< The underlying SCSI interface object.
	uint32_t m_blockSize;
	uint32_t m_blockCount;
	std::string m_vendorID;
	std::string m_productID;
	bool m_isRemovable;
	
	//! \brief Pure virtual factory method.
	virtual CStScsi * createConcreteScsi()=0;
	
	virtual void performInquiry(INQUIRYDATA * inquiryData);
	virtual void performReadCapacity(READ_CAPACITY_DATA * readCapacity);
	
	std::string convertInquiryString(unsigned char inquiryString[], unsigned length);
};

#endif // _CStScsiBlockDevice_h_
