/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include "CDlgOpenDisk.h"
#include "CDlgOpenVolume.h"
#include "CDlgOpenPath.h"
#include <afxdialogex.h>
#include <memory>
#include <string>
#include <vector>

class CDlgOpenDevice final : public CDialogEx
{
public:
	CDlgOpenDevice(CWnd* pParent = nullptr);
	INT_PTR DoModal(int iTab = 0);
	[[nodiscard]] auto GetPaths() -> std::vector<std::wstring>&;
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	void SetCurrentTab(int iTab);
	afx_msg void OnTabSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP();
	DECLARE_DYNAMIC(CDlgOpenDevice);
private:
	CTabCtrl m_tabMain;
	const std::unique_ptr<CDlgOpenDisk> m_pDlgDisk { std::make_unique<CDlgOpenDisk>() };
	const std::unique_ptr<CDlgOpenVolume> m_pDlgVolume { std::make_unique<CDlgOpenVolume>() };
	const std::unique_ptr<CDlgOpenPath> m_pDlgPath { std::make_unique<CDlgOpenPath>() };
	int m_iCurTab { }; //Current tab. To avoid call m_tabMain.GetCurSel after dialog destroyed.
};