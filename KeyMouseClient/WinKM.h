#pragma once
#include <Windows.h>

#define MOUSE_LEFT_BUTTON_DOWN   0x0001  // Left Button changed to down.
#define MOUSE_LEFT_BUTTON_UP     0x0002  // Left Button changed to up.
#define MOUSE_RIGHT_BUTTON_DOWN  0x0004  // Right Button changed to down.
#define MOUSE_RIGHT_BUTTON_UP    0x0008  // Right Button changed to up.
#define MOUSE_MIDDLE_BUTTON_DOWN 0x0010  // Middle Button changed to down.
#define MOUSE_MIDDLE_BUTTON_UP   0x0020  // Middle Button changed to up.

#define KEY_MAKE  0	//按下
#define KEY_BREAK 1	//松开
#define KEY_E0    2	//扩展标识
#define KEY_E1    4

//提取权限
BOOL ImproveProcPriv(TCHAR* szPrivilegeName = NULL);
// 初始化
BOOL InitializeWinKM(void);
// 卸载
BOOL ShutdownWinKM(void);
//发送鼠标消息
BOOL SendMouseOperate(ULONG ButtonFlags, ULONG dwX, ULONG dwY);
//发送键盘消息
BOOL SendKeyOperate(ULONG uFlags, ULONG vKeyCode);

//按键按下
BOOL WinKMKeyDown(ULONG vKeyCoad);
//按键松开
BOOL WinKMKeyUp(ULONG vKeyCoad);

//鼠标左键按下
BOOL WinKMLButtonDown();
//鼠标左键松开
BOOL WinKMLButtonLUp();
//鼠标右键按下
BOOL WinKMRButtonDown();
//鼠标右键松开
BOOL WinKMRButtonLUp();
//鼠标中键按下
BOOL WinKMMButtonDown();
//鼠标中键松开
BOOL WinKMMButtonLUp();