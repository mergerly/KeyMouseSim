/***************************************************************** 
文件名 : WssLockKey.h 
描述 : 键盘过滤驱动 
作者 : sinister 
最后修改日期 : 2007-02-26 
*****************************************************************/ 
#ifndef __WSS_LOCKKEY_H_ 
#define __WSS_LOCKKEY_H_ 
#include "ntddk.h" 
#include "ntddkbd.h" 
#include "string.h" 

#define MAXLEN 256 
#define KDBDEVICENAME L"\\Driver\\kbdhid" 
#define USBKEYBOARDNAME L"\\Driver\\hidusb" 
#define PS2KEYBOARDNAME L"\\Device\\KeyboardClass0" 

#define MOUDEVICENAME L"\\Driver\\mouhid"
#define PS2MOUSENAME L"\\Device\\PointerClass0"

typedef struct _OBJECT_CREATE_INFORMATION 
{ 
	ULONG Attributes; 
	HANDLE RootDirectory; 
	PVOID ParseContext; 
	KPROCESSOR_MODE ProbeMode; 
	ULONG PagedPoolCharge; 
	ULONG NonPagedPoolCharge; 
	ULONG SecurityDescriptorCharge; 
	PSECURITY_DESCRIPTOR SecurityDescriptor; 
	PSECURITY_QUALITY_OF_SERVICE SecurityQos; 
	SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService; 
} OBJECT_CREATE_INFORMATION, * POBJECT_CREATE_INFORMATION; 

typedef struct _OBJECT_HEADER 
{ 
	LONG PointerCount; 
	union 
	{ 
		LONG HandleCount; 
		PSINGLE_LIST_ENTRY SEntry; 
	}; 
	POBJECT_TYPE Type; 
	UCHAR NameInfoOffset; 
	UCHAR HandleInfoOffset; 
	UCHAR QuotaInfoOffset; 
	UCHAR Flags; 
	union 
	{ 
		POBJECT_CREATE_INFORMATION ObjectCreateInfo; 
		PVOID QuotaBlockCharged; 
	}; 
	PSECURITY_DESCRIPTOR SecurityDescriptor; 
	QUAD Body; 
} OBJECT_HEADER, * POBJECT_HEADER;

#define NUMBER_HASH_BUCKETS 37 

typedef struct _OBJECT_DIRECTORY 
{ 
	struct _OBJECT_DIRECTORY_ENTRY* HashBuckets[NUMBER_HASH_BUCKETS]; 
	struct _OBJECT_DIRECTORY_ENTRY** LookupBucket; 
	BOOLEAN LookupFound; 
	USHORT SymbolicLinkUsageCount; 
	struct _DEVICE_MAP* DeviceMap; 
} OBJECT_DIRECTORY, * POBJECT_DIRECTORY; 

typedef struct _OBJECT_HEADER_NAME_INFO 
{ 
	POBJECT_DIRECTORY Directory; 
	UNICODE_STRING Name; 
	ULONG Reserved; 
	#if DBG 
	ULONG Reserved2 ; 
	LONG DbgDereferenceCount ; 
	#endif 
} OBJECT_HEADER_NAME_INFO, * POBJECT_HEADER_NAME_INFO; 

#define OBJECT_TO_OBJECT_HEADER( o ) \
CONTAINING_RECORD( (o), OBJECT_HEADER, Body )

#define OBJECT_HEADER_TO_NAME_INFO( oh ) ((POBJECT_HEADER_NAME_INFO) \
((oh)->NameInfoOffset == 0 ? NULL : ((PCHAR)(oh) - (oh)->NameInfoOffset)))

typedef struct _DEVICE_EXTENSION 
{ 
	PDEVICE_OBJECT DeviceObject; 
	PDEVICE_OBJECT TargetDevice; 
	PFILE_OBJECT pFilterFileObject; 
	ULONG DeviceExtensionFlags; 
	LONG IrpsInProgress; 
	KSPIN_LOCK SpinLock; 
}DEVICE_EXTENSION, * PDEVICE_EXTENSION; 

NTSTATUS KeyDriverEntry( IN PDRIVER_OBJECT KeyDriverObject, IN PUNICODE_STRING RegistryPath );
VOID KeyDriverUnload( PDRIVER_OBJECT KeyDriver );
BOOLEAN CancelKeyboardIrp( IN PIRP Irp );
extern POBJECT_TYPE* IoDriverObjectType;

NTSYSAPI 
NTSTATUS 
NTAPI ObReferenceObjectByName( IN PUNICODE_STRING ObjectName, 
IN ULONG Attributes, 
IN PACCESS_STATE AccessState OPTIONAL, 
IN ACCESS_MASK DesiredAccess OPTIONAL, 
IN POBJECT_TYPE ObjectType, 
IN KPROCESSOR_MODE AccessMode, 
IN OUT PVOID ParseContext OPTIONAL, 
OUT PVOID* Object ); 

NTSTATUS 
GetUsbKeybordDevice( OUT PDEVICE_OBJECT* UsbKeyBoardDeviceObject, OUT PDEVICE_OBJECT* UsbMouseDeviceObject ); 

ULONG 
GetAttachedDeviceInfo( IN PDEVICE_OBJECT DevObj ); 

VOID 
GetDeviceObjectInfo( IN PDEVICE_OBJECT DevObj ); 

NTSTATUS 
AttachUSBKeyboardDevice( IN PDEVICE_OBJECT UsbDeviceObject, 
IN PDRIVER_OBJECT DriverObject ); 

NTSTATUS 
AttachPS2KeyboardDevice( IN UNICODE_STRING* DeviceName, 
IN PDRIVER_OBJECT DriverObject, 
OUT PDRIVER_OBJECT* FilterDriverObject ); 

NTSTATUS 
KeyReadCompletion( IN PDEVICE_OBJECT DeviceObject, 
IN PIRP Irp, 
IN PVOID Context ); 

NTSTATUS 
KeyReadPassThrough( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp ); 

WCHAR szUsbDeviceName[MAXLEN]; 

//
// Define the port connection data structure.
//

typedef struct _CONNECT_DATA {
	IN PDEVICE_OBJECT ClassDeviceObject;
	IN PVOID ClassService;
} CONNECT_DATA, *PCONNECT_DATA;

//
// Define the service callback routine's structure.
//

typedef
	VOID
	(*PSERVICE_CALLBACK_ROUTINE) (
	IN PVOID NormalContext,
	IN PVOID SystemArgument1,
	IN PVOID SystemArgument2,
	IN OUT PVOID SystemArgument3
	);
#endif