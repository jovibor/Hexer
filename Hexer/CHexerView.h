#pragma once
#include "HexCtrl.h"
import Utility;

class CMainFrame;
class CChildFrame;
class CHexerDoc;
class CHexerView final : public CView
{
public:
	[[nodiscard]] auto GetHexCtrl()const->HEXCTRL::IHexCtrl*;
	[[nodiscard]] auto GetHWNDForPane(UINT uPaneID) -> HWND;
	[[nodiscard]] auto GetFileProps() -> Utility::FILEPROPS&;
private:
	[[nodiscard]] auto GetMainFrame()const->CMainFrame*;
	[[nodiscard]] auto GetChildFrame()const->CChildFrame*;
	[[nodiscard]] auto GetDocument()const->CHexerDoc*;
	[[nodiscard]] bool IsAlreadyLaunch(UINT uPaneID);
	void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)override;
	void OnInitialUpdate()override;
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)override;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void OnDraw(CDC* pDC)override;
	afx_msg void OnFilePrintPreview();
	afx_msg void OnEditEditMode();
	afx_msg void OnHexCtrlDLG(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateEditEditMode(CCmdUI* pCmdUI);
	void SetPaneAlreadyLaunched(UINT uPaneID);
	DECLARE_DYNCREATE(CHexerView);
	DECLARE_MESSAGE_MAP();
private:
	HEXCTRL::IHexCtrlPtr m_pHexCtrl { HEXCTRL::CreateHexCtrl() };
	Utility::FILEPROPS m_stFP { }; //Current view file properties.
	bool m_fIsAlreadyLaunchDlgTemplMgr { false };
	bool m_fIsAlreadyLaunchDlgDataInterp { false };
};