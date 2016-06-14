/*
 * File: stscsi_nt.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stheader.h"
#include "stglobals.h"
#include "stbytearray.h"
#include "ddildl_defs.h"
#include "ddildl_defs.h"
#include "stversioninfo.h"
#include "stddiapi.h"
#include <ntddscsi.h>

#include "StScsi_Nt.h"

#define LOCK_TIMEOUT        1000       // 1 Second
#define LOCK_RETRIES        10

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

HANDLE CStScsi_Nt::m_sync_event = NULL;

CStScsi_Nt::CStScsi_Nt(char driveLetter, string _name):CStScsi(_name)
{
    m_volhandle = INVALID_HANDLE_VALUE;
	m_phyhandle = INVALID_HANDLE_VALUE;

    m_drive_letter = static_cast<wchar_t>(driveLetter);
    m_spt = NULL;
}

CStScsi_Nt::CStScsi_Nt(wchar_t driveLetter, string _name):CStScsi(_name)
{
    m_volhandle = INVALID_HANDLE_VALUE;
	m_phyhandle = INVALID_HANDLE_VALUE;
    m_drive_letter = driveLetter;
    m_spt = NULL;
}

CStScsi_Nt::~CStScsi_Nt()
{
    if( m_volhandle && ( m_volhandle != INVALID_HANDLE_VALUE ) )
    {
        CloseHandle(m_volhandle);
    }
	if( m_phyhandle && ( m_phyhandle != INVALID_HANDLE_VALUE ) )
    {
        CloseHandle(m_phyhandle);
    }
}

ST_ERROR CStScsi_Nt::Initialize()
{
//  wchar_t         DriveToOpen[] = L"\\\\.\\?:\\";
//  wchar_t         ch=L'';
//  INQUIRYDATA     InquiryData;
//  string          strSCSIProductString;
//  string          strSCSIMfgString;
//  long            Count = 0;
//  CStScsiInquiry  api_scsi_inquiry;
//  wstring         drives(L"");
// 
// //   GetUpdater()->GetConfigInfo()->GetSCSIMfgString(strSCSIMfgString);
// //   GetUpdater()->GetConfigInfo()->GetSCSIProductString(strSCSIProductString);
//  
//  drives = QueryAllLogicalDrives();
// 
//  m_drive_letter = '\0';
// 
//  for( size_t index=0; index<drives.length(); index++ )
//  {
//      ch=drives[ index ];
//      if( ch == L'A' )
//          continue;
//      
//      swprintf( DriveToOpen, L"%c:\\", ch );
//      
//      if( ::GetDriveType ( DriveToOpen ) != DRIVE_REMOVABLE )
//      {
//          continue;
//      }
// 
//      if( m_handle && (m_handle != INVALID_HANDLE_VALUE) )
//      {
//          CloseHandle( m_handle );
//          m_handle = INVALID_HANDLE_VALUE;
//      }
// 
//      swprintf( DriveToOpen, L"\\\\.\\%c:", ch );
// 
//      m_handle = CStGlobals::CreateFile(
//             DriveToOpen,                     // device interface name
//             GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
//             FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
//             NULL,                               // lpSecurityAttributes
//             OPEN_EXISTING,                      // dwCreationDistribution
//             0,                                  // dwFlagsAndAttributes
//             NULL                                // hTemplateFile
//         );
//                 
//      if( m_handle == INVALID_HANDLE_VALUE ) 
//      {
//          //CStTrace::trace( "CreateFile failed with error: %d\n", GetLastError() );
//          continue;
//      }
//      if( SendDdiApiCommand(&api_scsi_inquiry) == STERR_NONE )
//      {
//          api_scsi_inquiry.GetInquiryData(&InquiryData);
//      
//          ST_BOOLEAN product_id_match = FALSE;
//             if ( GetUpdater()->GetConfigInfo()->UseScsiProductSubstringQualifier() ) {
//                 size_t index = strSCSIProductString.find_last_not_of(' ');
//                 strSCSIProductString.resize(index+1);
//                 product_id_match = strstr( (const char*)InquiryData.ProductId, strSCSIProductString.c_str() ) != NULL;
//             }
//             else {
//                 product_id_match = !strnicmp( (const char*)InquiryData.ProductId, strSCSIProductString.c_str(), strSCSIProductString.length () );
//             }
// 
//             if( product_id_match )       
//          {
//              if( !strnicmp( (const char*)InquiryData.VendorId, strSCSIMfgString.c_str(), strSCSIMfgString.length () ) )      
//              {
//                  ST_BOOLEAN media_system=FALSE; 
//                  ST_ERROR err = STERR_NONE;
// 
//                  if( (err = IsSystemMedia(media_system)) != STERR_NONE )
//                  {
//                      // atleast log this to debug output.
// //                       CStTrace::trace( "IsSystemMedia call failed with error : %d\n", err );
// 
//                      CloseHandle( m_handle );
//                      m_handle = INVALID_HANDLE_VALUE;
//                      continue;
//                  }
//                  if(media_system)
//                  {
//                      m_drive_letter = ch;
//                      Count ++;
//                      CloseHandle(m_handle);
//                      m_handle = INVALID_HANDLE_VALUE;
//                      break;
//                  }
//              }
//          }
//      }
//      CloseHandle(m_handle);
//      m_handle = INVALID_HANDLE_VALUE;
//  }   
//  if (Count <= 0)
//  {
//      return STERR_FAILED_TO_LOCATE_SCSI_DEVICE;
//  }
    return STERR_NONE;
}

ST_ERROR CStScsi_Nt::SendCommand( 
    CStByteArray* _p_command_arr, 
    UCHAR _cdb_len, 
    BOOL _direction_out, 
    CStByteArray& _response_arr
)
{
    PSPT_WITH_BUFFERS   spt;
    size_t              data_length;
    DWORD               return_status = ERROR_SUCCESS;
    UCHAR               scsi_cmd;               
    DWORD               dwThreadId=0;
    HANDLE              thread_handle=INVALID_HANDLE_VALUE;
    DWORD               wait_result;

    if( m_volhandle == INVALID_HANDLE_VALUE )
    {
        ST_ERROR err = Open();
        if( err != STERR_NONE )
        {
            return err;
        }
    }

    _p_command_arr->GetAt(0, scsi_cmd);

    data_length = (size_t)CStGlobals::Max(_response_arr.GetCount(), 
        ( _p_command_arr->GetCount() - _cdb_len ));

    //
    // Allocate the buffer to send scsi_pass_through command.
    //
    spt = AllocateSPT(_cdb_len, _direction_out, scsi_cmd, data_length);

    if(spt == NULL)
    {
        return STERR_NO_MEMORY;
    }

    //
    // copy the command to cdb array
    //
    size_t index;
    for(index = 0; index < _cdb_len ; index ++)
    {
        _p_command_arr->GetAt(index, spt->Spt.Cdb[index]);
    }

    //
    // if command contains data copy it to the output buffer.
    //
    for(index = _cdb_len; index < _p_command_arr->GetCount(); index ++)
    {
        _p_command_arr->GetAt( index, spt->DataBuffer[index - _cdb_len] );
    }

    m_spt = spt;

    thread_handle = CreateThread(
        NULL,                        // default security attributes 
        0,                           // use default stack size  
        CStScsi_Nt::BeginSendCommandThread,      // thread function 
        this,                        // argument to thread function 
        0,                           // use default creation flags 
        &dwThreadId);  
    
    wait_result = CStGlobals::WaitForSingleObjectEx( thread_handle, 0, FALSE );
    while( wait_result != WAIT_OBJECT_0 )
    {   
//      if( GetUpdater()->GetProgress() )
//      {
//          GetUpdater()->GetProgress()->UpdateProgress(FALSE);
//      }
        wait_result = CStGlobals::WaitForSingleObjectEx( thread_handle, 0, FALSE );
    }   

    m_spt = NULL;

    if( m_system_last_error != ERROR_SUCCESS )
    {
        FreeSPT(spt);
        m_last_error = STERR_FAILED_DEVICE_IO_CONTROL;
//      GetUpdater()->GetErrorObject()->SaveStatus(this);
        m_system_last_error = 0;
        m_last_error = STERR_NONE;
        return STERR_FAILED_DEVICE_IO_CONTROL;
    }
    
    if (spt->Spt.ScsiStatus != SCSISTAT_GOOD)
    {
        return_status = !ERROR_SUCCESS; // DeviceTypeQualifier == 0x01 means the external disk is removed
        m_last_error = STERR_FAILED_TO_SEND_SCSI_COMMAND;
        SaveSenseData(spt->SenseInfoBuffer, spt->Spt.SenseInfoLength);
//      GetUpdater()->GetErrorObject()->SaveStatus(this, GetSenseData());
        m_last_error = STERR_NONE;
    }
    else
    {
        for(index=0; index<_response_arr.GetCount(); index++)
        {
            _response_arr.SetAt(index, spt->DataBuffer[index]);
        }
    }

    FreeSPT(spt);

    if(return_status == ERROR_SUCCESS)
        return STERR_NONE;
    return STERR_FAILED_TO_SEND_SCSI_COMMAND;
}

DWORD WINAPI CStScsi_Nt::BeginSendCommandThread( LPVOID pParam )
{
    CStScsi_Nt* _p_scsi_nt = (CStScsi_Nt*)pParam;
    DWORD       returned   = 0;
	HANDLE handle = _p_scsi_nt->GetPhyHandle();
	if( handle == INVALID_HANDLE_VALUE )
		handle = _p_scsi_nt->GetVolHandle();

    if(!CStGlobals::DeviceIoControl(_p_scsi_nt->GetVolHandle(),
                         IOCTL_SCSI_PASS_THROUGH,
                         _p_scsi_nt->m_spt,
                         (ULONG)_p_scsi_nt->m_spt->TotalSize,
                         _p_scsi_nt->m_spt,
                         (ULONG)_p_scsi_nt->m_spt->TotalSize,
                         &returned,
                         NULL))
    {
        _p_scsi_nt->SetSystemLastError(CStGlobals::GetLastError());
    }
    else
    {
        _p_scsi_nt->SetSystemLastError(ERROR_SUCCESS);
    }
    return 0;
}

PSPT_WITH_BUFFERS CStScsi_Nt::AllocateSPT (
    UCHAR       _cdb_len,
    BOOL        _data_out,
    UCHAR       /*_scsi_cmd*/,
    size_t      _data_size
)
{
    size_t              total_size;
    PSPT_WITH_BUFFERS   pspt;

    total_size = sizeof(SPT_WITH_BUFFERS) + _data_size;

    pspt = (PSPT_WITH_BUFFERS)LocalAlloc(LPTR, total_size); // LPTR includes LMEM_ZEROINIT

    if (pspt == NULL)
    {
        return pspt;
    }

    memset(pspt, 0, total_size);

    pspt->TotalSize = total_size;

    pspt->Spt.Length = sizeof(pspt->Spt);

    pspt->Spt.CdbLength = _cdb_len;

    pspt->Spt.SenseInfoLength = sizeof(pspt->SenseInfoBuffer);

    pspt->Spt.DataIn = (_data_out) ? SCSI_IOCTL_DATA_OUT : SCSI_IOCTL_DATA_IN;

    pspt->Spt.DataTransferLength = (ULONG)_data_size;

    pspt->Spt.TimeOutValue = 2*60;

    pspt->Spt.SenseInfoOffset =
        (DWORD)((ULONG_PTR)&pspt->SenseInfoBuffer[0] - (ULONG_PTR)pspt);

    pspt->Spt.DataBufferOffset =
        (DWORD)((ULONG_PTR)&pspt->DataBuffer[0] - (ULONG_PTR)pspt);

    return pspt;
}

