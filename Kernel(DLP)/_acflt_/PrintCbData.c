#include "PrintCbData.h"

void PrintCallbackData(PFLT_CALLBACK_DATA data)
{
	unsigned int disp;

	if (!data)
	{
		DbgLog("AC/> data for callback is NULL.\n");
		return;
	}
	PWCH flName = data->Iopb->TargetFileObject->FileName.Buffer;
	DbgPrint("AC/> IRP callback data information:\n");
	DbgPrint("AC/> -----------------------------------------\n");
	DbgPrint("AC/> Thread: %i\n", data->Thread);
	DbgPrint("AC/> Flags: %i\n", data->Flags);
	DbgPrint("AC/> Requestor mode: %i\n", data->RequestorMode);
	DbgPrint("AC/> Iopb->IrpFlags: %i\n", data->Iopb->IrpFlags);
	DbgPrint("AC/> Iopb->MajorFunction: %i\n", data->Iopb->MajorFunction);
	DbgPrint("AC/> Iopb->MinorFunction: %i\n", data->Iopb->MinorFunction);
	DbgPrint("AC/> Iopb->OperationFlags: %i\n", data->Iopb->OperationFlags);
	if (data->Iopb->OperationFlags & SL_CASE_SENSITIVE)
		DbgPrint("AC/> SL_CASE_SENSITIVE\n");
	if (data->Iopb->OperationFlags & SL_EXCLUSIVE_LOCK)
		DbgPrint("AC/> SL_EXCLUSIVE_LOCK\n");
	if (data->Iopb->OperationFlags & SL_FAIL_IMMEDIATELY)
		DbgPrint("AC/> SL_FAIL_IMMEDIATELY\n");
	if (data->Iopb->OperationFlags & SL_FORCE_ACCESS_CHECK)
		DbgPrint("AC/> SL_FORCE_ACCESS_CHECK\n");
	if (data->Iopb->OperationFlags & SL_FORCE_DIRECT_WRITE)
		DbgPrint("AC/> SL_FORCE_DIRECT_WRITE\n");
	if (data->Iopb->OperationFlags & SL_INDEX_SPECIFIED)
		DbgPrint("AC/> SL_INDEX_SPECIFIED\n");
	if (data->Iopb->OperationFlags & SL_OPEN_PAGING_FILE)
		DbgPrint("AC/> SL_OPEN_PAGING_FILE\n");
	if (data->Iopb->OperationFlags & SL_OPEN_TARGET_DIRECTORY)
		DbgPrint("AC/> SL_OPEN_TARGET_DIRECTORY\n");
	if (data->Iopb->OperationFlags & SL_OVERRIDE_VERIFY_VOLUME)
		DbgPrint("AC/> SL_OVERRIDE_VERIFY_VOLUME\n");
	if (data->Iopb->OperationFlags & SL_RESTART_SCAN)
		DbgPrint("AC/> SL_RESTART_SCAN\n");
	if (data->Iopb->OperationFlags & SL_RETURN_SINGLE_ENTRY)
		DbgPrint("AC/> SL_RETURN_SINGLE_ENTRY\n");
	if (data->Iopb->OperationFlags & SL_WATCH_TREE)
		DbgPrint("AC/> SL_WATCH_TREE\n");
	if (data->Iopb->OperationFlags & SL_WRITE_THROUGH)
		DbgPrint("AC/> SL_WRITE_THROUGH\n");

	if (data->Iopb->MajorFunction == IRP_MJ_CREATE)
	{
		
		DbgPrint("AC/> data->Iopb->Parameters.Create.SecurityContext->DesiredAccess: %i, file name: %ws\n" ,flName ,
			data->Iopb->Parameters.Create.SecurityContext->DesiredAccess);

		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & DELETE)
			DbgPrint("AC/> DELETE, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_READ_DATA)
			DbgPrint("AC/> FILE_READ_DATA, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_READ_ATTRIBUTES)
			DbgPrint("AC/> FILE_READ_ATTRIBUTES, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_READ_EA)
			DbgPrint("AC/> FILE_READ_EA, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & READ_CONTROL)
			DbgPrint("AC/> READ_CONTROL, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_WRITE_DATA)
			DbgPrint("AC/> FILE_WRITE_DATA, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_WRITE_ATTRIBUTES)
			DbgPrint("AC/> FILE_WRITE_ATTRIBUTES, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_WRITE_EA)
			DbgPrint("AC/> FILE_WRITE_EA , file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_APPEND_DATA)
			DbgPrint("AC/> FILE_APPEND_DATA, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & WRITE_DAC)
			DbgPrint("AC/> WRITE_DAC , file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & WRITE_OWNER)
			DbgPrint("AC/> WRITE_OWNER , file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & SYNCHRONIZE)
			DbgPrint("AC/> SYNCHRONIZE, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_EXECUTE)
			DbgPrint("AC/> FILE_EXECUTE, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_LIST_DIRECTORY)
			DbgPrint("AC/> FILE_LIST_DIRECTORY, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.SecurityContext->DesiredAccess & FILE_TRAVERSE)
			DbgPrint("AC/> FILE_TRAVERSE, file name: %ws\n" ,flName );
		
		DbgPrint("AC/> data->Iopb->Parameters.Create.FileAttributes: %i, file name: %ws\n" ,flName , data->Iopb->Parameters.Create.FileAttributes);

		if (data->Iopb->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_NORMAL)
			DbgPrint("AC/> FILE_ATTRIBUTE_NORMAL, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_READONLY)
			DbgPrint("AC/> FILE_ATTRIBUTE_READONLY, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_HIDDEN)
			DbgPrint("AC/> FILE_ATTRIBUTE_HIDDEN, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_SYSTEM)
			DbgPrint("AC/> FILE_ATTRIBUTE_SYSTEM, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_ARCHIVE)
			DbgPrint("AC/> FILE_ATTRIBUTE_ARCHIVE, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.FileAttributes & FILE_ATTRIBUTE_TEMPORARY)
			DbgPrint("AC/> FILE_ATTRIBUTE_TEMPORARY, file name: %ws\n" ,flName );


		DbgPrint("AC/> data->Iopb->Parameters.Create.Options(CreateOptions): %i, file name: %ws\n" ,flName ,
			(data->Iopb->Parameters.Create.Options & 0xFFFFFF));

		if (data->Iopb->Parameters.Create.Options & FILE_DIRECTORY_FILE)
			DbgPrint("AC/> FILE_DIRECTORY_FILE, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_NON_DIRECTORY_FILE)
			DbgPrint("AC/> FILE_NON_DIRECTORY_FILE, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_WRITE_THROUGH)
			DbgPrint("AC/> FILE_WRITE_THROUGH, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_SEQUENTIAL_ONLY)
			DbgPrint("AC/> FILE_SEQUENTIAL_ONLY, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_RANDOM_ACCESS)
			DbgPrint("AC/> FILE_RANDOM_ACCESS, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_NO_INTERMEDIATE_BUFFERING)
			DbgPrint("AC/> FILE_NO_INTERMEDIATE_BUFFERING, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_SYNCHRONOUS_IO_ALERT)
			DbgPrint("AC/> FILE_SYNCHRONOUS_IO_ALERT, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_SYNCHRONOUS_IO_NONALERT)
			DbgPrint("AC/> FILE_SYNCHRONOUS_IO_NONALERT, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_CREATE_TREE_CONNECTION)
			DbgPrint("AC/> FILE_CREATE_TREE_CONNECTION, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_OPEN_REPARSE_POINT)
			DbgPrint("AC/> FILE_OPEN_REPARSE_POINT, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_DELETE_ON_CLOSE)
			DbgPrint("AC/> FILE_DELETE_ON_CLOSE, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_OPEN_BY_FILE_ID)
			DbgPrint("AC/> FILE_OPEN_BY_FILE_ID, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_OPEN_FOR_BACKUP_INTENT)
			DbgPrint("AC/> FILE_OPEN_FOR_BACKUP_INTENT, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.Options & FILE_RESERVE_OPFILTER)
			DbgPrint("AC/> FILE_RESERVE_OPFILTER , file name: %ws\n" ,flName );

		//		if (data->Iopb->Parameters.Create.Options & FILE_OPEN_REQUIRING_OPLOCK)
		//			DbgPrint("AC/> FILE_OPEN_REQUIRING_OPLOCK, file name: %ws\n" ,flName );


		if (data->Iopb->Parameters.Create.Options & FILE_COMPLETE_IF_OPLOCKED)
			DbgPrint("AC/> FILE_COMPLETE_IF_OPLOCKED, file name: %ws\n" ,flName );


		disp = (data->Iopb->Parameters.Create.Options >> 24) & 0xFF;
		DbgPrint("AC/> data->Iopb->Parameters.Create.Options(CreateDisposition): %i, file name: %ws\n" ,flName ,
			(disp));

		if (disp & FILE_SUPERSEDE)
			DbgPrint("AC/> FILE_SUPERSEDE, file name: %ws\n" ,flName );
		if (disp & FILE_CREATE)
			DbgPrint("AC/> FILE_CREATE, file name: %ws\n" ,flName );
		if (disp & FILE_OPEN)
			DbgPrint("AC/> FILE_OPEN, file name: %ws\n" ,flName );
		if (disp & FILE_OPEN_IF)
			DbgPrint("AC/> FILE_OPEN_IF, file name: %ws\n" ,flName );
		if (disp & FILE_OVERWRITE)
			DbgPrint("AC/> FILE_OVERWRITE, file name: %ws\n" ,flName );
		if (disp & FILE_OVERWRITE_IF)
			DbgPrint("AC/> FILE_OVERWRITE_IF, file name: %ws\n" ,flName );


		DbgPrint("AC/> data->Iopb->Parameters.Create.ShareAccess: %i, file name: %ws\n" ,flName ,
			data->Iopb->Parameters.Create.ShareAccess);

		if (data->Iopb->Parameters.Create.ShareAccess & FILE_SHARE_READ)
			DbgPrint("AC/> FILE_SHARE_READ, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.ShareAccess & FILE_SHARE_WRITE)
			DbgPrint("AC/> FILE_SHARE_WRITE, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.Create.ShareAccess & FILE_SHARE_DELETE)
			DbgPrint("AC/> FILE_SHARE_DELETE, file name: %ws\n" ,flName );

		DbgPrint("AC/> data->Iopb->Parameters.Create.EaLength: %i, file name: %ws\n" ,flName ,
			data->Iopb->Parameters.Create.EaLength);
	}
	else 
		if (data->Iopb->MajorFunction == IRP_MJ_QUERY_INFORMATION)
	   {
		DbgPrint("AC/> data->Iopb->Parameters.QueryFileInformation.FileInformationClass: %i, file name: %ws\n" ,flName ,
			data->Iopb->Parameters.QueryFileInformation.FileInformationClass);

		if (data->Iopb->Parameters.QueryFileInformation.FileInformationClass ==
			FileAllInformation)
			DbgPrint("AC/> FileAllInformation, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.QueryFileInformation.FileInformationClass ==
			FileAttributeTagInformation)
			DbgPrint("AC/> FileAttributeTagInformation, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.QueryFileInformation.FileInformationClass ==
			FileBasicInformation)
			DbgPrint("AC/> FileBasicInformation, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.QueryFileInformation.FileInformationClass ==
			FileCompressionInformation)
			DbgPrint("AC/> FileCompressionInformation, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.QueryFileInformation.FileInformationClass ==
			FileEaInformation)
			DbgPrint("AC/> FileEaInformation, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.QueryFileInformation.FileInformationClass ==
			FileInternalInformation)
			DbgPrint("AC/> FileInternalInformation, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.QueryFileInformation.FileInformationClass ==
			FileMoveClusterInformation)
			DbgPrint("AC/> FileMoveClusterInformation, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.QueryFileInformation.FileInformationClass ==
			FileNameInformation)
			DbgPrint("AC/> FileNameInformation, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.QueryFileInformation.FileInformationClass ==
			FileNetworkOpenInformation)
			DbgPrint("AC/> FileNetworkOpenInformation, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.QueryFileInformation.FileInformationClass ==
			FilePositionInformation)
			DbgPrint("AC/> FilePositionInformation, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.QueryFileInformation.FileInformationClass ==
			FileStandardInformation)
			DbgPrint("AC/> FileStandardInformation, file name: %ws\n" ,flName );
		if (data->Iopb->Parameters.QueryFileInformation.FileInformationClass ==
			FileStreamInformation)
			DbgPrint("AC/> FileStreamInformation, file name: %ws\n" ,flName );
	}






	DbgPrint("AC/> IoStatus.Status: %i, file name: %ws\n" ,flName , data->IoStatus.Status);
	DbgPrint("AC/> IoStatus.Information: %i, file name: %ws\n" ,flName , data->IoStatus.Information);

	if (data->IoStatus.Information == FILE_CREATED)
		DbgPrint("AC/> FILE_CREATED, file name: %ws\n" ,flName );
	if (data->IoStatus.Information == FILE_OPENED)
		DbgPrint("AC/> FILE_OPENED, file name: %ws\n" ,flName );
	if (data->IoStatus.Information == FILE_OVERWRITTEN)
		DbgPrint("AC/> FILE_OVERWRITTEN, file name: %ws\n" ,flName );
	if (data->IoStatus.Information == FILE_SUPERSEDED)
		DbgPrint("AC/> FILE_SUPERSEDED, file name: %ws\n" ,flName );
	if (data->IoStatus.Information == FILE_EXISTS)
		DbgPrint("AC/> FILE_EXISTS, file name: %ws\n" ,flName );
	if (data->IoStatus.Information == FILE_DOES_NOT_EXIST)
		DbgPrint("AC/> FILE_DOES_NOT_EXIST, file name: %ws\n" ,flName );


	DbgPrint("AC/> -----------------------------------------, file name: %ws\n" ,flName );
}

