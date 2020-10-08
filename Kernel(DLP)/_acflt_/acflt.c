

#include "acflt.h"
#include "Common.h"
#include "io_ctrl_code.h"

PEPROCESS g_UserProcess;
ULONG g_usr_nameLength;
char* g_RegstrSID = NULL;

PUNICODE_STRING g_pDomainFFO;
PUNICODE_STRING g_pUserFFO;
PUNICODE_STRING g_pSidFFO;

PUNICODE_STRING g_pDomainREG;
PUNICODE_STRING g_pUserREG;

ULONG g_DomainSize;
ULONG g_UserSize;
ULONG g_SidSize;

ULONG g_DomainSizeREG;
ULONG g_UserSizeREG;

LARGE_INTEGER g_CmCookie = { 0 };

bool g_unload_dev = false;
bool g_copy_op = 0;
bool g_save_op = 0;

UNICODE_STRING  g_processName; 
UNICODE_STRING  g_Parent_processName;
UNICODE_STRING  g_cp_processName;

UNICODE_STRING  g_filename_prev;
UNICODE_STRING  g_ParentDir_prev;

UNICODE_STRING  g_filename_curr;
UNICODE_STRING  g_TrgVolume;

UINT32          g_SourceDev;
PFILE_OBJECT    g_FileObject;

AC_FLT_MANAGER  filterManager;
UNICODE_STRING  system_sid;
UNICODE_STRING  service_sid;
UNICODE_STRING  network_sid;
LARGE_INTEGER   timeout_msg;
LARGE_INTEGER   timeout_msg_r;
LIST_ENTRY      g_fileOpened_list; 
LIST_ENTRY      g_fltUser_list;

KSPIN_LOCK g_fltSpinLockAc;
KSPIN_LOCK g_fltSpinLockOp;
KSPIN_LOCK g_fltSpinLockMsg;
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-= DriverEntry =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

