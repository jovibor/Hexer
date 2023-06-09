/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxdialogex.h>
#include <string>
#include <vector>
import Utility;

class CDlgOpenVolume final : public CDialogEx
{
public:
	[[nodiscard]] auto GetPaths() -> std::vector<std::wstring>&;
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	void OnOK()override;
	void OnCancel()override;
	afx_msg void OnListDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_DYNAMIC(CDlgOpenVolume);
	DECLARE_MESSAGE_MAP();
	[[nodiscard]] static auto GetVolumes() -> std::vector<Utility::VOLUME>;
private:
	CListCtrl m_list;
	std::vector<Utility::VOLUME> m_vecVolumes;
	std::vector<std::wstring> m_vecPaths; //Paths to open.
};