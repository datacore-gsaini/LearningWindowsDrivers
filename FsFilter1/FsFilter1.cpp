#include <fltKernel.h>
#include <dontuse.h>
#include <ntifs.h>
#include "FsFilter1.h"

const FLT_OPERATION_REGISTRATION Callbacks[] = {
	{IRP_MJ_CREATE, 0, MiniPreCreate, MiniPostCreate},
	{IRP_MJ_WRITE, 0, MiniPreWrite, NULL},
	{IRP_MJ_OPERATION_END}
};

const FLT_REGISTRATION FilterRegistration = {
	sizeof(FLT_REGISTRATION),
	FLT_REGISTRATION_VERSION,
	0,
	NULL,
	Callbacks,
	MiniUnload,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

PFLT_FILTER FilterHandle = NULL;


FLT_POSTOP_CALLBACK_STATUS MiniPostCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, 
	PVOID CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);

	KdPrint(("Inside Post Create \r\n"));
	return FLT_POSTOP_FINISHED_PROCESSING;

}

FLT_PREOP_CALLBACK_STATUS MiniPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	WCHAR Name[1024] = { 0 };
	PFLT_FILE_NAME_INFORMATION FileNameInfo;
	NTSTATUS status;
	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);
	
	if (NT_SUCCESS(status)) 
	{
		status = FltParseFileNameInformation(FileNameInfo);
		if (NT_SUCCESS(status)) {
			RtlCopyMemory(Name, FileNameInfo->Name.Buffer, FileNameInfo->Name.MaximumLength);
			KdPrint(("Create File. Filename = %ws \r\n", Name));
		}
		FltReleaseFileNameInformation(FileNameInfo);
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}


FLT_PREOP_CALLBACK_STATUS MiniPreWrite(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);

	WCHAR FileName[1024] = { 0 };
	PFLT_FILE_NAME_INFORMATION FileNameInfo;
	NTSTATUS status;
	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);

	if (NT_SUCCESS(status))
	{
		status = FltParseFileNameInformation(FileNameInfo);
		if (NT_SUCCESS(status)) {
			RtlCopyMemory(FileName, FileNameInfo->Name.Buffer, FileNameInfo->Name.MaximumLength);

			WCHAR UserName[1024] = { 0 };
			PSecurityUserData SecData;
			status = GetSecurityUserInfo(NULL, 0, &SecData);

			if(NT_SUCCESS(status))
				RtlCopyMemory(UserName, SecData->UserName.Buffer, SecData->UserName.MaximumLength);
				
			KdPrint(("Write File : User = %ws, Filename = %ws, Length of write = %lu, Offset = %lld\n", UserName, FileName, Data->Iopb->Parameters.Write.Length, Data->Iopb->Parameters.Write.ByteOffset.QuadPart));
		}

		FltReleaseFileNameInformation(FileNameInfo);
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Flags);
	
	KdPrint(("Driver Unload.\r\n"));
	FltUnregisterFilter(FilterHandle);
	return STATUS_SUCCESS;
}

extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS status;
	status = FltRegisterFilter(DriverObject, &FilterRegistration, &FilterHandle);

	if (NT_SUCCESS(status))
	{
		status = FltStartFiltering(FilterHandle);

		if (!NT_SUCCESS(status))
			FltUnregisterFilter(FilterHandle);
	}

	return status;

}