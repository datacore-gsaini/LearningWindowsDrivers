#define _NO_CRT_STDIO_INLINE  1

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
LONGLONG op_code = 0;
LONGLONG process_id_filter = 0;
LONGLONG thread_id_filter = 0;

#define LOG_LINE_LEN 1048

typedef struct {
    PUNICODE_STRING PMessage;
    LIST_ENTRY ListEntry;
} LOG_ENTRY, *PLOG_ENTRY;


LIST_ENTRY log_list_head;


#define MAX_LOG_ENTRIES 10000
int log_entry_count = 0;

void PushLogEntry(PUNICODE_STRING log_line)
{
    if (log_entry_count > MAX_LOG_ENTRIES)
        return;

    PLOG_ENTRY new_node = (PLOG_ENTRY)ExAllocatePoolWithTag(NonPagedPool, sizeof(LOG_ENTRY), POOL_TAG);

    if (new_node != NULL)
    {
        new_node->PMessage = log_line;

        InsertTailList(&log_list_head, &(new_node->ListEntry));

        KdPrint(("Pushed %ws \r\n", log_line->Buffer));
        ++log_entry_count;
    }
}

PLOG_ENTRY
PopLogEntry()
{
    PLIST_ENTRY ListEntry;

    if (IsListEmpty(&log_list_head))
        return NULL;

    ListEntry = RemoveHeadList(&log_list_head);

    --log_entry_count;
    return CONTAINING_RECORD(ListEntry, LOG_ENTRY, ListEntry);
}

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


bool allowed_by_filter(PUNICODE_STRING fileName)
{

    if (process_id_filter != 0)
    {
        LONGLONG pid = (LONGLONG)PsGetCurrentProcessId();
        if (process_id_filter != pid)
        {
            //KdPrint(("%ws filtered out. PID %lld. Expected %lld.", fileName->Buffer, pid, process_id_filter));
            return false;
        }
    }

    if (thread_id_filter != 0)
    {
        LONGLONG tid = (LONGLONG)PsGetCurrentThreadId();
        if (thread_id_filter != tid)
        {
            //KdPrint(("%ws filtered out. TID %lld. Expected %lld.", fileName->Buffer, tid, thread_id_filter));
            return false;
        }
    }

    if (drive_filter.Buffer == NULL)
        return true;

    UNICODE_STRING ucase_file_name;
    RtlUpcaseUnicodeString(&ucase_file_name, fileName, TRUE);

    int x = wcsncmp(drive_filter.Buffer, ucase_file_name.Buffer, drive_filter.Length / sizeof(WCHAR));

    if (x == 0)
        return true;

    //KdPrint(("%ws filtered out. Expected %ws.\n", fileName->Buffer, drive_filter.Buffer));

    return false;
}

