/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CDlgOpenDevice.h"
#include "CDlgOpenDisk.h"
#include "CDlgOpenVolume.h"
#include "CDlgOpenPath.h"
#include "resource.h"
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib") //For Wbemidl.h

BEGIN_MESSAGE_MAP(CDlgOpenDevice, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_OPENDEVICE_TAB, &CDlgOpenDevice::OnTabSelChanged)
	ON_WM_GETMINMAXINFO()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CDlgOpenDevice::CDlgOpenDevice(CWnd* pParent) : CDialogEx(IDD_OPENDEVICE, pParent)
{
}

CDlgOpenDevice::~CDlgOpenDevice() = default;

INT_PTR CDlgOpenDevice::DoModal(int iTab)
{
	m_iCurTab = iTab;

	return CDialogEx::DoModal();
}

auto CDlgOpenDevice::GetPaths()->std::vector<std::wstring>&
{
	switch (m_iCurTab) {
	case 0:
		return m_pDlgDisk->GetPaths();
	case 1:
		return m_pDlgVolume->GetPaths();
	case 2:
		return m_pDlgPath->GetPaths();
	default:
		std::unreachable();
	}
}

void CDlgOpenDevice::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPENDEVICE_TAB, m_tabMain);
}

BOOL CDlgOpenDevice::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	GetWindowRect(m_rcWnd);

	m_tabMain.InsertItem(0, L"Open Physical Disk");
	m_tabMain.InsertItem(1, L"Open Volume");
	m_tabMain.InsertItem(3, L"Open Path");
	CRect rcTab;
	m_tabMain.GetItemRect(0, rcTab);

	//Initialize COM and IWbemServices interface for the "Root\\Microsoft\\Windows\\Storage".
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
	CComPtr<IWbemLocator> pWbemLocator;
	CComPtr<IWbemServices> pWbemServices;
	if (CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator,
		reinterpret_cast<LPVOID*>(&pWbemLocator)) == S_OK) {
		pWbemLocator->ConnectServer(_bstr_t(L"Root\\Microsoft\\Windows\\Storage"), nullptr, nullptr, nullptr, 0,
			nullptr, nullptr, &pWbemServices);
		CoSetProxyBlanket(pWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
			RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
	}

	m_pDlgDisk = std::make_unique<CDlgOpenDisk>();
	m_pDlgDisk->SetIWbemServices(pWbemServices);
	m_pDlgDisk->Create(IDD_OPEN_DISK, this);
	m_pDlgDisk->SetWindowPos(nullptr, rcTab.left, rcTab.bottom + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

	m_pDlgVolume = std::make_unique<CDlgOpenVolume>();
	m_pDlgVolume->SetIWbemServices(pWbemServices);
	m_pDlgVolume->Create(IDD_OPEN_VOLUME, this);
	m_pDlgVolume->SetWindowPos(nullptr, rcTab.left, rcTab.bottom + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);

	m_pDlgPath = std::make_unique<CDlgOpenPath>();
	m_pDlgPath->Create(IDD_OPEN_PATH, this);
	m_pDlgPath->SetWindowPos(nullptr, rcTab.left, rcTab.bottom + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);

	EnableDynamicLayout(TRUE);
	const auto pLayout = GetDynamicLayout();
	pLayout->Create(this);
	pLayout->AddItem(m_pDlgDisk->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
	pLayout->AddItem(m_pDlgVolume->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
	pLayout->AddItem(m_pDlgPath->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));

	SetCurrentTab(m_iCurTab);

	const auto hIcon = AfxGetApp()->LoadIconW(IDR_HEXER_FRAME);
	SetIcon(hIcon, TRUE);
	SetIcon(hIcon, FALSE);

	return TRUE;
}

void CDlgOpenDevice::OnDestroy()
{
	CDialogEx::OnDestroy();

	//CoUninitialize must be called only after all CComPtr<> objects are destroyed.
	//Othervise exceptions occur.
	CoUninitialize();
}

void CDlgOpenDevice::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	lpMMI->ptMinTrackSize.x = m_rcWnd.Width();
	lpMMI->ptMinTrackSize.y = m_rcWnd.Height();

	CDialogEx::OnGetMinMaxInfo(lpMMI);
}

void CDlgOpenDevice::OnTabSelChanged(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	SetCurrentTab(m_tabMain.GetCurSel());
}

void CDlgOpenDevice::SetCurrentTab(int iTab)
{
	m_iCurTab = iTab;
	m_tabMain.SetCurSel(iTab);
	switch (iTab) {
	case 0:
		m_pDlgVolume->ShowWindow(SW_HIDE);
		m_pDlgPath->ShowWindow(SW_HIDE);
		m_pDlgDisk->ShowWindow(SW_SHOW);
		break;
	case 1:
		m_pDlgDisk->ShowWindow(SW_HIDE);
		m_pDlgPath->ShowWindow(SW_HIDE);
		m_pDlgVolume->ShowWindow(SW_SHOW);
		break;
	case 2:
		m_pDlgDisk->ShowWindow(SW_HIDE);
		m_pDlgVolume->ShowWindow(SW_HIDE);
		m_pDlgPath->ShowWindow(SW_SHOW);
		break;
	default:
		std::unreachable();
	}
}