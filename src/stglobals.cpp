/*
 * File: stglobals.cpp
 *
 * Copyright (c) 2010-2011 Freescale Semiconductor, Inc. All rights reserved.
 * See included license file for license details.
*/
#include "stheader.h"
#include "stglobals.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CStGlobals::CStGlobals(string _name):CStBase(_name)
{
}

CStGlobals::~CStGlobals()
{

}

ST_ERROR CStGlobals::SpacesToUnderScores(string& _str)
{
	for( size_t i=0; i<_str.length(); i++ )
	{
		if( _str[i] == ' ')
		{
			_str[i] = '_';
		}
	}
	return STERR_NONE;
}

PLATFORM CStGlobals::GetPlatform()
{
    OSVERSIONINFOEX osvi = {0};
    
	osvi.dwOSVersionInfoSize = sizeof(osvi);
    
	GetVersionEx((OSVERSIONINFO*)&osvi);
	
	if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion >= 90)
		return OS_ME;
	else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion <= 10)
		return OS_98;
	else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
		return OS_2K;
	else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion >= 1)
		return OS_XP;
	
	return OS_UNSUPPORTED;
}



long CStGlobals::GetLastError()
{
	return ::GetLastError();
}

size_t CStGlobals::Max(size_t _a, size_t _b)
{
	return max(_a, _b);
}

ST_ERROR CStGlobals::MakeMemoryZero(PUCHAR _buf, size_t _size)
{
	memset(_buf, 0, _size);
	return STERR_NONE;
}

HANDLE CStGlobals::CreateEvent(
	LPSECURITY_ATTRIBUTES _lpEventAttributes, 
	BOOL _bManualReset, 
	BOOL _bInitialState, 
	LPCWSTR _lpName
)
{
	return ::CreateEventW(_lpEventAttributes, _bManualReset, _bInitialState, _lpName);
}

HANDLE CStGlobals::CreateFile(
	LPCWSTR _lpFileName, 
	DWORD _dwDesiredAccess, 
	DWORD _dwShareMode,
	LPSECURITY_ATTRIBUTES _lpSecurityAttributes, 
	DWORD _dwCreationDisposition, 
	DWORD _dwFlagsAndAttributes,
	HANDLE _hTemplateFile
)
{
	return ::CreateFileW(_lpFileName, _dwDesiredAccess, _dwShareMode, _lpSecurityAttributes, 
		_dwCreationDisposition, _dwFlagsAndAttributes, _hTemplateFile);
}

BOOL CStGlobals::DeviceIoControl(
	HANDLE _hDevice, 
	DWORD _dwIoControlCode, 
	LPVOID _lpInBuffer, 
	DWORD _nInBufferSize, 
	LPVOID _lpOutBuffer, 
	DWORD _nOutBufferSize, 
	LPDWORD _lpBytesReturned, 
	LPOVERLAPPED _lpOverlapped
)
{
	return ::DeviceIoControl( _hDevice, _dwIoControlCode, _lpInBuffer, _nInBufferSize, _lpOutBuffer, 
		_nOutBufferSize, _lpBytesReturned, _lpOverlapped);
}


BOOL CStGlobals::WriteFileEx(
	HANDLE _hFile, 
	LPCVOID _lpBuffer, 
	DWORD _nNumberOfBytesToWrite,
	LPOVERLAPPED _lpOverlapped, 
	LPOVERLAPPED_COMPLETION_ROUTINE _lpCompletionRoutine
)
{
	return ::WriteFileEx(_hFile, _lpBuffer, _nNumberOfBytesToWrite, _lpOverlapped, _lpCompletionRoutine);
}

DWORD CStGlobals::WaitForSingleObjectEx(
	HANDLE _hHandle,
	DWORD _dwMilliseconds,
	BOOL _bAlertable
)
{
	return ::WaitForSingleObjectEx(_hHandle, _dwMilliseconds, _bAlertable);
}

BOOL CStGlobals::SetEvent(HANDLE _hEvent)
{
	return ::SetEvent(_hEvent);
}

BOOL CStGlobals::ResetEvent(HANDLE _hEvent)
{
	return ::ResetEvent(_hEvent);
}

BOOL CStGlobals::CancelIo(HANDLE _hFile)
{
	return ::CancelIo(_hFile);
}
