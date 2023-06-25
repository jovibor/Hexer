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
private:
	auto CreateGridFileProps() -> HWND;
	[[nodiscard]] auto GetMainFrame()const->CMainFrame*;
	[[nodiscard]] auto GetChildFrame()const->CChildFrame*;
	[[nodiscard]] auto GetDocument()const->CHexerDoc*;
	[[nodiscard]] bool IsPaneAlreadyLaunch(UINT uPaneID)const;
	void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)override;
	void OnInitialUpdate()override;
	void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)override;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	void OnDraw(CDC* pDC)override;
	afx_msg void OnFilePrintPreview();
	afx_msg void OnEditEditMode();
	afx_msg void OnHexCtrlDLG(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateEditEditMode(CCmdUI* pCmdUI);
	void SetPaneAlreadyLaunch(UINT uPaneID);
	void UpdateGridFileProps();
	DECLARE_DYNCREATE(CHexerView);
	DECLARE_MESSAGE_MAP();
private:
	HEXCTRL::IHexCtrlPtr m_pHexCtrl { HEXCTRL::CreateHexCtrl() };
	bool m_fIsAlreadyLaunchGridFileProps { false };
	bool m_fIsAlreadyLaunchDlgTemplMgr { false };
	bool m_fIsAlreadyLaunchDlgDataInterp { false };

	//"File Properties grid related."
	enum class EPropName : std::uint8_t {
		FILE_PATH = 0x1, FILE_NAME, FILE_SIZE, PAGE_SIZE, IS_MUTABLE
	};
	CMFCPropertyGridCtrl m_wndGridFileProps;
	std::vector<CMFCPropertyGridProperty*> m_vecPropsFileProps;
	CFont m_fntFilePropsGrid;
};