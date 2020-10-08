#include "Handle.h"
void init_system_info();
NTSTATUS GetFileRoot(UNICODE_STRING* u_filename);
wchar_t *stristrW(wchar_t *Str, wchar_t *Pat);

wchar_t *stristrW(wchar_t *Str, wchar_t *Pat)
{
    wchar_t *pptr, *sptr, *start;

    for (start = (wchar_t *)Str; *start != '\0'; start++)
    {
        for ( ; ((*start!='\0') && (toupper(*start) != toupper(*Pat))); start++);
        if ('\0' == *start) return NULL;
        pptr = (wchar_t *)Pat;
        sptr = (wchar_t *)start;
        while (toupper(*sptr) == toupper(*pptr))
        {
            sptr++;
            pptr++;
            if ('\0' == *pptr) return (start);
        }
    }
    return NULL;
}

//==============================================================================
void init_system_info()
{
	if (ZwQuerySystemInformation == NULL)
	{

		UNICODE_STRING routineName;

		RtlInitUnicodeString(&routineName, L"ZwQuerySystemInformation");

		ZwQuerySystemInformation = (QuerySysInfo)MmGetSystemRoutineAddress(&routineName);

		if (NULL == ZwQuerySystemInformation)
		{
			DbgLog("Cannot resolve ZwQuerySystemInformation\n");
		}
	}
}
NTSTATUS GetFileRoot(UNICODE_STRING* u_filename)
{
    NTSTATUS                   status;
    PSYSTEM_HANDLE_INFORMATION pSysHandleInformation;
    ULONG                      handleInfoSize = 0x10000;
    HANDLE                     processHandle;
    ULONG                      i;

	pSysHandleInformation = (PSYSTEM_HANDLE_INFORMATION)ExAllocatePool(NonPagedPool, handleInfoSize);
	while ((status = ZwQuerySystemInformation(SystemHandleInformation, pSysHandleInformation, handleInfoSize, NULL)) == STATUS_INFO_LENGTH_MISMATCH)
	  {
		ExFreePool(pSysHandleInformation);
		pSysHandleInformation = (PSYSTEM_HANDLE_INFORMATION)ExAllocatePool( NonPagedPool, handleInfoSize *= 2);
	  }
	if (!NT_SUCCESS(status))
	{
		if(pSysHandleInformation)ExFreePool(pSysHandleInformation);
		return;
	}
 
	for (i = 0; i < pSysHandleInformation->HandleCount; i++)
	{
		SYSTEM_HANDLE handle_0 = pSysHandleInformation->Handles[i];
		if (
			(    handle_0.GrantedAccess == 0x0012019f)
			 || (handle_0.GrantedAccess == 0x001a019f)
			 || (handle_0.GrantedAccess == 0x00120189)
			 || (handle_0.GrantedAccess == 0x00100000)
			)
		{
			continue;
		}


		unsigned int  g_CurrentIndex = 0;
		for (g_CurrentIndex; g_CurrentIndex < pSysHandleInformation->HandleCount; )
		{

			SYSTEM_HANDLE handle_1 = pSysHandleInformation->Handles[g_CurrentIndex];
			g_CurrentIndex++;
			HANDLE hDup = (HANDLE)sh.Handle;
			CLIENT_ID ID;
			ID.UniqueProcess = (HANDLE)sh.ProcessId;
			ID.UniqueThread = (HANDLE)0;
			status = ZwOpenProcess(&processHandle, PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, &ID);
			if (status != STATUS_SUCCESS)
			{
				return;
			}

			if (processHandle)
			{
				status = ZwDuplicateObject(processHandle, (HANDLE)sh.Handle, ZwCurrentProcess(), &hDup, 0, FALSE, DUPLICATE_SAME_ACCESS);

				if (!NT_SUCCESS(status))
				{
					hDup = (HANDLE)sh.Handle;
				}
				CloseHandle(processHandle);
			}
			OBJECT_ATTRIBUTES objAttr;
			IO_STATUS_BLOCK     iosb;
			FILE_NAME_INFORMATION  fileInfo;
			status = ZwQueryInformationFile(hDup,
				&iosb,
				&fileInfo,
				sizeof(FileNameInformation),
				FileNameInformation);

			if (hDup && (hDup != (HANDLE)sh.Handle))
			{
				CloseHandle(hDup);
			}
			if (!NT_SUCCESS(status))continue;
		}

	}


	
		OF_INFO_t stOFInfo;
		stOFInfo.dwPID = handle.ProcessId;
		stOFInfo.lpFile = csFileName;
		stOFInfo.hFile = (HANDLE)pSysHandleInformation->Handles[g_CurrentIndex - 1].wValue;

		objectNameInfo = ExAllocatePool(NonPagedPool,MAX_PATH);

        if(!NT_SUCCESS(ZwQueryObject(dupHandle,ObjectNameInformation,objectNameInfo, MAX_PATH,&returnLength)))
            {
				ExFreePool(objectNameInfo);
                objectNameInfo = ExAllocatePool(NonPagedPool,returnLength);
                if(!NT_SUCCESS(ZwQueryObject(dupHandle,ObjectNameInformation,objectNameInfo,returnLength,NULL)))
                {
					ExFreePool(objectTypeInfo);
					ExFreePool(objectNameInfo);
                    continue;
                }
            }

        objectName = *(PUNICODE_STRING)objectNameInfo;
		DbgLog("AC/>"__FUNCTION__":  objectName  %s\n", objectName);
  		ExFreePool(objectTypeInfo);
		ExFreePool(objectNameInfo);
    
    }
	ExFreePool(handleInfo);
    ZwClose(processHandle);
    return;
}
