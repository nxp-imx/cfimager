/*!
 * \file cfimager.cpp
 *
 * Command line interface for CF Boot Imager tool.
 */
/*
 * File: cfimager.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stdafx.h"
#include <stdio.h>
#include <exception>
#include <string>
#include <vector>
#include <stdexcept>
#include "CStDriveLetter.h"
#include "CStNTScsiBlockDevice.h"
#include "CStCFBootImager.h"
#include "st_error.h"
#include "version.h"

//! Sentinel value for default extra space.
#define USE_DEFAULT_EXTRA (-1)

#define ONE_MB (1 * 1024 * 1024)		//!< 1 MB in bytes
#define DEFAULT_EXTRA_KB (1 * 1024)		//!< 1 MB in KB

//! Format string for usage text. Expects one parameter, the executable name.
const char k_usage[] = "Usage: %s [options]\n\
Options:\n\
  -h, -?, -help                 Show this help\n\
  -v, -version                  Display the version\n\
  -f, -file <path>              Input firmware file\n\
  -d, -drive <char>             Drive letter (no colon, case does not matter)\n\
  -a, -always-format            Always reformat entire drive\n\
  -x, -extra <int>              Extra kilobytes in firmware region\n\
  -e, -extra-image <path>       Extra (3rd) partition image file\n\
  -n, -no-format                Do not format the FAT32 partition\n\
  -i, -info                     Show info about the drive, do not format\n\
  -TA3                          Format drive for TA3 ROM only\n\
  -TA4                          Format drive for TA4+ ROM only\n\
  -imx51                        Format driver for i.MX51 ROM only\n\
  -imx53                        Format driver for i.MX53 ROM only\n\
  -img, -bin                    Preformatted image is provided, dump it on the device starting at block 0\n\
  -redundant_boot, -dual_boot, \n\
  -secondary_boot               Format the drive for secondary boot with primary and secondary image and config block\n\
  -bcb, -BCB                    Format the drive for BCB boot\n\
  -raw                          Write Image to physical location\n\
  -offset						physical location offset, must be used with -raw\n\
  -skip                         skip how many byte of firmware image, must be used with -raw\n\
                                typical usage for linux uboot.bin is \n\
                                   cfimager -raw -offset 0x400 -skip 0x400 -f uboot.bin -d G\n\
\n\
The -f and -d options are required. All other options are optional. By default,\n\
the tool will not reformat the entire drive. If it cannot place the firmware\n\
without reformatting, it will display a message. You can then run the tool\n\
again using the -always-format option.\n\
\n\
\n\
The -e option used created 3rd partition with extra image file such ext3 for linux\
boot image create\
\n\
By default, the drive will be formatted so that it can work on both TA3 and\n\
TA4 (and above) devices. Using the -TA3 or -TA4 switch will cause the drive\n\
to be formatted so that it works with only that ROM version.\n\
\n\
Based on the -imx51 option and the name of the nb0 file, the address to flash\n\
the i.MX51 file on the card is determined automatically by cfimager\n\
\n\
The -no-format option causes the tool to skip writing the FAT partition, but\n\
still writes the corresponding partition entry in the MBR. This lets you use\n\
the operating system's native formatter.\n";

//! Version string
const char k_version[] = PRODUCT_STRING_VERSION;

//! Copyright string
const char k_copyright[] = "Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.";

//! Error message for when there is no room for the firmware without a
//! full format.
const char k_no_room[] = "\nThe firmware is larger than the space currently allocated \
for it, or there is not already a firmware partition. You may run this tool again \
with the -always-format option to reformat the entire disk. But, this will erase \
the FAT partition. Please remember to back up the drive contents before formatting.\n";

/*!
 * \brief Class to handle command line for the CF Imager tool.
 *
 * This class processes the command line and manages the top-level
 * logic of the CF Boot Imager.
 */
class CFBootImagerTool : public CStCFBootImager::Listener
{
public:
	//! \brief Constructor.
	//!
	//! Fills the #m_arguments vector with the elements of the argv array.
    CFBootImagerTool(int argc, const char * argv[])
    :	m_driveLetter(0),
    	m_alwaysFormat(false),
    	m_skipFatFormat(false),
		m_romVersion(CStCFBootImager::kSTMP3600ROMVersion_All),
		m_showInfo(false),
    	m_extraKilobytes(USE_DEFAULT_EXTRA),
    	m_stageCount(0), 
        m_bImage(false),
        m_bRedundantBoot(false),
        m_bBCBBoot(false),
		m_offset(0),
		m_bRaw(0),
		m_skip(0)
    {
    	// fill argument vector with values
        int i;
        for (i=0; i < argc; ++i)
        {
            std::string arg(argv[i]);
            m_arguments.push_back(arg);
        }
    }
    
