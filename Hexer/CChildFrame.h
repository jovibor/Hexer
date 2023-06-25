/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once

class CHexerView;
class CChildFrame final : public CMDIChildWndEx
{
public:
	[[nodiscard]] auto GetHexerView()const->CHexerView*;
	void SetHexerView(CHexerView* pView);
private:
	afx_msg void OnClose();
	BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)override;
	afx_msg void OnDestroy();
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
	DECLARE_DYNCREATE(CChildFrame);
	DECLARE_MESSAGE_MAP();
private:
	CHexerView* m_pHexerView { };
	bool m_fClosing { false }; //Indicates that the tab is closing now.
};