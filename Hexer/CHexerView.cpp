/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CHexerApp.h"
#include "CMainFrame.h"
#include "CChildFrame.h"
#include "CHexerDoc.h"
#include "CHexerView.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHexerView, CView)

BEGIN_MESSAGE_MAP(CHexerView, CView)
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CHexerView::OnFilePrintPreview)
	ON_COMMAND(IDM_EDIT_EDITMODE, &CHexerView::OnEditEditMode)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_DLGBKMMGR, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlDLG)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_DLGDATAINTERP, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlDLG)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_DLGTEMPLMGR, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlDLG)
	ON_UPDATE_COMMAND_UI(IDM_EDIT_EDITMODE, &CHexerView::OnUpdateEditEditMode)
	ON_WM_SIZE()
END_MESSAGE_MAP()

auto CHexerView::GetFileInfo()const->Ut::FILEINFO
{
	const auto pDoc = GetDocument();
	return { .wsvFilePath = pDoc->GetFilePath(),
		.wsvFileName = pDoc->GetFileName(), .ullFileSize = pDoc->GetFileSize(),
		.dwPageSize = GetHexCtrl()->GetPageSize(), .fMutable = GetHexCtrl()->IsMutable() };
}

auto CHexerView::GetHexCtrl()const->HEXCTRL::IHexCtrl*
{
	return &*m_pHexCtrl;
}

auto CHexerView::GetHWNDForPane(UINT uPaneID)->HWND
{
	//If Pane with HexCtrl's dialog inside.
	if (const auto optDlg = Ut::GetEHexWndFromPaneID(uPaneID); optDlg) {
		if (!IsPaneAlreadyLaunch(uPaneID)) {
			SetPaneAlreadyLaunch(uPaneID);
			return GetHexCtrl()->SetDlgData(*optDlg, theApp.GetAppSettings().GetPaneData(uPaneID));
		}
		return GetHexCtrl()->GetWindowHandle(*optDlg);
	}

	return { };
}


//Private methods.

auto CHexerView::GetMainFrame()const->CMainFrame*
{
	return static_cast<CMainFrame*>(AfxGetMainWnd());
}

auto CHexerView::GetChildFrame()const->CChildFrame*
{
	return static_cast<CChildFrame*>(GetParentFrame());
}

auto CHexerView::GetDocument()const->CHexerDoc*
{
	return reinterpret_cast<CHexerDoc*>(m_pDocument);
}

bool CHexerView::IsPaneAlreadyLaunch(UINT uPaneID)const
{
	switch (uPaneID) {
	case IDC_PANE_BKMMGR:
		return m_fIsAlreadyLaunchDlgBkmMgr;
	case IDC_PANE_DATAINTERP:
		return m_fIsAlreadyLaunchDlgDataInterp;
	case IDC_PANE_TEMPLMGR:
		return m_fIsAlreadyLaunchDlgTemplMgr;
	default:
		return { };
	}
}

void CHexerView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}

void CHexerView::OnDraw(CDC* /*pDC*/)
{
}

void CHexerView::OnEditEditMode()
{
	GetHexCtrl()->SetMutable(!GetHexCtrl()->IsMutable());
	GetMainFrame()->UpdatePaneFileInfo();
}

void CHexerView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

void CHexerView::OnHexCtrlDLG(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	UINT uPaineID;
	switch (pNMHDR->code) {
	case HEXCTRL::HEXCTRL_MSG_DLGBKMMGR:
		uPaineID = IDC_PANE_BKMMGR;
		break;
	case HEXCTRL::HEXCTRL_MSG_DLGDATAINTERP:
		uPaineID = IDC_PANE_DATAINTERP;
		break;
	case HEXCTRL::HEXCTRL_MSG_DLGTEMPLMGR:
		uPaineID = IDC_PANE_TEMPLMGR;
		break;
	default:
		return;
	}

	GetMainFrame()->ShowPane(uPaineID, true, true);
}

void CHexerView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	GetChildFrame()->SetHexerView(this);
	const auto pDoc = GetDocument();
	GetHexCtrl()->Create({ .hWndParent { m_hWnd }, .uID { IDC_HEXCTRL_MAIN }, .dwStyle { WS_VISIBLE | WS_CHILD } });
	GetHexCtrl()->SetData({ .spnData{ std::span<std::byte>{ pDoc->GetFileData(), pDoc->GetFileSize() } },
		 .pHexVirtData { pDoc->GetVirtualInterface() }, .dwCacheSize { pDoc->GetCacheSize() }, .fMutable { pDoc->IsFileMutable() } });
}

void CHexerView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if (!GetHexCtrl()->IsCreated()) {
		return;
	}

	::SetWindowPos(GetHexCtrl()->GetWindowHandle(HEXCTRL::EHexWnd::WND_MAIN), m_hWnd, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CHexerView::OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/)
{
}

void CHexerView::OnUpdateEditEditMode(CCmdUI* pCmdUI)
{
	if (GetDocument()->IsFileMutable()) {
		pCmdUI->Enable(TRUE);
		pCmdUI->SetCheck(GetHexCtrl()->IsMutable());
	}
	else {
		pCmdUI->Enable(FALSE);
		pCmdUI->SetCheck(FALSE);
	}
}

void CHexerView::SetPaneAlreadyLaunch(UINT uPaneID)
{
	switch (uPaneID) {
	case IDC_PANE_BKMMGR:
		m_fIsAlreadyLaunchDlgBkmMgr = true;
		break;
	case IDC_PANE_DATAINTERP:
		m_fIsAlreadyLaunchDlgDataInterp = true;
		break;
	case IDC_PANE_TEMPLMGR:
		m_fIsAlreadyLaunchDlgTemplMgr = true;
		break;
	default:
		break;
	}
}