NTSTATUS DriverEntry(IN PDRIVER_OBJECT fsDriverObject, IN PUNICODE_STRING ac_RegistryPath)
{	 
	UNREFERENCED_PARAMETER(ac_RegistryPath);
   // DbgBreakPoint();
	//DbgLog("AC/>"__FUNCTION__":Run\n");
  	PCHAR ConfigInfo;

	UNICODE_STRING test;
	PDEVICE_OBJECT fdo;
	UNICODE_STRING devName;
	UNICODE_STRING devLink;

	OBJECT_ATTRIBUTES obj_attr;
		
	ULONG length;
	
	int i = 0;

	NTSTATUS status=STATUS_UNSUCCESSFUL;
	//UNICODE_STRING AltitudeString = RTL_CONSTANT_STRING(L"360000");
	RtlInitUnicodeString(&devName, (PWSTR)AC_FLT_DEV_NAME);
	RtlInitUnicodeString(&devLink, (PWSTR)AC_FLT_DOS_DEV_NAME);
    fsDriverObject->DriverUnload = OnUnload;

	InitGlobalVar();

   	status = IoCreateDevice(fsDriverObject, sizeof(PDEVICE_OBJECT), &devName, AC_DEVICE, 0, FALSE, &fdo);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	fdo->Flags |= DO_BUFFERED_IO;
	//============================================IoCreateDevice status ok======================================================

	status = IoCreateSymbolicLink(&devLink, &devName);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	
	
	//=========================================== Регистрация функций диспетчера ===============================================

	for (i = 0; i <=IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		fsDriverObject->MajorFunction[i] = OnDispatchIRQ;
	}
 
	filterManager.pDriverObject = fsDriverObject;
	g_processName.Length = 0;
	g_processName.MaximumLength = MAX_STR_LEN;
	g_processName.Buffer = ExAllocatePool(NonPagedPool, g_processName.MaximumLength);
	if (g_processName.Buffer == NULL)
	{
		
		return status;
	}

	g_cp_processName.Length = 0;
	g_cp_processName.MaximumLength = MAX_STR_LEN;
	g_cp_processName.Buffer = ExAllocatePool(NonPagedPool, g_cp_processName.MaximumLength);
	if (g_cp_processName.Buffer == NULL)
	{

		return status;
	}
 	RtlZeroMemory(g_processName.Buffer, MAX_STR_LEN);
	
//============================================== Регистрация фильтра ===================================================

	status = FltRegisterFilter(fsDriverObject, &FilterRegistration, &filterManager.pFilter);
	if (!NT_SUCCESS(status))
	{
		DbgLog("AC/>"__FUNCTION__":Driver not started. ERROR FltRegisterFilter - 0x%x\n", status);
		return status;
	}

	init_process_info();

	//============================================= Инициализация  портов коммуникации ===================================
	UNICODE_STRING simb_linkFlt;
	UNICODE_STRING simb_linkReg;
	PSECURITY_DESCRIPTOR sc_descflt;
	PSECURITY_DESCRIPTOR sc_descreg;
	RtlInitUnicodeString(&simb_linkFlt, acPortCommFlt);
	RtlInitUnicodeString(&simb_linkReg, acPortCommReg);

	status = FltBuildDefaultSecurityDescriptor(&sc_descflt, FLT_PORT_ALL_ACCESS);

	if (!NT_SUCCESS(status))
	{
		DbgLog("AC/>"__FUNCTION__":Driver not started. ERROR FltBuildDefaultSecurityDescriptor - 0x%x\n", status);
		return status;
	}
	InitializeObjectAttributes( &obj_attr,
								&simb_linkFlt,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								NULL,
								sc_descflt);
	for (int i = 0; i < msg_connections; i++)
	{
		filterManager.connections[i].port=NULL;
		filterManager.connections[i].busy = true;
	}
	status = FltCreateCommunicationPort(filterManager.pFilter,
										&filterManager.pSFltPort,
										&obj_attr,
										NULL,
										FltPortConnect,
										FltPortDisconnect,
										NULL,
		                                msg_connections);
	FltFreeSecurityDescriptor(sc_descflt);
	if(!NT_SUCCESS(status))
	{
		DbgLog("AC/>"__FUNCTION__":Driver not started. ERROR FltCreateCommunicationPort - 0x%x\n", status);
		return status;
	}

	status = FltBuildDefaultSecurityDescriptor(&sc_descreg, FLT_PORT_ALL_ACCESS);

	if (!NT_SUCCESS(status))
	{
		DbgLog("AC/>"__FUNCTION__":Driver not started. ERROR FltBuildDefaultSecurityDescriptor - 0x%x\n", status);
		return status;
	}
	InitializeObjectAttributes( &obj_attr,
								&simb_linkReg,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								NULL,
								sc_descreg);
    for (int i = 0; i < msg_Rconnections; i++)
	{
		filterManager.connectionsR[i].port=NULL;
		filterManager.connectionsR[i].busy = true;
	}
	status = FltCreateCommunicationPort(filterManager.pFilter,
										&filterManager.pSRegPort,
										&obj_attr,
										NULL,
										RegPortConnect,
										RegPortDisconnect,
										NULL,
										msg_Rconnections);

	FltFreeSecurityDescriptor(sc_descreg);
	if (!NT_SUCCESS(status))
	{
		DbgLog("AC/>"__FUNCTION__":Driver not started. ERROR FltCreateCommunicationPort - 0x%x\n", status);
		return status;
	}

	g_UserProcess = PsGetCurrentProcess();
    DbgLog("\nAC/>"__FUNCTION__": Filter STARTED(ver 1.07g)\n");
	KeInitializeSpinLock(&g_fltSpinLockAc);
	KeInitializeSpinLock(&g_fltSpinLockOp);
	KeInitializeSpinLock(&g_fltSpinLockMsg);
    KeInitializeEvent( &filterManager.kEvtEndUnregConnect , NotificationEvent , FALSE );
	return status;
}
