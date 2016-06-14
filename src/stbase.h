/*
 * File: stbase.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#if !defined(AFX_STBASE_H__C6DD00FD_38FC_438D_BD15_685AE4AEDAD6__INCLUDED_)
#define AFX_STBASE_H__C6DD00FD_38FC_438D_BD15_685AE4AEDAD6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _DEBUG
#define CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

class CStBase  
{
public:

	CStBase(string name="CStBase");
	CStBase(const CStBase&);
	virtual ~CStBase();
	CStBase& operator=(const CStBase&);

	virtual ST_ERROR GetLastError() const;
	virtual long GetSystemLastError() const;
	string GetObjName() const;

protected:

	ST_ERROR	m_last_error;
	long		m_system_last_error;
	string		m_obj_name;
};


#endif // !defined(AFX_STBASE_H__C6DD00FD_38FC_438D_BD15_685AE4AEDAD6__INCLUDED_)
