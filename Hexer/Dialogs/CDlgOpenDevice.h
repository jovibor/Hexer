/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxdialogex.h>
#include <memory>
#include <string>
#include <vector>

class CDlgOpenDisk;
class CDlgOpenVolume;
class CDlgOpenPath;
class CDlgOpenDevice final : public CDialogEx
{
public:
	CDlgOpenDevice(CWnd* pParent = nullptr);
	~CDlgOpenDevice();
	INT_PTR DoModal(int iTab = 0);
	[[nodiscard]] auto GetPaths() -> std::vector<std::wstring>&;
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	afx_msg void OnDestroy();
	afx_msg void OnTabSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
	void SetCurrentTab(int iTab);
	DECLARE_MESSAGE_MAP();
private:
	CTabCtrl m_tabMain;
	CRect m_rcWnd; //Dialog rect to set minimum size in the OnGetMinMaxInfo.
	std::unique_ptr<CDlgOpenDisk> m_pDlgDisk;
	std::unique_ptr<CDlgOpenVolume> m_pDlgVolume;
	std::unique_ptr<CDlgOpenPath> m_pDlgPath;
	int m_iCurTab { }; //Current tab. To avoid call m_tabMain.GetCurSel after dialog destroyed.
};