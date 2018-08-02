/*
 * File: StExtraComponent.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stglobals.h"
#include "ddildl_defs.h"
#include "stbytearray.h"
#include "stversioninfo.h"
//#include "stconfiginfo.h"
#include "stfwcomponent.h"
#include "StExtraComponent.h"

#define MAX_CHARS_PER_LINE			80
#define EXTRA_RESERVERD_SIZE (0x100000)  

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStExtraComponent::CStExtraComponent(std::string & extraFileName, 
							   string _name):CStBase(_name)
{
	m_extra_filename = extraFileName;

    m_extra_filesize = 0;

	m_extra_reserved_size = EXTRA_RESERVERD_SIZE;

	m_last_error	= PrepareData();
}

CStExtraComponent::CStExtraComponent(const CStExtraComponent& _fw):CStBase( _fw )
{
	*this = _fw;
}

CStExtraComponent& CStExtraComponent::operator=(const CStExtraComponent& _fw)
{
	m_extra_filename = _fw.m_extra_filename;
    m_extra_filesize = _fw.m_extra_filesize;

	return *this;	
}

CStExtraComponent::~CStExtraComponent()
{

}

void CStExtraComponent::GetExtraFilename(std::string& outFilename)
{
	outFilename = m_extra_filename;
}

ST_ERROR CStExtraComponent::PrepareData()
{
	uint8_t *binary_data=NULL;
	size_t			length=0, index=0;
	ST_ERROR 		err=STERR_NONE;
	string			filename, line;
	
	//
	// get the file name from ConfigInfo object.
	//
	GetExtraFilename(filename);

	//
	// open the file in binary mode. return error on fail to open or read.
	//
	ifstream fw_file(filename.c_str(), ios::in | ios::binary);
	if(fw_file.fail())
	{
		err = STERR_FAILED_TO_OPEN_FILE;
		return err;
	}
    
    fw_file.seekg( 0 , std::ios::end );

    m_extra_filesize = fw_file.tellg();
    return err;
}

uint64_t CStExtraComponent::GetSizeInBytes()
{
	return m_extra_filesize + m_extra_reserved_size;
}

uint64_t CStExtraComponent::GetSizeInSectors(uint32_t sector_size)
{
	uint64_t sectors=0;

	if( !sector_size )
	{
		return sectors;
	}

	sectors = GetSizeInBytes() / sector_size;
	if( GetSizeInBytes() % sector_size )
	{
		sectors += 1;
	}

	return sectors;
}

void CStExtraComponent::WriteToDisk(int32_t start_block, CStBlockDevice * pDevice, uint64_t input_skip_byte)
{
    int buffersize;
    buffersize = 0x100000;

    char *buff =new char[buffersize];
    
    uint8_t *binary_data=NULL;
	size_t			length=0, index=0;
	ST_ERROR 		err=STERR_NONE;
	string			filename, line;
	
	//
	// get the file name from ConfigInfo object.
	//
	GetExtraFilename(filename);

	//
	// open the file in binary mode. return error on fail to open or read.
	//
	ifstream fw_file(filename.c_str(), ios::in | ios::binary);
	if(fw_file.fail())
	{
		err = STERR_FAILED_TO_OPEN_FILE;
        printf("Open file %s failure\n",filename.c_str());

		return ;
	}
    
	uint64_t i=0;
    uint64_t precent, old;
    precent= 0;
	old = 1000;

	fw_file.seekg(input_skip_byte, ifstream::beg);
	
	DWORD start = GetTickCount();

	uint64_t total_blocks = this->m_extra_filesize;
	total_blocks += pDevice->getBlockSize() - 1;
	total_blocks /= pDevice->getBlockSize();

    while(1)
    {
		fw_file.read(buff, buffersize);
		if (fw_file.gcount() == 0)
			break;

		uint64_t sz = fw_file.gcount();
		sz += pDevice->getBlockSize() - 1;
		sz /= pDevice->getBlockSize();
		
        pDevice->writeBlocks(start_block+i, sz,buff);
        i += sz;

        precent = (i)*100 / total_blocks;
        if(precent != old)
        {
            printf("write %d%%\r", precent);
            old =precent;
        }
    }

	DWORD end = GetTickCount();

	uint64_t speed;
	if (end - start == 0)
		speed = 1000;
	else
		speed = (total_blocks * pDevice->getBlockSize()) * 1000 / (end - start);

	printf("\n\nTotal: %d.%03ds  speed:%lld.%03lldMB/s", (end - start) / 1000, (end - start) % 1000,
		speed / 10000000, speed / 1000);
    delete buff;

}