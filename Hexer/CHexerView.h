#pragma once
#include "HexCtrl.h"

import Utility;

class CMainFrame;
class CChildFrame;
class CHexerDoc;
class CHexerView final : public CView {
public:
	[[nodiscard]] auto GetDataInfo()const->Ut::DATAINFO;
	[[nodiscard]] auto GetHexCtrl()const->HEXCTRL::IHexCtrl*;
	[[nodiscard]] auto GetHWNDForPane(UINT uPaneID) -> HWND;
private:
	[[nodiscard]] auto GetMainFrame()const->CMainFrame*;
	[[nodiscard]] auto GetChildFrame()const->CChildFrame*;
	[[nodiscard]] auto GetDocument()const->CHexerDoc*;
	[[nodiscard]] bool IsPaneAlreadyLaunch(UINT uPaneID)const;
	void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)override;
	void OnDraw(CDC* pDC)override;
	afx_msg void OnEditEditMode();
	afx_msg void OnFilePrint();
	afx_msg void OnHexCtrlDLG(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHexCtrlSetFont(NMHDR* pNMHDR, LRESULT* pResult);
	void OnInitialUpdate()override;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)override;
	afx_msg void OnUpdateEditEditMode(CCmdUI* pCmdUI);
	void SetPaneAlreadyLaunch(UINT uPaneID);
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
	bool m_fIsAlreadyLaunchDlgBkmMgr { false };
	bool m_fIsAlreadyLaunchDlgTemplMgr { false };
	bool m_fIsAlreadyLaunchDlgDataInterp { false };
};