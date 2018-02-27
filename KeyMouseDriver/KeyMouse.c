/*

  KeyMouse.C

  Author: mergerly
  Last Updated: 2009-11-19

*/

#include <ntddk.h>
#include <ntddkbd.h>

#include "KeyMouse.h"
#include "struct.h"
#include "pe.h"

#define MOUSE_EVENT
#ifdef MOUSE_EVENT
	#include <ntddmou.h>
#endif

#define dprintf if (DBG) DbgPrint
#define nprintf DbgPrint

#define kmalloc(_s)	ExAllocatePoolWithTag(NonPagedPool, _s, 'SYSQ')
//#define kfree(_p)	ExFreePoolWithTag(_p, 'SYSQ')
#define kfree(_p)	ExFreePool(_p)

//
// A structure representing the instance information associated with
// a particular device
//

typedef struct _DEVICE_EXTENSION
{
    ULONG  StateVariable;

} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// Device driver routine declarations.
//

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
	);

NTSTATUS
KeymouseDispatchCreate(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	);

NTSTATUS
KeymouseDispatchClose(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	);

NTSTATUS
KeymouseDispatchDeviceControl(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	);

VOID
KeymouseUnload(
	IN PDRIVER_OBJECT		DriverObject
	);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, KeymouseDispatchCreate)
#pragma alloc_text(PAGE, KeymouseDispatchClose)
#pragma alloc_text(PAGE, KeymouseDispatchDeviceControl)
#pragma alloc_text(PAGE, KeymouseUnload)
#endif // ALLOC_PRAGMA

//////////////////////////////////////////////////////////////////////////

#define PROLOG __asm { pushad } __asm { pushfd } 
#define RETURN __asm { popfd } __asm { popad } __asm { pop eax } __asm{ mov eax, 0xC000000D } __asm{ ret } 
#define EPILOG __asm { popfd } __asm { popad } __asm { ret } 



typedef VOID
(*My_KeyboardClassServiceCallback) (
									PDEVICE_OBJECT  DeviceObject,
									PKEYBOARD_INPUT_DATA  InputDataStart,
									PKEYBOARD_INPUT_DATA  InputDataEnd,
									PULONG  InputDataConsumed
									);
#ifdef MOUSE_EVENT

typedef VOID
(*My_MouseClassServiceCallback) (
								    PDEVICE_OBJECT  DeviceObject,
									PMOUSE_INPUT_DATA  InputDataStart,
									PMOUSE_INPUT_DATA  InputDataEnd,
									PULONG  InputDataConsumed
									);
My_MouseClassServiceCallback orig_MouseClassServiceCallback = NULL;
#endif
My_KeyboardClassServiceCallback orig_KeyboardClassServiceCallback = NULL;

//////////////////////////////////////////////////////////////////////////

ULONG g_kbdclass_base = 0, g_kbdclass_size = 0;
ULONG g_lpKbdServiceCallback = 0;
PDEVICE_OBJECT g_kbDeviceObject = NULL;
KEYBOARD_INPUT_DATA kid;

ULONG code1_sp2=0x8b55ff8b, code2_sp2=0x8b5151ec, code3_sp2=0x65830845,code4_sp2=0x8b530008;

#ifdef MOUSE_EVENT
ULONG g_mouclass_base = 0, g_mouclass_size = 0;
ULONG g_lpmouServiceCallback = 0;
PDEVICE_OBJECT g_mouDeviceObject = NULL;
MOUSE_INPUT_DATA mid;

