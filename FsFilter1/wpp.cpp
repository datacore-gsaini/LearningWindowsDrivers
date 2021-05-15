#include <ntddk.h>
#include "Trace.h"

void
WppInit(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pRegistryPath)
{
    WPP_INIT_TRACING(pDriverObject, pRegistryPath);
}

void
WppCleanup(PDRIVER_OBJECT pDriverObject)
{
    WPP_CLEANUP(pDriverObject);
}