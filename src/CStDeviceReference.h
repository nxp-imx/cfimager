/*
 * File: CStDeviceReference.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#if !defined(_CStDeviceReference_h_)
#define _CStDeviceReference_h_

/*!
 * Abstract base class for all references to (mass storage) devices.
 */
class CStDeviceReference
{
public:
    CStDeviceReference();
    virtual ~CStDeviceReference();
    
    virtual bool isRemovable() const { return false; }
};

#endif // _CStDeviceReference_h_