//******************************************************************************
//
// FreeSPT()
//
//******************************************************************************

VOID CStScsi_Nt::FreeSPT (
    PSPT_WITH_BUFFERS _p_spt
)
{
    LocalFree((void*)_p_spt);
}

ST_ERROR CStScsi_Nt::Open()
{
    ST_ERROR err = STERR_NONE;
    if( m_volhandle == INVALID_HANDLE_VALUE )
    {
        wchar_t DriveToOpen[16];

        swprintf_s( DriveToOpen, 16, L"\\\\.\\%c:", GetDriveLetter() );
        m_volhandle = CStGlobals::CreateFile(
            DriveToOpen,                        // device interface name
            GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,                               // lpSecurityAttributes
            OPEN_EXISTING,                      // dwCreationDistribution
            FILE_FLAG_WRITE_THROUGH,            // dwFlagsAndAttributes
            NULL                                // hTemplateFile
        );
                
        if( m_volhandle == INVALID_HANDLE_VALUE ) 
        {
            //CStTrace::trace( "CreateFile failed with error: %d\n", GetLastError() );
            m_system_last_error = CStGlobals::GetLastError();
            err = STERR_INVALID_DEVICE_HANDLE;
        }
    }
    return err;
}



ST_ERROR CStScsi_Nt::Close()
{
    if( m_volhandle != INVALID_HANDLE_VALUE )
    {
        CloseHandle(m_volhandle);
        m_volhandle = INVALID_HANDLE_VALUE;
    }

	 if( m_phyhandle != INVALID_HANDLE_VALUE )
    {
        CloseHandle(m_phyhandle);
        m_phyhandle = INVALID_HANDLE_VALUE;
    }
    return STERR_NONE;
}

