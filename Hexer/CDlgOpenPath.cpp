/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CDlgOpenPath.h"
#include "resource.h"

IMPLEMENT_DYNAMIC(CDlgOpenPath, CDialogEx)

BEGIN_MESSAGE_MAP(CDlgOpenPath, CDialogEx)
END_MESSAGE_MAP()

auto CDlgOpenPath::GetPaths()->std::vector<std::wstring>&
{
	return m_vecPaths;
}

void CDlgOpenPath::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CDlgOpenPath::OnOK()
{
	CString cstrText;
	GetDlgItemTextW(IDC_OPEN_PATH_EDIT_PATH, cstrText);
	m_vecPaths.clear();
	m_vecPaths.emplace_back(cstrText);

	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDOK);
}

void CDlgOpenPath::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}