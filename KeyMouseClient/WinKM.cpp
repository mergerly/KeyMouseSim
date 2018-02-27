#include "WinKM.h"
#include "DriverManage.h"

BOOL ImproveProcPriv(TCHAR* szPrivilegeName)
{
	HANDLE token;
	//提升权限
	if(!OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES,&token))
	{
		//MessageBox(NULL,"打开进程令牌失败...","错误",MB_ICONSTOP);
		return FALSE;
	}
	TOKEN_PRIVILEGES tkp;
	tkp.PrivilegeCount = 1;
	//::LookupPrivilegeValue(NULL,SE_DEBUG_NAME,&tkp.Privileges[0].Luid); // 获得 SE_DEBUG_NAME 特权
	if(szPrivilegeName)
		::LookupPrivilegeValue(NULL,szPrivilegeName,&tkp.Privileges[0].Luid);
	else
		::LookupPrivilegeValue(NULL,SE_LOAD_DRIVER_NAME,&tkp.Privileges[0].Luid);
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if(!AdjustTokenPrivileges(token,FALSE,&tkp,sizeof(tkp),NULL,NULL))
	{
		//MessageBox(NULL,"调整令牌权限失败...","错误",MB_ICONSTOP);
		return FALSE;
	}
	CloseHandle(token);
	return TRUE;
}

// 初始化
BOOL InitializeWinKM(void)
{
	ImproveProcPriv();			
	return g_DriverManage.StartDriver();
}

// 卸载
BOOL ShutdownWinKM(void)
{
	return g_DriverManage.StopDriver();
}
//发送鼠标消息
BOOL SendMouseOperate(ULONG ButtonFlags, ULONG dwX, ULONG dwY)
{
	ULONG dwMouseData[3];
	dwMouseData[0] = ButtonFlags;
	dwMouseData[1] = dwX;
	dwMouseData[2] = dwY;
	return g_DriverManage.SendDeviceControl(IOCTL_SEND_MOUSE, &dwMouseData, sizeof(ULONG)*3, 0, NULL);
}
//发送键盘消息
BOOL SendKeyOperate(ULONG uFlags, ULONG vKeyCode)
{
	ULONG uMakeCode = MapVirtualKey(vKeyCode, 0);	//获取扫描码
	switch(vKeyCode)
	{
	case VK_INSERT:	
	case VK_DELETE:
	case VK_HOME:
	case VK_END:
	case VK_PRIOR:	//Page Up
	case VK_NEXT:	//Page Down

	case VK_LEFT:
	case VK_UP:
	case VK_RIGHT:
	case VK_DOWN:

	case VK_DIVIDE:

	case VK_LWIN:
	case VK_RCONTROL:
	case VK_RWIN:
	case VK_RMENU:	//ALT
		uFlags |= KEY_E0;
		break;
		//case VK_PAUSE:
		//	uFlags |= KEY_E1;
	}

	ULONG dwKeyData[2];
	dwKeyData[0] = uFlags;
	dwKeyData[1] = uMakeCode;
	return g_DriverManage.SendDeviceControl(IOCTL_SEND_KEY, &dwKeyData, sizeof(ULONG)*2, 0, NULL);
}

//按键按下
BOOL WinKMKeyDown(ULONG vKeyCoad)
{
	return SendKeyOperate(KEY_MAKE, vKeyCoad);
	
}
//按键松开
BOOL WinKMKeyUp(ULONG vKeyCoad)
{
	return SendKeyOperate(KEY_BREAK, vKeyCoad);
}
//鼠标左键按下
BOOL WinKMLButtonDown()
{
	return SendMouseOperate(MOUSE_LEFT_BUTTON_DOWN, 0, 0);
}
//鼠标左键松开
BOOL WinKMLButtonLUp()
{
	return SendMouseOperate(MOUSE_LEFT_BUTTON_UP, 0, 0);
}
//鼠标右键按下
BOOL WinKMRButtonDown()
{
	return SendMouseOperate(MOUSE_RIGHT_BUTTON_DOWN, 0, 0);
}
//鼠标右键松开
BOOL WinKMRButtonLUp()
{
	return SendMouseOperate(MOUSE_RIGHT_BUTTON_UP, 0, 0);
}
//鼠标中键按下
BOOL WinKMMButtonDown()
{
	return SendMouseOperate(MOUSE_MIDDLE_BUTTON_DOWN, 0, 0);
}
//鼠标中键松开
BOOL WinKMMButtonLUp()
{
	return SendMouseOperate(MOUSE_MIDDLE_BUTTON_UP, 0, 0);
}