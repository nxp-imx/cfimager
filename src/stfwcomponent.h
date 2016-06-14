/*
 * File: stfwcomponent.h
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#if !defined(AFX_STFWCOMPONENT_H__C01170F6_65CF_42C1_82D4_DA2D3BA71751__INCLUDED_)
#define AFX_STFWCOMPONENT_H__C01170F6_65CF_42C1_82D4_DA2D3BA71751__INCLUDED_

#include "stheader.h"
#include "stbase.h"
#include <iostream>
#include "stversioninfo.h"

using namespace std;

//!
typedef enum _EXTRACTVERSIONSTATUS {
	PRODUCT_VERSION_FOUND	= 0,
	COMPONENT_VERSION_FOUND = 1,
	ALL_VERSIONS_FOUND		= 2,
	NO_VERSION_FOUND		= 3
} EXTRACT_VERSION_STATUS;

#define HEADER_TAG "STMP"
#define HEADER_TAG_LENGTH (sizeof(HEADER_TAG)-1)	// remove null char

#define BOOT_LOADER_WORD_SIZE		3
#define RESET_SEQUENCE_SIZE			2
#define RESET_SEQUENCE_WORD0		0xFFFFFF
#define RESET_SEQUENCE_WORD1		0x000000 

/*!
 * Firmware component file reader.
 */
class CStFwComponent : public CStBase
{
public:
	CStFwComponent(std::string & firmwareFilename, string name="CStFwComponent");
	CStFwComponent(const CStFwComponent& comp);
	CStFwComponent& operator=(const CStFwComponent& comp);
	virtual ~CStFwComponent();

	CStByteArray  * GetData();
	ST_ERROR GetData(size_t _from, size_t _count, CStByteArray* _p_arr);
	ST_ERROR GetData(size_t _from, size_t _count, uint8_t *_p_arr);
	uint64_t GetSizeInBytes();
	uint64_t GetSizeInSectors(uint32_t sector_size);
	ST_ERROR GetProjectVersion(CStVersionInfo& ver);
	ST_ERROR GetComponentVersion(CStVersionInfo& ver);
	void GetFirmwareFilename(std::string& outFilename);
private:

#pragma pack(push,1)

	struct Version
	{
		uint32_t m_major;
		uint32_t m_minor;
		uint32_t m_revision;
	};

	struct FirstBlockHeader
	{
		uint32_t m_romVersion;
		uint32_t m_imageSize;
		uint32_t m_cipherTextOffset;
		uint32_t m_userDataOffset;
		uint32_t m_keyTransformCode;
		uint32_t m_tag[4];
		Version m_productVersion;
		Version m_componentVersion;
		uint32_t m_reserved;
	};
	
#pragma pack(pop)

	std::string				m_firmware_filename;
	CStByteArray*			m_p_arr_data;
	CStVersionInfo			m_project_version;
	CStVersionInfo			m_component_version;
	EXTRACT_VERSION_STATUS	m_version_status;
	FirstBlockHeader		m_header;
	

	ST_ERROR PrepareData ();
	ST_ERROR ExtractVersionInformation();
	ST_ERROR ExtractBinaryData(ifstream& fw_file, uint8_t** file_data, size_t& length);
};

#endif // !defined(AFX_STFWCOMPONENT_H__C01170F6_65CF_42C1_82D4_DA2D3BA71751__INCLUDED_)
