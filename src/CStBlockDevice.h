/*
 * File: CStBlockDevice.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#if !defined(_CStBlockDevice_h_)
#define _CStBlockDevice_h_

#include "CStDeviceReference.h"
#include "stbytearray.h"

/*!
 * \brief Abstract base class for block-oriented devices.
 *
 * Defines the interface for accessing block devices. Concrete subclasses
 * implement the specifics of how the device is controlled.
 *
 * An instance of CStDeviceReference is passed to the constructor to tell the
 * class with which device you wish to communicate. A reference to this object
 * is simply retained, and later used by CStDeviceReference::open(). The
 * concrete implementation of CStDeviceReference::open() expects the device
 * reference to be a particular class and will throw an exception if the wrong
 * subclass of CStDeviceReference was used.
 */
class CStBlockDevice
{
public:
	//! \brief Constructor.
    CStBlockDevice(CStDeviceReference * deviceRef);
    
    //! \brief Destructor.
    virtual ~CStBlockDevice();
    
    inline const CStDeviceReference * deviceReference() const { return m_deviceRef; }
    
    //! \name Device information
    //@{
    virtual unsigned getBlockSize() const=0;
    virtual uint32_t getBlockCount() const=0;
    //@}
    
    //! \name Control
    //@{
    virtual void open()=0;
    virtual void close()=0;
    virtual bool isOpen()=0;
    //@}
    
    //! \name I/O
    //@{
    virtual void readBlocks(uint32_t firstBlock, uint32_t blockCount, void * buffer)=0;
    virtual void writeBlocks(uint32_t firstBlock, uint32_t blockCount, const void * data)=0;
    
    virtual void readBlocks(uint32_t firstBlock, uint32_t blockCount, CStByteArray & data);
    virtual void writeBlocks(uint32_t firstBlock, uint32_t blockCount, const CStByteArray & data);
    
    virtual void readOneBlock(uint32_t block, void * buffer);
    virtual void writeOneBlock(uint32_t block, const void * buffer);
    
    virtual void readOneBlock(uint32_t block, CStByteArray & data);
    virtual void writeOneBlock(uint32_t block, const CStByteArray & data);
    //@}
    
protected:
	CStDeviceReference * m_deviceRef;
};

#endif // _CStBlockDevice_h_