ULONG mcode1_sp2=0x8b55ff8b, mcode2_sp2=0x8b5151ec, mcode3_sp2=0x4d8b0845,mcode4_sp2=0x8b56530c;
#endif
/*
XP SP2 SP3
kd> dd kbdclass!KeyboardClassServiceCallback
f9ca5192  8b55ff8b 8b5151ec 65830845 8b530008
f9ca51a2  5d2b105d 708b560c 14458b28 57002083

kbdclass!KeyboardClassServiceCallback:
f9ca5192 8bff            mov     edi,edi
f9ca5194 55              push    ebp
f9ca5195 8bec            mov     ebp,esp
f9ca5197 51              push    ecx
f9ca5198 51              push    ecx
f9ca5199 8b4508          mov     eax,dword ptr [ebp+8]
f9ca519c 83650800        and     dword ptr [ebp+8],0
f9ca51a0 53              push    ebx
f9ca51a1 8b5d10          mov     ebx,dword ptr [ebp+10h]
f9ca51a4 2b5d0c          sub     ebx,dword ptr [ebp+0Ch]
f9ca51a7 56              push    esi
f9ca51a8 8b7028          mov     esi,dword ptr [eax+28h]
f9ca51ab 8b4514          mov     eax,dword ptr [ebp+14h]
f9ca51ae 832000          and     dword ptr [eax],0
f9ca51b1 57              push    edi
f9ca51b2 6a04            push    4

kd> dd mouclass!MouseClassServiceCallback
f9cb4f34  8b55ff8b 8b5151ec 4d8b0845 8b56530c
f9cb4f44  458b2870 89c12b10 458b0845 89db3314

mouclass!MouseClassServiceCallback:
f9cb4f34 8bff            mov     edi,edi
f9cb4f36 55              push    ebp
f9cb4f37 8bec            mov     ebp,esp
f9cb4f39 51              push    ecx
f9cb4f3a 51              push    ecx
f9cb4f3b 8b4508          mov     eax,dword ptr [ebp+8]
f9cb4f3e 8b4d0c          mov     ecx,dword ptr [ebp+0Ch]
f9cb4f41 53              push    ebx
f9cb4f42 56              push    esi
f9cb4f43 8b7028          mov     esi,dword ptr [eax+28h]
f9cb4f46 8b4510          mov     eax,dword ptr [ebp+10h]
f9cb4f49 2bc1            sub     eax,ecx
f9cb4f4b 894508          mov     dword ptr [ebp+8],eax
f9cb4f4e 8b4514          mov     eax,dword ptr [ebp+14h]
f9cb4f51 33db            xor     ebx,ebx
f9cb4f53 8918            mov     dword ptr [eax],ebx

f9ca4000 f9ca9b00   kbdclass
f9cb4000 f9cb9500   mouclass

//////////////////////////////////////////////////////////////////////////
Windows7
kd> dd kbdclass!KeyboardClassServiceCallback
9207a4a8  8b55ff8b 10ec83ec a1575653 9207d000
9207a4b8  07c1bcbf 2d6a5792 db33036a 3070ff53


kd> u kbdclass!KeyboardClassServiceCallback
kbdclass!KeyboardClassServiceCallback:
9207a4a8 8bff            mov     edi,edi
9207a4aa 55              push    ebp
9207a4ab 8bec            mov     ebp,esp
9207a4ad 83ec10          sub     esp,10h
9207a4b0 53              push    ebx
9207a4b1 56              push    esi
9207a4b2 57              push    edi
9207a4b3 a100d00792      mov     eax,dword ptr [kbdclass!WPP_GLOBAL_Control (9207d000)]
9207a4b8 bfbcc10792      mov     edi,offset kbdclass!WPP_ThisDir_CTLGUID_KbdClassTraceGuid+0x10 (9207c1bc)
9207a4bd 57              push    edi
9207a4be 6a2d            push    2Dh
9207a4c0 6a03            push    3
9207a4c2 33db            xor     ebx,ebx
9207a4c4 53              push    ebx
9207a4c5 ff7030          push    dword ptr [eax+30h]
9207a4c8 e867ebffff      call    kbdclass!WPP_RECORDER_SF_ (92079034)


kd> dd mouclass!MouseClassServiceCallback
920892e6  8b55ff8b 10ec83ec a1575653 9208c000
920892f6  08b1bcbf 2d6a5792 db33036a 3070ff53

kd> u mouclass!MouseClassServiceCallback
mouclass!MouseClassServiceCallback:
920892e6 8bff            mov     edi,edi
920892e8 55              push    ebp
920892e9 8bec            mov     ebp,esp
920892eb 83ec10          sub     esp,10h
920892ee 53              push    ebx
920892ef 56              push    esi
920892f0 57              push    edi
920892f1 a100c00892      mov     eax,dword ptr [mouclass!WPP_GLOBAL_Control (9208c000)]
920892f6 bfbcb10892      mov     edi,offset mouclass!WPP_ThisDir_CTLGUID_MouClassTraceGuid+0x10 (9208b1bc)
920892fb 57              push    edi
920892fc 6a2d            push    2Dh
920892fe 6a03            push    3
92089300 33db            xor     ebx,ebx
92089302 53              push    ebx
92089303 ff7030          push    dword ptr [eax+30h]
92089306 e829edffff      call    mouclass!WPP_RECORDER_SF_ (92088034)

92078000 92085000   kbdclass
92087000 92094000   mouclass
*/

