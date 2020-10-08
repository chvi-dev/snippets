#include "acflt.h"
#include "Helper.h"

void init_process_info();
void Sleep(ULONG time_out);
PWCH wcssplt(PWCH str, PWCH substr);
QUERY_INFO_PROCESS ZwQueryInformationProcess;
bool GetProcessName();
int  port_flt_cn=1;
int  port_flt_dsk=1;
int  port_reg_cn=1;
int  port_reg_dsk=1;
NTSTATUS Int2UnicodeStrPP(DWORD val, PUNICODE_STRING str);
NTSTATUS Int2ANSIStr(DWORD val, PANSI_STRING str);
bool FindSubStr(PUNICODE_STRING _FNameInfo, const wchar_t* sub_str);
bool FindStr(PUNICODE_STRING _FNameInfo);
bool StrIsEqual(PUNICODE_STRING filename);
bool CurrentIRQL();
bool IsLoggedUser(user_info *user);
void  GetFileName(UNICODE_STRING full_file_name, PUNICODE_STRING file_name);
NTSTATUS RunCompletionDPC(PFLT_CALLBACK_DATA Data,PAC_FILTER_MESSAGE msg, PVOID *completionCtx, byte operation);
NTSTATUS IsDesiredOperation(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects,__in UCHAR MajorFunction,__out PUNICODE_STRING filename, bool after);
NTSTATUS FilterUnload(IN FLT_FILTER_UNLOAD_FLAGS Flags);
NTSTATUS InstanceSetup(IN PCFLT_RELATED_OBJECTS  FltObjects, IN FLT_INSTANCE_SETUP_FLAGS  Flags, IN DEVICE_TYPE  VolumeDeviceType, IN FLT_FILESYSTEM_TYPE  VolumeFilesystemType);
NTSTATUS InstanceQueryTeardown(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags);
void InitGlobalVar();
void EraseFlt();
void  GetDosDeviceName(PDEVICE_OBJECT pDeviceObject, PUNICODE_STRING pShareName, PUNICODE_STRING pDosName);
NTSTATUS FltPortConnect( __in PFLT_PORT ClientPort,
						 __in_opt PVOID ServerPortCookie,
						 __in_opt PVOID ConnectionContext,
						 __in ULONG SizeOfContext,
						 __deref_out_opt PVOID *ConnectionCookie
					     );

VOID FltPortDisconnect(__in_opt PVOID ConnectionCookie);

NTSTATUS RegPortConnect(__in PFLT_PORT ClientPort,
						__in_opt PVOID ServerPortCookie,
						__in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
						__in ULONG SizeOfContext,
						__deref_out_opt PVOID *ConnectionCookie
					   );


VOID RegPortDisconnect(__in_opt PVOID ConnectionCookie);

NTSTATUS InstanceQueryTeardown(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	return STATUS_SUCCESS;
}

NTSTATUS InstanceSetup(IN PCFLT_RELATED_OBJECTS  FltObjects, IN FLT_INSTANCE_SETUP_FLAGS  Flags, IN DEVICE_TYPE  VolumeDeviceType, IN FLT_FILESYSTEM_TYPE  VolumeFilesystemType)
{
    //DbgBreakPoint();
	DbgLog("AC/>"__FUNCTION__" run\n");
	if (VolumeDeviceType == FILE_DEVICE_NETWORK_FILE_SYSTEM)
	{
       DbgLog("AC/>"__FUNCTION__" VolumeDeviceType = FILE_DEVICE_NETWORK_FILE_SYSTEM\n");
	//	return STATUS_FLT_DO_NOT_ATTACH;
	}
    if (VolumeDeviceType == FILE_DEVICE_DISK_FILE_SYSTEM)
	{
       DbgLog("AC/>"__FUNCTION__" VolumeDeviceType = FILE_DEVICE_DISK_FILE_SYSTEM\n");
    }
    
	return STATUS_SUCCESS;
}

