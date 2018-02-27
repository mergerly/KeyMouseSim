/***************************************************************** 
文件名 : WssLockKey.c 
描述 : 键盘过滤驱动 
作者 : sinister 
最后修改日期 : 2007-02-26 
*****************************************************************/ 
#include "WssLockKey.h" 
//#include "kbdmou.h"
//#include <ntddmou.h>
#define IOCTL_INTERNAL_MOUSE_CONNECT    CTL_CODE(FILE_DEVICE_MOUSE, 0x0080, METHOD_NEITHER, FILE_ANY_ACCESS)

BOOLEAN g_bUsbKeyBoard = FALSE, g_bUsbMouse = FALSE;

USHORT g_UnitId;
PDEVICE_OBJECT g_MouseDev;

PSERVICE_CALLBACK_ROUTINE g_SeviceCallback;

typedef NTSTATUS
(NTAPI*	INTERNIOCTL)(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	);
INTERNIOCTL g_OldInternIoCtl;
NTSTATUS
	MouFilter_InternIoCtl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp
	)
{
	PIO_STACK_LOCATION          irpStack;
	PDEVICE_EXTENSION           devExt;
	PCONNECT_DATA               connectData;
	NTSTATUS                    status = STATUS_SUCCESS;

	devExt = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
	Irp->IoStatus.Information = 0;
	irpStack = IoGetCurrentIrpStackLocation(Irp);

	switch (irpStack->Parameters.DeviceIoControl.IoControlCode) 
	{
		//
		// Connect a mouse class device driver to the port driver.
		//
	case IOCTL_INTERNAL_MOUSE_CONNECT:
		//
		// Only allow one connection.
		//
		if (irpStack->Parameters.DeviceIoControl.InputBufferLength <
			sizeof(CONNECT_DATA)) {
				//
				// invalid buffer
				//
				status = STATUS_INVALID_PARAMETER;
				break;
		}

		//
		// Copy the connection parameters to the device extension.
		//
		connectData = ((PCONNECT_DATA)
			(irpStack->Parameters.DeviceIoControl.Type3InputBuffer));

		g_MouseDev = connectData->ClassDeviceObject;
		g_SeviceCallback = *(PSERVICE_CALLBACK_ROUTINE)connectData->ClassService;
		DbgPrint("KeyClass = %x,Service Callback = %x\n", g_MouseDev, g_SeviceCallback);
		break;
	}
	return g_OldInternIoCtl(DeviceObject, Irp);
}
NTSTATUS KeyDriverEntry( IN PDRIVER_OBJECT KeyDriverObject, IN PUNICODE_STRING RegistryPath )
{ 
	UNICODE_STRING KeyDeviceName; 
	PDRIVER_OBJECT KeyDriver; 
	PDEVICE_OBJECT UsbKeyBoardDeviceObject, UsbMouseDeviceObject, HookDeviceObject; 
	NTSTATUS ntStatus; 
	ULONG i; 
	// 
	// 保存设备名，调试使用 
	// 
	WCHAR szDeviceName[MAXLEN + MAXLEN] = {0}; 
	//KeyDriverObject->DriverUnload = KeyDriverUnload; 
	// 
	// 先尝试获得 USB 键盘设备对象，如果成功则挂接 USB 键盘 
	// 
	// 注意：因为 USB 键盘设备名不固定，且即使得到名称也无法 
	// 使用 IoGetDeviceObjectPointer() 函数根据设备名称得到其 
	// 设备对象，所以这里我们只能自己枚举 USB 设备栈，并得到 
	// USB 键盘设备来进行挂接 
	// 
	ntStatus = GetUsbKeybordDevice( &UsbKeyBoardDeviceObject, &UsbMouseDeviceObject); 
	if ( NT_SUCCESS( ntStatus )) 
	{ 
		if(UsbKeyBoardDeviceObject != NULL)
		{
			// 
			// 调试使用，USB 键盘设备 kbdhid 没有设备名只有驱动名 
			// 所以这里打印为空 
			// 
			RtlInitUnicodeString( &KeyDeviceName, szDeviceName ); // USB KEYBOARD 
			DbgPrint( "KeyDeviceName:%S\n", KeyDeviceName.Buffer ); 
			DbgPrint( "Find UsbKeybordDriverName:%S\n", UsbKeyBoardDeviceObject->DriverObject->DriverName.Buffer); 
			
			//
			// 
			// 挂接 USB 键盘设备 
			// 
			//ntStatus = AttachUSBKeyboardDevice( UsbKeyBoardDeviceObject, KeyDriverObject ); 
			if ( !NT_SUCCESS( ntStatus ) ) 
			{ 
				DbgPrint( "Attach USB Keyboard Device to failed!\n" ); 
				return STATUS_INSUFFICIENT_RESOURCES; 
			} 
		}
		else if(UsbMouseDeviceObject != NULL)
		{
			DbgPrint( "Find UsbMouse DriverName:%S\n", UsbMouseDeviceObject->DriverObject->DriverName.Buffer); 
			HookDeviceObject = UsbMouseDeviceObject->AttachedDevice;
			g_OldInternIoCtl = (INTERNIOCTL)(HookDeviceObject->DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL]);
			DbgPrint( "Find UsbMouse Hook DriverName:%S, Driver Function:%X\n", HookDeviceObject->DriverObject->DriverName.Buffer, g_OldInternIoCtl); 
			HookDeviceObject->DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = MouFilter_InternIoCtl;
			// 
			// 挂接 USB 鼠标设备
			// 
			//ntStatus = AttachUSBKeyboardDevice( UsbMouseDeviceObject, KeyDriverObject ); 
			if ( !NT_SUCCESS( ntStatus ) ) 
			{ 
				DbgPrint( "Attach USB Keyboard Device to failed!\n" ); 
				return STATUS_INSUFFICIENT_RESOURCES; 
			} 
		}

	} 
	return STATUS_SUCCESS; 
	if (UsbKeyBoardDeviceObject == NULL || UsbMouseDeviceObject == NULL)
	{ 
		// 
		// 如果没有 USB 键盘，则尝试挂接 PS/2 键盘设备 
		// 
		RtlInitUnicodeString( &KeyDeviceName, PS2KEYBOARDNAME ); 
		ntStatus = AttachPS2KeyboardDevice( &KeyDeviceName, 
			KeyDriverObject, 
			&KeyDriver ); 
		if ( !NT_SUCCESS( ntStatus ) || KeyDriver == NULL ) 
		{ 
			DbgPrint( "Attach PS2 Keyboard Device to failed!\n" ); 
			return STATUS_INSUFFICIENT_RESOURCES; 
		} 
	} 
	// 
	// 这里没有过滤其他例程，仅处理了按键操作。这样处理会禁止 
	// 休眠。因在锁定时不允许休眠，所以也就无须处理其他例程 
	// 
	//KeyDriverObject->MajorFunction[IRP_MJ_READ] = KeyReadPassThrough; 
	return STATUS_SUCCESS; 
} 
///////////////////////////////////////////////////////////////// 
// 函数类型 : 系统函数 
// 函数模块 : 键盘过滤模块 
//////////////////////////////////////////////////////////////// 
// 功能 : 尝试取消队列里的异步 IRP，如果失败则等待用户按键， 
// 卸载键盘过滤驱动 
// 注意 : 取消 IRP 操作在 2000 系统上可以成功，在 XP / 2003 上 
// 则需要等待用户按键，以后有待完善 
///////////////////////////////////////////////////////////////// 
// 作者 : sinister 
// 发布版本 : 1.00.00 
// 发布日期 : 2005.12.27 
///////////////////////////////////////////////////////////////// 
// 重 大 修 改 历 史 
//////////////////////////////////////////////////////////////// 
// 修改者 : 
// 修改日期 : 
// 修改内容 : 
///////////////////////////////////////////////////////////////// 
VOID KeyDriverUnload( PDRIVER_OBJECT KeyDriver ) 
{ 
	PDEVICE_OBJECT KeyFilterDevice ; 
	PDEVICE_OBJECT KeyDevice ; 
	PDEVICE_EXTENSION KeyExtension; 
	PIRP Irp; 
	NTSTATUS ntStatus; 
	KeyFilterDevice = KeyDriver->DeviceObject; 
	KeyExtension = ( PDEVICE_EXTENSION ) KeyFilterDevice->DeviceExtension; 
	KeyDevice = KeyExtension->TargetDevice; 
	IoDetachDevice( KeyDevice ); 
	// 
	// 如果还有 IRP 未完成，且当前 IRP 有效则尝试取消这个 IRP 
	// 
	if ( KeyExtension->IrpsInProgress >0 &&KeyDevice->CurrentIrp != NULL ) 
	{ 
		if ( CancelKeyboardIrp( KeyDevice->CurrentIrp ) ) 
		{ 
			// 
			// 成功则直接退出删除键盘过滤设备 
			// 
			DbgPrint( "CancelKeyboardIrp() is ok\n" ); 
			goto __End; 
		} 
	} 
	// 
	// 如果取消失败，则一直等待按键 
	// 
	while ( KeyExtension->IrpsInProgress >0 ) 
	{ 
		DbgPrint( "Irp Count:%d\n", KeyExtension->IrpsInProgress ); 
	} 
__End: 
	IoDeleteDevice( KeyFilterDevice ); 
	return ; 
} 
///////////////////////////////////////////////////////////////// 
// 函数类型 : 自定义工具函数 
// 函数模块 : 键盘过滤模块 
///////////////////////////////////////////////////////////////// 
// 功能 : 取消 IRP 操作 
// 注意 : 这个函数仅是为配合在 UNLOAD 例程使用，其他例程中不能 
// 使用此方法来取消 IRP 
///////////////////////////////////////////////////////////////// 
// 作者 : sinister 
// 发布版本 : 1.00.00 
// 发布日期 : 2007.02.20 
///////////////////////////////////////////////////////////////// 
// 重 大 修 改 历 史 
///////////////////////////////////////////////////////////////// 
// 修改者 : 
// 修改日期 : 
// 修改内容 : 
///////////////////////////////////////////////////////////////// 
BOOLEAN CancelKeyboardIrp( IN PIRP Irp ) 
{ 
	if ( Irp == NULL ) 
	{ 
		DbgPrint( "CancelKeyboardIrp: Irp error\n" ); 
		return FALSE; 
	} 
	// 
	// 这里有些判断应该不是必须的，比如对 CancelRoutine 字段， 
	// 因为 IoCancelIrp() 函数中有判断了。但只有偏执狂才能生存 :)。 
	// 小波说 低智、偏执、思想贫乏是最不可容忍的。我这一行代码就占 
	// 了两条 :D，不知 xiaonvwu 看过后会作何感想？:DDD 
	// 
	// 
	// 如果正在取消或没有取消例程则直接返回 FALSE 
	// 
	if ( Irp->Cancel || Irp->CancelRoutine == NULL ) 
	{ 
		DbgPrint( "Can't Cancel the irp\n" ); 
		return FALSE; 
	} 
	if ( FALSE == IoCancelIrp( Irp ) ) 
	{ 
		DbgPrint( "IoCancelIrp() to failed\n" ); 
		return FALSE; 
	} 
	// 
	// 取消后重设此例程为空 
	// 
	IoSetCancelRoutine( Irp, NULL ); 
	return TRUE; 
} 
///////////////////////////////////////////////////////////////// 
// 函数类型 : 自定义工具函数 
// 函数模块 : 设备栈信息模块 
///////////////////////////////////////////////////////////////// 
// 功能 : 遍历 DEVICE_OBJECT 中 AttachedDevice 域，找到 USB 键盘 
// 设备上名为 kbdhid 的过滤驱动（Upper Filter Driver） 
// 注意 : 
///////////////////////////////////////////////////////////////// 
// 作者 : sinister 
// 发布版本 : 1.00.00 
// 发布日期 : 2005.06.02 
///////////////////////////////////////////////////////////////// 
// 重 大 修 改 历 史 
///////////////////////////////////////////////////////////////// 
// 修改者 : sinister 
// 修改日期 : 2007.2.12 
// 修改内容 : 为匹配 USB 键盘驱动做了相应的修改 
///////////////////////////////////////////////////////////////// 
ULONG GetAttachedDeviceInfo( IN PDEVICE_OBJECT DevObj ) 
{ 
	PDEVICE_OBJECT DeviceObject; 
	ULONG uFound = FALSE; 
	if ( DevObj == NULL ) 
	{ 
		DbgPrint( "DevObj is NULL!\n" ); 
		return FALSE; 
	} 
	DeviceObject = DevObj->AttachedDevice; 
	while ( DeviceObject ) 
	{ 
		// 
		// 一些 OBJECT 的名称都存在分页区，虽然大部分时候不会被交换出去，但 
		// 有一次足够了。这算是经验之谈 
		// 
		if ( MmIsAddressValid( DeviceObject->DriverObject->DriverName.Buffer ) ) 
		{ 
			DbgPrint( "Attached Driver Name:%S,Attached Driver Address:0x%x,Attached DeviceAddress:0x%x\n", 
				DeviceObject->DriverObject->DriverName.Buffer, 
				DeviceObject->DriverObject, 
				DeviceObject ); 
			// 
			// 找到 USB 键盘驱动的 kbdhid 设备了么？找到了就不继续了 
			// 
			if ( _wcsnicmp( DeviceObject->DriverObject->DriverName.Buffer, 
				KDBDEVICENAME, 
				wcslen( KDBDEVICENAME ) ) == 0 ) 
			{ 
				DbgPrint( "Found kbdhid Device\n" ); 
				uFound = 1;
				break; 
			} 
			// 
			// 找到 USB鼠标驱动的 mouhid 设备了么？找到了就不继续了 
			// 
			if ( _wcsnicmp( DeviceObject->DriverObject->DriverName.Buffer, 
				MOUDEVICENAME, 
				wcslen( MOUDEVICENAME ) ) == 0 ) 
			{ 
				DbgPrint( "Found mouhid Device\n" ); 
				uFound = 2;
				break; 
			} 
		} 
		DeviceObject = DeviceObject->AttachedDevice; 
	} 
	return uFound; 
} 
///////////////////////////////////////////////////////////////// 
// 函数类型 : 自定义工具函数 
// 函数模块 : 设备栈信息模块 
///////////////////////////////////////////////////////////////// 
// 功能 : 从 DEVICE_OBJECT 中得到设备与驱动名称并打印地址 
// 注意 : 函数功能只是打印信息，不同环境使用中应该会做修改 
///////////////////////////////////////////////////////////////// 
// 作者 : sinister 
// 发布版本 : 1.00.00 
// 发布日期 : 2006.05.02 
///////////////////////////////////////////////////////////////// 
// 重 大 修 改 历 史 
///////////////////////////////////////////////////////////////// 
// 修改者 : sinister 
// 修改日期 : 2007.2.12 
// 修改内容 : 打印出 USB 键盘驱动的设备名称，仅作调试使用 
///////////////////////////////////////////////////////////////// 
VOID GetDeviceObjectInfo( IN PDEVICE_OBJECT DevObj ) 
{ 
	POBJECT_HEADER ObjectHeader; 
	POBJECT_HEADER_NAME_INFO ObjectNameInfo; 
	if ( DevObj == NULL ) 
	{ 
		DbgPrint( "DevObj is NULL!\n" ); 
		return; 
	} 
	// 
	// 得到对象头 
	// 
	ObjectHeader = OBJECT_TO_OBJECT_HEADER( DevObj ); 
	if ( ObjectHeader ) 
	{ 
		// 
		// 查询设备名称并打印 
		// 
		ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader ); 
		if ( ObjectNameInfo &&ObjectNameInfo->Name.Buffer ) 
		{ 
			DbgPrint( "Device Name:%S - Device Address:0x%x\n", 
				ObjectNameInfo->Name.Buffer, 
				DevObj ); 
			// 
			// 复制 USB 键盘设备名到一个全局 BUFFER 里，为调试时显示 
			// 用，没有实际的功能用途 
			// 
			RtlZeroMemory( szUsbDeviceName, sizeof( szUsbDeviceName ) ); 
			wcsncpy( szUsbDeviceName, 
				ObjectNameInfo->Name.Buffer, 
				ObjectNameInfo->Name.Length / sizeof( WCHAR ) ); 
		} 
		// 
		// 对于没有名称的设备，则打印 NULL 
		// 
		else if ( DevObj->DriverObject ) 
		{ 
			DbgPrint( "Driver Name:%S - Device Name:%S - Driver Address:0x%x - Device Address:0x%x\n", 
				DevObj->DriverObject->DriverName.Buffer, 
				L"NULL", 
				DevObj->DriverObject, 
				DevObj ); 
		} 
	} 
} 
///////////////////////////////////////////////////////////////// 
// 函数类型 : 自定义工具函数 
// 函数模块 : 键盘过滤模块 
///////////////////////////////////////////////////////////////// 
// 功能 : 得到 USB 驱动 hidusb 的驱动对象，并遍历以上所有设备 
// 对象，过滤出 USB 键盘设备，将其设备对象返回 
// 注意 : 
///////////////////////////////////////////////////////////////// 
// 作者 : sinister 
// 发布版本 : 1.00.00 
// 发布日期 : 2007.02.13 
///////////////////////////////////////////////////////////////// 
// 重 大 修 改 历 史 
///////////////////////////////////////////////////////////////// 
// 修改者 : 
// 修改日期 : 
// 修改内容 : 
///////////////////////////////////////////////////////////////// 
NTSTATUS GetUsbKeybordDevice( OUT PDEVICE_OBJECT* UsbKeyBoardDeviceObject, OUT PDEVICE_OBJECT* UsbMouseDeviceObject) 
{ 
	UNICODE_STRING DriverName; 
	PDRIVER_OBJECT DriverObject = NULL; 
	PDEVICE_OBJECT DeviceObject = NULL; 
	BOOLEAN bFound = FALSE; 
	ULONG	uGetInfo = 0;
	RtlInitUnicodeString( &DriverName, USBKEYBOARDNAME ); 
	ObReferenceObjectByName( &DriverName, 
		OBJ_CASE_INSENSITIVE, 
		NULL, 
		0, 
		( POBJECT_TYPE ) IoDriverObjectType, 
		KernelMode, 
		NULL, 
		&DriverObject ); 
	if ( DriverObject == NULL ) 
	{ 
		DbgPrint( "Not found USB Keyboard Device hidusb!\n" ); 
		return STATUS_UNSUCCESSFUL; 
	} 
	*UsbKeyBoardDeviceObject = NULL;
	*UsbMouseDeviceObject = NULL;
	DeviceObject = DriverObject->DeviceObject; 
	while ( DeviceObject ) 
	{ 
		GetDeviceObjectInfo( DeviceObject ); 
		if ( DeviceObject->AttachedDevice ) 
		{ 
			// 
			// 查找 USB 键盘设备 
			// 
			uGetInfo = GetAttachedDeviceInfo( DeviceObject );
			if (uGetInfo == 1 && !g_bUsbKeyBoard)	//找到USB键盘设备
			{ 
				// 
				// 找到则返回 USB 键盘设备对象 
				// 
				g_bUsbKeyBoard = TRUE;
				*UsbKeyBoardDeviceObject = DeviceObject;
				if (bFound)
					goto __End;
				else
					bFound = TRUE; 				 
			}
			else if(uGetInfo == 2 && !g_bUsbMouse) //找到USB鼠标设备
			{
				g_bUsbMouse = TRUE;
				*UsbMouseDeviceObject = DeviceObject;
				if (bFound)
					goto __End;
				else
					bFound = TRUE;
			}
		} 
		DeviceObject = DeviceObject->NextDevice; 
	} 
__End: 
	return STATUS_SUCCESS; 
} 
///////////////////////////////////////////////////////////////// 
// 函数类型 : 自定义工具函数 
// 函数模块 : 键盘过滤模块 
//////////////////////////////////////////////////////////////// 
// 功能 : 创建过滤设备将其附加到需要跟踪的设备上，保存设备相关 
// 信息，返回附加后的驱动对象 
// 注意 : 此函数仅挂接 USB 键盘设备 
///////////////////////////////////////////////////////////////// 
// 作者 : sinister 
// 发布版本 : 1.00.00 
// 发布日期 : 2005.12.27 
///////////////////////////////////////////////////////////////// 
// 重 大 修 改 历 史 
//////////////////////////////////////////////////////////////// 
// 修改者 : 
// 修改日期 : 
// 修改内容 : 
///////////////////////////////////////////////////////////////// 
NTSTATUS AttachUSBKeyboardDevice( IN PDEVICE_OBJECT UsbDeviceObject, IN PDRIVER_OBJECT DriverObject ) 
{ 
	PDEVICE_OBJECT DeviceObject; 
	PDEVICE_OBJECT TargetDevice; 
	PDEVICE_EXTENSION DevExt; 
	NTSTATUS ntStatus; 
	// 
	// 创建过滤设备对象 
	// 
	ntStatus = IoCreateDevice( DriverObject, 
		sizeof( DEVICE_EXTENSION ), 
		NULL, 
		FILE_DEVICE_UNKNOWN, 
		0, 
		FALSE, 
		&DeviceObject ); 
	if ( !NT_SUCCESS( ntStatus ) ) 
	{ 
		DbgPrint( "IoCreateDevice() 0x%x!\n", ntStatus ); 
		return ntStatus; 
	} 
	DevExt = ( PDEVICE_EXTENSION ) DeviceObject->DeviceExtension; 
	// 
	// 初始化自旋锁 
	// 
	KeInitializeSpinLock( &DevExt->SpinLock ); 
	// 
	// 初始化 IRP 计数器 
	// 
	DevExt->IrpsInProgress = 0; 
	// 
	// 将过滤设备对象附加在目标设备对象之上，并返回附加后的原设备对象 
	// 
	TargetDevice = IoAttachDeviceToDeviceStack( DeviceObject, UsbDeviceObject ); 
	if ( !TargetDevice ) 
	{ 
		IoDeleteDevice( DeviceObject ); 
		DbgPrint( "IoAttachDeviceToDeviceStack() 0x%x!\n", ntStatus ); 
		return STATUS_INSUFFICIENT_RESOURCES; 
	} 
	// 
	// 保存过滤设备信息 
	// 
	DevExt->DeviceObject = DeviceObject; 
	DevExt->TargetDevice = TargetDevice; 
	// 
	// 设置过滤设备相关信息与标志 
	// 
	DeviceObject->Flags |= ( DO_BUFFERED_IO | DO_POWER_PAGABLE ); 
	DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING; 
	return STATUS_SUCCESS; 
} 
///////////////////////////////////////////////////////////////// 
// 函数类型 : 自定义工具函数 
// 函数模块 : 键盘过滤模块 
//////////////////////////////////////////////////////////////// 
// 功能 : 创建过滤设备将其附加到需要跟踪的设备上，保存设备相关 
// 信息，返回附加后的驱动对象 
// 注意 : 此函数仅挂接 PS/2 键盘设备 
///////////////////////////////////////////////////////////////// 
// 作者 : sinister 
// 发布版本 : 1.00.00 
// 发布日期 : 2005.12.27 
///////////////////////////////////////////////////////////////// 
// 重 大 修 改 历 史 
//////////////////////////////////////////////////////////////// 
// 修改者 : 
// 修改日期 : 
// 修改内容 : 
///////////////////////////////////////////////////////////////// 
NTSTATUS AttachPS2KeyboardDevice( IN UNICODE_STRING* DeviceName, // 需要跟踪的设备名 
	IN PDRIVER_OBJECT DriverObject, // 过滤驱动也就是本驱动的驱动对象 
	OUT PDRIVER_OBJECT* FilterDriverObject ) // 返回附加后的驱动对象 
{ 
	PDEVICE_OBJECT DeviceObject; 
	PDEVICE_OBJECT FilterDeviceObject; 
	PDEVICE_OBJECT TargetDevice; 
	PFILE_OBJECT FileObject; 
	PDEVICE_EXTENSION DevExt; 
	NTSTATUS ntStatus; 
	// 
	// 根据设备名称找到需要附加的设备对象 
	// 
	ntStatus = IoGetDeviceObjectPointer( DeviceName, 
		FILE_ALL_ACCESS, 
		&FileObject, 
		&DeviceObject ); 
	if ( !NT_SUCCESS( ntStatus ) ) 
	{ 
		DbgPrint( "IoGetDeviceObjectPointer() 0x%x\n", ntStatus ); 
		return ntStatus; 
	} 
	DbgPrint( "Attached PS2 Keyboard Driver Name:%S,Driver Address:0x%x,DeviceAddress:0x%x\n", 
		DeviceObject->DriverObject->DriverName.Buffer, 
		DeviceObject->DriverObject,
		DeviceObject );
	// 
	// 创建过滤设备对象 
	// 
	ntStatus = IoCreateDevice( DriverObject, 
		sizeof( DEVICE_EXTENSION ), 
		NULL, 
		FILE_DEVICE_KEYBOARD, 
		0, 
		FALSE, 
		&FilterDeviceObject ); 
	if ( !NT_SUCCESS( ntStatus ) ) 
	{ 
		ObDereferenceObject( FileObject ); 
		DbgPrint( "IoCreateDevice() 0x%x!\n", ntStatus ); 
		return ntStatus; 
	} 
	// 
	// 得到设备扩展结构，以便下面保存过滤设备信息 
	// 
	DevExt = ( PDEVICE_EXTENSION ) FilterDeviceObject->DeviceExtension; 
	// 
	// 初始化自旋锁 
	// 
	KeInitializeSpinLock( &DevExt->SpinLock ); 
	// 
	// 初始化 IRP 计数器 
	// 
	DevExt->IrpsInProgress = 0; 
	// 
	// 将过滤设备对象附加在目标设备对象之上，并返回附加后的原设备对象 
	// 
	TargetDevice = IoAttachDeviceToDeviceStack( FilterDeviceObject, 
		DeviceObject ); 
	if ( !TargetDevice ) 
	{ 
		ObDereferenceObject( FileObject ); 
		IoDeleteDevice( FilterDeviceObject ); 
		DbgPrint( "IoAttachDeviceToDeviceStack() 0x%x!\n", ntStatus ); 
		return STATUS_INSUFFICIENT_RESOURCES; 
	} 
	// 
	// 保存过滤设备信息 
	// 
	DevExt->DeviceObject = FilterDeviceObject; 
	DevExt->TargetDevice = TargetDevice; 
	DevExt->pFilterFileObject = FileObject; 
	// 
	// 设置过滤设备相关信息与标志 
	// 
	FilterDeviceObject->DeviceType = TargetDevice->DeviceType; 
	FilterDeviceObject->Characteristics = TargetDevice->Characteristics; 
	FilterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING; 
	FilterDeviceObject->Flags |= ( TargetDevice->Flags &( DO_DIRECT_IO | 
		DO_BUFFERED_IO ) ); 
	// 
	// 返回附加后的驱动对象 
	// 
	*FilterDriverObject = TargetDevice->DriverObject; 
	ObDereferenceObject( FileObject ); 
	return STATUS_SUCCESS; 
} 
///////////////////////////////////////////////////////////////// 
// 函数类型 : 自定义工具函数 
// 函数模块 : 键盘过滤模块 
//////////////////////////////////////////////////////////////// 
// 功能 : 键盘过滤驱动的 IRP_MJ_READ 派遣例程，所有按键将触发 
// 这个 IRP 的完成 
// 注意 : 
///////////////////////////////////////////////////////////////// 
// 作者 : sinister 
// 发布版本 : 1.00.00 
// 发布日期 : 2007.2.15 
///////////////////////////////////////////////////////////////// 
// 重 大 修 改 历 史 
//////////////////////////////////////////////////////////////// 
// 修改者 : 
// 修改日期 : 
// 修改内容 : 
///////////////////////////////////////////////////////////////// 
NTSTATUS KeyReadPassThrough( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp ) 
{ 
	NTSTATUS status; 
	KIRQL IrqLevel; 
	PDEVICE_OBJECT pDeviceObject; 
	PDEVICE_EXTENSION KeyExtension = ( PDEVICE_EXTENSION ) 
		DeviceObject->DeviceExtension; 
	IoCopyCurrentIrpStackLocationToNext( Irp ); 
	// 
	// 将 IRP 计数器加一，为支持 SMP 使用自旋锁 
	// 
	KeAcquireSpinLock( &KeyExtension->SpinLock, &IrqLevel ); 
	InterlockedIncrement( &KeyExtension->IrpsInProgress ); 
	KeReleaseSpinLock( &KeyExtension->SpinLock, IrqLevel ); 
	IoSetCompletionRoutine( Irp, 
		KeyReadCompletion, 
		DeviceObject, 
		TRUE, 
		TRUE, 
		TRUE ); 
	return IoCallDriver( KeyExtension->TargetDevice, Irp ); 
} 
///////////////////////////////////////////////////////////////// 
// 函数类型 ：系统回调函数 
// 函数模块 : 键盘过滤模块 
//////////////////////////////////////////////////////////////// 
// 功能 ： 获得键盘按键，用无效扫描码替换，以达到屏蔽键盘的目的 
// 注意 ： 
///////////////////////////////////////////////////////////////// 
// 作者 : sinister 
// 发布版本 : 1.00.00 
// 发布日期 : 2007.2.12 
///////////////////////////////////////////////////////////////// 
// 重 大 修 改 历 史 
//////////////////////////////////////////////////////////////// 
// 修改者 : 
// 修改日期 : 
// 修改内容 : 
///////////////////////////////////////////////////////////////// 
NTSTATUS KeyReadCompletion( IN PDEVICE_OBJECT DeviceObject, 
	IN PIRP Irp, 
	IN PVOID Context ) 
{ 
	PIO_STACK_LOCATION IrpSp; 
	PKEYBOARD_INPUT_DATA KeyData; 
	PDEVICE_EXTENSION KeyExtension = ( PDEVICE_EXTENSION ) 
		DeviceObject->DeviceExtension; 
	int numKeys, i; 
	KIRQL IrqLevel; 
	IrpSp = IoGetCurrentIrpStackLocation( Irp ); 
	if ( Irp->IoStatus.Status != STATUS_SUCCESS ) 
	{ 
		DbgPrint( "ntStatus:0x%x", Irp->IoStatus.Status ); 
		goto __RoutineEnd; 
	} 
	// 
	// 系统在 SystemBuffer 中保存按键信息 
	// 
	KeyData = Irp->AssociatedIrp.SystemBuffer; 
	if ( KeyData == NULL ) 
	{ 
		DbgPrint( "KeyData is NULL\n" ); 
		goto __RoutineEnd; 
	} 
	// 
	// 得到按键数 
	// 
	numKeys = Irp->IoStatus.Information / sizeof( KEYBOARD_INPUT_DATA ); 
	if ( numKeys <0 ) 
	{ 
		DbgPrint( "numKeys less zero\n" ); 
		goto __RoutineEnd; 
	} 
	// 
	// 使用 0 无效扫描码替换，屏蔽所有按键 
	// 
	for ( i = 0; i < numKeys; i++)
	{ 
		DbgPrint( "KeyDwon: 0x%x\n", KeyData[i].MakeCode ); 
			KeyData[i].MakeCode = 0x00; 
	} 
__RoutineEnd : 
	if ( Irp->PendingReturned ) 
	{ 
		IoMarkIrpPending( Irp ); 
	} 
	// 
	// 将 IRP 计数器减一，为支持 SMP 使用自旋锁 
	// 
	KeAcquireSpinLock( &KeyExtension->SpinLock, &IrqLevel ); 
	InterlockedDecrement( &KeyExtension->IrpsInProgress ); 
	KeReleaseSpinLock( &KeyExtension->SpinLock, IrqLevel ); 
	return Irp->IoStatus.Status ; 
}