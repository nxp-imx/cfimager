/*
 * File: CStDriveLetter.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/

#if !defined(_CStDriveLetter_h_)
#define _CStDriveLetter_h_

#include "CStDeviceReference.h"

/*!
 *
 */
class CStDriveLetter : public CStDeviceReference
{
public:
    CStDriveLetter();
    CStDriveLetter(char letter);
    CStDriveLetter(wchar_t letter);
    virtual ~CStDriveLetter();
    
    char getDriveLetter() const { return m_driveLetter; }
    void setDriveLetter(char letter) { m_driveLetter = letter; }
    
    virtual bool isRemovable() const;
    
    inline operator char () const { return getDriveLetter(); }
    inline operator wchar_t () const { return static_cast<wchar_t>(getDriveLetter()); }
    
    inline char operator = (char letter) { setDriveLetter(letter); }
    inline wchar_t operator = (wchar_t letter) { setDriveLetter(static_cast<char>(letter)); }

protected:
	char m_driveLetter;
};

#endif // _CStDriveLetter_h_
