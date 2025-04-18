/*******************************************************************************
* Copyright © 2023-present Jovibor https://github.com/jovibor/                 *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once

class CMainFrame; //Forward declarations.
class CHexerView;
namespace HEXCTRL { class IHexCtrl; }
class CChildFrame final : public CMDIChildWndEx {
public:
	[[nodiscard]] auto GetHexerView()const->CHexerView*;
	[[nodiscard]] bool OnBeforeClose();
	void SetHexerView(CHexerView* pView);
private:
	auto GetFrameIcon()const->HICON override;
	[[nodiscard]] auto GetMainFrame()const->CMainFrame*;
	[[nodiscard]] auto GetHexCtrl()const->HEXCTRL::IHexCtrl*;
	afx_msg void OnClose();
	BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)override;
	afx_msg void OnDestroy();
	void OnFrameActivate();
	void OnFrameDisactivate();
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	DECLARE_DYNCREATE(CChildFrame);
	DECLARE_MESSAGE_MAP();
private:
	CHexerView* m_pHexerView { };
	std::vector<HWND> m_vecWndHidden; //HWNDs that must be shown/hidden on tab activating/disactivating.
	int m_iMDIActivateCreation { 2 };
	bool m_fClosing { false };  //Indicates that a child-frame is closing.
	bool m_fCreating { false }; //Indicates that a child-frame is creating.
};