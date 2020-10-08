#include <algorithm>
#include "Services.h"
#pragma comment(lib, "version.lib")
#pragma comment(lib, "user32.lib")
Services::Services()
{
	numCPU=0;
}
Services::~Services()
{

}
void Services::GetNcPU()
{
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	numCPU = sysinfo.dwNumberOfProcessors;
}
void Services::UnixTimeToFileTime(time_t t, LPFILETIME pft)
{
	LONGLONG ll;

	ll = Int32x32To64(t, 10000000) + 116444736000000000;
	pft->dwLowDateTime = (DWORD)ll;
	pft->dwHighDateTime = ll >> 32;
}
void Services::UnixTimeToSystemTime(time_t t, LPSYSTEMTIME pst)
{
	FILETIME ft;

	UnixTimeToFileTime(t, &ft);
	FileTimeToSystemTime(&ft, pst);
}
std::string Services::GetVersionInfo(TCHAR *szValue)
{
	std::string csRet="camera rtsp client load ver. ";
	HMODULE hZRTSPCam=GetModuleHandle("ZRTSPCam");
	if(hZRTSPCam==NULL)return NULL;
	HRSRC hVersion = FindResource(hZRTSPCam,MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
	DWORD dwSize = SizeofResource(hZRTSPCam,hVersion);
	if (hVersion != NULL)
	{
		HGLOBAL hGlobal = LoadResource(hZRTSPCam, hVersion);

		if (hGlobal != NULL)
		{
			LPVOID ver = LocalAlloc(LPTR, dwSize*2);
			if (ver != NULL)
			{
				memcpy(ver, hGlobal, dwSize);
				DWORD *codepage;
				UINT len;
				char fmt[0x40];
				PVOID ptr = 0;
				if (VerQueryValue(ver, "\\VarFileInfo\\Translation", (LPVOID*) & codepage, &len))
				{
					wsprintf(fmt, "\\StringFileInfo\\%08x\\%s", (*codepage) << 16 | (*codepage) >> 16, szValue);
					if (VerQueryValue(ver, fmt, &ptr, &len))
					{
						std::string	vers=(char*)ptr;
						csRet.append(vers);
					}
				}
				LocalFree(ver);
			}
			FreeResource(hGlobal);
		}
	}
	time_t t = time(0);
	time_start = localtime( & t );
	return csRet;
}
long Services::CheckIfValidFrame( IplImage* frame )
{
	return  xiplCheckImageHeader(frame );
}

void Services::InitTableCRC32()
{
	for(int i=0; i < 256; i++ )
	{
		DWORD crc_accum = (i << 24);
		for(int j=0;  j < 8;  j++)
		{
			if(crc_accum & 0x80000000L)
				crc_accum = (crc_accum << 1) ^ CRC32_POLYNOMIAL;
			else crc_accum = (crc_accum << 1);
		}
		CrcTable32[i] = crc_accum;
	}
}
DWORD Services::Get_CRC32(PVOID data,int len)
{
	DWORD crc=0xFFFFFFFF;
	PUCHAR buffer=(PUCHAR)data;
	for(int j=0; j < len; j++)
	{
		int i = ((int)(crc >> 24) ^ *buffer++) & 0xFF;
		crc = (crc << 8) ^ CrcTable32[i];
	}
	return crc;
}