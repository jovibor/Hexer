/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
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

import Utility;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHexerView, CView)

BEGIN_MESSAGE_MAP(CHexerView, CView)
	ON_COMMAND(IDM_FILE_PRINT, &CHexerView::OnFilePrint)
	ON_COMMAND(IDM_EDIT_EDITMODE, &CHexerView::OnEditEditMode)
	ON_COMMAND(IDM_EDIT_UNDO, &CHexerView::OnEditUndo)
	ON_COMMAND(IDM_EDIT_REDO, &CHexerView::OnEditRedo)
	ON_COMMAND(IDM_EDIT_COPYHEX, &CHexerView::OnEditCopyHex)
	ON_COMMAND(IDM_EDIT_PASTEHEX, &CHexerView::OnEditPasteHex)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_DLGBKMMGR, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlDLG)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_DLGDATAINTERP, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlDLG)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_DLGTEMPLMGR, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlDLG)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_SETFONT, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlSetFont)
	ON_UPDATE_COMMAND_UI(IDM_EDIT_EDITMODE, &CHexerView::OnUpdateEditEditMode)
	ON_WM_SIZE()
END_MESSAGE_MAP()

auto CHexerView::GetDataInfo()const->Ut::DATAINFO
{
	const auto pDoc = GetDocument();
	return { .wsvDataPath = pDoc->GetDataPath(), .wsvFileName = pDoc->GetFileName(), .ullDataSize = pDoc->GetDataSize(),
		.dwPageSize = GetHexCtrl()->GetPageSize(), .eMode { pDoc->GetOpenMode() }, .fMutable = GetHexCtrl()->IsMutable() };
}

auto CHexerView::GetHexCtrl()const->HEXCTRL::IHexCtrl*
{
	return &*m_pHexCtrl;
}

