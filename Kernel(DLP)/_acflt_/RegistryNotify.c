#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <Ntifs.h>
#include "acflt.h"
#include "FileUtil.h"
#include "ServiceRule.h"
#include "Helper.h"
#include "DispatchIRQ.h"
#include "RegistryNotify.h"

NTSTATUS SendMessageRg(PUNICODE_STRING key_name, PUNICODE_STRING domen_name, PUNICODE_STRING user_name, enum RegOperation op);

NTSTATUS Ac_RegistryCallback(__in PVOID CallbackContext, __in PVOID Argument1, __in PVOID Argument2);
bool IsLoggedUserReg(user_info *user);

//-=07/10/2017=-
void  AC_RgWorkItemCallback(_In_ PFLT_DEFERRED_IO_WORKITEM WorkItem, PFLT_CALLBACK_DATA data, _In_ PVOID DataPtr);
NTSTATUS  SetCompletionDPCforReg(void *Data,PAC_FILTER_MESSAGErg msg,PORT_CONNECT *connection);

void  AC_RgWorkItemCallback(_In_ PFLT_DEFERRED_IO_WORKITEM WorkItem, PFLT_CALLBACK_DATA data, _In_ PVOID DataPtr)
{
 	ULONG replylen = sizeof(FILTER_REPLY_HEADER) + sizeof(unsigned char);
	PAC_WORKITEMSTRUCTRG work_data = (PAC_WORKITEMSTRUCTRG)DataPtr;
	DbgLog("AC/>"__FUNCTION__":.Number connection is %d\n", work_data->connection->num_connect);

	PAC_FILTER_MESSAGErg msg = work_data->msg;
	ULONG pktlen = sizeof(MessageCtxRg);
 	PFLT_PORT client_port = work_data->connection->port; 
	PVOID reply = ExAllocatePool(NonPagedPool, replylen);
	RtlZeroMemory(reply, replylen);

	NTSTATUS status = FltSendMessage(filterManager.pFilter,
										&client_port,
										msg,
										pktlen,
										reply,
										&replylen,
										&timeout_msg_r);

	work_data->connection->busy = false;
	bool allow = ((PAC_FLT_REPLY)reply)->bAllow;
	ExFreePool(msg);
	ExFreePool(reply);
	if (NT_SUCCESS(status))
	{
		if (status == STATUS_TIMEOUT)
		{
			DbgLog("\nAC/>"__FUNCTION__": FltSendMessage TIMEOUT(no error,successfull)\n");
		}
		else
		{
			DbgLog("\nAC/>"__FUNCTION__":(D)elete OK\n");
		}
        
       FltCompletePendedPreOperation(data, FLT_PREOP_SUCCESS_NO_CALLBACK, NULL);
       ExFreePool(work_data);
	   FltFreeDeferredIoWorkItem(WorkItem);
    }
}

