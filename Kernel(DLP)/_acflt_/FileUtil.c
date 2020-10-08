#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <Ntifs.h>
#include "acflt.h"
#include "FileUtil.h"
#include "ServiceRule.h"
#include "Helper.h"
#include "DispatchIRQ.h"

NTSTATUS SetMessageContext(MessageCtx  *msg, int op, PUNICODE_STRING previos_file, PUNICODE_STRING current_file);

NTSTATUS GetDosRoot2File(PCFLT_RELATED_OBJECTS FltObjects,PFLT_FILE_NAME_INFORMATION  flNameInfo,PUNICODE_STRING ph_full_file_name);

void GetSimbolickOp(PANSI_STRING _str_ev, int op);

NTSTATUS SetMessageContext (	 MessageCtx  *msg,  int op,	  PUNICODE_STRING previos_file,	   PUNICODE_STRING current_file	 )
{

	NTSTATUS status = STATUS_UNSUCCESSFUL;
	LARGE_INTEGER CurrentSysTime;
	PROCESS_BASIC_INFORMATION* buffer;
	ULONG sizeInfo = sizeof(PROCESS_BASIC_INFORMATION);
	ULONG returnedLength;
	RtlZeroMemory(msg->procces_name, F_MAX_PATH);
	RtlZeroMemory(msg->filename_prev, F_MAX_PATH);
	RtlZeroMemory(msg->filename_curr, F_MAX_PATH);
	RtlZeroMemory(msg->domen_name, STR_LEN);
	RtlZeroMemory(msg->user_name, STR_LEN);

	buffer = ExAllocatePool(NonPagedPool, sizeInfo);
	if (buffer != NULL)
	{
		KeQuerySystemTime(&CurrentSysTime);
		ExSystemTimeToLocalTime(&CurrentSysTime, &msg->CurrentLocTime);

		msg->pId = ((PROCESS_BASIC_INFORMATION*)buffer)->UniqueProcessId;
		msg->op = op;

		if (g_processName.Length > 0 && g_processName.Length <= F_MAX_PATH)
		{
			RtlCopyMemory(msg->procces_name, g_processName.Buffer, g_processName.Length);
		}

		if (previos_file != NULL)
		{
   			RtlCopyMemory(msg->filename_prev, previos_file->Buffer, previos_file->Length);
 	    }

		if (current_file != NULL)
		{
 			RtlCopyMemory(msg->filename_curr, current_file->Buffer, current_file->Length);
		}
		bool rez = FindSubStr(g_pUserFFO, L"SYSTEM");
		if (!rez)
		{
			rez = FindSubStr(g_pUserFFO, L"ÑÈÑÒÅÌÀ");
		}
		if (rez)
		{
			return STATUS_UNSUCCESSFUL;
		}
		RtlCopyMemory(msg->domen_name, g_pDomainFFO->Buffer, g_pDomainFFO->Length);
		RtlCopyMemory(msg->user_name, g_pUserFFO->Buffer, g_pUserFFO->Length);
        RtlCopyMemory(msg->sid, g_pSidFFO->Buffer, g_pSidFFO->Length);
		return STATUS_SUCCESS;
	}
	DbgLog("AC/>"__FUNCTION__":ZwQueryInformationProcess error\n");
	return STATUS_UNSUCCESSFUL;
}

void GetSimbolickOp(PANSI_STRING _str_ev,int op )
{
	switch (op)
	{
	case FDRVCONVO_NEW:
		RtlInitAnsiString(_str_ev, "(C)CREATE_NEW");
		break;
	case FDRVCONVO_OPEN:
		RtlInitAnsiString(_str_ev, "(C)OPEN");
		break;
	case FDRVCONVO_COPY:
		RtlInitAnsiString(_str_ev, "(C)COPY");
		break;
	case  FDRVCONVO_RENAME:
		RtlInitAnsiString(_str_ev, "(R)RENAME");
		break;
	case  FDRVCONVO_MOVE:
		RtlInitAnsiString(_str_ev, "(M)MOVE");
		break;
	case FDRVCONVO_DELETE:
		RtlInitAnsiString(_str_ev, "(D)DELETE");
		break;
	case FDRVCONVO_CLOSE:
		RtlInitAnsiString(_str_ev, "(Cl)CLOSE");

		break;
	}
}

