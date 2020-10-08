#include "RegUtil.h"
#define MODULE_TAG 'GERK'

NTSTATUS RegOpenKeyW(IN PHANDLE hKey,IN PUNICODE_STRING usKeyName)
{
	OBJECT_ATTRIBUTES ObjectAttributes;

	InitializeObjectAttributes(&ObjectAttributes,usKeyName,OBJ_CASE_INSENSITIVE,NULL,NULL);

	return	ZwOpenKey(hKey,	KEY_READ,&ObjectAttributes);
}

NTSTATUS RegOpenKey(IN PHANDLE hKey, IN PWCHAR szKeyName)
{
	UNICODE_STRING    usKeyName;
	RtlInitUnicodeString(&usKeyName, szKeyName);

	return	RegOpenKeyW(hKey,&usKeyName);
}

// NOTE: The caller should deallocate memory for ValueData
NTSTATUS RegGenericValue(IN HANDLE hKey,IN PWCHAR ValueName,IN PKEY_VALUE_PARTIAL_INFORMATION *ValueData,IN PUSHORT ValueSize)
{
	NTSTATUS ntStatus;
	PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
	UNICODE_STRING usValueName;
	ULONG ReturnedLength = 0;

	RtlInitUnicodeString(&usValueName, ValueName);

	ntStatus =	ZwQueryValueKey(hKey,&usValueName,KeyValuePartialInformation,*ValueData,(ULONG)*ValueSize,&ReturnedLength);

	if ((ntStatus == STATUS_BUFFER_OVERFLOW) ||	(ntStatus == STATUS_BUFFER_TOO_SMALL))
	{
		if (ReturnedLength <= 0xFFFF)
		{
			int sz = ReturnedLength + sizeof(UNICODE_NULL);
			KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)	ExAllocatePool(NonPagedPool,sz);
			
			if (KeyValueInfo != NULL)
			{
				RtlZeroMemory(KeyValueInfo,sz);
				ntStatus =	ZwQueryValueKey(hKey,&usValueName,KeyValuePartialInformation,KeyValueInfo,ReturnedLength,&ReturnedLength);

				if (NT_SUCCESS(ntStatus))
				{
					*ValueData = KeyValueInfo;
					*ValueSize = (USHORT)ReturnedLength;
				}

			}
			else
			    {
				  ntStatus = STATUS_INSUFFICIENT_RESOURCES;
			     }
		}
		else
		{
			ntStatus = STATUS_BUFFER_TOO_SMALL;
		}
	}

	return ntStatus;
}

NTSTATUS RegReadDword(IN HANDLE hKey,IN PWCHAR szValueName,OUT PULONG ValueData)
{
	NTSTATUS ntStatus;
	PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo = NULL;
	CHAR KeyValueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
	UNICODE_STRING usValueName;
	ULONG ReturnedLength = 0;

	RtlInitUnicodeString(&usValueName, szValueName);

	KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;
	RtlZeroMemory(KeyValueBuffer, sizeof(KeyValueBuffer));

	ntStatus =ZwQueryValueKey(
								hKey,
								&usValueName,
								KeyValuePartialInformation,
								KeyValueInfo,
								sizeof(KeyValueBuffer),
								&ReturnedLength
							);

	if (NT_SUCCESS(ntStatus)) {
		if (KeyValueInfo->Type != REG_DWORD || KeyValueInfo->DataLength != sizeof(ULONG)) {
			ntStatus = STATUS_INVALID_PARAMETER_MIX;
		}
		else {
			*ValueData = *((ULONG UNALIGNED *)((PCHAR)KeyValueInfo->Data));
		}
	}
	return ntStatus;
}

NTSTATUS RegReadStringValue(IN HANDLE hKey,IN PWCHAR szValueName,OUT PUNICODE_STRING ValueData)
{
	NTSTATUS ntStatus;
	PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo = NULL;
	UNICODE_STRING usValueName;
	USHORT KeyValueInfoSize = 0;
	unsigned int i = 0;

	ntStatus =RegGenericValue(hKey,	szValueName,&KeyValueInfo,	&KeyValueInfoSize);

	if (NT_SUCCESS(ntStatus))
	{

		if ((KeyValueInfo->Type == REG_SZ) || (KeyValueInfo->Type == REG_EXPAND_SZ))
		{
			PWSTR DataStr = (PWSTR)KeyValueInfo->Data;
			ValueData->Length = 0;
			ValueData->MaximumLength = (USHORT)KeyValueInfoSize;
			ValueData->Buffer = (PWCHAR)KeyValueInfo;

			while (ValueData->Length <= KeyValueInfo->DataLength)
			{
				if (DataStr == UNICODE_NULL)
				{
					break;
				}
				ValueData->Length += sizeof(WCHAR);
			}
			RtlCopyMemory(ValueData->Buffer, KeyValueInfo->Data, ValueData->Length);

		}
		 else
		   {
			ntStatus = STATUS_INVALID_PARAMETER_MIX;
		   }
	}

	return ntStatus;
}

NTSTATUS RegReadMultiSZValue( IN HANDLE hKey,IN PWCHAR szValueName,OUT PUNICODE_STRING  ValueData)
{
	NTSTATUS ntStatus;
	PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo = NULL;
	UNICODE_STRING usValueName;
	USHORT KeyValueInfoSize = 0;

	ntStatus =	RegGenericValue(
								hKey,
								szValueName,
								&KeyValueInfo,
								&KeyValueInfoSize
								);

	if (NT_SUCCESS(ntStatus))
	{

		if (KeyValueInfo->Type == REG_MULTI_SZ)
		{
			ValueData->Buffer = (PWCHAR)KeyValueInfo;
			ValueData->Length = (USHORT)KeyValueInfo->DataLength;
			ValueData->MaximumLength = (USHORT)KeyValueInfoSize;
			RtlCopyMemory(ValueData->Buffer, KeyValueInfo->Data, KeyValueInfo->DataLength);
		}
		else
		    {
			  ntStatus = STATUS_INVALID_PARAMETER_MIX;
		    }
	}

	return ntStatus;
}

NTSTATUS RegReadStringFromMultiSZ(IN HANDLE hKey,IN PWCHAR szValueName,OUT PUNICODE_STRING ValueData)
{
	NTSTATUS ntStatus;
	USHORT i = 0;
	ntStatus =	RegReadMultiSZValue(
									hKey,
									szValueName,
									ValueData
									);

	if (NT_SUCCESS(ntStatus))
	{
		USHORT Length = 0;
		while (Length <= ValueData->Length)
		{
			if (ValueData->Buffer[i] == UNICODE_NULL){
				break;
			}
			Length += sizeof(WCHAR);
			i++;
		}
		ValueData->Length = Length;
	}
	return ntStatus;
}