NTSTATUS  SetCompletionDPCforReg(void *Data,PAC_FILTER_MESSAGErg msg,PORT_CONNECT *connection)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	int buff_len = sizeof(AC_WORKITEMSTRUCTRG);
	PAC_WORKITEMSTRUCTRG work_data = (PAC_WORKITEMSTRUCTRG)ExAllocatePool(NonPagedPool, buff_len);
	if (work_data == NULL)
	{
        DbgLog("\nAC/>"__FUNCTION__":(E)rror   work_data = (PAC_WORKITEMSTRUCTRG)ExAllocatePool \n");
		return STATUS_UNSUCCESSFUL;
	}
	work_data->msg = msg;
	work_data->connection = connection;
	PFLT_DEFERRED_IO_WORKITEM workitem = NULL;
	try
	{
        PFLT_CALLBACK_DATA callbackData =  filterManager.data;
        if(callbackData == NULL)
        {
          DbgLog("\nAC/>"__FUNCTION__":(E)rror   callbackData NULL \n");
		  return STATUS_UNSUCCESSFUL;
        }
        //PDRIVER_OBJECT pDriverObject = filterManager.pDriverObject;
        //PIRP Irp=pDriverObject->DeviceObject->CurrentIrp;
        //if(Irp == NULL)
        //{
        //  DbgLog("\nAC/>"__FUNCTION__":(E)rror Get current IRP \n");
        //  return STATUS_UNSUCCESSFUL;
        //}
        //PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
   
        //status = FltAllocateCallbackData(filterManager.flt_instance,NULL,&callbackData);
        //if(status == STATUS_INSUFFICIENT_RESOURCES)
        //{
        //  DbgLog("\nAC/>"__FUNCTION__":(E)rror FltAllocateCallbackData \n");
        //  return STATUS_UNSUCCESSFUL;
        //}
        ////callbackData->Iopb->MajorFunction = IrpStack->MajorFunction;
        ////callbackData->Iopb->MinorFunction = IrpStack->MinorFunction;

		workitem = FltAllocateDeferredIoWorkItem();

		if (workitem == NULL)
		{
            DbgLog("\nAC/>"__FUNCTION__":(E)rror  FltAllocateDeferredIoWorkItem() \n");
			ExFreePool(work_data);
			return STATUS_UNSUCCESSFUL;
		}
		KIRQL prvLv = KeGetCurrentIrql();
        if(prvLv!=PASSIVE_LEVEL)
        {
		 KeLowerIrql(PASSIVE_LEVEL);
         DbgLog("\nAC/>"__FUNCTION__": SET PASSIVE_LEVEL \n");
        }
       
		status = FltQueueDeferredIoWorkItem(workitem, callbackData, AC_RgWorkItemCallback, DelayedWorkQueue, (PVOID)work_data);
		
        if(prvLv!=PASSIVE_LEVEL)
        {
          KfRaiseIrql(prvLv);
          DbgLog("\nAC/>"__FUNCTION__": RETURN TO OLD LEVEL \n");
        }
		if (!NT_SUCCESS(status))
		{
            DbgLog("\nAC/>"__FUNCTION__":(E)rror  FltQueueDeferredIoWorkItem() \n");
            FltFreeCallbackData(callbackData);
            ExFreePool(work_data->msg);
			ExFreePool(work_data);
			return STATUS_UNSUCCESSFUL;
		}

	}
	except(EXCEPTION_EXECUTE_HANDLER)
	{
        DbgLog("\nAC/>"__FUNCTION__":(E)rror  except(EXCEPTION_EXECUTE_HANDLER) \n");
		ExFreePool(work_data);
		FltFreeDeferredIoWorkItem(workitem);
		return STATUS_UNSUCCESSFUL;
	}
	return STATUS_SUCCESS;
}
NTSTATUS SendDeleteMessageRg(void *Data,PUNICODE_STRING key_name, PUNICODE_STRING domen_name, PUNICODE_STRING user_name, enum RegOperation op)
{
  	ULONG replylen = sizeof(FILTER_REPLY_HEADER) + sizeof(unsigned char);
	ULONG pktlen = sizeof(MessageCtxRg);
	PVOID reply = ExAllocatePool(NonPagedPool, replylen);
	PAC_FILTER_MESSAGErg msg = ExAllocatePool(NonPagedPool, pktlen);

	RtlZeroMemory(msg, pktlen);
	RtlZeroMemory(reply, replylen);

	RtlZeroMemory(msg->Notification.key_name, F_MAX_PATH);
	RtlZeroMemory(msg->Notification.domen_name,STR_LEN);
	RtlZeroMemory(msg->Notification.user_name, STR_LEN);

	RtlCopyMemory(msg->Notification.key_name, key_name->Buffer, key_name->Length);
	RtlCopyMemory(msg->Notification.domen_name, domen_name->Buffer, domen_name->Length);
	RtlCopyMemory(msg->Notification.user_name, user_name->Buffer, user_name->Length);

	msg->Notification.op = op;

    NTSTATUS status;
    bool port_is_free = false;
    PORT_CONNECT *pPortConnect = NULL;
	for (INT8 i = 0; i < msg_Rconnections; i++)
	{
		if (filterManager.connectionsR[i].busy==false)
		{	
			filterManager.connectionsR[i].busy = true;
			port_is_free = true;
            pPortConnect =  &filterManager.connectionsR[i];
			break;
		}
	}
	if (!port_is_free || pPortConnect == NULL)
	{
        DbgLog("AC / >"__FUNCTION__":System resoureces updown!!!\n");
 		return STATUS_UNSUCCESSFUL;
	}

    status = SetCompletionDPCforReg(Data,msg,pPortConnect);

	if (!NT_SUCCESS(status))
		{
			ExFreePool(msg);
			DbgLog("AC/>"__FUNCTION__": SetCompletionDPCforReg->status  error = 0x%08x \n", status);
			return STATUS_SUCCESS;
		}
 		
  return FLT_PREOP_PENDING;
 
}
NTSTATUS SendMessageRg(PUNICODE_STRING key_name, PUNICODE_STRING domen_name, PUNICODE_STRING user_name, enum RegOperation op)
{

	ULONG replylen = sizeof(FILTER_REPLY_HEADER) + sizeof(unsigned char);
	ULONG pktlen = sizeof(MessageCtxRg);
	PVOID reply = ExAllocatePool(NonPagedPool, replylen);
	PAC_FILTER_MESSAGErg msg = ExAllocatePool(NonPagedPool, pktlen);

	RtlZeroMemory(msg, pktlen);
	RtlZeroMemory(reply, replylen);

	RtlZeroMemory(msg->Notification.key_name, F_MAX_PATH);
	RtlZeroMemory(msg->Notification.domen_name,STR_LEN);
	RtlZeroMemory(msg->Notification.user_name, STR_LEN);

	RtlCopyMemory(msg->Notification.key_name, key_name->Buffer, key_name->Length);
	RtlCopyMemory(msg->Notification.domen_name, domen_name->Buffer, domen_name->Length);
	RtlCopyMemory(msg->Notification.user_name, user_name->Buffer, user_name->Length);

	msg->Notification.op = op;

	KIRQL prvLv = KeGetCurrentIrql();
	KeLowerIrql(PASSIVE_LEVEL);

	NTSTATUS status =	FltSendMessage( filterManager.pFilter,
										&filterManager.pCRegPort,
										msg,
										pktlen,
										(PVOID)reply,
										&replylen,
										&timeout_msg_r);
	
	KfRaiseIrql(prvLv);

	bool allow = ((PAC_FLT_REPLY)reply)->bAllow;
	ExFreePool(msg);
	ExFreePool(reply);

	if (NT_SUCCESS(status))
	{
		if (status == STATUS_TIMEOUT)
		{
			DbgLog("\nAC/>"__FUNCTION__": FltSendMessage TIMEOUT(no error,successfull)\n");
		}
		else
		{
			UNICODE_STRING operation={0};
			switch (op)
			{
			case REGO_PST_UNKNOW:
				 RtlInitUnicodeString(&operation, (PWSTR)L"(U)NKNOW");
			break;
			case REGO_PST_CREATE:
				 RtlInitUnicodeString(&operation, (PWSTR)L"(C)REATE KEY");
			break;
			case REGO_PST_OPEN:
				 RtlInitUnicodeString(&operation, (PWSTR)L"(O)OPEN KEY");
			break;
			case REGO_PRE_DELETE:
				 RtlInitUnicodeString(&operation, (PWSTR)L"(D)ELETE KEY");
			break;

			}
			//DbgLog("\nAC/>"__FUNCTION__":%ws\n", operation.Buffer);

		}
		if (allow == 0 && status != STATUS_TIMEOUT)
		{

		}

	}
	//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
	return status;
}

