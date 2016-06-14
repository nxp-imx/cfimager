/*
 * File: st_error.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#if !defined(_st_error_h_)
#define _st_error_h_

#include <stdexcept>
#include <stdio.h>
#include <string>
#include "sterror.h"

/*!
 * Wrapper exception class for ST_ERROR.
 */
class st_error : public std::runtime_error
{
public:
	st_error() : std::runtime_error("error"), m_error(STERR_NONE) {}
	st_error(ST_ERROR err) : std::runtime_error("error"), m_error(err) {}
	virtual ~st_error() throw() {}
	
	//! \brief Accessor for the error code.
	ST_ERROR error() const { return m_error; }
	
	//! \brief Formats a simple error message with the error code.
	virtual const char * what() const throw()
	{
		char buffer[32];
		sprintf_s(buffer, 32, "%d ", static_cast<int>(m_error));
		m_what = buffer;
		
		if(m_error>=STERR_END && m_error <=0)
			m_what += g_ERROR_STR[-m_error];

		return m_what.c_str();
	}

protected:
	ST_ERROR m_error;			//!< The error code passed to the constructor.
	mutable std::string m_what;	//!< Mutable so what() can have some storage.
};

//! Macro to make throwing an st_error easy.
#define THROW_IF_ST_ERROR(err) do { ST_ERROR error=err; if (error != STERR_NONE) \
								throw st_error(error); } while (0);

#endif // _st_error_h_