void InitGlobalVar()
{
	ZwQueryInformationProcess = NULL;


	RtlInitUnicodeString(&system_sid, (PWSTR)SYSTEM_SID);
	RtlInitUnicodeString(&service_sid, (PWSTR)SERVICE_SID);
	RtlInitUnicodeString(&network_sid, (PWSTR)NETWORK_SID);


	g_usr_nameLength = STR_LEN;//0x400
	g_RegstrSID = ExAllocatePool(NonPagedPool, g_usr_nameLength);

	g_pDomainFFO = ExAllocatePool(NonPagedPool, sizeof(UNICODE_STRING));
	g_pDomainFFO->Length = 0;
	g_pDomainFFO->MaximumLength = STR_LEN;
	g_pDomainFFO->Buffer = ExAllocatePool(NonPagedPool, STR_LEN);
	g_DomainSize = STR_LEN;
	g_pUserFFO = ExAllocatePool(NonPagedPool, sizeof(UNICODE_STRING));
	g_pUserFFO->Length = 0;
	g_pUserFFO->MaximumLength = STR_LEN;
	g_pUserFFO->Buffer = ExAllocatePool(NonPagedPool, STR_LEN);
	g_UserSize = STR_LEN;
    g_pSidFFO = ExAllocatePool(NonPagedPool, sizeof(UNICODE_STRING));
	g_pSidFFO->Length = 0;
	g_pSidFFO->MaximumLength = STR_LEN;
	g_pSidFFO->Buffer = ExAllocatePool(NonPagedPool, STR_LEN);
	g_SidSize = STR_LEN;
 	g_pDomainREG = ExAllocatePool(NonPagedPool, sizeof(UNICODE_STRING));
	g_pDomainREG->Length = 0;
	g_pDomainREG->MaximumLength = STR_LEN;
	g_pDomainREG->Buffer = ExAllocatePool(NonPagedPool, STR_LEN);
	g_DomainSize = STR_LEN;
	g_pUserREG = ExAllocatePool(NonPagedPool, sizeof(UNICODE_STRING));
	g_pUserREG->Length = 0;
	g_pUserREG->MaximumLength = STR_LEN;
	g_pUserREG->Buffer = ExAllocatePool(NonPagedPool, STR_LEN);
	g_UserSize = STR_LEN;

	g_filename_prev.Length = 0;
	g_filename_prev.Buffer = ExAllocatePool(NonPagedPool, STR_LEN);
	g_filename_prev.MaximumLength = STR_LEN;
	g_ParentDir_prev.Length = 0;
	g_ParentDir_prev.Buffer = ExAllocatePool(NonPagedPool, STR_LEN);
	g_ParentDir_prev.MaximumLength = STR_LEN;

	g_filename_curr.Length = 0;
	g_filename_curr.Buffer = ExAllocatePool(NonPagedPool, STR_LEN);
	g_filename_curr.MaximumLength = STR_LEN;

	g_TrgVolume.Buffer = ExAllocatePool(NonPagedPool, max_VOL);
	g_TrgVolume.MaximumLength = max_VOL;
  
	
	g_copy_op = 0;

	InitializeListHead(&g_fltUser_list);
	InitializeListHead(&g_fileOpened_list);
	g_FileObject = NULL;
}

NTSTATUS
FltPortConnect(
	__in PFLT_PORT ClientPort,
	__in_opt PVOID ServerPortCookie,
	__in_opt PVOID ConnectionContext,
	__in ULONG SizeOfContext,
	__deref_out_opt PVOID *ConnectionCookie
)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionCookie);
	int sz = SizeOfContext;
	INT8 client_PortInd = *(INT8*)ConnectionContext;
	filterManager.connections[client_PortInd].port = ClientPort;
	filterManager.connections[client_PortInd].busy = false;
	filterManager.connections[client_PortInd].num_connect = client_PortInd;
	if (filterManager.pUserProcess == NULL)
	{
		filterManager.pUserProcess = PsGetCurrentProcess();
	}
	
    if(client_PortInd == msg_connections - 1)
    {
     DbgLog("\nAC/>"__FUNCTION__"Connections flt  num %d\n", port_flt_cn);
    }
     else
         {
           port_flt_cn++;
          }
	return STATUS_SUCCESS;
}

