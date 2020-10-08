#include "acflt.h"
#include "PnpDevice.h"
#include "Helper.h"
#include "FileUtil.h"
#include "ServiceRule.h"
#include "PrintCbData.h"
#include "DispatchIRQ.h"

FLT_PREOP_CALLBACK_STATUS  PrePnpCallback(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID *CompletionContext);
FLT_POSTOP_CALLBACK_STATUS PostPnpCallback(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags);


FLT_PREOP_CALLBACK_STATUS
PrePnpCallback(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID *CompletionContext)
{

	if (FltObjects->FileObject == NULL || Data == NULL || FLT_IS_FS_FILTER_OPERATION(Data) ||  g_unload_dev )
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

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

FLT_POSTOP_CALLBACK_STATUS PostPnpCallback(__inout PFLT_CALLBACK_DATA Data, __in PCFLT_RELATED_OBJECTS FltObjects, __deref_out_opt PVOID CompletionContext, __in FLT_POST_OPERATION_FLAGS Flags)
{

 return FLT_POSTOP_FINISHED_PROCESSING;
}
