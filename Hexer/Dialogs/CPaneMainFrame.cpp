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
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

void CPaneTabCtrl::OnLButtonDblClk(UINT /*nFlags*/, CPoint /*point*/)
{
	//Empty handler for tabs' doubleclicks.
}

void CPaneTabCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	//This method is overriden to resolve the issue with tabs' dragging described here:
	//https://developercommunity.visualstudio.com/t/problem-dragging-mdi-tabs-in-a-mfc-application-com/478457

	CWnd::OnMouseMove(nFlags, point);

	const auto iPrevHighlighted = m_iHighlighted;

	const auto bTabCloseButtonHighlighted = m_bTabCloseButtonHighlighted;
	m_bTabCloseButtonHighlighted = m_rectCloseButton.PtInRect(point);

	if (bTabCloseButtonHighlighted != m_bTabCloseButtonHighlighted) {
		RedrawWindow(m_rectCloseButton);
	}

	if (m_iHighlighted >= 0 && m_iPressed < 0 && !m_bReadyToDetach) {
		CPoint pt = point;
		ClientToScreen(&pt);
		const auto pWnd = CWnd::WindowFromPoint(pt);
		if (pWnd != nullptr && pWnd->GetSafeHwnd() != GetSafeHwnd()) {
			ReleaseCapture();
			m_iHighlighted = -1;
			InvalidateTab(iPrevHighlighted);
			return;
		}
	}

	m_iHighlighted = GetTabFromPoint(point);

	if (m_iPressed >= 0 && m_iHighlighted != m_iPressed) {
		m_iHighlighted = -1;
	}

	if (m_iHighlighted != iPrevHighlighted && (m_bHighLightTabs || IsActiveTabCloseButton())) {
		if (iPrevHighlighted < 0) {
			if (m_iHighlighted >= 0) {
				SetCapture();
			}
		}
		else {
			if (m_iHighlighted < 0 && m_iPressed < 0) {
				m_bTabCloseButtonHighlighted = FALSE;
				m_bTabCloseButtonPressed = FALSE;

				if (!m_bReadyToDetach) {
					ReleaseCapture();
				}
			}
		}

		InvalidateTab(m_iHighlighted);
		InvalidateTab(iPrevHighlighted);
	}

	if (m_bReadyToDetach) {
		const auto nNumTabs = m_iTabsNum; // how many tabs before detch

		// try to rearrange tabs if their number > 1
		if (IsPtInTabArea(point) && nNumTabs > 1 && m_bEnableTabSwap) {
			const auto nTabNum = GetTabFromPoint(point);

			if (nTabNum != m_iActiveTab && nTabNum != -1) {
				const auto nSecondTab = m_iActiveTab;
				SwapTabs(nTabNum, nSecondTab);
				RecalcLayout();
				SetActiveTab(nTabNum);
				/*
				int nCurrTabNum = GetTabFromPoint(point);
				if (nCurrTabNum != nTabNum) {
					CRect rectTab;
					GetTabRect(nTabNum, rectTab);
					CPoint ptCursorNewPos = point;

					ptCursorNewPos.x = rectTab.left + m_nOffsetFromTabLeft;

					ClientToScreen(&ptCursorNewPos);
					SetCursorPos(ptCursorNewPos.x, ptCursorNewPos.y);
				}
				*/
			}
			return;
		}

		if (IsPtInTabArea(point)) {
			return;
		}

		const auto bDetachSucceeded = DetachTab(DM_MOUSE);

		if (bDetachSucceeded && nNumTabs <= 2) {
			// last tab was detached successfully - run out, because the control
			// has been destroyed
			return;
		}

		if (bDetachSucceeded) {
			m_bReadyToDetach = FALSE;
		}

		return;
	}
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