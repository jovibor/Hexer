/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once

class CHexerView; class CMainFrame;
class CChildFrame final : public CMDIChildWndEx {
public:
	[[nodiscard]] auto GetHexerView()const->CHexerView*;
	void SetHexerView(CHexerView* pView);
private:
	[[nodiscard]] auto GetMainFrame()const->CMainFrame*;
	afx_msg void OnClose();
	BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)override;
	afx_msg void OnDestroy();
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	DECLARE_DYNCREATE(CChildFrame);
	DECLARE_MESSAGE_MAP();
private:
	CHexerView* m_pHexerView { };
	bool m_fClosing { false };  //Indicates that a child-frame is closing.
	bool m_fCreating { false }; //Indicates that a child-frame is creating.
	int m_iMDIActivateCreation { 2 };
};