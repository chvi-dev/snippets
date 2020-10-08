#include "acflt.h"
#include "io_ctrl_code.h"
#include "DispatchIRQ.h"
#include "ServiceRule.h"
#include "Helper.h"
#include "FileUtil.h"
#include  <Ntddk.h>
void Unload(PDRIVER_OBJECT DriverObject);
void OnUnload(PDRIVER_OBJECT DriverObject);
//============================== отложенная обработка файловой операции =========================================================================
void  AC_WorkItemCallback(_In_ PFLT_DEFERRED_IO_WORKITEM WorkItem, PFLT_CALLBACK_DATA data, _In_ PVOID DataPtr);
void  WorkItem(PDEVICE_OBJECT DeviceObject,PVOID  Context);

void  WorkItem( PDEVICE_OBJECT DeviceObject , PVOID  Context)
{     
  UNICODE_STRING uRegName;
  RtlInitUnicodeString(&uRegName, L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Services\\acflt");
  ZwUnloadDriver(&uRegName);      
  DbgLog("\n AC/>"__FUNCTION__":wait end  event \n");
  KeWaitForSingleObject( &filterManager.kEvtEndUnregConnect , Executive , KernelMode , FALSE , NULL );
  IoFreeWorkItem( (PIO_WORKITEM)Context );
  DbgLog("AC/>"__FUNCTION__" :End OK \n");
}
void  AC_WorkItemCallback(_In_ PFLT_DEFERRED_IO_WORKITEM WorkItem, PFLT_CALLBACK_DATA data, _In_ PVOID DataPtr)
{
 	ULONG replylen = sizeof(FILTER_REPLY_HEADER) + sizeof(unsigned char);
	PAC_WORKITEMSTRUCT work_data = (PAC_WORKITEMSTRUCT)DataPtr;
	DbgLog("AC/>"__FUNCTION__":.Number connection is %d\n", work_data->connection->num_connect);
	byte  type_op_ret = work_data->type_op;
	PAC_FILTER_MESSAGE msg = work_data->msg;
	PVOID *completionCtx = work_data->completionCtx;
	if (data == 0)
	{
		ExFreePool(msg);
		goto e_ret;
	}
	PFLT_CALLBACK_DATA Data= (PFLT_CALLBACK_DATA)data;
	UCHAR MajorFunction = Data->Iopb->MajorFunction;
	PFILE_OBJECT FileObject = Data->Iopb->TargetFileObject; 
	msg->Notification.trg_DeviceType = FileObject->DeviceObject->DeviceType;
	msg->Notification.src_DeviceType = msg->Notification.trg_DeviceType;
	ULONG pktlen = sizeof(MessageCtx);
 	PFLT_PORT client_port = work_data->connection->port; 
	PVOID reply = ExAllocatePool(NonPagedPool, replylen);
	RtlZeroMemory(reply, replylen);

	NTSTATUS status = FltSendMessage(filterManager.pFilter,
										&client_port,
										msg,
										pktlen,
										reply,
										&replylen,
										&timeout_msg);

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
		if (allow == 0 && status != STATUS_TIMEOUT)
		{
			switch (type_op_ret)
			{					
			case 0:	
			    *completionCtx = (PVOID)STS_OP_BLOCKED;
				FltCompletePendedPreOperation(Data, FLT_PREOP_SYNCHRONIZE, completionCtx);
				DbgLog("AC/>"__FUNCTION__" :FltCompletePendedPreOperation \n");
				break;
			case 1:
				if (MajorFunction == IRP_MJ_CREATE)
				{
					FltCancelFileOpen(Data->Iopb->TargetInstance, Data->Iopb->TargetFileObject);
				} 
				Data->IoStatus.Status = STATUS_NO_SUCH_PRIVILEGE;//STATUS_ACCESS_DENIED;//STATUS_FILE_INVALID;//
				Data->IoStatus.Information = 0;	
				FltCompletePendedPostOperation(Data);
				DbgLog("AC/>"__FUNCTION__" :FltCompletePendedPostOperation \n");
				break;
			default:
				DbgLog("AC/>"__FUNCTION__" :work_data->type_op =%d \n", work_data->type_op);

			}
			goto e_ret;
		}
		if (type_op_ret == 1)
		{
			byte cmp = (byte)completionCtx;
			if (cmp == STS_OP_EFL_OPEN)
			{
				PFLT_FILE_NAME_INFORMATION    FNameInfo;
				NTSTATUS status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FNameInfo);
				if (!NT_SUCCESS(status))
				{
					status = FltGetFileNameInformation(Data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP, &FNameInfo);
					if (!NT_SUCCESS(status))
					{
						status = FltGetFileNameInformation(Data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_DEFAULT, &FNameInfo);
						if (!NT_SUCCESS(status))
						{
							status = FltGetFileNameInformation(Data, FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_FILESYSTEM_ONLY, &FNameInfo);
							if (!NT_SUCCESS(status))
							{
								FltCompletePendedPostOperation(Data);
								goto e_ret;
							}
						}
					}
				}

				status = FltParseFileNameInformation(FNameInfo);
				if (!NT_SUCCESS(status))
				{
					FltReleaseFileNameInformation(FNameInfo);
					FltCompletePendedPostOperation(Data);
					goto e_ret;
				}
				FObj_Container* f_obj = ExAllocatePool(NonPagedPool, sizeof(FObj_Container));
				f_obj->FileObject = FileObject;
				f_obj->FNameInfo = FNameInfo;
				KIRQL OldIrql;
				KeAcquireSpinLock(&g_fltSpinLockAc, &OldIrql);
				InsertTailList(&g_fileOpened_list, (PLIST_ENTRY)f_obj);
				KeReleaseSpinLock(&g_fltSpinLockAc, OldIrql);
			}
		}
	}
	type_op_ret == 1 ? FltCompletePendedPostOperation(Data) : FltCompletePendedPreOperation(Data, FLT_PREOP_SUCCESS_NO_CALLBACK, NULL);
