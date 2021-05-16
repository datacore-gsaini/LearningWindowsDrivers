#include <fltKernel.h>
#include <dontuse.h>
#include <ntifs.h>
#include "FsFilter1.h"
#include "Trace.h"
#include <stdlib.h>
#include <ntstrsafe.h>

#define STRING_LEN 128

PDRIVER_OBJECT WppDriverObject = NULL;
PFLT_PORT port = NULL, ClientPort = NULL;
UNICODE_STRING drive_filter;


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

	TraceEvent(TRACE_INFO, "Inside Post Create \r\n");
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
			if (wcsncmp(Name, drive_filter.Buffer, wcslen(drive_filter.Buffer)) != 0) {
				KdPrint(("%ws is filtered out. Does not match %ws. Returning back\n", Name, drive_filter.Buffer));
				FltReleaseFileNameInformation(FileNameInfo);
				return FLT_PREOP_SUCCESS_NO_CALLBACK;
			}
			TraceEvent(TRACE_INFO, "Create File. Filename = %ws \r\n", Name);
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
			if (wcsncmp(FileName, drive_filter.Buffer, wcslen(drive_filter.Buffer)) != 0) {
				KdPrint(("%ws is filtered out. Does not match %ws. Returning back\n", FileName, drive_filter.Buffer));
				FltReleaseFileNameInformation(FileNameInfo);
				return FLT_PREOP_SUCCESS_NO_CALLBACK;
			}

			WCHAR UserName[1024] = { 0 };
			PSecurityUserData SecData;
			status = GetSecurityUserInfo(NULL, 0, &SecData);

			if(NT_SUCCESS(status))
				RtlCopyMemory(UserName, SecData->UserName.Buffer, SecData->UserName.MaximumLength);
				
			KdPrint(("Write File : User = %ws, Filename = %ws, Length of write = %lu, Offset = %lld\n", UserName, FileName, Data->Iopb->Parameters.Write.Length, Data->Iopb->Parameters.Write.ByteOffset.QuadPart));
			
			LsaFreeReturnBuffer(SecData);
		}

		FltReleaseFileNameInformation(FileNameInfo);
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


NTSTATUS MiniConnect(PFLT_PORT clientport, PVOID serverportcookie, PVOID Context, ULONG size, PVOID *Connectioncookie) {
	ClientPort = clientport;
	TraceEvent(TRACE_VERBOSE, "Connect\r\n");
	return STATUS_SUCCESS;
}


VOID MiniDisconnect(PVOID Connectioncookie) {
	TraceEvent(TRACE_VERBOSE, "Disconnect\r\n");
	FltCloseClientPort(FilterHandle, &ClientPort);
}


NTSTATUS MiniSendRecv(PVOID portcookie, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PULONG RetLength) {
	NTSTATUS status;
	PCHAR msg = "KERNEL MSG";
	TraceEvent(TRACE_INFO, "User msg: %s\r\n", (PCHAR)InputBuffer);
	KdPrint(("User msg: %s\r\n", (PCHAR)InputBuffer));
	strcpy((PCHAR)OutputBuffer, msg);
	UNICODE_STRING TargetString;

	TargetString.Buffer = (PWCH)ExAllocatePool(NonPagedPool, STRING_LEN * sizeof(WCHAR));
	RtlZeroMemory(TargetString.Buffer, STRING_LEN * sizeof(WCHAR));
	TargetString.MaximumLength = STRING_LEN;

	if (strcmp((PCHAR)InputBuffer, "All drives") == 0) {
		KdPrint(("ALL DRIVES\r\n"));
	}
	else {
		KdPrint(("Individual Drives\r\n"));
		wchar_t buf[STRING_LEN];
		char inp_buf[STRING_LEN];
		strcpy(inp_buf, "\\GLOBAL??\\");
		strcat(inp_buf, (PCHAR)InputBuffer);
		mbstowcs(buf, inp_buf, sizeof(inp_buf));
		UNICODE_STRING symlinkName;
		RtlInitUnicodeString(&symlinkName, buf);

		OBJECT_ATTRIBUTES objectAttributes =
			RTL_CONSTANT_OBJECT_ATTRIBUTES(&symlinkName,
				OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE);

		HANDLE symLinkHandle = NULL;
		status = ZwOpenSymbolicLinkObject(&symLinkHandle,
			SYMBOLIC_LINK_ALL_ACCESS,
			&objectAttributes);

		if (!NT_SUCCESS(status)) {
			if (symLinkHandle)
				ZwClose(symLinkHandle);
			ExFreePool(TargetString.Buffer);
			KdPrint(("ZwOpenSymbolicLinkObject failed\n"));
			return status;
		}

		ULONG Ret = 0;

		status = ZwQuerySymbolicLinkObject(symLinkHandle,
			&TargetString, &Ret);
		if (!NT_SUCCESS(status)) {
			if (symLinkHandle)
				ZwClose(symLinkHandle);
			ExFreePool(TargetString.Buffer);
			KdPrint(("ZwQuerySymbolicLinkObject failed\n"));
			return status;
		}
		ZwClose(symLinkHandle);
	}

	KdPrint(("AFTER Query SymLink %S\r\n", TargetString.Buffer));
	if (!wcsncmp(TargetString.Buffer, L"\\Device", 7)) {
		status = STATUS_SUCCESS;
		KdPrint(("Success: TargetStr has Device\n"));
	}
	RtlZeroMemory(drive_filter.Buffer, STRING_LEN * sizeof(WCHAR));
	RtlUnicodeStringCopy(&drive_filter, &TargetString);
	ExFreePool(TargetString.Buffer);
	return STATUS_SUCCESS;
}

NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags)
{
	UNREFERENCED_PARAMETER(Flags);
	
	TraceEvent(TRACE_INFO, "Driver Unload.\r\n");
	if (port) {
		FltCloseCommunicationPort(port);
		port = NULL;
	}
	FltUnregisterFilter(FilterHandle);
	if (WppDriverObject)
		WppCleanup(WppDriverObject);

	ExFreePool(drive_filter.Buffer);
	return STATUS_SUCCESS;
}

extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	PSECURITY_DESCRIPTOR sd;
	OBJECT_ATTRIBUTES oa = { 0 };
	UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\Mini-filter");
	drive_filter.Buffer = (PWCH)ExAllocatePool(NonPagedPool, STRING_LEN * sizeof(WCHAR));
	RtlZeroMemory(drive_filter.Buffer, STRING_LEN * sizeof(WCHAR));
	drive_filter.MaximumLength = STRING_LEN;

	WppDriverObject = DriverObject;
	WppInit(WppDriverObject, RegistryPath);

	NTSTATUS status;
	status = FltRegisterFilter(DriverObject, &FilterRegistration, &FilterHandle);

	if (!NT_SUCCESS(status))
	{
		ExFreePool(drive_filter.Buffer);
		return status;
	}
	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);

	if (NT_SUCCESS(status))
	{
		InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, sd);
		status = FltCreateCommunicationPort(FilterHandle, &port, &oa, NULL, MiniConnect, MiniDisconnect, MiniSendRecv, 1);

		FltFreeSecurityDescriptor(sd);
		if (NT_SUCCESS(status)) {
			status = FltStartFiltering(FilterHandle);
			if (NT_SUCCESS(status)) return status;	
		}
		if (port) {
			FltCloseCommunicationPort(port);
			port = NULL;
		}
	}
	FltUnregisterFilter(FilterHandle);
	ExFreePool(drive_filter.Buffer);
	return status;

}