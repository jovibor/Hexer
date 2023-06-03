/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CHexerApp.h"
#include "CChildFrame.h"
#include "CMainFrame.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWndEx)
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	ON_WM_MDIACTIVATE()
END_MESSAGE_MAP()

void CChildFrame::OnClose()
{
	m_fClosing = true;
	CMDIChildWndEx::OnClose();
}

BOOL CChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	++reinterpret_cast<CMainFrame*>(AfxGetMainWnd())->GetChildFramesCount();
	return CMDIChildWndEx::OnCreateClient(lpcs, pContext);
}

void CChildFrame::OnDestroy()
{
	CMDIChildWndEx::OnDestroy();
	--reinterpret_cast<CMainFrame*>(AfxGetMainWnd())->GetChildFramesCount();
}

void CChildFrame::OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd)
{
	CMDIChildWndEx::OnMDIActivate(bActivate, pActivateWnd, pDeactivateWnd);

	const auto pMainFrame = reinterpret_cast<CMainFrame*>(AfxGetMainWnd());
	//If the tab is closing we don't need to UpdateAllViews.
	//At this moment the Document can already be destroyed in the memory, so GetActiveDocument can point to a bad data.
	if (!m_fClosing) {
		//	GetActiveDocument()->UpdateAllViews(nullptr, bActivate == FALSE ? MSG_MDITAB_DISACTIVATE : MSG_MDITAB_ACTIVATE);
		if (pMainFrame->GetChildFramesCount() == 1) { //Indicates that the first tab is opening now.
			pMainFrame->OnOpenFirstTab();
		}
	}
	else {
		if (pMainFrame->GetChildFramesCount() == 1) { //Indicates that the last tab is closing now.
			pMainFrame->OnCloseLastTab();
		}
	}
}