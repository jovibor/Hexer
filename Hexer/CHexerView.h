#pragma once
#include "HexCtrl.h"
import Utility;

class CMainFrame;
class CChildFrame;
class CHexerView final : public CView
{
private:
	[[nodiscard]] auto GetMainFrame()const->CMainFrame*;
	[[nodiscard]] auto GetChildFrame()const->CChildFrame*;
	[[nodiscard]] auto GetDocument()const->CHexerDoc*;
	[[nodiscard]] auto GetHexCtrl()const->HEXCTRL::IHexCtrl*;
	[[nodiscard]] bool IsWritable()const;
	void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)override;
	void OnInitialUpdate()override;
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)override;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void OnDraw(CDC* pDC)override;
	afx_msg void OnFilePrintPreview();
	afx_msg void OnEditEditMode();
	afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFilePrintPreview(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFilePrintSetup(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSave(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditEditMode(CCmdUI* pCmdUI);
	DECLARE_DYNCREATE(CHexerView);
	DECLARE_MESSAGE_MAP();
private:
	HEXCTRL::IHexCtrlPtr m_pHexCtrl { HEXCTRL::CreateHexCtrl() };
	Utility::FILEPROPS m_stFP { }; //Current view file properties.
	bool m_fWritable { }; //HexCtrl current mode.
};