#ifndef _SSDT_H
#define _SSDT_H
#include "SSDT.h"
#endif

// ָ��ĳ�����̺�ĳ��ȫ�ֱ����������ҳ�ڴ滹�ǷǷ�ҳ�ڴ�
#define PAGEDCODE code_seg("PAGE")
#define LOCKEDCODE code_seg()
#define INITCODE code_seg("INIT")

#define PAGEDDATA data_seg("PAGE")
#define LOCKEDDATA data_seg()
#define INITDATA data_seg("INIT")

#define arraysize(p) (sizeof(p)/sizeof((p)[0]))

#pragma PAGECODE
VOID DriverUnLoad(IN PDRIVER_OBJECT DriverObject)
{
	KdPrint(("DriverUnload!\n"));
//	UnInstallSysServiceHook((ULONG)ZwQuerySystemInformation);

	KdPrint(("[DriverUnload] ZwSetValueKey uninstall."));
	UnInstallSysServiceHook((ULONG)ZwSetValueKey);

	KdPrint(("[DriverUnload] ZwCreateKey uninstall."));
	UnInstallSysServiceHook((ULONG)ZwCreateKey);

// 	KdPrint(("[DriverUnload] ZwOpenKey uninstall."));
// 	UnInstallSysServiceHook((ULONG)ZwOpenKey);

	KdPrint(("[DriverUnload] ZwDeleteKey uninstall."));
	UnInstallSysServiceHook((ULONG)ZwDeleteKey);

	KdPrint(("[DriverUnload] ZwDeleteValueKey uninstall."));
	UnInstallSysServiceHook((ULONG)ZwDeleteValueKey);

	PDEVICE_OBJECT pNextObj;
	pNextObj = DriverObject->DeviceObject;
	while (pNextObj != NULL)
	{
		PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)
			pNextObj->DeviceExtension;

		//ɾ����������
		UNICODE_STRING pLinkName = pDevExt->ustrSymLinkName;
		IoDeleteSymbolicLink(&pLinkName);
		pNextObj = pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
	}

	KdPrint(("[DriverUnload]Done.\n"));
}