ST_ERROR CStScsi_Nt::Lock(BOOL /*_media_new*/, BOOL bVol)
{
    DWORD dwBytesReturned;
    DWORD dwSleepAmount;
    int nTryCount;

    dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;

    // Do this in a loop until a timeout period has expired
    for( nTryCount = 0; nTryCount < LOCK_RETRIES; nTryCount++ ) 
    {
        if (CStGlobals::DeviceIoControl(
				bVol?m_volhandle:m_phyhandle,
                FSCTL_LOCK_VOLUME,
                NULL, 0,
                NULL, 0,
                &dwBytesReturned,
                NULL 
            ) )
        {
            return STERR_NONE;
        }

        Sleep( dwSleepAmount );
    }

    return STERR_FAILED_TO_LOCK_THE_DRIVE;
}

ST_ERROR CStScsi_Nt::Unlock(BOOL /*_media_new*/, BOOL bVol)
{
    DWORD dwBytesReturned;
    DWORD dwSleepAmount;
    int nTryCount;

    dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;

    // Do this in a loop until a timeout period has expired
    for( nTryCount = 0; nTryCount < LOCK_RETRIES; nTryCount++ ) 
    {
        if (CStGlobals::DeviceIoControl(
                bVol?m_volhandle:m_phyhandle,
                FSCTL_UNLOCK_VOLUME,
                NULL, 0,
                NULL, 0,
                &dwBytesReturned,
                NULL 
            ) )
        {       
            return STERR_NONE;
        }
        Sleep( dwSleepAmount );
    }

    return STERR_FAILED_TO_UNLOCK_THE_DRIVE;
}

