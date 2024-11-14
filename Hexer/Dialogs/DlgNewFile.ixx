module;
/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "resource.h"
#include <afxdlgs.h>
#include <afxdialogex.h>
#include <string>
export module DlgNewFile;

import Utility;

export class CDlgNewFile final : public CDialogEx {
public:
	CDlgNewFile(CWnd* pParent = nullptr) : CDialogEx(IDD_NEWFILE, pParent) { }
	[[nodiscard]] auto GetNewFileInfo()const->const Ut::DATAOPEN&;
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	afx_msg void OnBtnBrowse();
	afx_msg void OnChangeEditPath();
	DECLARE_MESSAGE_MAP();
private:
	Ut::DATAOPEN m_stDOS { };
	CEdit m_stEditSize;
	CEdit m_stEditPath;
	CComboBox m_stComboMeasure;
	virtual void OnOK();
};

BEGIN_MESSAGE_MAP(CDlgNewFile, CDialogEx)
	ON_BN_CLICKED(IDC_NEWFILE_BTN_BROWSE, &CDlgNewFile::OnBtnBrowse)
	ON_EN_CHANGE(IDC_NEWFILE_EDIT_PATH, &CDlgNewFile::OnChangeEditPath)
END_MESSAGE_MAP()

auto CDlgNewFile::GetNewFileInfo()const->const Ut::DATAOPEN&
{
	return m_stDOS;
}


//Private methods.

void CDlgNewFile::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NEWFILE_EDIT_SIZE, m_stEditSize);
	DDX_Control(pDX, IDC_NEWFILE_EDIT_PATH, m_stEditPath);
	DDX_Control(pDX, IDC_NEWFILE_COMBO_MEASURE, m_stComboMeasure);
}

BOOL CDlgNewFile::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	auto iIndex = m_stComboMeasure.AddString(L"Bytes");
	m_stComboMeasure.SetItemData(iIndex, 1);
	m_stComboMeasure.SetCurSel(iIndex);
	iIndex = m_stComboMeasure.AddString(L"KB (1024 Bytes)");
	m_stComboMeasure.SetItemData(iIndex, 1024);
	iIndex = m_stComboMeasure.AddString(L"MB (1024 KB)");
	m_stComboMeasure.SetItemData(iIndex, 1024 * 1024);
	iIndex = m_stComboMeasure.AddString(L"GB (1024 MB)");
	m_stComboMeasure.SetItemData(iIndex, 1024 * 1024 * 1024);

	const auto hIcon = AfxGetApp()->LoadIconW(IDR_HEXER_FRAME);
	SetIcon(hIcon, TRUE);
	SetIcon(hIcon, FALSE);

	return TRUE;
}

void CDlgNewFile::OnBtnBrowse()
{
	CFileDialog fd(FALSE, nullptr, nullptr,
		OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_DONTADDTORECENT | OFN_ENABLESIZING | OFN_PATHMUSTEXIST);
	if (fd.DoModal() == IDOK) {
		m_stEditPath.SetWindowTextW(fd.GetPathName());
	}
}

void CDlgNewFile::OnChangeEditPath()
{
	GetDlgItem(IDOK)->EnableWindow(m_stEditPath.GetWindowTextLengthW() > 0);
}

void CDlgNewFile::OnOK()
{
	m_stDOS.eMode = Ut::EOpenMode::NEW_FILE;
	CStringW wstr;
	m_stEditPath.GetWindowTextW(wstr);
	m_stDOS.wstrDataPath = wstr;

	if (m_stEditSize.GetWindowTextLengthW() == 0) {
		MessageBoxW(L"Enter the file size.", L"Size is empty", MB_ICONEXCLAMATION);
		return;
	}

	m_stEditSize.GetWindowTextW(wstr);
	if (const auto opt = stn::StrToUInt64(wstr.GetString()); opt) {
		if (*opt == 0) {
			MessageBoxW(L"File size can not be zero.", L"Size is zero", MB_ICONEXCLAMATION);
			return;
		}

		m_stDOS.ullNewFileSize = *opt * m_stComboMeasure.GetItemData(m_stComboMeasure.GetCurSel());
	}

	CDialogEx::OnOK();
}