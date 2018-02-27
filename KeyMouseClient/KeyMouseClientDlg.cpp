// KeyMouseClientDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "KeyMouseClient.h"
#include "KeyMouseClientDlg.h"
#include "WinKM.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
END_MESSAGE_MAP()


// CKeyMouseClientDlg 对话框




CKeyMouseClientDlg::CKeyMouseClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CKeyMouseClientDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CKeyMouseClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CKeyMouseClientDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP	
	ON_WM_CLOSE()	
	ON_BN_CLICKED(IDC_BUTTON_START, &CKeyMouseClientDlg::OnBnClickedButtonStart)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_PROCESS_OFF, &CKeyMouseClientDlg::OnBnClickedButtonProcessOff)
END_MESSAGE_MAP()

// CKeyMouseClientDlg 消息处理程序

BOOL CKeyMouseClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	
	if(InitializeWinKM())
		GetDlgItem(IDC_STATIC_STATUS)->SetWindowText("驱动加载成功");
	else
		GetDlgItem(IDC_STATIC_STATUS)->SetWindowText("驱动加载失败");
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CKeyMouseClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CKeyMouseClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
//
HCURSOR CKeyMouseClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CKeyMouseClientDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	ShutdownWinKM();
	CDialog::OnClose();
}




void CKeyMouseClientDlg::OnBnClickedButtonStart()
{
	// TODO: Add your control notification handler code here
	//KeyOpeate(VK_F1, FALSE);
	//KeyOpeate(VK_F1, TRUE);

// 	DWORD KeyCode = MapVirtualKey(VK_LWIN, 0);	//获取扫描码
// 	CString strTemp;
// 	strTemp.Format("%x", KeyCode);
// 	MessageBox(strTemp);
/*	
	SetCursorPos(100, 100);
	MouseOpeate(0, 0, 0);
	MouseOpeate(1, 0, 0);
	MouseOpeate(0, 0, 0);
	MouseOpeate(1, 0, 0);
	Sleep(1000);
	MouseOpeate(2, 0, 0);
	MouseOpeate(3, 0, 0);

	SetCursorPos(100, 100);
	SendMouseOperate(MOUSE_LEFT_BUTTON_DOWN, 0, 0);
	SendMouseOperate(MOUSE_LEFT_BUTTON_UP, 0, 0);
	SendMouseOperate(MOUSE_LEFT_BUTTON_DOWN, 0, 0);
	SendMouseOperate(MOUSE_LEFT_BUTTON_UP, 0, 0);
	Sleep(1000);
	SendMouseOperate(MOUSE_RIGHT_BUTTON_DOWN, 0, 0);
	SendMouseOperate(MOUSE_RIGHT_BUTTON_UP, 0, 0);
*/
	//SendKeyOperate(KEY_MAKE, VK_LWIN);
	//SendKeyOperate(KEY_BREAK, VK_LWIN);	

// 	SetCursorPos(200, 200);
// 	SendMouseOperate(MOUSE_LEFT_BUTTON_DOWN, 0, 0);
// 	SendMouseOperate(MOUSE_LEFT_BUTTON_UP, 0, 0);
// 	SendKeyOperate(KEY_MAKE, VK_CONTROL);
// 	SendKeyOperate(KEY_MAKE, 'A');	
// 	SendKeyOperate(KEY_BREAK, 'A');
// 	SendKeyOperate(KEY_BREAK, VK_CONTROL);
	SetTimer(100, 1000, NULL);
}


void CKeyMouseClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
// 	SendMouseOperate(MOUSE_LEFT_BUTTON_DOWN, 0, 0);
// 	Sleep(10);
// 	SendMouseOperate(MOUSE_LEFT_BUTTON_UP, 0, 0);
// 	SendKeyOperate(KEY_MAKE, VK_RETURN);
// 	SendKeyOperate(KEY_BREAK, VK_RETURN);
	WinKMLButtonDown();
	Sleep(10);
	WinKMLButtonLUp();
	WinKMKeyDown(VK_RETURN);
	WinKMKeyUp(VK_RETURN);
	CDialog::OnTimer(nIDEvent);
}


void CKeyMouseClientDlg::OnBnClickedButtonProcessOff()
{
	// TODO: Add your control notification handler code here
	KillTimer(100);
}
