/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CHexerApp.h"
#include "CChildFrame.h"
#include "CMainFrame.h"
#include "CHexerView.h"

import Utility;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWndEx)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_MDIACTIVATE()
END_MESSAGE_MAP()

auto CChildFrame::GetHexerView()const->CHexerView*
{
	return m_pHexerView;
}

void CChildFrame::SetHexerView(CHexerView* pView)
{
	m_pHexerView = pView;
}


//Private methods.

auto CChildFrame::GetMainFrame()const->CMainFrame*
{
	return reinterpret_cast<CMainFrame*>(AfxGetMainWnd());
}

auto CChildFrame::GetHexCtrl()const->HEXCTRL::IHexCtrl*
{
	if (const auto pView = GetHexerView(); pView != nullptr) {
		return pView->GetHexCtrl();
	}

	return { };
}

void CChildFrame::OnClose()
{
	m_fClosing = true;
	CMDIChildWndEx::OnClose();
}

BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	m_fCreating = true;
	++GetMainFrame()->GetChildFramesCount();
	return CMDIChildWndEx::OnCreateClient(lpcs, pContext);
}

void CChildFrame::OnDestroy()
{
	--GetMainFrame()->GetChildFramesCount();
	CMDIChildWndEx::OnDestroy();
}

void CChildFrame::OnFrameActivate()
{
	for (const auto hWnd : m_vecWndHidden) { //Show all hidden dialogs.
		::ShowWindow(hWnd, SW_SHOW);
	}
	m_vecWndHidden.clear();

	GetMainFrame()->OnChildFrameActivate();
}

void CChildFrame::OnFrameDisctivate()
{
	for (const auto eWnd : Ut::g_arrHexDlg) { //Hide all active dialogs.
		if (const auto hWnd = GetHexCtrl()->GetWndHandle(eWnd, false); ::IsWindowVisible(hWnd)) {
			::ShowWindow(hWnd, SW_HIDE);
			m_vecWndHidden.emplace_back(hWnd);
		}
	}

	GetMainFrame()->OnChildFrameDisactivate();
}

void CChildFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
	CMDIChildWndEx::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

	const auto pMainFrame = GetMainFrame();

	if (m_fCreating) {
		if (pMainFrame->GetChildFramesCount() == 1) { //Indicates that the first tab is opening now.
			pMainFrame->OnChildFrameOpenFirst();
			m_fCreating = false;
			return;
		}

		//Buggy MFC sends two WM_MDIACTIVATE messages to the child-frame, when second and subsequent tabs are created.
		//This count is to prevent unnecessary OnChildFrameActivate calls.
		if (--m_iMDIActivateCreation == 0) {
			pMainFrame->OnChildFrameActivate();
			m_fCreating = false;
		}

		return;
	}

	if (m_fClosing) {
		if (pMainFrame->GetChildFramesCount() == 1) { //Indicates that the last tab is closing now.
			pMainFrame->OnChildFrameCloseLast();
		}

		return;
	}

	bActivate ? OnFrameActivate() : OnFrameDisctivate();
}