//////////////////////////////////////////////////////////////////////////

PVOID 
GetModlueBaseAdress(
					char* ModlueName,
					BOOL bKernelBase,
					ULONG* psize
					)
{
	ULONG size,index;
	PULONG buf;
	NTSTATUS status;
	PSYSTEM_MODULE_INFORMATION module;
	PVOID driverAddress=0;

	ZwQuerySystemInformation(SystemModuleInformation,&size, 0, &size);
	if(NULL==(buf = (PULONG)ExAllocatePool(PagedPool, size))){
		DbgPrint("failed alloc memory failed \n");
		return 0;
	}

	status=ZwQuerySystemInformation(SystemModuleInformation,buf, size , 0);
	if(!NT_SUCCESS( status )) {
		DbgPrint("failed query\n");
		return 0;
	}

	module = (PSYSTEM_MODULE_INFORMATION)(( PULONG )buf + 1);

	// 系统模块基址
	if ( TRUE == bKernelBase )
	{
		driverAddress = module[0].Base;
		DbgPrint("KernelBase:%x\n",driverAddress);
		goto _x_;
	}

	// 其他模块基址
	for (index = 0; index < *buf; index++) {
		if (_stricmp(module[index].ImageName + module[index].ModuleNameOffset, ModlueName) == 0) 
		{
			driverAddress = module[index].Base;
			if(psize)
				*psize = module[index].Size;			
			DbgPrint("Module found at:%x\n",driverAddress);
			goto _x_;
		}
	}

_x_:		
	ExFreePool(buf);
	return driverAddress;
}

NTSTATUS KbdInit( IN PDRIVER_OBJECT DriverObject) 
{
	UNICODE_STRING ntUnicodeString;
	NTSTATUS ntStatus;
	PDEVICE_OBJECT DeviceObject = NULL;
	PDEVICE_OBJECT			TargetDeviceObject = NULL;
	PFILE_OBJECT			TargetFileObject	= NULL;

	PDRIVER_OBJECT		pTargetDriverObj = NULL;
	PDEVICE_OBJECT		pTargetDeviceObj = NULL;
	
	RtlInitUnicodeString( &ntUnicodeString, L"\\Driver\\Kbdclass");
	ntStatus = ObReferenceObjectByName(
		&ntUnicodeString, 
		OBJ_CASE_INSENSITIVE,
		NULL, 0,
		*IoDriverObjectType,
		KernelMode, NULL,
		(PVOID*)&pTargetDriverObj);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("KbdClass: can not open driver %wZ\n", &ntUnicodeString);
		return ntStatus;
	}
	//创建过滤设备 并遍历绑定
	pTargetDeviceObj = pTargetDriverObj->DeviceObject;
	g_kbdclass_base = (ULONG)pTargetDriverObj->DriverStart;
	g_kbdclass_size = pTargetDriverObj->DriverSize;
	while(pTargetDeviceObj)
	{
		if(NULL == pTargetDeviceObj->NextDevice)
		{
			g_kbDeviceObject = pTargetDeviceObj;
			dprintf("[KeyMouse] KbdClass Device = 0x%x\n", g_kbDeviceObject);
			break;
		}
		pTargetDeviceObj = pTargetDeviceObj->NextDevice;
	}
	ObDereferenceObject(pTargetDriverObj);