VOID  FltPortDisconnect(__in_opt PVOID ConnectionCookie)
{
	UNREFERENCED_PARAMETER(ConnectionCookie);
  	PAGED_CODE();
    static INT8 dsk=1;
    if(filterManager.connections [ port_flt_dsk - 1 ].port != 0)
    {
      FltCloseClientPort( filterManager.pFilter , &filterManager.connections[port_flt_dsk - 1].port);
    }
    port_flt_dsk++;
    if(port_flt_dsk==msg_connections-1)
    {
     DbgLog("\nAC/>"__FUNCTION__"DisConnection FFO ports OK, ports disconnect #%d\n",dsk);
    }
}
NTSTATUS
RegPortConnect(
	__in PFLT_PORT ClientPort,
	__in_opt PVOID ServerPortCookie,
	__in_bcount_opt(SizeOfContext) PVOID ConnectionContext,
	__in ULONG SizeOfContext,
	__deref_out_opt PVOID *ConnectionCookie
)
{
	PAGED_CODE();

	UNREFERENCED_PARAMETER(ServerPortCookie);
	UNREFERENCED_PARAMETER(ConnectionCookie);

	int sz = SizeOfContext;
	INT8 client_PortInd = *(INT8*)ConnectionContext;
	filterManager.connectionsR[client_PortInd].port = ClientPort;
	filterManager.connectionsR[client_PortInd].busy = false;
	filterManager.connectionsR[client_PortInd].num_connect = client_PortInd;
    if(client_PortInd == msg_Rconnections-1)
    {
      UNICODE_STRING AltitudeString = RTL_CONSTANT_STRING(L"360000");
      NTSTATUS status = CmRegisterCallbackEx(Ac_RegistryCallback, &AltitudeString, filterManager.pDriverObject, (PVOID)&g_unload_dev, &g_CmCookie, NULL);
      DbgLog("AC/>"__FUNCTION__"Connections flt  num %d, status = 0x%Xn", port_reg_cn,status);
    } 
     else
         {
           port_reg_cn++;
          }
	return STATUS_SUCCESS;
}

VOID  RegPortDisconnect(__in_opt PVOID ConnectionCookie)
{
    UNREFERENCED_PARAMETER(ConnectionCookie);
  	PAGED_CODE();
    static INT8 dskr=1;
    if(filterManager.connectionsR[ port_reg_dsk - 1 ].port != 0)
    {
      FltCloseClientPort( filterManager.pFilter , &filterManager.connectionsR[port_reg_dsk - 1].port);
    }
    port_reg_dsk++;
    if(port_reg_dsk==msg_Rconnections-1)
    {
     DbgLog("AC/>"__FUNCTION__"DisConnection REG ports OK, ports disconnect #%d\n",dskr);
    }
}

void EraseFlt()
{
    DbgLog("\n AC/>"__FUNCTION__": Enter \n");
	FltCloseCommunicationPort(filterManager.pSFltPort);
	FltCloseCommunicationPort(filterManager.pSRegPort);
	FltUnregisterFilter(filterManager.pFilter);
	DbgLog("\n AC/>"__FUNCTION__": run OK\n");
}

NTSTATUS FilterUnload(IN FLT_FILTER_UNLOAD_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Flags);  

    if(!g_unload_dev)
    {
      EraseFlt(); 
    }
	return STATUS_SUCCESS;
}