NTSTATUS GetDosRoot2File(PCFLT_RELATED_OBJECTS FltObjects, PFLT_FILE_NAME_INFORMATION  flNameInfo, PUNICODE_STRING ph_full_file_name)
{

	NTSTATUS status;
	UNICODE_STRING fileDosDrive;

	if ((flNameInfo->Name.Length == 0) && (flNameInfo->Volume.Length == 0))
	{
		DbgLog("AC/>"__FUNCTION__":GetDosRoot2File zero data\n");
		return STATUS_UNSUCCESSFUL;
	}

	int fn_len = flNameInfo->Name.Length - (flNameInfo->Volume.Length+ flNameInfo->Share.Length);
	if (fn_len < 2)
	{

		DbgLog("AC/>"__FUNCTION__":GetDosRoot2File data error fn_len < 2\n");
		return STATUS_UNSUCCESSFUL;
	}

 	GetDosDeviceName(FltObjects->FileObject->DeviceObject, &flNameInfo->Share, &fileDosDrive);
	if (fileDosDrive.Length < 2)
	{
		DbgLog("AC/>"__FUNCTION__":GetDosRoot2File data error fn_len < 2\n");
		return STATUS_UNSUCCESSFUL;
	}

	int vl_lenght = flNameInfo->Share.Length+flNameInfo->Volume.Length;
	int drv_len = fileDosDrive.Length - sizeof(wchar_t);
	int w_len =sizeof(wchar_t);
	KIRQL  oldIrql;
	
	KeAcquireSpinLock(&g_fltSpinLockMsg, &oldIrql);

	RtlZeroMemory(ph_full_file_name->Buffer, ph_full_file_name->MaximumLength);	
   
	wchar_t *pIm_buff = ph_full_file_name->Buffer;
	wchar_t *pfilename = flNameInfo->Name.Buffer;
	wchar_t *pbuff = fileDosDrive.Buffer;

	
	pfilename += vl_lenght / w_len;
		
	RtlCopyMemory(pIm_buff, pbuff + 1, drv_len);
   
	pIm_buff += (drv_len / w_len);
	*pIm_buff = L':';  
	pIm_buff ++;

  	if (fn_len == 2)
	{
		*pIm_buff = L'\\';
		ph_full_file_name->Length = wcslen(ph_full_file_name->Buffer);
		ph_full_file_name->Length *= w_len;
		KeReleaseSpinLock(&g_fltSpinLockMsg, oldIrql);
		return STATUS_SUCCESS;
	}

	try
	{
		RtlCopyMemory(pIm_buff, pfilename, fn_len);
	}
	except(EXCEPTION_EXECUTE_HANDLER)
	{
		KeReleaseSpinLock(&g_fltSpinLockMsg, oldIrql);
		DbgLog("AC/>"__FUNCTION__":GetDosRoot2File EXCEPTION_EXECUTE_HANDLER \n");
		return STATUS_UNSUCCESSFUL;
	}
	ph_full_file_name->Length = wcslen(ph_full_file_name->Buffer);
	ph_full_file_name->Length *= w_len;

	KeReleaseSpinLock(&g_fltSpinLockMsg, oldIrql);
	return STATUS_SUCCESS;
}
//NTSTATUS GetDosRoot2File(PCFLT_RELATED_OBJECTS FltObjects, PFLT_FILE_NAME_INFORMATION  flNameInfo, PUNICODE_STRING ph_full_file_name)
//{
//
//	NTSTATUS status;
//	UNICODE_STRING fileDosDrive;
//
//	if ((flNameInfo->Name.Length == flNameInfo->Volume.Length) && (flNameInfo->Volume.Length == 0))
//	{
//		DbgLog("AC/>"__FUNCTION__":GetDosRoot2File zero data\n");
//		return STATUS_UNSUCCESSFUL;
//	}
//
//	GetDosDeviceName(FltObjects->FileObject->DeviceObject, &flNameInfo->Share, &fileDosDrive);
//
//	int fn_len = flNameInfo->Name.Length - (flNameInfo->Volume.Length+ flNameInfo->Share.Length);
//	if (fn_len < 2)
//	{
//
//		DbgLog("AC/>"__FUNCTION__":GetDosRoot2File data error fn_len < 2\n");
//		return STATUS_UNSUCCESSFUL;
//	}
// 
//	int vl_lenght = flNameInfo->Share.Length+flNameInfo->Volume.Length;
//	int drv_len = fileDosDrive.Length - sizeof(wchar_t);
//	RtlZeroMemory(ph_full_file_name->Buffer, ph_full_file_name->MaximumLength);	
//
//	wchar_t *pIm_buff = ph_full_file_name->Buffer;
//	wchar_t *pfilename = flNameInfo->Name.Buffer;
//	wchar_t *pbuff = fileDosDrive.Buffer;
//	
//	pfilename += (vl_lenght / sizeof(wchar_t));
//	RtlCopyMemory(pIm_buff, pbuff + 1, drv_len);
//	pIm_buff += (drv_len / sizeof(wchar_t));
//	*pIm_buff = L':';
//  	if (fn_len == 2)
//	{
//		pIm_buff += sizeof(wchar_t);
//		*pIm_buff = L'\\';
//		ph_full_file_name->Length = wcslen(ph_full_file_name->Buffer);
//		ph_full_file_name->Length *= sizeof(wchar_t);
//		return STATUS_SUCCESS;
//	}
//
//	pIm_buff++;
//	try
//	{
//		RtlCopyMemory(pIm_buff, pfilename, fn_len);
//	}
//	except(EXCEPTION_EXECUTE_HANDLER)
//	{
//
//		DbgLog("AC/>"__FUNCTION__":GetDosRoot2File EXCEPTION_EXECUTE_HANDLER \n");
//		return STATUS_UNSUCCESSFUL;
//	}
//	ph_full_file_name->Length = wcslen(ph_full_file_name->Buffer);
//	ph_full_file_name->Length *= sizeof(wchar_t);
//	return STATUS_SUCCESS;
//}