ST_ERROR CStScsi_Nt::AcquireFormatLock(BOOL /*_media_new*/)
{
    //
    // There is nothing like obtaining a separate lock for formatting the media in NT based systems.
    //
    return STERR_NONE;
}

ST_ERROR CStScsi_Nt::ReleaseFormatLock(BOOL /*_media_new*/)
{
    //
    // This will be called after formatting the media.
    // A good place to dismount, this will make operating system to forget about the present file system on the media,
    // and refreshes.
    //

    return Dismount(true);
}

ST_ERROR CStScsi_Nt::Dismount(BOOL bVol)
{
    DWORD dwBytesReturned;
    DWORD dwSleepAmount;
    int nTryCount;

    dwSleepAmount = LOCK_TIMEOUT / LOCK_RETRIES;
    // Do this in a loop until a timeout period has expired
    for( nTryCount = 0; nTryCount < LOCK_RETRIES; nTryCount++ ) 
    {
        if (CStGlobals::DeviceIoControl(
				bVol?m_volhandle:m_phyhandle,
                FSCTL_DISMOUNT_VOLUME,
                NULL, 0,
                NULL, 0,
                &dwBytesReturned,
                NULL 
            ) )
        {       
            return STERR_NONE;
        }
        Sleep( dwSleepAmount );
    }

    return STERR_FAILED_TO_DISMOUNT_THE_DRIVE;
}

