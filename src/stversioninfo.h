/*
 * File: stversioninfo.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#if !defined(AFX_STVERSIONINFO_H__72EA2FE9_CF2B_403B_91DB_A14C9C3F14F7__INCLUDED_)
#define AFX_STVERSIONINFO_H__72EA2FE9_CF2B_403B_91DB_A14C9C3F14F7__INCLUDED_

#include "stheader.h"
#include "stbase.h"
#include "stdafx.h"

/*!
 *
 */
class CStVersionInfo : CStBase
{
public:
	//! Constructor.
	CStVersionInfo(string name="CStVersionInfo");
	
	//! Copy constructor.
	CStVersionInfo(const CStVersionInfo&);
	
	//! Assignment operator.
	CStVersionInfo& operator=(const CStVersionInfo& ver);
	
	//! Destructor.
	virtual ~CStVersionInfo();
	
	//! \name Operators
	//@{
	bool operator != (CStVersionInfo& _ver);
	bool operator == (CStVersionInfo& _ver);
	//@}
	
	//! \name Accessors
	//@{
	uint16_t GetHigh() const;
	uint16_t GetMid() const;
	uint16_t GetLow() const;

	void SetHigh(uint16_t high);
	void SetMid(uint16_t Mid);
	void SetLow(uint16_t low);
	
	wstring GetVersionString();
	//@}

private:
	uint16_t	m_high;
	uint16_t	m_mid;
	uint16_t	m_low;
};

/*!
 *
 */
class CStVersionInfoPtrArray : public CStArray<class CStVersionInfo*> {
public:
	CStVersionInfoPtrArray(size_t size, string name="CStVersionInfoPtrArray");
	~CStVersionInfoPtrArray();
};

#endif // !defined(AFX_STVERSIONINFO_H__72EA2FE9_CF2B_403B_91DB_A14C9C3F14F7__INCLUDED_)