void  GetDosDeviceName(PDEVICE_OBJECT pDeviceObject, PUNICODE_STRING pShareName, PUNICODE_STRING pDosName)
{
	NTSTATUS status;
	UNICODE_STRING tempDosDrive;

	ObReferenceObject(pDeviceObject);

	status = IoVolumeDeviceToDosName( (PVOID)pDeviceObject,	&tempDosDrive);

	pDosName->Length = 0;

	if (NT_SUCCESS(status))
	{
		int i = 0;
		int wchars = tempDosDrive.Length / 2;
		int pos = 0;

		pDosName->MaximumLength = tempDosDrive.MaximumLength + 2;
		pDosName->Buffer = ExAllocatePool(NonPagedPool, pDosName->MaximumLength);

		if (pDosName->Buffer != NULL)
		{
			pDosName->Buffer[pos++] = '\\';
			pDosName->Length += sizeof(WCHAR);
			for (i = 0; i < wchars; i++)
			{
				if (tempDosDrive.Buffer[i] != 0x003A)
				{
					pDosName->Buffer[pos++] = tempDosDrive.Buffer[i];
					pDosName->Length += sizeof(WCHAR); // Unicode is 2-bytes
				}
			}
			ExFreePool(tempDosDrive.Buffer);
		}
	}
	else {
		if (pShareName != NULL)
		{
			pDosName->MaximumLength = pShareName->MaximumLength;
			pDosName->Buffer = ExAllocatePool(NonPagedPool, pDosName->MaximumLength);
			if (pDosName->Buffer != NULL)
			{
				RtlUnicodeStringCopy(pDosName, pShareName);
			}
		}
		else 
		{
				pDosName->MaximumLength = 30; // Dont change this
				pDosName->Buffer = ExAllocatePool(NonPagedPool, pDosName->MaximumLength);
				if (pDosName->Buffer != NULL)
				{
					RtlUnicodeStringCatString(pDosName, L"\\UNKNOWN DRIVE");
				}
		}
	}
	ObDereferenceObject(pDeviceObject);
}

void init_process_info()
{

	if (ZwQueryInformationProcess == NULL)
	{

		UNICODE_STRING routineName;

		RtlInitUnicodeString(&routineName, L"ZwQueryInformationProcess");

		ZwQueryInformationProcess = (QUERY_INFO_PROCESS)MmGetSystemRoutineAddress(&routineName);

		if (NULL == ZwQueryInformationProcess)
		{
			DbgLog("Cannot resolve ZwQueryInformationProcess\n");
		}

	}

}

bool GetProcessName()
{
	NTSTATUS status;
	ULONG returnedLength;
	PVOID buffer;
	status = ZwQueryInformationProcess(NtCurrentProcess(),
		ProcessImageFileName,
		NULL,
		0,
		&returnedLength);

	if (status != STATUS_INFO_LENGTH_MISMATCH)
	{
	 return false;
	}
	buffer = ExAllocatePool(NonPagedPool, returnedLength);
	if (buffer == NULL)
	{
		return false;
	}

	
	status = ZwQueryInformationProcess(NtCurrentProcess(),
		                                ProcessImageFileNameWin32,
										(PVOID)buffer,
										returnedLength,
										&returnedLength
									);
	if (!NT_SUCCESS(status))
		{
     			return false;
		}
	RtlZeroMemory(g_processName.Buffer, g_processName.MaximumLength);
	RtlCopyUnicodeString(&g_processName, (PUNICODE_STRING)buffer);
	return true;
}

bool FindStr(PUNICODE_STRING _FNameInfo)
{
	KIRQL old = KeGetCurrentIrql();
	if (old != PASSIVE_LEVEL)
	{
		KeLowerIrql(PASSIVE_LEVEL);
	}

	wchar_t* find = wcsstr(_FNameInfo->Buffer, IGNORE_AC_FOLDER); //g_LogFileName.Buffer

	if (old != PASSIVE_LEVEL)
	{
		KfRaiseIrql(old);
	}
	if (find != NULL)return true;
	 else 
		return false;
}

