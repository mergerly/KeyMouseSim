#include "StdAfx.h"
#include "DriverManage.h"

#include <Windows.h>

CDriverManage g_DriverManage;

typedef struct _UNICODE_STRING {
	USHORT Length;
	USHORT MaximumLength;
	PWSTR  Buffer;
} UNICODE_STRING;
typedef UNICODE_STRING *PUNICODE_STRING;
typedef LONG NTSTATUS;
typedef NTSTATUS (NTAPI* ZW_LOAD_DRIVER)(
	IN PUNICODE_STRING  DriverServiceName
	);
typedef VOID (NTAPI* RTL_INIT_UNICODE_STRING)(
	IN OUT PUNICODE_STRING  DestinationString,
	IN PCWSTR  SourceString
	);
typedef NTSTATUS (NTAPI* ZW_UNLOAD_DRIVER)(
	IN PUNICODE_STRING  DriverServiceName
	);
/*
0 - 成功 
2 - 新建驱动键值失败
GetLaseError - 其他错误
*/
DWORD LoadDriver(char* lpszDriverFilePath, char* lpszServiceName, char* lpszDriverDosName,HANDLE* phDriver)
{
	*phDriver = INVALID_HANDLE_VALUE;
	char	szBuf[MAX_PATH+1];
	HKEY	hKey;
	NTSTATUS ntstatus;
	*phDriver = CreateFile(lpszDriverDosName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(*phDriver != INVALID_HANDLE_VALUE)	//驱动已经加载过，打开以前的驱动
	{
		return 0;
	}
	strcpy(szBuf,"SYSTEM\\CurrentControlSet\\Services\\");
	strcat(szBuf,lpszServiceName);
	if( ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,szBuf,0,KEY_ALL_ACCESS,&hKey))
	{
		DWORD dwDisposition;
		if(ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE, szBuf, 0, "", 0, KEY_READ | KEY_WRITE, NULL, &hKey, &dwDisposition))
			return 2;
	}

	strcpy(szBuf, lpszServiceName);
	RegSetValueEx(hKey, "DisplayName", 0, REG_EXPAND_SZ, (LPBYTE)szBuf, strlen(szBuf)+1);
	strcpy(szBuf, "\\??\\");
	strcat(szBuf, lpszDriverFilePath);
	RegSetValueEx(hKey, "ImagePath", 0, REG_EXPAND_SZ, (LPBYTE)szBuf, strlen(szBuf)+1);
	DWORD dwValue = 1;
	RegSetValueEx(hKey, "Type", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
	dwValue = 3;
	RegSetValueEx(hKey, "Start", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
	dwValue = 0;
	RegSetValueEx(hKey, "ErrorControl", 0, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
	RegCloseKey(hKey);

	HMODULE hModule = LoadLibrary("ntdll.dll");
	ZW_LOAD_DRIVER ZwLoadDriver = (ZW_LOAD_DRIVER)GetProcAddress(hModule,"ZwLoadDriver");
	ntstatus = 3;
	if(ZwLoadDriver != NULL)
	{
		RTL_INIT_UNICODE_STRING RtlInitUnicodeString = (RTL_INIT_UNICODE_STRING)GetProcAddress(hModule,"RtlInitUnicodeString");
		if(RtlInitUnicodeString != NULL)
		{
			WCHAR wsDriver[MAX_PATH+10];
			WCHAR wsTemp[MAX_PATH];
			wcscpy(wsDriver, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
			::MultiByteToWideChar(CP_ACP, 0, lpszServiceName, strlen(lpszServiceName) + 1, wsDriver + wcslen(wsDriver), MAX_PATH + 10 - wcslen(wsDriver));
			UNICODE_STRING uniCode;
			RtlInitUnicodeString(&uniCode, wsDriver);
			ntstatus = ZwLoadDriver(&uniCode);
		}
	}
	FreeLibrary(hModule);
	//无论成功与否，都打开驱动
	*phDriver = CreateFile(lpszDriverDosName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(*phDriver != INVALID_HANDLE_VALUE)	//驱动已经加载过，打开以前的驱动
	{
		return 0;
	}		
	return ntstatus;
}
/*
0 - 成功
2 - 获取函数失败
ntstatus - ZwUnloadDriver 返回值
*/
DWORD UnloadDriver(HANDLE hDriver,char* lpszServiceName)
{
	if(hDriver != NULL)
		CloseHandle(hDriver);
	HMODULE hMoudle = LoadLibrary("ntdll.dll");
	ZW_UNLOAD_DRIVER ZwUnloadDriver = (ZW_UNLOAD_DRIVER)GetProcAddress(hMoudle, "ZwUnloadDriver");
	NTSTATUS ntstatus = 2;
	if(ZwUnloadDriver != NULL)
	{
		RTL_INIT_UNICODE_STRING RtlInitUnicodeString = (RTL_INIT_UNICODE_STRING)GetProcAddress(hMoudle, "RtlInitUnicodeString");
		if(RtlInitUnicodeString != NULL)
		{
			WCHAR wsDriver[MAX_PATH+10];
			WCHAR wsTemp[MAX_PATH];
			wcscpy(wsDriver, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
			::MultiByteToWideChar(CP_ACP, 0, lpszServiceName, strlen(lpszServiceName) + 1, wsDriver + wcslen(wsDriver), MAX_PATH + 10 - wcslen(wsDriver));
			UNICODE_STRING uniCode;
			RtlInitUnicodeString(&uniCode, wsDriver);
			ntstatus = ZwUnloadDriver(&uniCode);
		}
	}
	FreeLibrary(hMoudle);
	return ntstatus;
}

CDriverManage::CDriverManage(void)
{
	hDriver = INVALID_HANDLE_VALUE;
}

CDriverManage::~CDriverManage(void)
{
}

BOOL CDriverManage::StartDriver()
{
	hDriver = CreateFile(KEYMOUSE_WIN32_DEVICE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(hDriver == INVALID_HANDLE_VALUE)	//没有加载过驱动
	{
		char szFilePath[MAX_PATH];
		GetModuleFileName(NULL, szFilePath, MAX_PATH);
		strrchr(szFilePath, '\\')[1] = 0;
		strcat(szFilePath, KEYMOUSE_DRIVER_NAME_A);
		if(GetFileAttributes(szFilePath) == -1)
		{
			MessageBox(NULL, szFilePath, "驱动文件不存在", MB_OK | MB_ICONERROR);
			return FALSE;
		}
		DWORD dwRet = LoadDriver(szFilePath, KEYMOUSE_SERVICE_NAME_A, KEYMOUSE_WIN32_DEVICE_NAME, &hDriver);
		if(dwRet != 0)
		{
			sprintf(szFilePath, "加载驱动失败,错误号:%x", dwRet);
			MessageBox(NULL, szFilePath, "错误", MB_OK | MB_ICONERROR);
			hDriver = INVALID_HANDLE_VALUE;
			return FALSE;			
		}
		return TRUE;
	}
	return TRUE;
}
BOOL CDriverManage::StopDriver()
{
	if(hDriver == INVALID_HANDLE_VALUE)
		return TRUE;
	DWORD dwRet = UnloadDriver(hDriver, KEYMOUSE_SERVICE_NAME_A);
	if(dwRet == 0)
		return TRUE;
	else
	{
		char szMsg[MAX_PATH];
		sprintf(szMsg, "卸载驱动出错:%x", dwRet);
		MessageBox(NULL, szMsg, "提示", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	
	return TRUE;
}

BOOL CDriverManage::SendMsg(ULONG uCmd)
{
	DWORD dwRet;
	char szMsg[MAX_PATH];
	if(hDriver == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, "驱动句柄错误", "错误", MB_OK|MB_ICONERROR);
		return FALSE;
	}
	if(DeviceIoControl(hDriver, uCmd, NULL, 0, NULL, 0, &dwRet, NULL))
		return TRUE;
	else
	{
		sprintf(szMsg, "发送驱动命令\"0x%x\"失败，错误码:%x", uCmd, GetLastError());
		MessageBox(NULL, szMsg, "错误", MB_OK|MB_ICONERROR);
		return FALSE;
	}

}

BOOL CDriverManage::SendDeviceControl(DWORD dwIoControlCode, LPVOID lpInBuffer, DWORD nInBufferSize, LPVOID lpOutBuffer, DWORD nOutBufferSize)
{
	DWORD dwRet;
	char szMsg[MAX_PATH];
	if(hDriver == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, "驱动句柄错误", "错误", MB_OK|MB_ICONERROR);
		return FALSE;
	}
	if(DeviceIoControl(hDriver, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, &dwRet, NULL))
		return TRUE;
	else
	{
		sprintf(szMsg, "发送驱动命令\"0x%x\"失败，错误码:%x", dwIoControlCode, GetLastError());
		MessageBox(NULL, szMsg, "错误", MB_OK|MB_ICONERROR);
		return FALSE;
	}
}