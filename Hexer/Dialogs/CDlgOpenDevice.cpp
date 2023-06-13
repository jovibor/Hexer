/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CDlgOpenDevice.h"
#include "resource.h"

IMPLEMENT_DYNAMIC(CDlgOpenDevice, CDialogEx)

BEGIN_MESSAGE_MAP(CDlgOpenDevice, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_OPENDEVICE_TAB, &CDlgOpenDevice::OnTabSelChanged)
	ON_WM_SIZE()
END_MESSAGE_MAP()

CDlgOpenDevice::CDlgOpenDevice(CWnd* pParent /*=nullptr*/) : CDialogEx(IDD_OPENDEVICE, pParent)
{
}

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

	m_tabMain.InsertItem(0, L"Open Physical Disk");
	m_tabMain.InsertItem(1, L"Open Volume");
	m_tabMain.InsertItem(3, L"Open Path");
	CRect rcTab;
	m_tabMain.GetItemRect(0, rcTab);

	m_pDlgDisk->Create(IDD_OPEN_DISK, this);
	m_pDlgDisk->SetWindowPos(nullptr, rcTab.left, rcTab.bottom + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

	m_pDlgVolume->Create(IDD_OPEN_VOLUME, this);
	m_pDlgVolume->SetWindowPos(nullptr, rcTab.left, rcTab.bottom + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);

	m_pDlgPath->Create(IDD_OPEN_PATH, this);
	m_pDlgPath->SetWindowPos(nullptr, rcTab.left, rcTab.bottom + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);

	EnableDynamicLayout(TRUE);
	const auto pLayout = GetDynamicLayout();
	pLayout->Create(this);
	pLayout->AddItem(m_pDlgDisk->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
	pLayout->AddItem(m_pDlgVolume->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
	pLayout->AddItem(m_pDlgPath->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));

	SetCurrentTab(m_iCurTab);

	return TRUE;
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