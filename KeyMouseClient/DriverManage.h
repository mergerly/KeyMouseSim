#pragma once

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
	((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
	)
#define METHOD_BUFFERED                 0
#define METHOD_IN_DIRECT                1
#define METHOD_OUT_DIRECT               2
#define METHOD_NEITHER                  3
#define FILE_ANY_ACCESS                 0

#define FILE_DEVICE_KEYMOUSE	0x8000
#define KEYMOUSE_IOCTL_BASE 0x800

//
// The device driver IOCTLs

//
#define CTL_CODE_KEYMOUSE(i)	CTL_CODE(FILE_DEVICE_KEYMOUSE, KEYMOUSE_IOCTL_BASE + i, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_KEYMOUSE_HELLO	CTL_CODE_KEYMOUSE(0)
#define IOCTL_KEYMOUSE_TEST		CTL_CODE_KEYMOUSE(1)

#define IOCTL_SEND_KEY					CTL_CODE_KEYMOUSE(20)
#define IOCTL_SEND_MOUSE				CTL_CODE_KEYMOUSE(21)
//
// Name that Win32 front end will use to open the KeyMouse device
//
#define KEYMOUSE_WIN32_DEVICE_NAME_A	"\\\\.\\KeyMouse"
#define KEYMOUSE_WIN32_DEVICE_NAME_W	L"\\\\.\\KeyMouse"
#define KEYMOUSE_DEVICE_NAME_A			"\\Device\\KeyMouse"
#define KEYMOUSE_DEVICE_NAME_W			L"\\Device\\KeyMouse"
#define KEYMOUSE_DOS_DEVICE_NAME_A		"\\DosDevices\\KeyMouse"
#define KEYMOUSE_DOS_DEVICE_NAME_W		L"\\DosDevices\\KeyMouse"

#ifdef _UNICODE
#define KEYMOUSE_WIN32_DEVICE_NAME	KEYMOUSE_WIN32_DEVICE_NAME_W
#define KEYMOUSE_DEVICE_NAME		KEYMOUSE_DEVICE_NAME_W
#define KEYMOUSE_DOS_DEVICE_NAME	KEYMOUSE_DOS_DEVICE_NAME_W
#else
#define KEYMOUSE_WIN32_DEVICE_NAME	KEYMOUSE_WIN32_DEVICE_NAME_A
#define KEYMOUSE_DEVICE_NAME		KEYMOUSE_DEVICE_NAME_A
#define KEYMOUSE_DOS_DEVICE_NAME	KEYMOUSE_DOS_DEVICE_NAME_A
#endif

#define KEYMOUSE_SERVICE_NAME_A		"KeyMouse"
#define KEYMOUSE_DRIVER_NAME_A		"KeyMouse.sys"

class CDriverManage
{
public:
	CDriverManage(void);
public:
	~CDriverManage(void);
public:
	HANDLE hDriver;
public:
	BOOL StartDriver();
	BOOL StopDriver();
	BOOL SendMsg(ULONG uCmd);
	BOOL SendDeviceControl(DWORD dwIoControlCode, LPVOID lpInBuffer = NULL, DWORD nInBufferSize = 0, LPVOID lpOutBuffer = NULL, DWORD nOutBufferSize = 0);
};
extern CDriverManage g_DriverManage;