    //! The core logic of the tool is handled here.
    int run()
    {
        try
        {
            // deal with arguments
            int result = processArguments();
            if (result != 0)
            {
                return result;
            }
            
            checkArguments();
            
            // create and open device
            CStDriveLetter drive(m_driveLetter);
            CStNTScsiBlockDevice device(&drive);
            device.openPhysicalDrive();
            
			printf("drive = %c\n", m_driveLetter);
            //printf("vendor = %s\n", device.getVendorID().c_str());
            //printf("product = %s\n", device.getProductID().c_str());
            printf("removable = %s\n", device.isMediaRemovable() ? "yes" : "no");
            printf("device block size = %u bytes\n", device.getBlockSize());
            printf("device block count = %#x\n\n", device.getBlockCount());
			
			// check if media is removable
//			if (!m_showInfo && !device.isMediaRemovable())
//			{
//				throw std::runtime_error("media is not removable");
//			}
			
            CStFwComponent firmware(m_filename,"CStFwComponent");
			// create firmware instance
            unsigned extraBlocks=0;
            if( !m_filename.empty() )
            {
			    
		    	THROW_IF_ST_ERROR( firmware.GetLastError());
	    		uint64_t firmwareSize = firmware.GetSizeInBytes();
	    		if (firmwareSize == 0)
	    		{
	    			throw std::runtime_error("empty firmware file");
	    		}
			
               	uint64_t firmwareBlocks = firmware.GetSizeInSectors(device.getBlockSize());
	    		printf("firmware size = %#x bytes (%#x blocks)\n", (uint32_t)firmwareSize, (uint32_t)firmwareBlocks);
		
            
			    // compute extra space in blocks
			    extraBlocks = computeExtraBlocks(device.getBlockSize(), firmwareSize);
			    printf("extra blocks = %u\n", extraBlocks);
			}
            
            CStExtraComponent extra(m_extra_filename,"CStFwComponent");
            if( !m_extra_filename.empty())
            {
                if(extra.GetSizeInBytes() == 0)
                    throw std::runtime_error("empty extra file");
            }
			// now do the real work
			try
			{
				CStCFBootImager imager;
				imager.setROMVersion(m_romVersion);
				imager.setListener(this);
				imager.setDevice(&device);
				imager.setFirmware(&firmware);
                imager.setExtra(&extra);
				imager.setAlwaysFormat(m_alwaysFormat);
				imager.setWriteFAT32(!m_skipFatFormat);
                imager.setWinCEVersion(m_WinCEVersion);
				imager.setExtraBlocks(extraBlocks);                
                imager.Image(m_bImage);
                imager.RedundantBoot(m_bRedundantBoot);
                imager.BCBBoot(m_bBCBBoot);
				imager.m_bRaw = m_bRaw;
				imager.m_offset = m_offset;
				imager.m_skip = m_skip;
				
				// handle info option
				if (m_showInfo)
				{
					bool isValid = imager.isFirmwarePartitionValid();
					printf("firmware partition valid = %s\n", isValid ? "yes" : "no");
					if (isValid)
					{
						printf("firmware partition block count = %#x\n", imager.getFirmwarePartitionBlockCount());
					}
				}
				else
				{
					// write firmware to device
					imager.writeImage();
					device.unmount();
					printf("done!");
				}
			}
			catch (CStCFBootImager::no_firmware_room_error &)
			{
				// tell user what's happening
				printf(k_no_room);
			}
            
            device.close();
        }
        catch (std::exception & e)
        {
            printf("Exception found: %s\n", e.what());
            return 1;
        }
        catch (...)
        {
            printf("Error: unexpected exception\n");
            return 1;
        }
        
        return 0;
    }
    
	virtual void setStageCount(int count)
	{
		m_stageCount = count;
	}
	
