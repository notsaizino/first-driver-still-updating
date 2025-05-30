#include <ntddk.h>
#include <dontuse.h>
#define DRIVER_TAG 'DCBA'

UNICODE_STRING g_regpath; //G_Registry_Path. Arbitrary var name to be used later. 

extern "C" NTSTATUS DriverEntry( PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) {
	
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	DriverObject->DriverUnload = SampleUnload;


	KdPrint(("Sample Driver initialized successfully.\n"));

	g_regpath.Buffer = (WCHAR*)ExAllocatePoolWithTag(PagedPool, RegistryPath->Length, DRIVER_TAG);

	if (g_regpath.Buffer == nullptr) {
		KdPrint(("Failed to allocate memory\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	g_regpath.MaximumLength = RegistryPath->Length;
	RtlCopyUnicodeString(&g_regpath, (PCUNICODE_STRING)RegistryPath);

	//%wZ is for UNICODE_STRING objects.
	KdPrint(("Original registry path: %wZ\n", RegistryPath));
	KdPrint(("Copied registry path: %wZ\n", &g_regpath));

	return STATUS_SUCCESS; 
}
//At some point, the driver may be unloaded. At that time, anything done in the DriverEntry function
//must be undone.Failure to do so creates a leak, which the kernel will not clean up until the next
//reboot.
void SampleUnload(PDRIVER_OBJECT DriverObject) {
	UNREFERENCED_PARAMETER(DriverObject);

	ExFreePool(g_regpath.Buffer);
	

	KdPrint(("Sample Driver unload called\n")); //Prints a message (to debug, for example). unfortunately, dbgprint has a lot of overhead- it's better to use Kdprint.
}

//OTHER PART OF DRIVER- OPEN FILE)
NTSTATUS OpenProcess(ACCESS_MASK accessMask, ULONG pid, PHANDLE phProcess) {
	CLIENT_ID cid;
	cid.UniqueProcess = ULongToHandle(pid); //To open a process, we need to specify the process ID in the UniqueProcess member. 
	//The HANDLE type is that process and thread IDs are generated froma private HANDLE table. 
	//The ULongToHandle function performs the required casts so that the compiler is happy (HANDLE is 64-bit on a 64-bit system, but ULONG = 32bits).
	cid.UniqueThread = nullptr; //this is typically null

	OBJECT_ATTRIBUTES procAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(nullptr, OBJ_KERNEL_HANDLE); //WHY IS THERE AN ERROR, SAVE ME

	return ZwOpenProcess(phProcess, accessMask, &procAttributes, &cid);
}
NTSTATUS OpenFileForRead(PCWSTR path, PHANDLE phFile) {
	UNICODE_STRING name;
	RtlInitUnicodeString(&name, path);

	OBJECT_ATTRIBUTES fileAttributes;
	InitializeObjectAttributes(&fileAttributes, &name, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);
	IO_STATUS_BLOCK ioStatus;

	return ZwOpenFile(phFile, FILE_GENERIC_READ, &fileAttributes, &ioStatus, FILE_SHARE_READ, 0);
}