auto CHexerView::GetHWNDForPane(UINT uPaneID)->HWND
{
	//If Pane with HexCtrl's dialog inside.
	if (const auto optDlg = Ut::GetEHexWndFromPaneID(uPaneID); optDlg) {
		const auto hWnd = GetHexCtrl()->GetWndHandle(*optDlg);
		if (!IsPaneAlreadyLaunch(uPaneID)) {
			SetPaneAlreadyLaunch(uPaneID);
			UpdateHexCtrlDlgData(uPaneID);
			GetHexCtrl()->SetDlgProperties(*optDlg, HEXCTRL::HEXCTRL_FLAG_DLG_NOESC);
		}

		return hWnd;
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
	const auto fNewAccess = !GetHexCtrl()->IsMutable();
	GetHexCtrl()->SetMutable(fNewAccess);
	GetMainFrame()->UpdatePaneFileInfo();

	const auto pDoc = GetDocument();
	Ut::Log::AddLogEntryInfo(L"File access changed: " + pDoc->GetFileName() + std::wstring { fNewAccess ? L" (RW)" : L" (RO)" });
}

void CHexerView::OnEditCopyHex()
{
	GetHexCtrl()->ExecuteCmd(HEXCTRL::EHexCmd::CMD_CLPBRD_COPY_HEX);
}

void CHexerView::OnEditPasteHex()
{
	GetHexCtrl()->ExecuteCmd(HEXCTRL::EHexCmd::CMD_CLPBRD_PASTE_HEX);
}

void CHexerView::OnEditUndo()
{
	GetHexCtrl()->ExecuteCmd(HEXCTRL::EHexCmd::CMD_MODIFY_UNDO);
}

void CHexerView::OnEditRedo()
{
	GetHexCtrl()->ExecuteCmd(HEXCTRL::EHexCmd::CMD_MODIFY_REDO);
}

void CHexerView::OnFilePrint()
{
	GetHexCtrl()->ExecuteCmd(HEXCTRL::EHexCmd::CMD_PRINT_DLG);
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

void CHexerView::OnHexCtrlSetFont(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	theApp.GetAppSettings().GetHexCtrlSettings().stLogFont = GetHexCtrl()->GetFont();
	theApp.GetAppSettings().GetHexCtrlSettings().stClrs = GetHexCtrl()->GetColors(); //Because font color could be changed.
}

void CHexerView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	GetChildFrame()->SetHexerView(this);
	const auto pDoc = GetDocument();
	const auto pHex = GetHexCtrl();
	const auto& refHexSet = theApp.GetAppSettings().GetHexCtrlSettings();
	pHex->Create({ .hWndParent { m_hWnd }, .pColors { &refHexSet.stClrs }, .pLogFont { &refHexSet.stLogFont },
		.uID { IDC_HEXCTRL_MAIN }, .dwStyle { WS_VISIBLE | WS_CHILD }, .dwCapacity { refHexSet.dwCapacity },
		.dwGroupSize { refHexSet.dwGroupSize }, .flScrollRatio { refHexSet.flScrollRatio }, .fScrollLines { refHexSet.fScrollLines },
		.fInfoBar { refHexSet.fInfoBar }, .fOffsetHex { refHexSet.fOffsetHex } });
	pHex->SetCharsExtraSpace(refHexSet.dwCharsExtraSpace);
	pHex->SetDateInfo(refHexSet.dwDateFormat, refHexSet.wchDateSepar);
	pHex->SetPageSize(pDoc->IsProcess() ? pDoc->GetMemPageSize() : refHexSet.dwPageSize);
	pHex->SetUnprintableChar(refHexSet.wchUnprintable);
	for (const auto& p : theApp.GetAppSettings().GetHexCtrlTemplates()) {
		pHex->GetTemplates()->AddTemplate(*p);
	}
	pHex->SetData({ .spnData { std::span<std::byte>{ pDoc->GetFileMapData(), pDoc->GetDataSize() } },
		.pHexVirtData { pDoc->GetVirtualInterface() }, .ullMaxVirtOffset { pDoc->GetMaxVirtOffset() },
		.dwCacheSize { pDoc->GetCacheSize() }, .fMutable { pDoc->IsFileMutable() } });
}

void CHexerView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if (!GetHexCtrl()->IsCreated()) {
		return;
	}

	::SetWindowPos(GetHexCtrl()->GetWndHandle(HEXCTRL::EHexWnd::WND_MAIN), m_hWnd, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CHexerView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
	if (lHint == Ut::WM_APP_SETTINGS_CHANGED) {
		const auto& refHexSet = theApp.GetAppSettings().GetHexCtrlSettings();
		const auto pHex = GetHexCtrl();
		pHex->SetRedraw(false);
		pHex->SetColors(refHexSet.stClrs);
		pHex->SetFont(refHexSet.stLogFont);
		pHex->SetGroupSize(refHexSet.dwGroupSize);
		pHex->SetCapacity(refHexSet.dwCapacity);
		pHex->SetScrollRatio(refHexSet.flScrollRatio, refHexSet.fScrollLines);
		pHex->ShowInfoBar(refHexSet.fInfoBar);
		pHex->SetOffsetMode(refHexSet.fOffsetHex);
		pHex->SetCharsExtraSpace(refHexSet.dwCharsExtraSpace);
		pHex->SetDateInfo(refHexSet.dwDateFormat, refHexSet.wchDateSepar);
		pHex->SetPageSize(refHexSet.dwPageSize);
		pHex->SetUnprintableChar(refHexSet.wchUnprintable);
		pHex->SetRedraw(true);
		pHex->Redraw();
	}
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

void CHexerView::UpdateDlgBkmMgr()const
{
	using enum HEXCTRL::EHexWnd;
	using enum HEXCTRL::EHexDlgItem;
	const auto pHex = GetHexCtrl();
	const auto u64Data = theApp.GetAppSettings().GetPaneData(IDC_PANE_BKMMGR);

	const auto hWnd = pHex->GetDlgItemHandle(DLG_BKMMGR, BKMMGR_CHK_HEX);
	const auto pBtn = static_cast<CButton*>(CWnd::FromHandle(hWnd));
	if (pBtn->GetCheck() != (u64Data & Ut::HEXCTRL_FLAG_BKMMGR_HEX) > 0) {
		pBtn->SendMessageW(BM_CLICK);
	}
}

void CHexerView::UpdateDlgDataInterp()const
{
	using enum HEXCTRL::EHexWnd;
	using enum HEXCTRL::EHexDlgItem;
	const auto pHex = GetHexCtrl();
	const auto u64Data = theApp.GetAppSettings().GetPaneData(IDC_PANE_DATAINTERP);

	const auto hWndHex = pHex->GetDlgItemHandle(DLG_DATAINTERP, DATAINTERP_CHK_HEX);
	const auto pBtnHex = static_cast<CButton*>(CWnd::FromHandle(hWndHex));
	if (pBtnHex->GetCheck() != (u64Data & Ut::HEXCTRL_FLAG_DATAINTERP_HEX) > 0) {
		pBtnHex->SendMessageW(BM_CLICK);
	}

	const auto hWndBE = pHex->GetDlgItemHandle(DLG_DATAINTERP, DATAINTERP_CHK_BE);
	const auto pBtnBE = static_cast<CButton*>(CWnd::FromHandle(hWndBE));
	if (pBtnBE->GetCheck() != (u64Data & Ut::HEXCTRL_FLAG_DATAINTERP_BE) > 0) {
		pBtnBE->SendMessageW(BM_CLICK);
	}
}

void CHexerView::UpdateDlgModify()const
{
}

void CHexerView::UpdateDlgSearch()const
{
}

void CHexerView::UpdateDlgTemplMgr()const
{
	using enum HEXCTRL::EHexWnd;
	using enum HEXCTRL::EHexDlgItem;
	const auto pHex = GetHexCtrl();
	const auto u64Data = theApp.GetAppSettings().GetPaneData(IDC_PANE_TEMPLMGR);

	const auto hWndMin = pHex->GetDlgItemHandle(DLG_TEMPLMGR, TEMPLMGR_CHK_MIN);
	const auto pBtnMin = static_cast<CButton*>(CWnd::FromHandle(hWndMin));
	if (pBtnMin->GetCheck() != (u64Data & Ut::HEXCTRL_FLAG_TEMPLMGR_MIN) > 0) {
		pBtnMin->SendMessageW(BM_CLICK);
	}

	const auto hWndHex = pHex->GetDlgItemHandle(DLG_TEMPLMGR, TEMPLMGR_CHK_HEX);
	const auto pBtnHex = static_cast<CButton*>(CWnd::FromHandle(hWndHex));
	if (pBtnHex->GetCheck() != (u64Data & Ut::HEXCTRL_FLAG_TEMPLMGR_HEX) > 0) {
		pBtnHex->SendMessageW(BM_CLICK);
	}

	const auto hWndTT = pHex->GetDlgItemHandle(DLG_TEMPLMGR, TEMPLMGR_CHK_TT);
	const auto pBtnTT = static_cast<CButton*>(CWnd::FromHandle(hWndTT));
	if (pBtnTT->GetCheck() != (u64Data & Ut::HEXCTRL_FLAG_TEMPLMGR_TT) > 0) {
		pBtnTT->SendMessageW(BM_CLICK);
	}

	const auto hWndHgl = pHex->GetDlgItemHandle(DLG_TEMPLMGR, TEMPLMGR_CHK_HGL);
	const auto pBtnHgl = static_cast<CButton*>(CWnd::FromHandle(hWndHgl));
	if (pBtnHgl->GetCheck() != (u64Data & Ut::HEXCTRL_FLAG_TEMPLMGR_HGL) > 0) {
		pBtnHgl->SendMessageW(BM_CLICK);
	}

	const auto hWndSwap = pHex->GetDlgItemHandle(DLG_TEMPLMGR, TEMPLMGR_CHK_SWAP);
	const auto pBtnSwap = static_cast<CButton*>(CWnd::FromHandle(hWndSwap));
	if (pBtnSwap->GetCheck() != (u64Data & Ut::HEXCTRL_FLAG_TEMPLMGR_SWAP) > 0) {
		pBtnSwap->SendMessageW(BM_CLICK);
	}
}

void CHexerView::UpdateHexCtrlDlgData(UINT uPaneID)const
{
	using enum HEXCTRL::EHexWnd;
	switch (*Ut::GetEHexWndFromPaneID(uPaneID)) {
	case DLG_BKMMGR:
		UpdateDlgBkmMgr();
		break;
	case DLG_DATAINTERP:
		UpdateDlgDataInterp();
		break;
	case DLG_MODIFY:
		UpdateDlgModify();
		break;
	case DLG_SEARCH:
		UpdateDlgSearch();
		break;
	case DLG_TEMPLMGR:
		UpdateDlgTemplMgr();
		break;
	default:
		break;
	}
}