bool FindSubStr(PUNICODE_STRING _FNameInfo,const wchar_t* sub_str)
{
	KIRQL old = KeGetCurrentIrql();
	if (old != PASSIVE_LEVEL)
	{
		KeLowerIrql(PASSIVE_LEVEL);
	}
	if ( _FNameInfo->Length <= 0 )
	{
		return false;
	}
	wchar_t* find = wcsstr(_FNameInfo->Buffer, sub_str);

	if (old != PASSIVE_LEVEL)
	{
		KfRaiseIrql(old);
	}
	if (find != NULL)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool IsLoggedUser(user_info *user)
{
	KIRQL old = KeGetCurrentIrql();
	if (old != PASSIVE_LEVEL )
	{
		return false;
	}
	NTSTATUS status = STATUS_SUCCESS;
	bool rez = true;
	TOKEN_USER*  pUser = NULL;
	TOKEN_USER** ppUser = &pUser;
	ANSI_STRING a_strSID;
	UNICODE_STRING w_strSID;
	unsigned long dwStrSidSize = 0;
	ULONG nameLength = SECURITY_MAX_SID_SIZE + 100;

	SID* sid = NULL;
	ULONG IdAuth;
	char* strSID = NULL;
	ULONG len;
	HANDLE token;
 	try
	{
		status = ZwOpenThreadTokenEx(ZwCurrentThread(), GENERIC_READ, TRUE, OBJ_KERNEL_HANDLE, &token);
		if (!NT_SUCCESS(status))
		{
			status = ZwOpenProcessTokenEx(ZwCurrentProcess(), GENERIC_READ,OBJ_KERNEL_HANDLE, &token);
		}
		if (!NT_SUCCESS(status))
		{
			DbgLog("AC/>"__FUNCTION__":ZwOpenProcessTokenEx error = 0x%08x \n", status);
			return false;
		}

		ZwQueryInformationToken(token, TokenUser, NULL, 0, &len);
		pUser = ExAllocatePool(NonPagedPool, len);
		if (pUser == NULL)
		{
			ZwClose(token);
			DbgLog("AC/>"__FUNCTION__":pUser => ExAllocatePool error = 0x%08x \n", STATUS_INSUFFICIENT_RESOURCES);
			return false;
		}

		status = ZwQueryInformationToken(token, TokenUser, pUser, len, &len);
		if (!NT_SUCCESS(status))
		{

			ExFreePool(pUser);
			DbgLog("AC/>"__FUNCTION__": ZwQueryInformationToken error = 0x%08x \n", status);
			ZwClose(token);
			return false;
		}
		if (pUser->User.Sid == NULL)
		{
			ExFreePool(pUser);
			DbgLog("AC/>"__FUNCTION__":pUser->User.Sid=NULL error = 0x%08x \n", STATUS_INSUFFICIENT_RESOURCES);
			ZwClose(token);
			return false;
		}
		ZwClose(token);
		sid = (SID*)pUser->User.Sid;

		unsigned long dwStrSidSize = 0;
		ULONG IdAuth;

		dwStrSidSize = sprintf(g_RegstrSID, "S-%u-", sid->Revision);
		IdAuth = (sid->IdentifierAuthority.Value[5]) + (sid->IdentifierAuthority.Value[4] << 8) + (sid->IdentifierAuthority.Value[3] << 16) + (sid->IdentifierAuthority.Value[2] << 24);
		dwStrSidSize += sprintf(g_RegstrSID + strlen(g_RegstrSID), "%u", IdAuth);
		if (sid->SubAuthorityCount)
		{
			for (int i = 0; i < sid->SubAuthorityCount; i++)
			{
				dwStrSidSize += sprintf(g_RegstrSID + dwStrSidSize, "-%u", sid->SubAuthority[i]);
			}
		}

		ANSI_STRING a_strSID;
		UNICODE_STRING w_strSID;
		SID_NAME_USE NameUse;

		RtlInitAnsiString(&a_strSID, g_RegstrSID);
		RtlAnsiStringToUnicodeString(&w_strSID, &a_strSID, TRUE);
        *user->SidSize = user->pSid->Length;
 
		NTSTATUS status = SecLookupAccountSid(sid, user->UserSize, user->pUser, user->DomainSize, user->pDomain, &NameUse);
        
        //DbgLog("AC/>"__FUNCTION__":SID ->%ws ;user ->%ws \n",user->pSid->Buffer , user->pUser->Buffer);
        ExFreePool(pUser);
        if (!NT_SUCCESS(status))
		{
			DbgLog("AC/>"__FUNCTION__": SecLookupAccountSid error = 0x%08x \n", status);
			return false;
		}
		bool rez = FindSubStr(user->pUser, L"SYSTEM");
		if (!rez)
		{
			rez = FindSubStr(user->pUser, L"ÑÈÑÒÅÌÀ");
  		}
		if (rez)
		{
			return false;
		}
		USERS_Container* ctrl_users;
		PLIST_ENTRY head = &g_fltUser_list;
		PLIST_ENTRY entry = head->Flink;
		if (!IsListEmpty(head))
		{

			while (entry != head)
			{
				ctrl_users = CONTAINING_RECORD(entry, USERS_Container, entry_user);

				if (ctrl_users)
				{
					PUNICODE_STRING ctrl_user = ctrl_users->user;
					PUNICODE_STRING curr_user = user->pUser;

					rez = RtlEqualUnicodeString(ctrl_user, curr_user, TRUE);
					if (rez)
					{     
                        RtlCopyUnicodeString(user->pSid,&w_strSID);
                        *user->SidSize = w_strSID.Length;
					    return true;
					}
					entry = ctrl_users->entry_user.Flink;
				}
				else
					return false;
			}
		}

	}
	except(EXCEPTION_EXECUTE_HANDLER)
	{
		DbgLog("AC/>"__FUNCTION__":except error = 0x%08x \n", EXCEPTION_EXECUTE_HANDLER);
	}

	return false;
}

NTSTATUS ReturnOperationState(bool after)
{
   if(after==true) return (NTSTATUS)FLT_POSTOP_FINISHED_PROCESSING;
	return (NTSTATUS)FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS IsDesiredOperation(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects,UCHAR MajorFunction,PUNICODE_STRING filename,bool after)
{
	if (Data->Iopb->MajorFunction != MajorFunction)
	{
		Return(after);
	}
	
	if (FltObjects->FileObject == NULL || Data == NULL || FLT_IS_FS_FILTER_OPERATION(Data) ||  g_unload_dev )
	{
		Return(after);
	}

	if (FLT_IS_FASTIO_OPERATION(Data))
	{
		//DbgLog("AC/>"__FUNCTION__":(CR)FLT_PREOP_DISALLOW_FASTIO \n");

		return FLT_PREOP_DISALLOW_FASTIO;
	}
 	
	PFLT_FILE_NAME_INFORMATION    FNameInfo;
	ANSI_STRING str_ev;
	PFILE_OBJECT FileObject = Data->Iopb->TargetFileObject;

	NTSTATUS status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FNameInfo);
	if (!NT_SUCCESS(status))
	{
		Return(after);
	}

	status = FltParseFileNameInformation(FNameInfo);

	if (FindStr(&FNameInfo->Name))
	{
		FltReleaseFileNameInformation(FNameInfo);
		Return(after);
	}

	user_info user_flt_info;

	user_flt_info.pUser = g_pUserFFO;
	user_flt_info.pDomain = g_pDomainFFO;
    user_flt_info.pSid = g_pSidFFO;
	user_flt_info.DomainSize = &g_DomainSize;
	user_flt_info.UserSize = &g_UserSize;
    user_flt_info.SidSize = &g_SidSize;

	if (!IsLoggedUser(&user_flt_info))
	{
		FltReleaseFileNameInformation(FNameInfo);
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	filename->Length = FNameInfo->Name.Length;
	filename->MaximumLength = FNameInfo->Name.MaximumLength;
	filename->Buffer = ExAllocatePool(NonPagedPool, FNameInfo->Name.Length);
	if (filename->Buffer == NULL)
	{
		FltReleaseFileNameInformation(FNameInfo);
		Return(after);
	}
	RtlCopyMemory(filename->Buffer, FNameInfo->Name.Buffer, FNameInfo->Name.Length);
	FltReleaseFileNameInformation(FNameInfo);
	return STATUS_SUCCESS;
}

bool StrIsEqual(PUNICODE_STRING filename)
{
	KIRQL old = KeGetCurrentIrql();
	if (old != PASSIVE_LEVEL)
	{
		KeLowerIrql(PASSIVE_LEVEL);
	}
	bool res=  RtlEqualUnicodeString(filename, &g_filename_curr,TRUE);
	if (old != PASSIVE_LEVEL)
	{
		KfRaiseIrql(old);
	}
	return res;
}

void  CreateFileNameByTime(IN PUNICODE_STRING folder_root, OUT PUNICODE_STRING filename, PUNICODE_STRING extention, OUT PUNICODE_STRING ctrl_filename )
{
	TIME_FIELDS TimeFields;
	LARGE_INTEGER CurrentSysTime;
	LARGE_INTEGER CurrentLocTime;
	UNICODE_STRING day;
	UNICODE_STRING month;
	UNICODE_STRING year;
	OBJECT_ATTRIBUTES objAtr;
	IO_STATUS_BLOCK iosb = { 0 };

	KeQuerySystemTime(&CurrentSysTime);
	ExSystemTimeToLocalTime(&CurrentSysTime, &CurrentLocTime);
	RtlTimeToTimeFields(&CurrentLocTime, &TimeFields);

	Int2UnicodeStrPP(TimeFields.Day, &day);
	Int2UnicodeStrPP(TimeFields.Month, &month);
	Int2UnicodeStrPP(TimeFields.Year, &year);

	int sz_nf = day.Length + month.Length + year.Length + extention->Length + sizeof(UNICODE_NULL);
	int sz_f = folder_root->Length + sizeof(L"\\") + +sizeof(L"_") * 2 + sz_nf;
	filename->Length = 0;
	filename->MaximumLength = sz_f;
	filename->Buffer = ExAllocatePool(NonPagedPool, sz_f);
	RtlZeroMemory(filename->Buffer, sz_f);
	ctrl_filename->Length = 0;
	ctrl_filename->MaximumLength = sz_f;
	ctrl_filename->Buffer = ExAllocatePool(NonPagedPool, sz_f);
	RtlZeroMemory(ctrl_filename->Buffer, sz_nf);
	RtlZeroMemory(filename->Buffer, sz_f);

	RtlAppendUnicodeToString(filename, (PWSTR)folder_root->Buffer);
	RtlAppendUnicodeToString(filename, (PWSTR)L"\\");

	RtlAppendUnicodeToString(filename, (PWSTR)day.Buffer);
	RtlAppendUnicodeToString(filename, (PWSTR)L"_");
	RtlAppendUnicodeToString(filename, (PWSTR)month.Buffer);
	RtlAppendUnicodeToString(filename, (PWSTR)L"_");
	RtlAppendUnicodeToString(filename, (PWSTR)year.Buffer);
	RtlAppendUnicodeToString(filename, (PWSTR)extention->Buffer);

	RtlAppendUnicodeToString(ctrl_filename, (PWSTR)day.Buffer);
	RtlAppendUnicodeToString(ctrl_filename, (PWSTR)L"_");
	RtlAppendUnicodeToString(ctrl_filename, (PWSTR)month.Buffer);
	RtlAppendUnicodeToString(ctrl_filename, (PWSTR)L"_");
	RtlAppendUnicodeToString(ctrl_filename, (PWSTR)year.Buffer);
	RtlAppendUnicodeToString(ctrl_filename, (PWSTR)extention->Buffer);

	ExFreePool(day.Buffer);
	ExFreePool(month.Buffer);
	ExFreePool(year.Buffer);
}

void Sleep(ULONG time_out)
{
	LARGE_INTEGER due_time;
	due_time.QuadPart = -((LONGLONG)time_out) * 10000;
	NTSTATUS status = KeDelayExecutionThread(KernelMode, FALSE, &due_time);
}

bool CurrentIRQL()
{
	PCHAR LvInfo = "";
	KIRQL lv = KeGetCurrentIrql();
	switch (lv)
	{
	case PASSIVE_LEVEL://0 Passive release level
		LvInfo = "PASSIVE_LEVEL";
		break;
	case APC_LEVEL://1 APC interrupt level
		LvInfo = "PASSIVE_LEVEL";
		break;
	case DISPATCH_LEVEL:// 2  Dispatcher level
		LvInfo = "LOW_LEVEL";
		break;
	}
	DbgLog("AC / >"__FUNCTION__":Current IRQ level %s\n", LvInfo);
	if (lv == PASSIVE_LEVEL)
	{

		return true;
	}
	return false;
}

NTSTATUS Int2UnicodeStrPP(DWORD val, PUNICODE_STRING str)
{
	str->MaximumLength = 16;
	str->Buffer = ExAllocatePool(PagedPool, str->MaximumLength);
	return RtlIntegerToUnicodeString(val, 0, str);
}

NTSTATUS Int2ANSIStr(DWORD val, PANSI_STRING str)
{
	NTSTATUS status;
	UNICODE_STRING ustr;
	ustr.MaximumLength = 16;
	ustr.Buffer = ExAllocatePool(PagedPool, ustr.MaximumLength);
	str->MaximumLength = 32;
	str->Buffer = ExAllocatePool(PagedPool, str->MaximumLength);

	status = RtlIntegerToUnicodeString(val, 16, &ustr);
	if (!NT_SUCCESS(status))
	{
		ExFreePool(ustr.Buffer);
		return status;
	}
	status = RtlUnicodeStringToAnsiString(str, &ustr, false);
	ExFreePool(ustr.Buffer);
	return status;
}

PWCH wcssplt(PWCH str, PWCH substr)
{
	DbgLog("AC/>"__FUNCTION__" run\n");
	PWCHAR result = NULL;
	PWCHAR subStPos = wcswcs(str, substr);
	if (subStPos != 0)
	{
		int pos = subStPos - str;
		result = (PWCHAR)ExAllocatePoolWithTag(NonPagedPool, sizeof(WCHAR)*pos + sizeof(UNICODE_NULL), VOLUME_TAG);
		RtlCopyMemory(result, str, sizeof(WCHAR)*pos);
		*(result + pos) = UNICODE_NULL;
	}
	return result;
}

void  GetFileName(UNICODE_STRING full_file_name,PUNICODE_STRING file_name)
{
	int g = 0;
	int i = full_file_name.Length - sizeof(wchar_t);
	PWCH data = full_file_name.Buffer;
	while (data[i--] != L'\\');
	while (data[i++] != L'.')
	{
		file_name->Buffer[g] = data[i];
		g++;
	}
}
 //---=23.04.2017=-------
NTSTATUS  RunCompletionDPC(PFLT_CALLBACK_DATA Data,PAC_FILTER_MESSAGE msg, PVOID *completionCtx, byte operation)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	int buff_len = sizeof(AC_WORKITEMSTRUCT);
	PAC_WORKITEMSTRUCT work_data = (PAC_WORKITEMSTRUCT)ExAllocatePool(NonPagedPool, buff_len);
	if (work_data == NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	work_data->msg = msg;
	work_data->type_op = operation;
	work_data->completionCtx = completionCtx;
	
	bool port_is_liber = false;
	for (INT8 i = 0; i < msg_connections; i++)
	{
		if (filterManager.connections[i].busy==false)
		{	
			filterManager.connections[i].busy = true;
			work_data->connection = &filterManager.connections[i];
			port_is_liber = true;
			break;
		}
	}
	if (!port_is_liber)
	{
        DbgLog("AC / >"__FUNCTION__":System resoureces updown!!!\n");
       	ExFreePool(work_data);
		return STATUS_UNSUCCESSFUL;
	}

	PFLT_DEFERRED_IO_WORKITEM workitem = NULL;
	try
	{

		workitem = FltAllocateDeferredIoWorkItem();
		if (workitem == NULL)
		{
			ExFreePool(work_data);
			return STATUS_UNSUCCESSFUL;
		}
		KIRQL prvLv = KeGetCurrentIrql();
		KeLowerIrql(PASSIVE_LEVEL);
		status = FltQueueDeferredIoWorkItem(workitem, Data, AC_WorkItemCallback, DelayedWorkQueue, (PVOID)work_data);
		KfRaiseIrql(prvLv);
		if (!NT_SUCCESS(status))
		{

			ExFreePool(work_data);
			return STATUS_UNSUCCESSFUL;
		}

	}
	except(EXCEPTION_EXECUTE_HANDLER)
	{
		Data->IoStatus.Status = GetExceptionCode();
		Data->IoStatus.Information = 0;
		ExFreePool(work_data);
		FltFreeDeferredIoWorkItem(workitem);
		return STATUS_UNSUCCESSFUL;
	}
	return STATUS_SUCCESS;
}
