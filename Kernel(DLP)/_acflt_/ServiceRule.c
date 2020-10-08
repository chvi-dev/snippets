
#include "DispatchIRQ.h"
#include "Helper.h"
#include "acflt.h"
bool FindSubString(PWCH str, PWCH eXp)
{
	if (str == NULL)return false;
	
	bool find = false;
	wchar_t* f_prname = wcsstr(str,eXp);
	
	if (f_prname != NULL)
	{
		int eln = wcslen(f_prname);
		if (wcsncmp(f_prname, eXp, eln) == 0)
	                                     find=true;
	}

  return find;
}

bool FindNameInExpression(PUNICODE_STRING pNm_str, PUNICODE_STRING Expression)
{
	int exp_len = Expression->MaximumLength;
	PWCH expression = Expression->Buffer;
	bool begin = false;
	wchar_t *sExpr = expression;
	wchar_t* s_wstr = NULL;
	wchar_t  symbol = *sExpr;
	int len_substr = 0;
	ULONG a = 0;
	ULONG b = (INT_PTR)expression;
	ULONG d;
	NTSTATUS status;
	while (symbol != L')')
	{
		a = (INT_PTR)sExpr;
		d = a - b;
		if (d >= (exp_len))
			return false;
		if (begin == false)
		{
			if (symbol != L'(')
			{
				sExpr++;

			}
			else
			{
				begin = true;
				sExpr++;
				s_wstr = sExpr;
				len_substr = 0;
			}
			symbol = *sExpr;
			continue;
		}
		if (symbol == L'|' || symbol == L',')
		{
			UNICODE_STRING expr;
			int wlen = (len_substr + 1)*sizeof(wchar_t);
			int len =len_substr*sizeof(wchar_t);
			PWCH w_exp = ExAllocatePool(NonPagedPool, wlen);
			RtlZeroMemory(w_exp, wlen);
			RtlCopyBytes(w_exp, s_wstr, len);
			status = RtlInitUnicodeStringEx(&expr, (PCWSTR)w_exp);
			if (!NT_SUCCESS(status))
			{
				if (w_exp!=NULL)
					 ExFreePool(w_exp);
				return false;
			}
			bool find = false;
			try
			{
				UNICODE_STRING  DestinationString;
				NTSTATUS status = RtlUpcaseUnicodeString(&DestinationString, pNm_str, TRUE);
				if (!NT_SUCCESS(status))
				{
					if (w_exp != NULL)
						ExFreePool(w_exp);
					return false;
				}
				find = FsRtlIsNameInExpression(&expr, &DestinationString, TRUE, NULL);
				//DbgLog("AC/>"__FUNCTION__":DestinationString %ws \n", DestinationString.Buffer);
				RtlFreeUnicodeString(&DestinationString);
			}
			except(EXCEPTION_EXECUTE_HANDLER)
			{
				if (w_exp != NULL)
				 	      ExFreePool(w_exp);
				
				DbgLog("AC/>"__FUNCTION__":EXCEPTION_EXECUTE_HANDLER \n");
				return false;
			}
			
         
			if (find == true)
			{
				if (w_exp != NULL)
					ExFreePool(w_exp);
				return true;
			}
			sExpr++;
			s_wstr = sExpr;
			len_substr = 0;
			if (w_exp != NULL)
				ExFreePool(w_exp);
		}
		len_substr++;
		sExpr++;
		symbol = *sExpr;
	}
	return false;
}

int  PostSetInformationOpRun(int op_ID, PVOID _data, PVOID _FltObjects)
{
	DbgLog("AC/>"__FUNCTION__" enter with proccess %ws \n", g_processName.Buffer);
	FLT_POSTOP_CALLBACK_STATUS flt_status = FLT_POSTOP_FINISHED_PROCESSING;

	NTSTATUS status;
	
	PCFLT_RELATED_OBJECTS FltObjects = (PCFLT_RELATED_OBJECTS)_FltObjects;
	PFILE_OBJECT FileObject = FltObjects->FileObject;

	PFLT_CALLBACK_DATA data = (PFLT_CALLBACK_DATA)_data;
	PVOID fileinformation = data->Iopb->Parameters.SetFileInformation.InfoBuffer;
	int length = data->Iopb->Parameters.SetFileInformation.Length;

	//if (rule->rule_header.block )
	{
			
		PFILE_DISPOSITION_INFORMATION fileDsInfo = (PFILE_DISPOSITION_INFORMATION)fileinformation;
		fileDsInfo->DeleteFile = 0;
		status = FltSetInformationFile(FltObjects->Instance, FileObject, fileDsInfo, length, FileDispositionInformation);
		data->IoStatus.Information = 0;
		data->IoStatus.Status = STATUS_ACCESS_DENIED;
		DbgLog("AC/>"__FUNCTION__":STATUS_ACCESS_DENIED  %ws \n", g_processName.Buffer);
	}


  return flt_status;
}



