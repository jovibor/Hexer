/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxcontrolbars.h>

//Tab Control class used within Panes.
//Needed to override OnLButtonDblClk message.
class CPaneTabCtrl final : public CMFCTabCtrl {
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	DECLARE_SERIAL(CPaneTabCtrl);
	DECLARE_MESSAGE_MAP();
};

//This class is used in SetTabbedPaneRTC.
//Needed to set the m_pTabWndRTC (pointer to a Tab Control)
//to our own overridden CPaneTabCtrl class.
class CPaneTabbedPane final : public CTabbedPane {
	CPaneTabbedPane() { m_pTabWndRTC = RUNTIME_CLASS(CPaneTabCtrl); };
	DECLARE_SERIAL(CPaneTabbedPane);
};

class CPaneMainFrame final : public CDockablePane
{
public:
	void SetNestedHWND(HWND hWnd);
	[[nodiscard]] auto GetNestedHWND()const->HWND;
private:
	void AdjustLayout()override;
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP();
private:
	HWND m_hWndNested { };
	HWND m_hWndOrigParent { };
};