#if 0
	RtlInitUnicodeString( &ntUnicodeString, L"\\Device\\KeyboardClass0" );
	ntStatus = IoGetDeviceObjectPointer(IN	&ntUnicodeString,
		IN	FILE_ALL_ACCESS,
		OUT	&TargetFileObject,   
		OUT	&g_kbDeviceObject
		);
	if( !NT_SUCCESS(ntStatus) )
	{
		DbgPrint(("Couldn't Get the Keyboard Device Object\n"));

		TargetFileObject	= NULL;
		g_kbDeviceObject = NULL;	
		return( ntStatus );
	}	 
#endif
	return STATUS_SUCCESS;
}

NTSTATUS MouInit( IN PDRIVER_OBJECT DriverObject) 
{
	UNICODE_STRING ntUnicodeString;
	NTSTATUS ntStatus;
	PDEVICE_OBJECT DeviceObject = NULL;
	PDEVICE_OBJECT			TargetDeviceObject = NULL;
	PFILE_OBJECT			TargetFileObject	= NULL;

	PDRIVER_OBJECT		pTargetDriverObj = NULL;
	PDEVICE_OBJECT		pTargetDeviceObj = NULL;

	RtlInitUnicodeString( &ntUnicodeString, L"\\Driver\\MouClass");
	ntStatus = ObReferenceObjectByName(
		&ntUnicodeString, 
		OBJ_CASE_INSENSITIVE,
		NULL, 0,
		*IoDriverObjectType,
		KernelMode, NULL,
		(PVOID*)&pTargetDriverObj);
	if(!NT_SUCCESS(ntStatus))
	{
		DbgPrint("MouClass: can not open driver %wZ\n", &ntUnicodeString);
		return ntStatus;
	}
	//创建过滤设备 并遍历绑定
	pTargetDeviceObj = pTargetDriverObj->DeviceObject;
	g_mouclass_base = (ULONG)pTargetDriverObj->DriverStart;
	g_mouclass_size = pTargetDriverObj->DriverSize;
	while(pTargetDeviceObj)
	{
		if(NULL == pTargetDeviceObj->NextDevice)
		{
			g_mouDeviceObject = pTargetDeviceObj;
			dprintf("[KeyMouse] MouClass Device = 0x%x\n", g_mouDeviceObject);
			break;
		}
		pTargetDeviceObj = pTargetDeviceObj->NextDevice;
	}
	ObDereferenceObject(pTargetDriverObj);
#if 0
	RtlInitUnicodeString( &ntUnicodeString, L"\\Device\\PointerClass0" );
	ntStatus = IoGetDeviceObjectPointer(IN	&ntUnicodeString,
		IN	FILE_ALL_ACCESS,
		OUT	&TargetFileObject,   
		OUT	&g_mouDeviceObject
		);
	if( !NT_SUCCESS(ntStatus) )
	{
		DbgPrint(("Couldn't Get the Mouse Device Object\n"));

		TargetFileObject	= NULL;
		g_mouDeviceObject = NULL;	
		return( ntStatus );
	}	 
#endif
	return STATUS_SUCCESS;
}
NTSTATUS GetCallBackAddr() 
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ULONG i, curAddr;
	ULONG MajorVersion = 0, MinorVersion = 0, BuildNumber = 0, dwOSVer = 0;	
	ULONG codeoffer4 = 0, mcodeoffer4 = 0;
	PsGetVersion(&MajorVersion, &MinorVersion, &BuildNumber, NULL);
	if ( MajorVersion == 5 )
	{
		switch(MinorVersion) 
		{
		case 0:
			dwOSVer = 1;	//2000
			break;
		case 1:
			dwOSVer = 2;//Windows XP
			break;
		case 2:
			dwOSVer = 3;//Windows 2003
			break;
		}
	}
	else if( MajorVersion == 6 )
	{
		switch (MinorVersion)
		{
		case 0:
			dwOSVer = 4;	//Vista
			break;
		case 1:
			dwOSVer = 5;	//Windows 7
			break;
		}
	}

	//获取模块基地址
	if(0 == g_kbdclass_base)
		g_kbdclass_base = (ULONG)GetModlueBaseAdress( "kbdclass.sys",0, &g_kbdclass_size);
	DbgPrint("kbdclass.sys: 0x%08lx\n", (PVOID)g_kbdclass_base);

	if ( 0 == g_kbdclass_base ) 
	{
		DbgPrint("ERROR: g_kbdclass_base == 0\n");		
		goto __failed;
	}

	if(0 == g_mouclass_base)
		g_mouclass_base = (ULONG)GetModlueBaseAdress( "mouclass.sys",0, &g_mouclass_size);
	DbgPrint("mouclass.sys: 0x%08lx\n", (PVOID)g_mouclass_base);

	if ( 0 == g_mouclass_base ) 
	{
		DbgPrint("ERROR: g_mouclass_base == 0\n");
		goto __failed;
	}

	//搜索特征
	switch (dwOSVer)
	{
	case 5:	//Windows 7
		{
			code1_sp2 = 0x8b55ff8b;
			code2_sp2 = 0x10ec83ec;
			code3_sp2 = 0xa1575653;
			code4_sp2 = 0xdb33036a;
			codeoffer4 = 24;

			mcode1_sp2 = 0x8b55ff8b;
			mcode2_sp2 = 0x10ec83ec;
			mcode3_sp2 = 0xa1575653;
			mcode4_sp2 = 0xdb33036a;
			mcodeoffer4 = 24;
			break;
		}
	default:
		{
			codeoffer4 = 12;
			mcodeoffer4 = 12;
			break;
		}
	}
	curAddr = g_kbdclass_base;
	//	DbgPrint("curAddr: 0x%08lx\n", curAddr);
	//for (i=curAddr;i<=curAddr+g_kbdclass_size-codeoffer4-4;i++)
	for (i=curAddr;i<=curAddr+0x3000;i++)
	{
		//	DbgPrint("i: 0x%08lx\n", i);
		if (*((ULONG *)i)==code1_sp2) {
			if (*((ULONG *)(i+4))==code2_sp2) {
				if (*((ULONG *)(i+8))==code3_sp2) {
					if (*((ULONG *)(i+codeoffer4))==code4_sp2) {
						g_lpKbdServiceCallback=i;
						break ;
					}
				}
			}
		}
	}


	curAddr = g_mouclass_base;
	//	DbgPrint("curAddr: 0x%08lx\n", curAddr);
	//for (i=curAddr;i<=curAddr+g_mouclass_size-codeoffer4-4;i++)
	for (i=curAddr;i<=curAddr+0x3000;i++)
	{
		//	DbgPrint("i: 0x%08lx\n", i);
		if (*((ULONG *)i)==mcode1_sp2) {
			if (*((ULONG *)(i+4))==mcode2_sp2) {
				if (*((ULONG *)(i+8))==mcode3_sp2) {
					if (*((ULONG *)(i+mcodeoffer4))==mcode4_sp2) {
						g_lpmouServiceCallback=i;
						break ;
					}
				}
			}
		}
	}

	//判断地址是否成功获取
	orig_KeyboardClassServiceCallback = (My_KeyboardClassServiceCallback)g_lpKbdServiceCallback;
	DbgPrint("KeyboardClassServiceCallback: 0x%08lx\n", (PVOID)g_lpKbdServiceCallback);

	if ( 0 == g_lpKbdServiceCallback ) {
		DbgPrint("ERROR: g_lpKbdServiceCallback == 0\n");
		goto __failed;
	}

	orig_MouseClassServiceCallback = (My_MouseClassServiceCallback)g_lpmouServiceCallback;
	DbgPrint("MouseClassServiceCallback: 0x%08lx g_mouDeviceObject:0x%08lx\n", (PVOID)g_lpmouServiceCallback, (PVOID)g_mouDeviceObject);

	if ( 0 == g_lpmouServiceCallback ) 
	{
		DbgPrint("ERROR: g_lpmouServiceCallback == 0\n");
		goto __failed;
	}
	status = STATUS_SUCCESS;
