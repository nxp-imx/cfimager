/*
 * File: stfwcomponent.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stglobals.h"
#include "ddildl_defs.h"
#include "stbytearray.h"
#include "stversioninfo.h"
#include "stfwcomponent.h"

#define MAX_CHARS_PER_LINE			80

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStFwComponent::CStFwComponent(std::string & firmwareFilename, 
							   string _name):CStBase(_name)
{
	m_firmware_filename = firmwareFilename;
    
    m_p_arr_data	= NULL;

	m_last_error = ExtractVersionInformation();
	if (m_last_error == STERR_NONE)
	{
		m_last_error	= PrepareData();
	}
}

CStFwComponent::CStFwComponent(const CStFwComponent& _fw):CStBase( _fw )
{
	*this = _fw;
}

CStFwComponent& CStFwComponent::operator=(const CStFwComponent& _fw)
{
	if( m_p_arr_data )
	{
		delete m_p_arr_data;
	}

	uint8_t byte;
	
	m_p_arr_data			= new CStByteArray(_fw.m_p_arr_data->GetCount());
	
	size_t index;
	for( index=0; index < _fw.m_p_arr_data->GetCount(); index++)
	{
		_fw.m_p_arr_data->GetAt(index, byte);
		m_p_arr_data->SetAt(index, byte);
	}

	m_project_version		= _fw.m_project_version;
	m_component_version		= _fw.m_component_version;
	m_version_status		= _fw.m_version_status;
	
	m_last_error			= _fw.m_last_error;
	m_system_last_error		= _fw.m_system_last_error;
	m_obj_name				= _fw.m_obj_name;

	return *this;	
}

CStFwComponent::~CStFwComponent()
{
	if( m_p_arr_data )
		delete m_p_arr_data;
}

void CStFwComponent::GetFirmwareFilename(std::string& outFilename)
{
	outFilename = m_firmware_filename;
}

ST_ERROR CStFwComponent::PrepareData()
{
	uint8_t *binary_data=NULL;
	size_t			length=0, index=0;
	ST_ERROR 		err=STERR_NONE;
	string			filename, line;
	
	//
	// get the file name from ConfigInfo object.
	//
	GetFirmwareFilename(filename);

	//
	// open the file in binary mode. return error on fail to open or read.
	//
	ifstream fw_file(filename.c_str(), ios::in | ios::binary);
	if(fw_file.fail())
	{
		err = STERR_FAILED_TO_OPEN_FILE;
		goto done;
	}

	//
	// Create the buffer and read the binary data from the file.
	//
	err = ExtractBinaryData(fw_file, &binary_data, length);
	if( err != STERR_NONE )
	{
		goto done;
	}

	//
	// construct the member array variable to save the decrypted data. 
	//
	m_p_arr_data = new CStByteArray(length);
	if( !m_p_arr_data )
	{
		err = STERR_NO_MEMORY;
		goto done;
	}

	//
	// m_p_arr_data will always hold the binary data ready to tranfer on a system drive image.
	//
	for(index=0; index<length; index++)
	{
		m_p_arr_data->SetAt(index, binary_data[index]);
	}

done:

	if( binary_data )
	{
		delete[] binary_data;
	}
	return err;
}

CStByteArray* CStFwComponent::GetData()
{
	return m_p_arr_data;
}

uint64_t CStFwComponent::GetSizeInBytes()
{
	return m_p_arr_data->GetCount();
}

uint64_t CStFwComponent::GetSizeInSectors(uint32_t sector_size)
{
	uint64_t sectors=0;

	if( !sector_size )
	{
		return sectors;
	}

	sectors = m_p_arr_data->GetCount() / sector_size;
	if( m_p_arr_data->GetCount() % sector_size )
	{
		sectors += 1;
	}

	return sectors;
}

ST_ERROR CStFwComponent::GetProjectVersion(CStVersionInfo& _ver)
{
	_ver = m_project_version;
	return STERR_NONE;
}

ST_ERROR CStFwComponent::GetComponentVersion(CStVersionInfo& _ver)
{
	_ver = m_component_version;
	return STERR_NONE;
}

ST_ERROR CStFwComponent::ExtractVersionInformation()
{
	ST_ERROR 					err = STERR_NONE;
	string						filename;
	
	//
	// get filename from configinfo object.
	//
	GetFirmwareFilename(filename);
	
	//
	// open the file in binary mode. return error on fail to open or read.
	//
	ifstream fw_file(filename.c_str(), ios_base::in | ios_base::binary);
	if(fw_file.fail())
	{
		err = STERR_FAILED_TO_OPEN_FILE;
		goto done;
	}

	m_version_status = NO_VERSION_FOUND;
	fw_file.seekg(ios::beg);
	
	// read file header
	fw_file.read((char *)&m_header, sizeof(m_header));
	if (fw_file.fail())
	{
		err = STERR_FAILED_TO_READ_FILE_DATA;
		goto done;
	}
	
	m_project_version.SetHigh(static_cast<uint16_t>(m_header.m_productVersion.m_major));
	m_project_version.SetMid(static_cast<uint16_t>(m_header.m_productVersion.m_minor));
	m_project_version.SetLow(static_cast<uint16_t>(m_header.m_productVersion.m_revision));
	
	m_component_version.SetHigh(static_cast<uint16_t>(m_header.m_componentVersion.m_major));
	m_component_version.SetMid(static_cast<uint16_t>(m_header.m_componentVersion.m_minor));
	m_component_version.SetLow(static_cast<uint16_t>(m_header.m_componentVersion.m_revision));
	
	m_version_status = COMPONENT_VERSION_FOUND;

done:
	if( fw_file.is_open() )
	{
		fw_file.close();
	}

	return err;
}

ST_ERROR CStFwComponent::ExtractBinaryData(ifstream& _fw_file, uint8_t** _binary_data, size_t& _len)
{
	ST_ERROR err = STERR_NONE;
	uint8_t *data = NULL;

	filebuf *p_buf = _fw_file.rdbuf();
	
	uint32_t size = p_buf->pubseekoff (0, ios::end, ios::in);
	p_buf->pubseekpos (0,ios::in);

	data = new uint8_t[size];
	
	p_buf->sgetn((char*)data, size);

	*_binary_data = data;	
	_len = size;
	err = STERR_NONE;
	
	return err;
}

ST_ERROR CStFwComponent::GetData(size_t _from_offset, size_t _count, uint8_t *_p_ch)
{
	CStByteArray arr(_count);
	ST_ERROR err = STERR_NONE;

	err = GetData(_from_offset, _count, &arr);
	if( err != STERR_NONE )
		return err;
	
	return arr.Read( _p_ch, _count, 0 );
}

//////////////////////////////////////////////////////////////////////////////////////////
// should always return STERR_NONE; 
// if the _from_offset and _count are invalid then it must fill _arr with 0xFF.
//
ST_ERROR CStFwComponent::GetData(size_t _from_offset, size_t _count, CStByteArray* _p_arr)
{
	if( _from_offset > GetData()->GetCount() )
	{
		//
		// _from_offset is out of range return arr fill with 0xff.
		//
		size_t index;
		for( index = 0; index < _p_arr->GetCount(); index ++ )
		{
			uint8_t uch = 0xFF;
	
			_p_arr->SetAt(index, uch);
		}
		return STERR_NONE;
	}

	if( _count > ( GetData()->GetCount() - _from_offset ) )
	{
		//
		// data is less than asked for so fill the remaining with 0xff
		//
		size_t index;
		for( index = 0; index < ( GetData()->GetCount() - _from_offset ); index ++ )
		{
			uint8_t uch;
		
			GetData()->GetAt(index + _from_offset, uch);
			_p_arr->SetAt(index, uch);
		}

		for( ; index < _count; index ++ )
		{
			uint8_t uch = 0xFF;
	
			_p_arr->SetAt(index, uch);
		}
		return STERR_NONE;
	}

	//
	// found _from_offset and _count in range so return the right data.
	//
	size_t index;
	for( index = 0; index < _count; index ++ )
	{
		uint8_t uch;
	
		GetData()->GetAt(index + _from_offset, uch);
		_p_arr->SetAt(index, uch);
	}
	
	return STERR_NONE;
}
		
