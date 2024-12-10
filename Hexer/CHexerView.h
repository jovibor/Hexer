/*******************************************************************************
* Copyright © 2023-present Jovibor https://github.com/jovibor/                 *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include "HexCtrl.h"

import Utility;
import DlgProcMemory;

class CMainFrame;
class CChildFrame;
class CHexerDoc;
class CHexerView final : public CView {
public:
	[[nodiscard]] auto GetDataInfo()const->Ut::DATAINFO;
	[[nodiscard]] auto GetDlgProcMemory()const->HWND;
	[[nodiscard]] auto GetDocument()const->CHexerDoc*;
	[[nodiscard]] auto GetHexCtrl()const->HEXCTRL::IHexCtrl*;
	[[nodiscard]] auto GetHWNDForPane(UINT uPaneID) -> HWND;
	[[nodiscard]] bool OnBeforeClose();
private:
	void ChangeDataAccessMode(Ut::DATAACCESS stDAC);
	void ChangeDataIOMode(Ut::EDataIOMode eDataIOMode);
	[[nodiscard]] auto GetChildFrame()const->CChildFrame*;
	[[nodiscard]] auto GetMainFrame()const->CMainFrame*;
	void HexCtrlSetData(bool fAdjust = false);
	[[nodiscard]] bool IsPaneAlreadyLaunch(UINT uPaneID)const;
	void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)override;
	afx_msg void OnDataAccessRO();
	afx_msg void OnDataAccessRWSAFE();
	afx_msg void OnDataAccessRWINPLACE();
	afx_msg void OnDataIOMMAP();
	afx_msg void OnDataIOBuff();
	afx_msg void OnDataIOImmediate();
	void OnDraw(CDC* pDC)override;
	afx_msg void OnEditCopyHex();
	afx_msg void OnEditPasteHex();
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnFilePrint();
	afx_msg void OnFileSave();
	afx_msg void OnHexCtrlDLG(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHexCtrlSetData(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHexCtrlSetFont(NMHDR* pNMHDR, LRESULT* pResult);
	void OnInitialUpdate()override;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)override;
	afx_msg void OnUpdateDataAccessMode(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDataIOMode(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateProcMemory(CCmdUI* pCmdUI);
	afx_msg void OnViewProcMemory();
	void SetPaneAlreadyLaunch(UINT uPaneID);
	void SaveDataToDisk();
	void UpdateDlgBkmMgr()const;
	void UpdateDlgDataInterp()const;
	void UpdateDlgModify()const;
	void UpdateDlgSearch()const;
	void UpdateDlgTemplMgr()const;
	void UpdateHexCtrlDlgData(UINT uPaneID)const;
	DECLARE_DYNCREATE(CHexerView);
	DECLARE_MESSAGE_MAP();
private:
	HEXCTRL::IHexCtrlPtr m_pHexCtrl { HEXCTRL::CreateHexCtrl() };
	CDlgProcMemory m_dlgProcMem;
	bool m_fIsAlreadyLaunchDlgBkmMgr { false };
	bool m_fIsAlreadyLaunchDlgTemplMgr { false };
	bool m_fIsAlreadyLaunchDlgDataInterp { false };
	bool m_fIsHexCtrlDataModified { false };
};