__failed:
	return status;
}
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT		DriverObject,
	IN PUNICODE_STRING		RegistryPath
	)
{
	NTSTATUS			status = STATUS_SUCCESS;    
    UNICODE_STRING		ntDeviceName;
	UNICODE_STRING		dosDeviceName;
    PDEVICE_EXTENSION	deviceExtension;
	PDEVICE_OBJECT		deviceObject = NULL;
	BOOLEAN				fSymbolicLink = FALSE;
	
	PUCHAR FileContent;
	DWORD dwRet,dwSize;
	PVOID pTmp;

	//KdBreakPoint();

    dprintf("[KeyMouse] DriverEntry: %wZ\n", RegistryPath);

    //
    // A real driver would:
    //
    //     1. Report it's resources (IoReportResourceUsage)
    //
    //     2. Attempt to locate the device(s) it supports

    //
    // OK, we've claimed our resources & found our h/w, so create
    // a device and initialize stuff...
    //

    RtlInitUnicodeString(&ntDeviceName, KEYMOUSE_DEVICE_NAME_W);

    //
    // Create an EXCLUSIVE device, i.e. only 1 thread at a time can send
    // i/o requests.
    //

    status = IoCreateDevice(
		DriverObject,
		sizeof(DEVICE_EXTENSION), // DeviceExtensionSize
		&ntDeviceName, // DeviceName
		FILE_DEVICE_KEYMOUSE, // DeviceType
		0, // DeviceCharacteristics
		TRUE, // Exclusive
		&deviceObject // [OUT]
		);

    if (!NT_SUCCESS(status))
	{
		dprintf("[KeyMouse] IoCreateDevice = 0x%x\n", status);
		goto __failed;
	}

	deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;

	//
	// Set up synchronization objects, state info,, etc.
	//

    //
    // Create a symbolic link that Win32 apps can specify to gain access
    // to this driver/device
    //

    RtlInitUnicodeString(&dosDeviceName, KEYMOUSE_DOS_DEVICE_NAME_W);

    status = IoCreateSymbolicLink(&dosDeviceName, &ntDeviceName);

    if (!NT_SUCCESS(status))
    {
        dprintf("[KeyMouse] IoCreateSymbolicLink = 0x%x\n", status);
		goto __failed;
    }

	fSymbolicLink = TRUE;

    //
    // Create dispatch points for device control, create, close.
    //

    DriverObject->MajorFunction[IRP_MJ_CREATE]         = KeymouseDispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = KeymouseDispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KeymouseDispatchDeviceControl;
    DriverObject->DriverUnload                         = KeymouseUnload;

	//
	// 添加执行代码
	//
	status = KbdInit(DriverObject);
	if(!NT_SUCCESS(status))
	{
		DbgPrint("ERROR: KbdInit = 0x%08lx\n", status);
		goto __failed;
	}

	status = MouInit(DriverObject);
	if(!NT_SUCCESS(status))
	{
		DbgPrint("ERROR: MouInit = 0x%08lx\n", status);
		goto __failed;
	}

	if(STATUS_SUCCESS != GetCallBackAddr())
	{
		status = STATUS_UNSUCCESSFUL;
		DbgPrint("ERROR: GetCallBackAddr = 0x%08lx\n", status);
		goto __failed;
	}

	if (NT_SUCCESS(status))
	    return status;

__failed:

	if (fSymbolicLink)
		IoDeleteSymbolicLink(&dosDeviceName);

	if (deviceObject)
		IoDeleteDevice(deviceObject);

	return status;
}

