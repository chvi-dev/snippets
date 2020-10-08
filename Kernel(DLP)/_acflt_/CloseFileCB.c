#include "acflt.h"
#include "CloseFileCB.h"
#include "Helper.h"
#include "FileUtil.h"
#include "ServiceRule.h"
#include "PrintCbData.h"
#include "DispatchIRQ.h"

FLT_PREOP_CALLBACK_STATUS  PreFileCloseCallback(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID *CompletionContext);
FLT_POSTOP_CALLBACK_STATUS PostFileCloseCallback(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags);


FLT_PREOP_CALLBACK_STATUS
PreFileCloseCallback(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID *CompletionContext)
{

	if (FltObjects->FileObject == NULL || Data == NULL || FLT_IS_FS_FILTER_OPERATION(Data) ||  g_unload_dev  )
	{
		return 	FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (FLT_IS_FASTIO_OPERATION(Data))
	{
		DbgLog("AC/>"__FUNCTION__":(CR)FLT_PREOP_DISALLOW_FASTIO \n");
		return FLT_PREOP_DISALLOW_FASTIO;
	}

	if (Data->Iopb->MajorFunction != IRP_MJ_CLOSE)
	{
		return 	FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
  
	//PrintCallbackData(Data);

	PFILE_OBJECT FileObject = FltObjects->FileObject;
	FLT_PREOP_CALLBACK_STATUS  flt_status = FLT_PREOP_SUCCESS_NO_CALLBACK;
	if (g_FileObject == FileObject)
	{
	
		return 	FLT_PREOP_SUCCESS_NO_CALLBACK;
	}
	FObj_Container* ctrlFileObject = NULL;
	PLIST_ENTRY head = &g_fileOpened_list;
	PLIST_ENTRY entry = head->Flink;
	if (!IsListEmpty(head))
	{
		
		KIRQL OldIrql;
		while (entry != head)
		{
			ctrlFileObject = CONTAINING_RECORD(entry, FObj_Container, entry_lst);
			if (ctrlFileObject == NULL)
			{	
		
		        KeAcquireSpinLock(&g_fltSpinLockAc, &OldIrql);

				RemoveEntryList(entry);

				KeReleaseSpinLock(&g_fltSpinLockAc, OldIrql);
				return FLT_PREOP_SUCCESS_NO_CALLBACK;
			}

			if (ctrlFileObject->FileObject == FileObject)
			{
				//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//
				ULONG replylen = sizeof(FILTER_REPLY_HEADER) + sizeof(unsigned char);
				ULONG pktlen = sizeof(MessageCtx);
				PVOID reply = ExAllocatePool(NonPagedPool, replylen);
				PAC_FILTER_MESSAGE msg = ExAllocatePool(NonPagedPool, pktlen);

				RtlZeroMemory(msg, pktlen);			  
				RtlZeroMemory(reply, replylen);
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
 								return FLT_PREOP_SUCCESS_NO_CALLBACK;
							}
						}

					}

				}
				status = GetDosRoot2File(FltObjects, FNameInfo, &g_filename_curr);
				if (!NT_SUCCESS(status))
				{
					FltReleaseFileNameInformation(FNameInfo);
					return FLT_POSTOP_FINISHED_PROCESSING;
				}

				status = SetMessageContext(&msg->Notification, FDRVCONVO_CLOSE, NULL, &g_filename_curr);
				if (!NT_SUCCESS(status))
				{
					ExFreePool(msg);
					ExFreePool(reply);
					KeAcquireSpinLock(&g_fltSpinLockAc, &OldIrql);

					RemoveEntryList(entry);

					KeReleaseSpinLock(&g_fltSpinLockAc, OldIrql);
					return FLT_PREOP_SUCCESS_NO_CALLBACK;
				}

				 msg->Notification.trg_DeviceType = FileObject->DeviceObject->DeviceType; 
				 msg->Notification.src_DeviceType = FileObject->DeviceObject->DeviceType;

				 //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=- WorkItem(IGNORE) =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//	
				 PFLT_PORT clientPort;
				 bool port_is_liber = false;
				 INT8 i = 0;
				 for ( i= 0; i < msg_connections; i++)
				 {
					 if (filterManager.connections[i].busy==false)
					 {
						 clientPort = filterManager.connections[i].port;
						 filterManager.connections[i].busy = true;
						 port_is_liber = true;
						 break;
					 }
				 }
				 if (!port_is_liber)
				 {

				 }
				 status = FltSendMessage(filterManager.pFilter,
										 &clientPort,
										 msg,
										 pktlen,
										 (PVOID)reply,
										 &replylen,
										 &timeout_msg);

				bool allow = ((PAC_FLT_REPLY)reply)->bAllow;
				ExFreePool(msg);
				ExFreePool(reply);
				filterManager.connections[i].busy = false;
				//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=//	

				KeAcquireSpinLock(&g_fltSpinLockAc, &OldIrql);

				RemoveEntryList(entry);

				KeReleaseSpinLock(&g_fltSpinLockAc, OldIrql);

				ExFreePool(ctrlFileObject);
				break;
			}
			entry = ctrlFileObject->entry_lst.Flink;
		}
	}
	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS PostFileCloseCallback(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags)
{

 return FLT_POSTOP_FINISHED_PROCESSING;
}