FLT_PREOP_CALLBACK_STATUS MiniPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
    //KdPrint(("Inside MiniPreCreate\r\n"));
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    PFLT_FILE_NAME_INFORMATION FileNameInfo;
    NTSTATUS status;
    status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);

    if (NT_SUCCESS(status))
    {
        status = FltParseFileNameInformation(FileNameInfo);
        if (NT_SUCCESS(status)) {
            if (!allowed_by_filter(&FileNameInfo->Name))
            {
                FltReleaseFileNameInformation(FileNameInfo);
                return FLT_PREOP_SUCCESS_NO_CALLBACK;
            }
            //KdPrint(("Create File. Filename = %ws \r\n", FileNameInfo->Name.Buffer));

            UNICODE_STRING log_line;
            log_line.Buffer = (PWCH)ExAllocatePoolWithTag(NonPagedPool, LOG_LINE_LEN * sizeof(WCHAR), POOL_TAG2);
            RtlZeroMemory(log_line.Buffer, LOG_LINE_LEN * sizeof(WCHAR));
            log_line.MaximumLength = LOG_LINE_LEN;

            RtlStringCchPrintfExW(log_line.Buffer, LOG_LINE_LEN, NULL, NULL, 0, L"Create File. Filename = %ws \r\n", FileNameInfo->Name.Buffer);

            PushLogEntry(&log_line);
        }
        FltReleaseFileNameInformation(FileNameInfo);
    }

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS MiniPreWrite(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
    //KdPrint(("Inside MiniPreWrite\r\n"));
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(CompletionContext);

    PFLT_FILE_NAME_INFORMATION FileNameInfo;
    NTSTATUS status;
    status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);

    if (NT_SUCCESS(status))
    {
        status = FltParseFileNameInformation(FileNameInfo);
        if (NT_SUCCESS(status)) {

            if (!allowed_by_filter(&FileNameInfo->Name))
            {
                FltReleaseFileNameInformation(FileNameInfo);
                return FLT_PREOP_SUCCESS_NO_CALLBACK;
            }

            WCHAR UserName[1024] = { 0 };
            PSecurityUserData SecData;
            status = GetSecurityUserInfo(NULL, 0, &SecData);

            if (NT_SUCCESS(status))
                RtlCopyMemory(UserName, SecData->UserName.Buffer, SecData->UserName.MaximumLength);

            KdPrint(("Write File : User = %ws, Filename = %ws, Length of write = %lu, Offset = %lld\n", UserName, FileNameInfo->Name.Buffer, Data->Iopb->Parameters.Write.Length, Data->Iopb->Parameters.Write.ByteOffset.QuadPart));

            LsaFreeReturnBuffer(SecData);
        }

        FltReleaseFileNameInformation(FileNameInfo);
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


NTSTATUS MiniConnect(PFLT_PORT clientport, PVOID serverportcookie, PVOID Context, ULONG size, PVOID* Connectioncookie) {
    ClientPort = clientport;
    TraceEvent(TRACE_VERBOSE, "Connect\r\n");
    return STATUS_SUCCESS;
}


VOID MiniDisconnect(PVOID Connectioncookie) {
    TraceEvent(TRACE_VERBOSE, "Disconnect\r\n");
    FltCloseClientPort(FilterHandle, &ClientPort);
}



NTSTATUS MiniSendRecv(PVOID portcookie, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PULONG RetLength)
{
    NTSTATUS status;
    ULONG dataLength;
    UNICODE_STRING dataString;


    //command = ((PCOMMAND)InputBuffer)->OpCode;
    UNREFERENCED_PARAMETER(InputBufferLength);

    op_code = *((LONGLONG*)InputBuffer);


    switch (op_code)
    {
    case 1:
        KdPrint(("GetLogs\r\n"));
        break;
    case 0:
        KdPrint(("SetPath"));

        InputBuffer = ((LONGLONG*)InputBuffer) + 1;
        process_id_filter = *((LONGLONG*)InputBuffer);

        InputBuffer = ((LONGLONG*)InputBuffer) + 1;
        thread_id_filter = *((LONGLONG*)InputBuffer);

        InputBuffer = ((LONGLONG*)InputBuffer) + 1;
        RtlInitUnicodeString(&dataString, (PCWSTR)(InputBuffer));

        if (drive_filter.Buffer != NULL)
            RtlFreeUnicodeString(&drive_filter);

        if (dataString.Buffer != NULL)
            RtlUpcaseUnicodeString(&drive_filter, &dataString, TRUE);

        KdPrint(("Filter : %lld %lld %lld %S\r\n", op_code, process_id_filter,
            thread_id_filter, drive_filter.Buffer));
        break;
    }

    return STATUS_SUCCESS;
}


NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags)
{
    UNREFERENCED_PARAMETER(Flags);

    TraceEvent(TRACE_INFO, "Driver Unload.\r\n");

    KdPrint(("Driver Unload\r\n"));
    if (port) {
        FltCloseCommunicationPort(port);
        port = NULL;
    }
    FltUnregisterFilter(FilterHandle);
    if (WppDriverObject)
        WppCleanup(WppDriverObject);

    RtlFreeUnicodeString(&drive_filter);

    if(IsListEmpty(&log_list_head))
        KdPrint(("Unload True\r\n"));
    else
        KdPrint(("Unload False\r\n"));

    while (!IsListEmpty(&log_list_head))
    {
        KdPrint(("Unload Deleting list entry 2\r\n"));
        PLOG_ENTRY entry = PopLogEntry();

        if (entry != NULL)
        {
            //KdPrint(("Unload A\r\n"));
            ExFreePoolWithTag(entry->PMessage->Buffer, POOL_TAG2);
            //KdPrint(("Unload B\r\n"));
            ExFreePoolWithTag(entry, POOL_TAG);
            //KdPrint(("Unload C\r\n"));
        }
    }
    /*
    

    while (entry != NULL)
    {
        KdPrint(("Deleting list entry\r\n"));
        ExFreePoolWithTag(entry->PMessage->Buffer, POOL_TAG);
        ExFreePoolWithTag(entry, POOL_TAG);
        entry = PopLogEntry();
    }

    */
    return STATUS_SUCCESS;
}

extern "C"
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    PSECURITY_DESCRIPTOR sd;
    OBJECT_ATTRIBUTES oa = { 0 };
    UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\Mini-filter");


    KdPrint(("Driver Entry\r\n"));

    WppDriverObject = DriverObject;
    WppInit(WppDriverObject, RegistryPath);

    InitializeListHead(&log_list_head);
    NTSTATUS status;
    status = FltRegisterFilter(DriverObject, &FilterRegistration, &FilterHandle);

    if (!NT_SUCCESS(status))
    {
        return status;
    }
    status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);

    if (NT_SUCCESS(status))
    {
        InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, sd);
        status = FltCreateCommunicationPort(FilterHandle, &port, &oa, NULL, MiniConnect, MiniDisconnect, MiniSendRecv, 1);

        KdPrint(("COM Port Created\r\n"));

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
    RtlFreeUnicodeString(&drive_filter);
    return status;

}