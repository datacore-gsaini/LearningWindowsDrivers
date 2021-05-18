
// MFCApplication1Dlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "MFCApplication1.h"
#include "MFCApplication1Dlg.h"
#include "afxdialogex.h"
#include "fltuser.h"
#include <string>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "fltlib.lib")
#pragma comment(lib, "fltmgr.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

TCHAR szDrive[] = _T("A:");

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCApplication1Dlg dialog



CMFCApplication1Dlg::CMFCApplication1Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFCAPPLICATION1_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCApplication1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO2, DriveCombo);
}

BEGIN_MESSAGE_MAP(CMFCApplication1Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_CBN_SELCHANGE(IDC_COMBO2, &CMFCApplication1Dlg::OnSelchangeCombo2)
	ON_BN_CLICKED(IDOK, &CMFCApplication1Dlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CMFCApplication1Dlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2, &CMFCApplication1Dlg::OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3, &CMFCApplication1Dlg::OnBnClickedButton3)
END_MESSAGE_MAP()


// CMFCApplication1Dlg message handlers

BOOL CMFCApplication1Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	// VT
	DWORD uDriveMask = GetLogicalDrives();
	if (uDriveMask != 0) {
		int i = 0;
		while (uDriveMask) {
			char drive[4];
			// Use the bitwise AND, 1â€"available, 0-not available
			if (uDriveMask & 1) {
				//printf("%S ", (const char*)szDrive);
				sprintf_s(drive, 3, "%c:", 'A' + i);
				int type = GetDriveTypeA(drive);
				if (type != 4) // Network
					DriveCombo.AddString(szDrive);
			}
			// increment, check next drive
			++szDrive[0];
			// shift the bitmask binary right
			uDriveMask >>= 1;
			i++;
		}
		DriveCombo.AddString(_T("All drives"));
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CMFCApplication1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CMFCApplication1Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMFCApplication1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

CHAR buffer[50] = { 0 };

void CMFCApplication1Dlg::OnSelchangeCombo2()
{
	CStringW str;

	// TODO: Add your control notification handler code here
	//DriveCombo CComboBox DriveCombo;
	int DriveComboSelectedItem = DriveCombo.GetCurSel();
	if (DriveComboSelectedItem != LB_ERR) {		
		DriveCombo.GetLBText(DriveComboSelectedItem, str);
		//AfxMessageBox(str);
	}
	CT2CA pszConvertedAnsiString(str);
	// construct a std::string using the LPCSTR input
	std::string strStd(pszConvertedAnsiString);
	strcpy_s(buffer, 49, strStd.c_str());
}

HANDLE port = NULL;

void CMFCApplication1Dlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	if (port) {
		CloseHandle(port);
		port = NULL;
	}
	CDialogEx::OnOK();
}


void CMFCApplication1Dlg::OnBnClickedButton1()
{
	// TODO: Add your control notification handler code here
	DWORD bytesret = 0;
	PCHAR sendBuf;
	CHAR recvBuf[50] = { 0 };
	sendBuf = buffer;
	if (port == NULL) {
		if (FilterConnectCommunicationPort(L"\\Mini-filter", 0, NULL, 0, NULL, &port)) {
			MessageBoxA(0, 0, 0, 0);
			return;
		}
	}
	if (FilterSendMessage(port, sendBuf, strlen(sendBuf), recvBuf, 50, &bytesret) == 0) {
		MessageBoxA(0, recvBuf, 0, 0);
	}
}


void CMFCApplication1Dlg::OnBnClickedButton2()
{
	// TODO: Add your control notification handler code here
	char command[128];
	sprintf_s(command, "net start %s", "FsFilter1");
	system(command);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON2)->EnableWindow(FALSE);
}


void CMFCApplication1Dlg::OnBnClickedButton3()
{
	// TODO: Add your control notification handler code here
	char command[128];
	sprintf_s(command, "net stop %s", "FsFilter1");
	system(command);
	if (port) {
		CloseHandle(port);
		port = NULL;
	}
	GetDlgItem(IDC_BUTTON2)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON3)->EnableWindow(FALSE);
}
