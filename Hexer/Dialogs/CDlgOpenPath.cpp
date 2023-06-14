/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CDlgOpenPath.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(CDlgOpenPath, CDialogEx)
	ON_CBN_EDITUPDATE(IDC_OPEN_PATH_COMBO_PATH, &CDlgOpenPath::OnComboPathEdit)
END_MESSAGE_MAP()

auto CDlgOpenPath::GetPaths()->std::vector<std::wstring>&
{
	return m_vecPaths;
}

void CDlgOpenPath::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_OPEN_PATH_COMBO_PATH, m_stComboPath);
}

void CDlgOpenPath::OnOK()
{
	m_vecPaths.clear();
	CString cstrText;
	m_stComboPath.GetWindowTextW(cstrText);
	m_vecPaths.emplace_back(cstrText);

	if (!m_vecPaths.empty()) {
		static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDOK);
	}
}

void CDlgOpenPath::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

void CDlgOpenPath::OnComboPathEdit()
{
	GetDlgItem(IDOK)->EnableWindow(m_stComboPath.GetWindowTextLengthW() > 0);
}