	virtual void setStage(CStCFBootImager::imaging_stage_t stage)
	{
		switch (stage)
		{
			case CStCFBootImager::kWritingLDTStage:
				printf("Writing LDT...\n");
				break;
			case CStCFBootImager::kWritingFirmwareStage:
				printf("Writing firmware...\n");
				break;
			case CStCFBootImager::kWritingMBRStage:
				printf("Writing MBR...\n");
				break;
			case CStCFBootImager::kWritingFatPartitionStage:
				printf("Writing FAT partition...\n");
				break;
            case CStCFBootImager::kWritingExtraData:
                printf("Writing Extra Data ...\n");
                break;
		}
	}

protected:
    
    //! Prints the help text.
    void printUsage()
    {
        printf(k_usage, m_arguments[0].c_str());
    }
    
    //! Print the version information
    void printVersion()
    {
    	printf("%s %s\n%s", m_arguments[0].c_str(), k_version, k_copyright);
    }
    
	//! Process the command line and set member variables based on
	//! arguments the user provided.
    int processArguments()
    {
        // print help if no arguments are provided. size should always be at
        // least 1 because the executable name is the first argument.
        if (m_arguments.empty() || m_arguments.size() == 1)
        {
            printUsage();
            return 1;
        }
        
        string_vector_t::const_iterator it = m_arguments.begin();
        ++it;   // skip process name
        for (; it != m_arguments.end(); ++it)
        {
            const std::string & arg = *it;
            
            if (arg == "-h" || arg == "-?" || arg == "-help")
            {
            	printUsage();
            	return 1;
            }
            else if (arg == "-v" || arg == "-version")
            {
            	printVersion();
            	return 1;
            }
            else if (arg == "-f" || arg == "-file")
            {
            	if (++it == m_arguments.end())
             	{
            		throw std::runtime_error("missing argument");
            	}
            	
				m_filename = *it;
            }
            else if (arg == "-d" || arg == "-drive")
            {
            	if (++it == m_arguments.end())
            	{
            		throw std::runtime_error("missing argument");
            	}
            	
            	// argument must not be an empty string
            	if (it->empty())
            	{
            		throw std::runtime_error("empty argument");
            	}
            	
            	// get first letter of whatever the argument is
				m_driveLetter = it->at(0);
				
				// convert to uppercase
				if (m_driveLetter >= 'a' && m_driveLetter <= 'z')
				{
					m_driveLetter = m_driveLetter - 'a' + 'A';
				}
            }
            else if (arg == "-a" || arg == "-always-format")
            {
            	m_alwaysFormat = true;
            }
            else if (arg == "-x" || arg == "-extra")
            {
            	if (++it == m_arguments.end())
             	{
            		throw std::runtime_error("missing argument");
            	}
            	
            	m_extraKilobytes = strtol(it->c_str(), NULL, 0);
            }
            else if (arg == "-e" || arg == "-extra-image")
            {
                if (++it == m_arguments.end())
             	{
            		throw std::runtime_error("missing argument");
            	}
            	m_extra_filename = *it;
            }
            else if (arg == "-n" || arg == "-no-format")
            {
            	m_skipFatFormat = true;
            }
			else if (arg == "-i" || arg == "-info")
			{
				m_showInfo = true;
			}
			else if (arg == "-TA3")
			{
				m_romVersion = CStCFBootImager::kSTMP3600ROMVersion_TA3;
			}
			else if (arg == "-TA4")
			{
				m_romVersion = CStCFBootImager::kSTMP3600ROMVersion_TA4;
			}
            else if (arg == "-imx51" || arg == "-iMX51" || arg == "-IMX51")
            {
                m_romVersion = CStCFBootImager::kiMX51ROMVersion;
            }
            else if (arg == "-imx53" || arg == "-iMX53" || arg == "-IMX53")
            {
                m_romVersion = CStCFBootImager::kiMX53ROMVersion;
            }
            else if (arg == "-CE7")
            {
                m_WinCEVersion = CStCFBootImager::kWinCE7;
            }
            else if (arg == "-CE6")
            {
                m_WinCEVersion = CStCFBootImager::kWinCE6;
            }
            else if (arg == "-img" || arg == "-bin")
            {
            	if (++it == m_arguments.end())
             	{
            		throw std::runtime_error("missing argument");
            	}
            	
                m_bImage = true;
				m_filename = *it;
            }
            else if (arg == "-redundant_boot" || arg == "-dual_boot" || arg == "-secondary_boot")
            {
                m_bRedundantBoot = true;
            }
            else if (arg == "-bcb" || arg == "-BCB")
            {
                m_bBCBBoot = true;
                m_bRedundantBoot = true; // This has to be set for BCB boot
            }
            else if (arg == "-raw")
			{
				m_bRaw = true;
			}
			else if (arg == "-offset")
			{
				if (++it == m_arguments.end())
             	{
            		throw std::runtime_error("missing argument");
            	}
				if(!m_bRaw)
					throw std::runtime_error("-raw must set if use -offset");
				
				m_offset = Ctol(*it);
				
			}
			else if (arg == "-skip")
			{
				if (++it == m_arguments.end())
             	{
            		throw std::runtime_error("missing argument");
            	}
				if(!m_bRaw)
					throw std::runtime_error("-raw must set if use -skip");
				
				m_skip = Ctol(*it);
			}
			else
            {
                printf("Invalid argument: %s\n", arg.c_str());
                return 1;
            }
        }
        
        return 0;
    }
    