// DeviceControlDispatch
NTSTATUS DeviceIoControlDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP pIrp)
{
	PIO_STACK_LOCATION irpStack;
	NTSTATUS status = STATUS_SUCCESS;
	PVOID pInBuffer;
	ULONG pInbufferSize;
	PVOID pOutBuffer;
	ULONG pOutbufferSize;
	// ������
	ULONG uIoControlCode = NULL;
	HANDLE hEvent;
	HANDLE hWaitForUserRequestEvent;

	irpStack = IoGetCurrentIrpStackLocation(pIrp);

 	switch (irpStack->Parameters.DeviceIoControl.IoControlCode /*uIoControlCode*/)
 	{
	case IOCTL_SETEVENT:
		KdPrint(("IOCTL_SETEVENT called."));
		pInBuffer = pIrp->AssociatedIrp.SystemBuffer;
		pInbufferSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;
		if (pInBuffer == NULL || pInbufferSize < sizeof(HANDLE))
		{
			KdPrint(("[DeviceIoControlDispatch] Set event error!\n"));
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		hEvent = *(HANDLE*)pInBuffer;
		KdPrint(("hEvent: %x\n", hEvent));
		status = ObReferenceObjectByHandle(hEvent,
			GENERIC_ALL,
			NULL,
			KernelMode,
			(PVOID*)&gpEventObject,
			objHandleInfo);
		KdPrint(("gpEventObject: %x\n",gpEventObject));
		break;
	case IOCTL_SETWAITEVENT:
		KdPrint(("IOCTL_SETWAITEVENT called.\r"));
		pInBuffer = pIrp->AssociatedIrp.SystemBuffer;
		pInbufferSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;
		if (pInBuffer == NULL || pInbufferSize < sizeof(HANDLE))
		{
			KdPrint(("IOCTL_SETWAITEVENT set event error.\n"));
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		hWaitForUserRequestEvent = *(HANDLE*)pInBuffer;
		KdPrint(("hWaitForUserRequestEvent: %x\n", hWaitForUserRequestEvent));
		status = ObReferenceObjectByHandle(hWaitForUserRequestEvent,
			GENERIC_ALL,
			NULL,
			KernelMode,
			(PVOID*)&gpWaitForUserRequestEvent,
			pobjWaitHandleInfo);
		KdPrint(("gpEventObject: %x\n",gpEventObject));
		break;
	case IOCTL_GETINFO:
		KdPrint(("IOCTL_GETINFO Called.%s", RegData));
		pOutBuffer = pIrp->AssociatedIrp.SystemBuffer;
		pOutbufferSize = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
		RtlCopyMemory(pOutBuffer, RegData, sizeof(RegData));

		pIrp->IoStatus.Information = pOutbufferSize;
		break;
	case IOCTL_PASSUSERRES:
		KdPrint(("IOCTL_PASSUSERRES called.\n"));
		pInBuffer = pOutBuffer = pIrp->AssociatedIrp.SystemBuffer;
		pInbufferSize = irpStack->Parameters.DeviceIoControl.InputBufferLength;
		pOutbufferSize = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
		if (pInbufferSize)
		{
			RtlCopyMemory(&userRes, pInBuffer, pInbufferSize);
			KeSetEvent(gpWaitForUserRequestEvent, 0, FALSE);
		}
		else
		{
			KdPrint(("IOCTL_PASSUSERRES no pInbufferSize.\n"));
			break;
		}
		break;
	default:
		break;
	}

	switch (irpStack->MajorFunction)
	{
	case IRP_MJ_CREATE:
		KdPrint(("[DeviceIoControlDispatch] Call IRP_MJ_CREATE\n"));
		break;
	case IRP_MJ_CLOSE:
		KdPrint(("[DeviceIoControlDispatch] Call IRP_MJ_CLOSE\n"));
		break;
	case IRP_MJ_DEVICE_CONTROL:
		KdPrint(("[DeviceIoControlDispatch] Call IRP_MJ_DEVICE_CONTROL\n"));
		break;
		
	}

	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = pOutbufferSize;	//ǧ�������ó�0 ���ó�0��ʾʧ��
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	return status;
}



NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, 
						IN PUNICODE_STRING RegistryPath)
{
	KdPrint(("[DriverEntry]DriverEntry called!\n"));

// 	KeInitializeEvent(gpEventObject, NotificationEvent, FALSE);
 	KeInitializeMutex(&kmutexStockProcess, 0);

	KdPrint(("[DriverEntry]BackupSysServicesTable\n"));
	BackupSysServiceTable();

	KeInitializeMutex(&g_setValueKeyMutex, 0);
// 	KdPrint(("[DriverEntry]Install hook!\n"));
// 	InstallSysServiceHook((ULONG)ZwQuerySystemInformation, (ULONG)HookNtQuerySystemInformation);

	KdPrint(("[DriverEntry] Install ZwSetvalueKey Hook"));
	InstallSysServiceHook((ULONG)ZwSetValueKey, (ULONG)HookZwSetValueKey);

	KdPrint(("[DriverEntry] Install ZwCreateKey Hook"));
	InstallSysServiceHook((ULONG)ZwCreateKey, (ULONG)HookZwCreateKey);

// 	KdPrint(("[DriverEntry] Install ZwOpenKey Hook"));
// 	InstallSysServiceHook((ULONG)ZwOpenKey, (ULONG)HookZwOpenKey);

	KdPrint(("[DriverEntry] Install ZwDeleteKey Hook"));
	InstallSysServiceHook((ULONG)ZwDeleteKey, (ULONG)HookZwDeleteKey);

	KdPrint(("[DriverEntry] Install ZwDeleteValueKey Hook"));
	InstallSysServiceHook((ULONG)ZwDeleteValueKey, (ULONG)HookZwDeleteValueKey);

	KdPrint(("[DriverEntry]DriverEntry Done.\n"));
	//����ж�غ���
	DriverObject->DriverUnload = DriverUnLoad;

	// ������ǲ����
//	DriverObject->MajorFunction[IRP_MJ_READ] = HookReadfn;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoControlDispatch;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DeviceIoControlDispatch;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DeviceIoControlDispatch;

	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	// �����豸���� 
	UNICODE_STRING devName;
	RtlInitUnicodeString(&devName, L"\\Device\\RegDef");

	// �����豸
	status = IoCreateDevice( DriverObject, sizeof(DEVICE_EXTENSION), &(UNICODE_STRING)devName, 
		FILE_DEVICE_UNKNOWN, 0, FALSE, &pDevObj);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	else
	{
		KdPrint(("[DriverEntry] IoCreateDevice success."));
	}

	pDevObj->Flags |= DO_BUFFERED_IO;
	pDevExt = (PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice = pDevObj;
	pDevExt->ustrDeviceName = devName;

	//������������
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName, L"\\DosDevices\\RegDef");
	pDevExt->ustrSymLinkName = symLinkName;
	status = IoCreateSymbolicLink(&symLinkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(pDevObj);
		return status;
	}
	else
	{
		KdPrint(("[DriverEntry] IoCreateSymbolicLink success."));
	}

	return STATUS_SUCCESS;
}

VOID DisableWriteProtect(ULONG oldAddr)
{
	__asm
	{
		mov eax, oldAddr
		mov cr0, eax
		sti;
	}
}

VOID EnableWriteProtect(PULONG pOldAttr)
{
	ULONG uAttr;
	__asm
	{
		cli
		mov eax, cr0
		mov uAttr, eax
		and eax, not 10000H
		mov cr0, eax
	}
	*pOldAttr = uAttr;
}

VOID BackupSysServiceTable()
{
	KdPrint(("[BackupSysServiceTable]\n"));
	ULONG i = 0;
	for (i = 0; (i < KeServiceDescriptorTable->ntoskrnl.NumberOfService) && (i < MAX_SYSTEM_SERVICE_NUMBER); i++)
	{
		oldSysServiceAddr[i] = KeServiceDescriptorTable->ntoskrnl.ServiceTableBase[i];
		KdPrint(("[BackupSysServiceTable]Function Information: Number: 0x%04x, Address: %08X\n",i,oldSysServiceAddr[i]));
	}
}

NTSTATUS InstallSysServiceHook(ULONG oldService, ULONG newService)
{
	ULONG uOldAttr = 0;
	EnableWriteProtect(&uOldAttr);

	SYSCALL_FUNCTION(oldService) = newService;

	DisableWriteProtect(uOldAttr);

	KdPrint(("[InstallSysServiceHook] Hooked!\n"));

	return STATUS_SUCCESS;
}

NTSTATUS UnInstallSysServiceHook(ULONG oldService)
{
	ULONG uOldAttr = 0;
	
	EnableWriteProtect(&uOldAttr);
	SYSCALL_FUNCTION(oldService) = oldSysServiceAddr[SYSCALL_INDEX(oldService)];
	DisableWriteProtect(uOldAttr);

	KdPrint(("[UnInstallSysServiceHook]UnHooked!\n"));
	return STATUS_SUCCESS;
}

NTSTATUS HookReadfn(IN PDEVICE_OBJECT pDevObj, IN PIRP pIrp)
{
	KdPrint(("[HookReadFn] HookReadFn called.\n"));


	return STATUS_SUCCESS;
}



NTSTATUS HookNtQuerySystemInformation(
		__in SYSTEM_INFORMATION_CLASS SystemInformationClass,
		__out_bcount_opt(SystemInformationLength) PVOID SystemInformation,
		__in ULONG SystemInformationLength,
		__out_opt PULONG ReturnLength
		)
{
	NTSTATUS status;
	KdPrint(("[HookNtQuerySystemInformation] Hook works!\n"));
	
	NTQUERYSYSTEMINFORMATION pOldNtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)
		oldSysServiceAddr[SYSCALL_INDEX(ZwQuerySystemInformation)];

	status = pOldNtQuerySystemInformation(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("[HookNtQuerySystemInformation]pOldNtQuerySystemInformation failed!\n"));
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS HookZwSetValueKey(
			IN HANDLE			KeyHandle,
			IN PUNICODE_STRING  ValueName, //Ҫ�½������޸ĵļ���
			IN ULONG			TitleIndex  OPTIONAL, //һ����Ϊ0
			IN ULONG			Type,  //��ֵ���ͣ��ϱ��е�һ��
			IN PVOID			Data, //����
			IN ULONG			DataSize //��¼��ֵ���ݴ�С
	)
{
	NTSTATUS status;

	KdPrint(("[HookZwSetValueKey] Hook works! %wZ\n", ValueName));

	NTSETVALUEKEY pOldZwSetValueKey = (NTSETVALUEKEY)
		oldSysServiceAddr[SYSCALL_INDEX(ZwSetValueKey)];

	// ����ZwQueryObject ��þ����Ϣ
	// ��ô��޸ĵ�ע���·�� valuename�������Ӽ�
	PUNICODE_STRING pustrKeyName = NULL;
	ULONG outLen = 0;
	pustrKeyName = (PUNICODE_STRING)ExAllocatePool(NonPagedPool, 512*2+2*sizeof(ULONG));
	status = ZwQueryObject(KeyHandle, ObjectNameInformation, pustrKeyName, 1024, &outLen);
	KdPrint(("pustrKeyName: %wZ",pustrKeyName));

	//�õ������ߵĽ�����
	char *ProcessName = (char *)PsGetCurrentProcess() + 0x174;
	//char *ProcessName = GetCurrentProcessFileNam
	KdPrint((" %s\n", ProcessName));

	// ��ȡ����·��
	ANSI_STRING ansiKeyName;
	ANSI_STRING ansiSubkeyName;
	RtlUnicodeStringToAnsiString(&ansiKeyName, pustrKeyName, TRUE);
	RtlUnicodeStringToAnsiString(&ansiSubkeyName, ValueName, TRUE);
	

	if (strstr(ansiKeyName.Buffer, "\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"))
	{
		// ��ʼ�ȴ�������
		KeWaitForMutexObject(&kmutexStockProcess, Executive, KernelMode, FALSE, 0);

		// �����Ҫ����ring3������
		RtlZeroMemory(RegData, 1024);
		strcpy(RegData, ProcessName);
		strcat(RegData, "|");
		strcat(RegData, ansiKeyName.Buffer);
		strcat(RegData, "|");
		strcat(RegData, ansiSubkeyName.Buffer);
		strcat(RegData, "|");
		KdPrint(("RegData: %s\n", RegData));
		// ֪ͨ�ϲ�Ӧ�ó���
		KeSetEvent(gpEventObject,0,FALSE);
		KeClearEvent(gpEventObject);

		KeClearEvent(gpWaitForUserRequestEvent); // ���ⲻ����
		// ������ring0 Ȼ��ȴ�ring3 �Ļظ�
		KdPrint(("Ready to call KeWaitForSingleObject - gpWaitForuserrequ..\r"));
		KeWaitForSingleObject(gpWaitForUserRequestEvent, Executive, KernelMode, FALSE, 0);
		KeClearEvent(gpWaitForUserRequestEvent);

		if (userRes == 1)
		{
			// �ͷŻ�����
			KeReleaseMutex(&kmutexStockProcess, FALSE);
			return pOldZwSetValueKey(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
		}
		else 
		{
			KeReleaseMutex(&kmutexStockProcess, FALSE);
			return STATUS_ACCESS_DENIED;
		}
	}
	else 
	{
		return pOldZwSetValueKey(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
	}


	KdPrint(("Clear gpWAIT"));


	
	status = pOldZwSetValueKey(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("[HookZwSetValueKey]pOldZwSetValueKey failed!\n"));
		return status;
	}
	else KdPrint(("[HookZwSetValueKey] pOldZwSetValue Success!\n"));

	return STATUS_SUCCESS;
}

NTSTATUS HookZwCreateKey(
	OUT PHANDLE  KeyHandle,
	IN ACCESS_MASK  DesiredAccess, //����Ȩ�ޣ�һ��ΪKEY_ALL_ACCLESS
	IN POBJECT_ATTRIBUTES  ObjectAttributes,
	IN ULONG  TitleIndex, //һ��ΪNULL
	IN PUNICODE_STRING  Class  OPTIONAL, //һ��ΪNULL
	IN ULONG  CreateOptions, //һ��ΪREG_OPTION_NON_VOLATILE
	OUT PULONG  Disposition  OPTIONAL //�����Ǵ򿪳ɹ����Ǵ����ɹ�
	)
{
	NTSTATUS status;
//	KdPrint(("[HookZwCreateKey] Hook works!\n"));

	// ����ZwQueryObject ��þ����Ϣ
	// ��ô��޸ĵ�ע���·��
	PUNICODE_STRING pustrKeyName = NULL;
	ULONG outLen = 0;
	pustrKeyName = (PUNICODE_STRING)ExAllocatePool(NonPagedPool, 512*2+2*sizeof(ULONG));
	status = ZwQueryObject(KeyHandle, ObjectNameInformation, pustrKeyName, 1024, &outLen);
	KdPrint(("pustrKeyName: %wZ",pustrKeyName));

	//�õ������ߵĽ�����
	char *ProcessName = (char *)PsGetCurrentProcess() + 0x174;
//	KdPrint((" %s\n", ProcessName));

	NTCREATEKEY pOldZwCreateKey = (NTCREATEKEY)
		oldSysServiceAddr[SYSCALL_INDEX(ZwCreateKey)];

	status = pOldZwCreateKey(KeyHandle, DesiredAccess, ObjectAttributes, TitleIndex, Class, CreateOptions, Disposition);;
	if (!NT_SUCCESS(status))
	{
//		KdPrint(("[HookZwCreateKey] pOldZwCreateKey failed!\n"));
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS HookZwOpenKey(
	OUT PHANDLE KeyHandle,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
	NTSTATUS status;
//	KdPrint(("[HookZwOpenKey] Hook works!\n"));

	// ����ZwQueryObject ��þ����Ϣ
	// ��ô��޸ĵ�ע���·��
	PUNICODE_STRING pustrKeyName = NULL;
	ULONG outLen = 0;
	pustrKeyName = (PUNICODE_STRING)ExAllocatePool(NonPagedPool, 512*2+2*sizeof(ULONG));
	status = ZwQueryObject(KeyHandle, ObjectNameInformation, pustrKeyName, 1024, &outLen);
//	KdPrint(("pustrKeyName: %wZ",pustrKeyName));

	//�õ������ߵĽ�����
	char *ProcessName = (char *)PsGetCurrentProcess() + 0x174;
//	KdPrint((" %s\n", ProcessName));

	NTOPENKEY pOldZwOpenKey = (NTOPENKEY)
		oldSysServiceAddr[SYSCALL_INDEX(ZwOpenKey)];

	status = pOldZwOpenKey(KeyHandle, DesiredAccess, ObjectAttributes);
	if (!NT_SUCCESS(status))
	{
//		KdPrint(("[HookZwOpenKey] pOldZwOpenKey failed!\n"));
		return status;
	}
	else
	{
//		KdPrint(("[HookZwOpenKey] pOldZwOpenKey success.\n"));
		return STATUS_SUCCESS;
	}

	return STATUS_SUCCESS;
}

NTSTATUS HookZwDeleteKey(
	IN HANDLE  KeyHandle
	)
{
	NTSTATUS status;
//	KdPrint(("[HookZwDeleteKey] Hook works!\n"));

	// ����ZwQueryObject ��þ����Ϣ
	// ��ô��޸ĵ�ע���·��
	PUNICODE_STRING pustrKeyName = NULL;
	ULONG outLen = 0;
	pustrKeyName = (PUNICODE_STRING)ExAllocatePool(NonPagedPool, 512*2+2*sizeof(ULONG));
	status = ZwQueryObject(KeyHandle, ObjectNameInformation, pustrKeyName, 1024, &outLen);
//	KdPrint(("pustrKeyName: %wZ",pustrKeyName));

	//�õ������ߵĽ�����
	char *ProcessName = (char *)PsGetCurrentProcess() + 0x174;
//	KdPrint((" %s\n", ProcessName));

	NTDELETEKEY pOldZwDeleteKey = (NTDELETEKEY)
		oldSysServiceAddr[SYSCALL_INDEX(ZwDeleteKey)];

	status = pOldZwDeleteKey(KeyHandle);
	if (!NT_SUCCESS(status))
	{
//		KdPrint(("[HookZwDeleteKey] pOldZwDeleteKey failed!\n"));
		return status;
	}
	else
	{
//		KdPrint(("[HookZwDeleteKey] pOldZwDeleteKey success.\n"));
		return STATUS_SUCCESS;
	}

	return STATUS_SUCCESS;
}

NTSTATUS HookZwDeleteValueKey(
	IN  HANDLE KeyHandle,
	IN  PUNICODE_STRING ValueName
	)
{
	NTSTATUS status;
//	KdPrint(("[HookZwDeleteValueKey] Hook works!\n"));

	// ����ZwQueryObject ��þ����Ϣ
	// ��ô��޸ĵ�ע���·��
	PUNICODE_STRING pustrKeyName = NULL;
	ULONG outLen = 0;
	pustrKeyName = (PUNICODE_STRING)ExAllocatePool(NonPagedPool, 512*2+2*sizeof(ULONG));
	status = ZwQueryObject(KeyHandle, ObjectNameInformation, pustrKeyName, 1024, &outLen);
//	KdPrint(("pustrKeyName: %wZ",pustrKeyName));

	//�õ������ߵĽ�����
	char *ProcessName = (char *)PsGetCurrentProcess() + 0x174;
	//KdPrint((" %s\n", ProcessName));

	NTDELETEVALUEKEY pOldZwDeleteValueKey = (NTDELETEVALUEKEY)
		oldSysServiceAddr[SYSCALL_INDEX(ZwDeleteValueKey)];

	status = pOldZwDeleteValueKey(KeyHandle, ValueName);
	if (!NT_SUCCESS(status))
	{
//		KdPrint(("[HookZwDeleteValueKey] pOldZwDeleteValueKey failed!\n"));
		return status;
	}
	else
	{
//		KdPrint(("[HookZwDeleteValueKey] pOldZwDeleteValueKey success.\n"));
		return STATUS_SUCCESS;
	}

	return STATUS_SUCCESS;
}