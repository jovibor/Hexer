/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CPaneMainFrame.h"
#include <cassert>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_SERIAL(CPaneTabCtrl, CMFCTabCtrl, 1)
IMPLEMENT_SERIAL(CPaneTabbedPane, CTabbedPane, 1)

BEGIN_MESSAGE_MAP(CPaneTabCtrl, CMFCTabCtrl)
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

void CPaneTabCtrl::OnLButtonDblClk(UINT /*nFlags*/, CPoint /*point*/)
{
	//Empty handler for tabs' doubleclicks.
}


BEGIN_MESSAGE_MAP(CPaneMainFrame, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_PAINT()
END_MESSAGE_MAP()

void CPaneMainFrame::SetNestedHWND(HWND hWnd)
{
	assert(hWnd != nullptr);
	if (hWnd == nullptr || hWnd == m_hWndNested)
		return;

	if (m_hWndNested != nullptr) {
		::SetParent(m_hWndNested, m_hWndOrigParent); //Restore original parent.
		::ShowWindow(m_hWndNested, SW_HIDE);
	}

	const auto llStyle = GetWindowLongPtrW(hWnd, GWL_STYLE);
	//WS_THICKFRAME is responsible for resizability.
	const auto llStyleNew = llStyle & ~(WS_BORDER | WS_DLGFRAME | WS_THICKFRAME);
	SetWindowLongPtrW(hWnd, GWL_STYLE, llStyleNew);
	::SetParent(hWnd, m_hWnd);
	m_hWndNested = hWnd;
	m_hWndOrigParent = ::GetParent(hWnd);
	AdjustLayout();
}

auto CPaneMainFrame::GetNestedHWND()const->HWND
{
	return m_hWndNested;
}

void CPaneMainFrame::AdjustLayout()
{
	if (const auto pMainWnd = AfxGetMainWnd(); GetSafeHwnd() == nullptr
		|| (pMainWnd != nullptr && pMainWnd->IsIconic()) || m_hWndNested == nullptr) {
		return;
	}

	CRect rcClient;
	GetClientRect(rcClient);
	::SetWindowPos(m_hWndNested, nullptr, rcClient.left, rcClient.top, rcClient.Width(),
		rcClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
}

int CPaneMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	SetTabbedPaneRTC(RUNTIME_CLASS(CPaneTabbedPane));

	return 0;
}

void CPaneMainFrame::OnPaint()
{
	CPaintDC dc(this);

	CRect rcClient;
	GetClientRect(rcClient);
	dc.FillSolidRect(rcClient, RGB(255, 255, 255)); //Default white bk.
}

void CPaneMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	AdjustLayout();
}