e_ret:

	ExFreePool(work_data);
	FltFreeDeferredIoWorkItem(WorkItem);
}
   
//===================================== Диспечер обслуживает запросы ввода / вывода user'a=======================================================
NTSTATUS OnDispatchIRQ(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG ulSize = 0;
	PVOID pBuffer = NULL;

	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(DeviceObject);
	PAGED_CODE();
	ASSERT(IrpStack->FileObject != NULL);
    if(g_unload_dev)
    {
        status = STATUS_NOT_SUPPORTED;
        goto io_complete;
    }
	switch (IrpStack->MajorFunction)
	{
		case IRP_MJ_DEVICE_CONTROL:
		{   
			ULONG ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
			ULONG InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
			ULONG OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
            
			PVOID inBuffer = Irp->AssociatedIrp.SystemBuffer;
			switch (ControlCode)
			{
			 case 	IOCTL_AC_FLT_SET_DBG_POINT:
#ifdef DBG
				DbgBreakPoint();
#endif
			 break;
			 case IOCTL_AC_FLT_ADD_USER:
			 {	
             	
				USERS_Container* agent_users;
				USERS_Container* ctrl_users;
				agent_users= ExAllocatePool(NonPagedPool, sizeof(USERS_Container));
				PUNICODE_STRING user= ExAllocatePool(NonPagedPool, sizeof(UNICODE_STRING));
				user->Length = InputLength-2;
				user->MaximumLength = InputLength;
				user->Buffer = ExAllocatePool(NonPagedPool, InputLength);
				RtlZeroMemory(user->Buffer, InputLength);
				RtlCopyBytes(user->Buffer, inBuffer, InputLength);
				agent_users->user = user;
                DbgLog("AC/>"__FUNCTION__" :IOCTL_AC_FLT_ADD_USER user name %ws \n",user->Buffer); 
				DbgLog("\nAC/>"__FUNCTION__":user %ws \n",user->Buffer);
				
				PLIST_ENTRY header = &g_fltUser_list;
			  	PLIST_ENTRY request = header->Flink;

				if(!IsListEmpty(header))
				{				
					while (request != header)
					{
						ctrl_users = CONTAINING_RECORD(request, USERS_Container, entry_user);
						if (ctrl_users)
						{
							request = ctrl_users->entry_user.Flink;
							PUNICODE_STRING user1 = ctrl_users->user;
							PUNICODE_STRING user =  agent_users->user;
							bool rez = RtlEqualUnicodeString(user1, user, TRUE);
							if (rez)
							{
								ExFreePool(agent_users->user->Buffer);
								ExFreePool(agent_users->user);
								ExFreePool(agent_users);
								goto io_complete;
							}
						}
						else
						{
							break;
						}
					}
				}
		        InsertTailList(&g_fltUser_list,(PLIST_ENTRY)agent_users);
			 }
			break;
 
			case IOCTL_AC_FLT_START_FILTER:
			{

				timeout_msg.QuadPart = WIN_TIMEOUT;   
                timeout_msg_r.QuadPart = WIN_TIMEOUTR;
				KIRQL old = KeGetCurrentIrql();
				KeLowerIrql(PASSIVE_LEVEL);

//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- Запуск фильтра  -=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
				status = FltStartFiltering(filterManager.pFilter);
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

				if (!NT_SUCCESS(status))
				{
					FltUnregisterFilter(filterManager.pFilter);
					DbgLog("AC/>"__FUNCTION__":???Failed start filter\n");
			 	}
				KfRaiseIrql(old);				
				DbgLog("AC/>"__FUNCTION__":start filter enter \n");
				goto io_complete;
			}
			break;



			}
		}
		break;
        case IRP_MJ_CREATE:
         DbgLog("AC/>"__FUNCTION__" :IRP_MJ_CREATE \n");
         filterManager.dvFHandle = IrpStack->FileObject;
        break;
        case IRP_MJ_CLOSE:
        {
         DbgLog("AC/>"__FUNCTION__" :IRP_MJ_CLOSE \n");
         PIO_WORKITEM wi = IoAllocateWorkItem(DeviceObject);
         if(wi==NULL)
         {
             goto io_complete;
         }
         IoQueueWorkItem(wi,(PIO_WORKITEM_ROUTINE)WorkItem,DelayedWorkQueue,wi);
        }
        break;
		case IRP_MJ_CLEANUP:
		{
            DbgLog("\n AC/>"__FUNCTION__":Cleanup Enter \n");
			status = STATUS_SUCCESS;
		}
		break;
	}
  io_complete:
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return status;
}