NTSTATUS Ac_RegistryCallback(__in PVOID CallbackContext, __in PVOID Argument1, __in PVOID Argument2)
{

	bool unload_dev = *(bool *)CallbackContext;
	if (unload_dev  ||  g_unload_dev  )
	{
		return STATUS_SUCCESS;
	}
	NTSTATUS status = STATUS_SUCCESS;

	PREG_CREATE_KEY_INFORMATION createKey;
	PREG_DELETE_KEY_INFORMATION deleteKey;

	REG_NOTIFY_CLASS NotifyClass = (REG_NOTIFY_CLASS)Argument1;

	switch (NotifyClass)
	{
	case RegNtPreCreateKeyEx:
	//case RegNtPreOpenKeyEx:
	{
		createKey = (PREG_CREATE_KEY_INFORMATION)Argument2;
		if (createKey != NULL)
		{
			createKey->CallContext = (PVOID)0;
    
			user_info userREG;
			userREG.pUser = g_pUserREG;
			userREG.pDomain = g_pDomainREG;
			userREG.DomainSize = &g_DomainSizeREG;
			userREG.UserSize = &g_UserSizeREG;
            userREG.pSid=NULL;
            userREG.SidSize=NULL;
			bool rez = IsLoggedUserReg(&userREG);
			if (rez!= true)
			{
				return STATUS_SUCCESS;
			}
			createKey->CallContext = (PVOID)1;
		}
	}
	break;
	case RegNtPostCreateKeyEx:
	//case RegNtPostOpenKeyEx:
	{

		PREG_POST_OPERATION_INFORMATION pPostOpInfo = (PREG_POST_OPERATION_INFORMATION)Argument2;
		PREG_CREATE_KEY_INFORMATION pPreOpInfo = (PREG_CREATE_KEY_INFORMATION)pPostOpInfo->PreInformation;

		if (pPostOpInfo != NULL
			&& pPreOpInfo != NULL
			&& pPostOpInfo->CallContext
			&& pPostOpInfo->Object)
		   {

			OBJECT_NAME_INFORMATION *objName = NULL;
			ULONG return_length = 0;
			PVOID Object = pPostOpInfo->Object;
			if (!NT_SUCCESS(pPostOpInfo->Status))
			{
				return STATUS_SUCCESS;
			}

			if((MmIsAddressValid(Object) != TRUE) || (Object <= 0))
			 {
				break;
			 }
			status = ObQueryNameString(Object, (POBJECT_NAME_INFORMATION)objName, 0, &return_length);

			if (status == STATUS_INFO_LENGTH_MISMATCH)
			{

				objName = (OBJECT_NAME_INFORMATION *)ExAllocatePool(NonPagedPool, return_length);
				if (objName == NULL)
				{
					return STATUS_SUCCESS;
				}

				status = ObQueryNameString(Object, (POBJECT_NAME_INFORMATION)objName, return_length, &return_length);

				if (NT_SUCCESS(status))
				{

					enum RegOperation op = REGO_PST_CREATE;
					if (NotifyClass == RegNtPostOpenKeyEx)op = REGO_PST_OPEN;
					status = SendMessageRg(&objName->Name,g_pDomainREG,g_pUserREG,op);
				}
				ExFreePool(objName);
			}

		}
	}
	break;

	case RegNtPreDeleteKey:
	{
		OBJECT_NAME_INFORMATION *objName = NULL;
		ULONG return_length = 1024;
		deleteKey = (PREG_DELETE_KEY_INFORMATION)Argument2;
		if (deleteKey != NULL)
		{
			user_info userREG;
			userREG.pUser = g_pUserREG;
			userREG.pDomain = g_pDomainREG;
			userREG.DomainSize = &g_DomainSizeREG;
			userREG.UserSize = &g_UserSizeREG;
            userREG.pSid=NULL;
            userREG.SidSize=NULL;
			bool rez = IsLoggedUserReg(&userREG);
			if (rez != true)
			{
				return STATUS_SUCCESS;
			}
			PVOID Object = deleteKey->Object;
			if ((MmIsAddressValid(Object) != TRUE) || (Object <= 0))
			{
				return STATUS_SUCCESS;
			}

			status = ObQueryNameString(Object, objName, 0, &return_length);

			if (status == STATUS_INFO_LENGTH_MISMATCH)
			{
				objName = (OBJECT_NAME_INFORMATION *)ExAllocatePool(NonPagedPool, return_length);
				status = ObQueryNameString(deleteKey->Object, (POBJECT_NAME_INFORMATION)objName, return_length, &return_length);
				if (NT_SUCCESS(status))
				{

					status = SendDeleteMessageRg(Object,&objName->Name,g_pDomainREG,	g_pUserREG,	REGO_PRE_DELETE);
					DbgLog("AC/>"__FUNCTION__":Domain  %ws \n", g_pDomainREG->Buffer);
					DbgLog("AC/>"__FUNCTION__":User Name  %ws \n", g_pUserREG->Buffer);
					DbgLog("AC/>"__FUNCTION__":delete key %ls \n", objName->Name.Buffer);
					//#ifndef   DBG
					//DbgLog"AC/>"__FUNCTION__":delete key %ls \n", objName->Name.Buffer);
					//#endif

				}
                
				ExFreePool(objName);
			}
		}
	}
	break;
 }
 return status;
}
bool IsLoggedUserReg(user_info *user)
{
	KIRQL old = KeGetCurrentIrql();
	if (old != PASSIVE_LEVEL)
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
	ULONG nameLength = SECURITY_MAX_SID_SIZE * 2;

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