#define IOCTL_DISK_DELETE_DRIVE_LAYOUT      CTL_CODE(IOCTL_DISK_BASE, 0x0040, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

wstring CStScsi_Nt::QueryAllLogicalDrives()
{
    DWORD drive_bits=0;
    DWORD mask = 0x0001;
    wstring drives(L"");

    drive_bits = ::GetLogicalDrives();
    for(wchar_t ch = L'A'; ch <= L'Z'; ch++ )
    {
        if( mask & drive_bits )
            drives = drives + ch;
        mask = mask * 2;
    }
    return drives;
}

ST_ERROR CStScsi_Nt::FormatPartition( PDISK_GEOMETRY _p_dg, ULONG _hidden_sectors, ULONG partitionSectors )
{
    DWORD dwBytesReturned;
    PDRIVE_LAYOUT_INFORMATION pdli = AllocPartitionInfo(4);

    pdli->PartitionEntry[0].StartingOffset.QuadPart = _hidden_sectors * _p_dg->BytesPerSector; 
        
    //_p_dg->Cylinders.QuadPart * _p_dg->SectorsPerTrack * _p_dg->TracksPerCylinder
    pdli->PartitionEntry[0].PartitionLength.QuadPart =
        ((LONGLONG)partitionSectors  * (LONGLONG)_p_dg->BytesPerSector ) - 
            pdli->PartitionEntry[0].StartingOffset.QuadPart;

    pdli->PartitionEntry[0].HiddenSectors = _hidden_sectors;
    pdli->PartitionEntry[0].PartitionNumber = 1;

    pdli->PartitionEntry[0].PartitionType = PARTITION_HUGE;  // >= 32MB partitions

    pdli->PartitionEntry[0].BootIndicator = 0;
    pdli->PartitionEntry[0].RecognizedPartition = TRUE;
    pdli->PartitionEntry[0].RewritePartition = TRUE;
    
    for (DWORD i = 1; i < pdli->PartitionCount; ++i)
    {
        pdli->PartitionEntry[i].StartingOffset.QuadPart  = 0;
        pdli->PartitionEntry[i].PartitionLength.QuadPart = 0;
        pdli->PartitionEntry[i].HiddenSectors = 0;
        pdli->PartitionEntry[i].PartitionNumber = i+1;   // partition numbers are 1-based
        pdli->PartitionEntry[i].PartitionType = PARTITION_ENTRY_UNUSED;
        pdli->PartitionEntry[i].BootIndicator = 0;
        pdli->PartitionEntry[i].RecognizedPartition = FALSE;
        pdli->PartitionEntry[i].RewritePartition = TRUE;
    }

    BOOL result = CStGlobals::DeviceIoControl(
            m_phyhandle,
            IOCTL_DISK_DELETE_DRIVE_LAYOUT,
            NULL, 0,
            NULL, 0,
            &dwBytesReturned,
            NULL
        );

/*  if( !result )
    {
        m_system_last_error = CStGlobals::GetLastError();
        m_last_error = STERR_FAILED_DEVICE_IO_CONTROL;
        GetUpdater()->GetErrorObject()->SaveStatus(this);
        m_system_last_error = 0;
        m_last_error = STERR_NONE;
        FreePartitionInfo( pdli );
        return STERR_FAILED_DEVICE_IO_CONTROL;
    }

*/  result = CStGlobals::DeviceIoControl(
            m_phyhandle,
            IOCTL_DISK_SET_DRIVE_LAYOUT,
            pdli, CalcPartitionInfoSizeBytes(4),
            NULL, 0,
            &dwBytesReturned,
            NULL
        );

    FreePartitionInfo( pdli );

    if( result )
    {
        return STERR_NONE;
    }

    m_system_last_error = CStGlobals::GetLastError();
    m_last_error = STERR_FAILED_DEVICE_IO_CONTROL;
//  GetUpdater()->GetErrorObject()->SaveStatus(this);
    m_system_last_error = 0;
    m_last_error = STERR_NONE;

    return STERR_FAILED_DEVICE_IO_CONTROL;
}