    //! Perform basic range checking and other validations on user-specified
    //! arguments. Any errors will generate an exception.
    void checkArguments()
    {
    	// check filename
        if (m_filename.empty() && m_extra_filename.empty())
    	{
    		throw std::runtime_error("empty filename");
    	}
    	
    	// check drive letter
    	if (!(m_driveLetter >= 'A' && m_driveLetter <= 'Z'))
    	{
    		throw std::runtime_error("invalid drive letter");
    	}
    }
	
	//! Calculate the number of extra blocks to use. If #m_extraKilobytes
	//! is set to #USE_DEFAULT_EXTRA, then enough blocks to round up to the
	//! next megabyte are used.
	unsigned computeExtraBlocks(unsigned blockSize, uint64_t firmwareSize)
	{
	     // no extra blocks needed for any i.MX51 and i.MX53 image 
        if ((m_romVersion == CStCFBootImager::kiMX51ROMVersion)
           ||(m_romVersion == CStCFBootImager::kiMX51ROMVersion))
                 return 0;
            
		uint64_t extraBytes;
		if (m_extraKilobytes == USE_DEFAULT_EXTRA)
		{
			uint64_t prePartitionSize = (firmwareSize + blockSize * 2);
			uint64_t megabytes = (prePartitionSize - 1) / ONE_MB + 1;
			megabytes *= 1024 * 1024;	// convert to bytes
			extraBytes = megabytes - prePartitionSize;
		}
		else
		{
		    extraBytes = m_extraKilobytes * 1024;
		}
		
		return static_cast<unsigned>((extraBytes - 1) / blockSize + 1);
	}
	__int64 Ctol(const std::string &str)
	{
		if(str.find("0x") == 0)
		{
			char *p;
			const char *ps=str.c_str();
			return strtoul(ps+2,&p, 16);
		}else
		{
			return _atoi64(str.c_str());
		}
	}
protected:
	//! Simple vector of strings.
    typedef std::vector<std::string> string_vector_t;
    
    string_vector_t m_arguments;	//!< Command line arguments.
    std::string m_filename;			//!< Path to firmware file.
    std::string m_extra_filename;    //!< Path to extra 3rd partition image file.
    char m_driveLetter;		//!< Drive letter to image.
    bool m_alwaysFormat;	//!< Always lay down new FAT partition?
    bool m_skipFatFormat;	//!< Should the FAT partition actually be written (even if it is allocated)?
    CStCFBootImager::wince_version_t  m_WinCEVersion;    //!< wince version which decides CE7 or CE6 image?
	CStCFBootImager::imx_rom_version_t m_romVersion;	//!< Use TA3 or TA4 format?
	bool m_showInfo;		//!< Turn on info mode?
    int m_extraKilobytes;	//!< Extra KB to reserve after firmware. -1 means default.
    int m_stageCount;		//!< Number of imaging stages. Set through callback from CStCFBootImager.
    bool m_bImage;          //!< The image to burn byte by byte
    bool m_bRedundantBoot;  //!< Device supports redundant boot, config block present at first block of firmware partition
    bool m_bBCBBoot;        //!< Program BCB on last sector instead of MBR
	bool m_bRaw;
	__int64 m_offset;
	__int64 m_skip;
};

//! Entry point. Just creates a CFBootImagerTool and calls
//! CFBootImagerTool::run().
int main(int argc, const char * argv[])
{
    CFBootImagerTool tool(argc, argv);
    return tool.run();
}