NTSTATUS
KeymouseDispatchCreate(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	)
{
	NTSTATUS status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;

	dprintf("[KeyMouse] IRP_MJ_CREATE\n");

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
KeymouseDispatchClose(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	)
{
	NTSTATUS status = STATUS_SUCCESS;

    Irp->IoStatus.Information = 0;

	dprintf("[KeyMouse] IRP_MJ_CLOSE\n");

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
KeymouseDispatchDeviceControl(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	)
{
	NTSTATUS			status = STATUS_SUCCESS;
    PIO_STACK_LOCATION	irpStack;
    PDEVICE_EXTENSION	deviceExtension;
    PVOID				ioBuf;
    ULONG				inBufLength, outBufLength;
	ULONG				ioControlCode;

	ULONG				lKeyCode;
	ULONG				lKeyFlags;
	DWORD				dwRet,dwSize;

	USHORT 				MouseFlags;
	ULONG 				InputDataConsumed;
	PMOUSE_INPUT_DATA 	InputDataStart, InputDataEnd;
	KIRQL 				oldirq;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    Irp->IoStatus.Information = 0;

    //
    // Get the pointer to the input/output buffer and it's length
    //

    ioBuf = Irp->AssociatedIrp.SystemBuffer;
    inBufLength = irpStack->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ioControlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;

    switch (ioControlCode)
    {
	case IOCTL_SEND_KEY:
		{
			if (ioBuf == NULL || inBufLength < sizeof(ULONG)*2)
			{
				status == STATUS_INVALID_PARAMETER;
				Irp->IoStatus.Information = 0;
			}
			__try
			{
				lKeyCode = *((ULONG*)ioBuf+1);
				lKeyFlags = *(ULONG*)ioBuf;
				dprintf("[KeyMouse] KeymouseDispatchDeviceControl IOCTL_SEND_KEY = 0x%x 0x%x\n", lKeyCode, lKeyFlags);
				RtlZeroMemory((void*)&kid, sizeof(KEYBOARD_INPUT_DATA));
				kid.Flags = lKeyFlags;
				kid.MakeCode = lKeyCode;
				orig_KeyboardClassServiceCallback(g_kbDeviceObject, &kid,  (PKEYBOARD_INPUT_DATA)&kid + 1, &InputDataConsumed);
				status = STATUS_SUCCESS;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				status = GetExceptionCode();
			}
			break;
		}
	case IOCTL_SEND_MOUSE:
		{
			if (ioBuf == NULL || inBufLength < sizeof(ULONG)*3)
			{
				status == STATUS_INVALID_PARAMETER;
				Irp->IoStatus.Information = 0;
			}
			__try
			{				
				RtlZeroMemory((void*)&mid, sizeof(MOUSE_INPUT_DATA));
				if(inBufLength == sizeof(ULONG)*4)		//4个参数时，坐标关系可以直接设定，主要用于鼠标移动
					mid.Flags = *((ULONG*)ioBuf+3);
				else
					mid.Flags = MOUSE_MOVE_RELATIVE;	//3个参数时，坐标关系采用相对关系，主要用于鼠标点击
				mid.ButtonFlags = *((ULONG*)ioBuf);
				mid.LastX = *((ULONG*)ioBuf+1);
				mid.LastY = *((ULONG*)ioBuf+2);
				dprintf("[KeyMouse] KeymouseDispatchDeviceControl IOCTL_SEND_MOUSE = 0x%x 0x%d 0x%d\n", mid.ButtonFlags, mid.LastX, mid.LastY);
				orig_MouseClassServiceCallback(g_mouDeviceObject, &mid,  (PMOUSE_INPUT_DATA)&mid + 1, &InputDataConsumed);
				status = STATUS_SUCCESS;
			}
			__except(EXCEPTION_EXECUTE_HANDLER)
			{
				status = GetExceptionCode();
			}
			break;
		}
    default:
        status = STATUS_INVALID_PARAMETER;

        dprintf("[KeyMouse] unknown IRP_MJ_DEVICE_CONTROL = 0x%x (%04x,%04x)\n",
			ioControlCode, DEVICE_TYPE_FROM_CTL_CODE(ioControlCode),
			IoGetFunctionCodeFromCtlCode(ioControlCode));

        break;
	}

    //
    // DON'T get cute and try to use the status field of
    // the irp in the return status.  That IRP IS GONE as
    // soon as you call IoCompleteRequest.
    //

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // We never have pending operation so always return the status code.
    //

    return status;
}

VOID
KeymouseUnload(
	IN PDRIVER_OBJECT		DriverObject
	)
{
    UNICODE_STRING dosDeviceName;

	//
    // Free any resources
    //

    //
    // Delete the symbolic link
    //

    RtlInitUnicodeString(&dosDeviceName, KEYMOUSE_DOS_DEVICE_NAME_W);

    IoDeleteSymbolicLink(&dosDeviceName);

    //
    // Delete the device object
    //

    IoDeleteDevice(DriverObject->DeviceObject);

    dprintf("[KeyMouse] unloaded\n");
}