/*-----------------------------------------------------------------------------
AllocPartitionInfo (numPartitions)

Allocates free store to hold a DRIVE_LAYOUT_INFORMATION structure and array
of PARTITION_INFORMATION structures big enough for the number of partitions
specified.

Parameters:
   numPartitons
      Number of partitions that the structure must be able to contain.  The
      array of PARTITION_INFORMATION_STRUCTURES will contain one record for
      each partition.

Return Value:
   Returns a pointer to the memory block if successful.  Returns NULL if
   it could not allocate the block.

Notes:
   Use FreePartitionInfo to free the memory block when finished with it.
-----------------------------------------------------------------------------*/
PDRIVE_LAYOUT_INFORMATION CStScsi_Nt::AllocPartitionInfo (int numPartitions)
{
   DWORD dwBufSize;
   PDRIVE_LAYOUT_INFORMATION pdli;

   dwBufSize = CalcPartitionInfoSizeBytes (numPartitions);

   pdli = ((PDRIVE_LAYOUT_INFORMATION) LocalAlloc (LPTR, dwBufSize));

   if (pdli != NULL)
   {
      pdli->PartitionCount = numPartitions;
      pdli->Signature = 0;
   }

   return pdli;
}


/*-----------------------------------------------------------------------------
FreePartitionInfo (pdli)

Call this to free the memory allocated by AllocPartitionInfo and
GetPartitionInfo.

Return Value:
   Returns TRUE if partition information was freed; FALSE otherwise.

-----------------------------------------------------------------------------*/
BOOL CStScsi_Nt::FreePartitionInfo (PDRIVE_LAYOUT_INFORMATION pdli)
{
   return (LocalFree(pdli) == NULL);
}

/*-----------------------------------------------------------------------------
CalcPartitionInfoSizeBytes (hDrive)
-----------------------------------------------------------------------------*/
DWORD CStScsi_Nt::CalcPartitionInfoSizeBytes (int numPartitions)
{
   DWORD dwBufSize;

   dwBufSize = sizeof(DRIVE_LAYOUT_INFORMATION) +
               sizeof(PARTITION_INFORMATION) * (numPartitions -1);

   return dwBufSize;
}

ST_ERROR CStScsi_Nt::ReadGeometry( PDISK_GEOMETRY _p_dg )
{
    DWORD BytesReturned;
    if( !DeviceIoControl( m_phyhandle,
            IOCTL_DISK_GET_DRIVE_GEOMETRY,
            NULL,
            0,
            _p_dg,
            sizeof(DISK_GEOMETRY),
            &BytesReturned,
            NULL ) )
    {
        m_system_last_error = CStGlobals::GetLastError();
        m_last_error = STERR_FAILED_DEVICE_IO_CONTROL;
//      GetUpdater()->GetErrorObject()->SaveStatus(this);
        m_system_last_error = 0;
        m_last_error = STERR_NONE;
        return STERR_FAILED_DEVICE_IO_CONTROL;
    }

    return STERR_NONE;
}

