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
	[[nodiscard]] auto GetOpenData()const->const Ut::DATAOPEN&;
private:
	void DoDataExchange(CDataExchange* pDX)override;
	[[nodiscard]] bool IsOK()const;
	BOOL OnInitDialog()override;
	afx_msg void OnBtnBrowse();
	afx_msg void OnChangeEditPathAndSize();
	void OnOK()override;
	DECLARE_MESSAGE_MAP();
private:
	CComboBox m_stComboMeasure;
	CEdit m_stEditPath;
	CEdit m_stEditSize;
	Ut::DATAOPEN m_stDOS;
};

BEGIN_MESSAGE_MAP(CDlgNewFile, CDialogEx)
	ON_BN_CLICKED(IDC_NEWFILE_BTN_BROWSE, &CDlgNewFile::OnBtnBrowse)
	ON_EN_CHANGE(IDC_NEWFILE_EDIT_PATH, &CDlgNewFile::OnChangeEditPathAndSize)
	ON_EN_CHANGE(IDC_NEWFILE_EDIT_SIZE, &CDlgNewFile::OnChangeEditPathAndSize)
END_MESSAGE_MAP()

auto CDlgNewFile::GetOpenData()const->const Ut::DATAOPEN&
{
	return m_stDOS;
}


//Private methods.

void CDlgNewFile::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NEWFILE_EDIT_PATH, m_stEditPath);
	DDX_Control(pDX, IDC_NEWFILE_EDIT_SIZE, m_stEditSize);
	DDX_Control(pDX, IDC_NEWFILE_COMBO_MEASURE, m_stComboMeasure);
}

bool CDlgNewFile::IsOK()const
{
	return (m_stEditPath.GetWindowTextLengthW() > 0) && (m_stEditSize.GetWindowTextLengthW() > 0);
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

void CDlgNewFile::OnChangeEditPathAndSize()
{
	GetDlgItem(IDOK)->EnableWindow(IsOK());
}

void CDlgNewFile::OnOK()
{
	CStringW cwstrSize;
	m_stEditSize.GetWindowTextW(cwstrSize);
	const auto optSize = stn::StrToUInt64(cwstrSize.GetString());
	if (!optSize || *optSize == 0) {
		MessageBoxW(L"Enter correct file size.", L"Incorrect file size", MB_ICONERROR);
		return;
	}

	CStringW cwstrPath;
	m_stEditPath.GetWindowTextW(cwstrPath);
	m_stDOS.eOpenMode = Ut::EOpenMode::NEW_FILE;
	m_stDOS.wstrDataPath = cwstrPath;
	m_stDOS.ullSizeNewFile = *optSize * m_stComboMeasure.GetItemData(m_stComboMeasure.GetCurSel());

	CDialogEx::OnOK();
}