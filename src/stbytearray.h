/*
 * File: stbytearray.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#if !defined(AFX_STBYTEARRAY_H__972C0C8B_9C01_409E_91BA_1C311970F8DC__INCLUDED_)
#define AFX_STBYTEARRAY_H__972C0C8B_9C01_409E_91BA_1C311970F8DC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"

class CStByteArray : public CStArray<UCHAR>  
{
public:
	CStByteArray(size_t size, string name="CStByteArray");
	~CStByteArray();
	ST_ERROR Read(USHORT&, size_t from_offset) const;
	ST_ERROR Read(ULONG&, size_t from_offset) const;
	ST_ERROR Read(ULONGLONG&, size_t from_offset) const;
	ST_ERROR Read(ST_BOOLEAN&, size_t from_offset) const;
	ST_ERROR Read(void*, size_t size, size_t from_offset) const;

	ST_ERROR Write(USHORT, size_t from_offset);
	ST_ERROR Write(ULONG, size_t from_offset);
	ST_ERROR Write(ULONGLONG, size_t from_offset);
	ST_ERROR Write(ST_BOOLEAN, size_t from_offset);
	ST_ERROR Write( const void*, size_t size, size_t from_offset);
	ST_ERROR Remove(size_t from, size_t count);

	BOOL operator!=(const CStByteArray& _arr) const;
	BOOL operator==(const CStByteArray& _arr) const;
	wstring GetAsString();
};

class CStArrayOfByteArrays : public CStArray<class CStByteArray*>
{
public:
	CStArrayOfByteArrays(size_t size, size_t _size_of_each_bytearray, string name="CStArrayOfByteArrays");
	~CStArrayOfByteArrays();
};

#endif // !defined(AFX_STBYTEARRAY_H__972C0C8B_9C01_409E_91BA_1C311970F8DC__INCLUDED_)