ST_ERROR CStScsi_Nt::DriveLayout( PDRIVE_LAYOUT_INFORMATION _p_dl )
{
    DWORD BytesReturned;
    if( !DeviceIoControl( m_phyhandle,
            IOCTL_DISK_GET_DRIVE_LAYOUT,
            NULL,
            0,
            _p_dl,
            sizeof(DRIVE_LAYOUT_INFORMATION) + ( sizeof(PARTITION_INFORMATION) * 4 ) ,
            &BytesReturned,
            NULL ) )
    {
        m_system_last_error = CStGlobals::GetLastError();
        m_last_error = STERR_FAILED_DEVICE_IO_CONTROL;
//      GetUpdater()->GetErrorObject()->SaveStatus(this);
        m_system_last_error = 0;
        m_last_error = STERR_NONE;
        return STERR_FAILED_DEVICE_IO_CONTROL;
    }

    return STERR_NONE;
}

ST_ERROR CStScsi_Nt::GetDeviceNumber(HANDLE handle, STORAGE_DEVICE_NUMBER * deviceNumber)
{
    DWORD actualBytesReturned;
    
    if (!DeviceIoControl(handle, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0,
            deviceNumber, sizeof(STORAGE_DEVICE_NUMBER), &actualBytesReturned, NULL))
    {
        m_system_last_error = CStGlobals::GetLastError();
        m_last_error = STERR_FAILED_DEVICE_IO_CONTROL;
        m_system_last_error = 0;
        m_last_error = STERR_NONE;
        return STERR_FAILED_DEVICE_IO_CONTROL;
    }
    
    return STERR_NONE;
}

ST_ERROR CStScsi_Nt::OpenPhysicalDrive()
{
    wchar_t         DriveToOpen[32];
    BOOL            drive_opened = FALSE;

    if (!(m_volhandle && (m_volhandle != INVALID_HANDLE_VALUE)))
    {
        Open();
    }
    
    STORAGE_DEVICE_NUMBER deviceNumber;
    DWORD originalDeviceNumber;
    if (GetDeviceNumber(m_volhandle, &deviceNumber) != STERR_NONE)
    {
        return STERR_FAILED_TO_LOCATE_SCSI_DEVICE;
    }
    originalDeviceNumber = deviceNumber.DeviceNumber;

    for( int drive = 0; drive < 100; drive ++ )
    {
        
        swprintf_s( DriveToOpen, 32, L"\\\\.\\PhysicalDrive%d", drive );
        
        m_phyhandle = CStGlobals::CreateFile(
            DriveToOpen,                        // device interface name
            GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
            FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
            NULL,                               // lpSecurityAttributes
            OPEN_EXISTING,                      // dwCreationDistribution
            FILE_FLAG_WRITE_THROUGH,            // dwFlagsAndAttributes
            NULL                                // hTemplateFile
        );
                
        if( m_phyhandle == INVALID_HANDLE_VALUE ) 
        {
            continue;
        }
        
        if (GetDeviceNumber(m_phyhandle, &deviceNumber) == STERR_NONE)
        {
            if (deviceNumber.DeviceNumber == originalDeviceNumber)
            {
                drive_opened = TRUE;
                break;
            }
        }
        
        if( m_phyhandle && (m_phyhandle != INVALID_HANDLE_VALUE) )
        {
            CloseHandle( m_phyhandle );
            m_phyhandle = INVALID_HANDLE_VALUE;
        }
    }   

    if ( !drive_opened )
    {
        return STERR_FAILED_TO_LOCATE_SCSI_DEVICE;
    }
    return STERR_NONE;
}


