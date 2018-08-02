/*
 * File: StExtraComponent.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#pragma once
#include "stbase.h"

#include "stheader.h"
#include "stbase.h"
#include <iostream>
#include "stversioninfo.h"
#include "CStBlockDevice.h"

using namespace std;

class CStExtraComponent :
    public CStBase
{

public:

    CStExtraComponent(std::string & firmwareFilename, std::string extraFileName="");
	CStExtraComponent(const CStExtraComponent& comp);
	CStExtraComponent& operator=(const CStExtraComponent& comp);
	virtual ~CStExtraComponent();

	uint64_t GetSizeInBytes();
	uint64_t GetSizeInSectors(uint32_t sector_size);

    void WriteToDisk(int32_t start_block, CStBlockDevice * pDevice, uint64_t input_skip_byte=0);

    void CStExtraComponent::GetExtraFilename(std::string& outFilename);

	void set_extra_reserved_size(uint64_t x) { m_extra_reserved_size = x; }
private:
    ST_ERROR PrepareData();
    std::string             m_extra_filename;
    uint64_t                m_extra_filesize; 
	uint64_t				m_extra_reserved_size;
};
