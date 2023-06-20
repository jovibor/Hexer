/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxdialogex.h>
#include <string>
import Utility;

class CDlgNewFile final : public CDialogEx
{
public:
	CDlgNewFile(CWnd* pParent = nullptr);
	[[nodiscard]] auto GetNewFileInfo() -> Utility::FILEOPEN;
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	afx_msg void OnBtnBrowse();
	afx_msg void OnChangeEditPath();
	DECLARE_MESSAGE_MAP();
private:
	Utility::FILEOPEN m_stFOS { };
	CEdit m_stEditSize;
	CEdit m_stEditPath;
	CComboBox m_stComboMeasure;
	virtual void OnOK();
};