void OnUnload(PDRIVER_OBJECT DriverObject)
{
	DbgLog("\n AC/>"__FUNCTION__": Enter \n");
	UNREFERENCED_PARAMETER(DriverObject);
	PAGED_CODE();
    if(g_unload_dev) 
    {
       DbgLog("\n AC/>"__FUNCTION__": g_unload_dev = true \n");
       return;
     }
    Unload(DriverObject);
    KeSetEvent(&filterManager.kEvtEndUnregConnect, 0, FALSE);
 }
void Unload(PDRIVER_OBJECT DriverObject)
{

	PLIST_ENTRY entry;
	PLIST_ENTRY head = &g_fltUser_list;
	USERS_Container* agent_users;
    g_unload_dev = true;
	while (!IsListEmpty(head))
	{
		entry = RemoveHeadList(head);
		agent_users = CONTAINING_RECORD(entry, USERS_Container, entry_user);
		if (agent_users != NULL)
		{
			if (agent_users->user)
			{
				ExFreePool(agent_users->user->Buffer);
				ExFreePool(agent_users->user);
			}
			ExFreePool(agent_users);
		}
	}

	DbgLog("AC/>"__FUNCTION__":List users clean \n");
	ExFreePool(g_RegstrSID);
	ExFreePool(g_pUserFFO->Buffer);
	ExFreePool(g_pUserFFO);
    ExFreePool(g_pDomainFFO->Buffer);
	ExFreePool(g_pDomainFFO);
    ExFreePool(g_pSidFFO->Buffer);
	ExFreePool(g_pSidFFO);
	ExFreePool(g_pUserREG->Buffer);
	ExFreePool(g_pUserREG);
	ExFreePool(g_pDomainREG->Buffer);
	ExFreePool(g_pDomainREG);
	ExFreePool(g_filename_prev.Buffer);
	ExFreePool(g_filename_curr.Buffer);
	ExFreePool(g_TrgVolume.Buffer);
    if (g_ParentDir_prev.Buffer)ExFreePool(g_ParentDir_prev.Buffer);
	if (g_processName.Buffer)ExFreePool(g_processName.Buffer);
	if (g_cp_processName.Buffer)ExFreePool(g_cp_processName.Buffer);
				
	UNICODE_STRING deviceLink;
	PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
   	
	RtlInitUnicodeString(&deviceLink, (PWSTR)AC_FLT_DOS_DEV_NAME);
	IoDeleteSymbolicLink(&deviceLink);
	IoDeleteDevice(DriverObject->DeviceObject);
	DbgLog("\n AC/>"__FUNCTION__":Filter unloaded OK \n");
 }