ST_ERROR CStScsi_Nt::WriteSector( CStByteArray* _p_sector, ULONG _num_sectors, ULONG _start_sector_number, 
                                ULONG _sector_size )
{
    ST_ERROR result=STERR_NONE;
    DWORD bytes_written=0;
    LARGE_INTEGER byteAddress;
    byteAddress.QuadPart = (LONGLONG)_start_sector_number * (LONGLONG)_sector_size;
    DWORD bufsize = 0x400 * _sector_size;
    
	// position it to the correct place for reading
    DWORD position = SetFilePointer( m_phyhandle, byteAddress.LowPart, &byteAddress.HighPart, FILE_BEGIN );
    if( ( position == INVALID_SET_FILE_POINTER ) && ( CStGlobals::GetLastError() != NO_ERROR ) )
    {
        m_system_last_error = CStGlobals::GetLastError();
        m_last_error = STERR_FAILED_TO_WRITE_SECTOR;
//      GetUpdater()->GetErrorObject()->SaveStatus(this);
        m_system_last_error = 0;
        m_last_error = STERR_NONE;

        result = STERR_FAILED_TO_WRITE_SECTOR;
        return result;
    }

    if (_num_sectors * _sector_size < bufsize)
        bufsize = _num_sectors * _sector_size;
    
    PUCHAR buf = new UCHAR[bufsize];

    ULONG i=0;
    
    while( i < _num_sectors * _sector_size )
    {
        _p_sector->Read( (void*)buf, bufsize, i );
        if( !WriteFile( m_phyhandle, buf, (DWORD)( bufsize ), &bytes_written, NULL ) )
        {
            m_system_last_error = CStGlobals::GetLastError();
            m_last_error = STERR_FAILED_TO_WRITE_SECTOR;
    //      GetUpdater()->GetErrorObject()->SaveStatus(this);
            m_system_last_error = 0;
            m_last_error = STERR_NONE;

            result = STERR_FAILED_TO_WRITE_SECTOR;
            break;
        }

        i += bufsize;
        if (i >= _num_sectors * _sector_size)
             break;

        if (_num_sectors * _sector_size - i < bufsize)
        {
            bufsize = _num_sectors * _sector_size -i;      
        }     
       
    }
       
    delete[] buf;

    return result;
}

ST_ERROR CStScsi_Nt::ReadSector( CStByteArray* _p_sector, ULONG _num_sectors, ULONG _start_sector_number, 
                                ULONG _sector_size )
{
    ST_ERROR result=STERR_NONE;
    DWORD bytes_read=0;
    LARGE_INTEGER byteAddress;
    byteAddress.QuadPart = (LONGLONG)_start_sector_number * (LONGLONG)_sector_size;
    
    // position it to the correct place for reading
    DWORD position = SetFilePointer( m_phyhandle, byteAddress.LowPart, &byteAddress.HighPart, FILE_BEGIN );
    if( ( position == INVALID_SET_FILE_POINTER ) && ( CStGlobals::GetLastError() != NO_ERROR ) )
    {
        m_system_last_error = CStGlobals::GetLastError();
        m_last_error = STERR_FAILED_TO_READ_SECTOR;
//      GetUpdater()->GetErrorObject()->SaveStatus(this);
        m_system_last_error = 0;
        m_last_error = STERR_NONE;

        result = STERR_FAILED_TO_READ_SECTOR;
        return result;
    }
    
    PUCHAR buf = new UCHAR[ _num_sectors * _sector_size ];
    
    memset( buf, 0, _num_sectors * _sector_size );
    
    if( !ReadFile( m_phyhandle, buf, (DWORD)(_num_sectors * _sector_size), &bytes_read, NULL ) )
    {
        m_system_last_error = CStGlobals::GetLastError();
        m_last_error = STERR_FAILED_TO_READ_SECTOR;
//      GetUpdater()->GetErrorObject()->SaveStatus(this);
        m_system_last_error = 0;
        m_last_error = STERR_NONE;

        result = STERR_FAILED_TO_READ_SECTOR;
    }
    
    if( result == STERR_NONE )
    {
        _p_sector->Write( (void*)buf, _num_sectors * _sector_size, 0 );
    }

    delete[] buf;

    return result;
}

