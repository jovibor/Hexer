/*******************************************************************************
* Copyright © 2023-present Jovibor https://github.com/jovibor/                 *
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
#include <format>

import Utility;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHexerView, CView)

BEGIN_MESSAGE_MAP(CHexerView, CView)
	ON_COMMAND(IDM_FILE_SAVE, &CHexerView::OnFileSave)
	ON_COMMAND(IDM_FILE_PRINT, &CHexerView::OnFilePrint)
	ON_COMMAND(IDM_EDIT_COPYHEX, &CHexerView::OnEditCopyHex)
	ON_COMMAND(IDM_EDIT_PASTEHEX, &CHexerView::OnEditPasteHex)
	ON_COMMAND(IDM_EDIT_UNDO, &CHexerView::OnEditUndo)
	ON_COMMAND(IDM_EDIT_REDO, &CHexerView::OnEditRedo)
	ON_COMMAND(IDM_FIND_SEARCH, &CHexerView::OnFindSearch)
	ON_COMMAND(IDM_VIEW_PROCMEMORY, &CHexerView::OnViewProcMemory)
	ON_COMMAND(IDM_DA_RO, &CHexerView::OnDataAccessRO)
	ON_COMMAND(IDM_DA_RWSAFE, &CHexerView::OnDataAccessRWSAFE)
	ON_COMMAND(IDM_DA_RWINPLACE, &CHexerView::OnDataAccessRWINPLACE)
	ON_COMMAND(IDM_DA_DATAIO_MMAP, &CHexerView::OnDataIOMMAP)
	ON_COMMAND(IDM_DA_DATAIO_IOBUFF, &CHexerView::OnDataIOBuff)
	ON_COMMAND(IDM_DA_DATAIO_IOIMMEDIATE, &CHexerView::OnDataIOImmediate)
	ON_MESSAGE(WM_DPICHANGED_AFTERPARENT, &CHexerView::OnDPIChangedAfterParent)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_DLGBKMMGR, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlDLG)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_DLGDATAINTERP, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlDLG)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_DLGTEMPLMGR, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlDLG)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_SETDATA, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlSetData)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_SETFONT, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlSetFont)
	ON_UPDATE_COMMAND_UI(IDM_FILE_SAVE, &CHexerView::OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(IDM_EDIT_COPYHEX, &CHexerView::OnUpdateEditCopyHex)
	ON_UPDATE_COMMAND_UI(IDM_EDIT_PASTEHEX, &CHexerView::OnUpdateEditPasteHex)
	ON_UPDATE_COMMAND_UI(IDM_EDIT_UNDO, &CHexerView::OnUpdateEditUndo)
	ON_UPDATE_COMMAND_UI(IDM_EDIT_REDO, &CHexerView::OnUpdateEditRedo)
	ON_UPDATE_COMMAND_UI(IDM_FIND_SEARCH, &CHexerView::OnUpdateFindSearch)
	ON_UPDATE_COMMAND_UI(IDM_VIEW_PROCMEMORY, &CHexerView::OnUpdateProcMemory)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_DA_RO, IDM_DA_RWINPLACE, &CHexerView::OnUpdateDataAccessMode)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_DA_DATAIO_MMAP, IDM_DA_DATAIO_IOIMMEDIATE, &CHexerView::OnUpdateDataIOMode)
	ON_WM_SIZE()
END_MESSAGE_MAP()

auto CHexerView::GetDataInfo()const->ut::DATAINFO
{
	const auto pDoc = GetDocument();
	return { .wsvDataPath { pDoc->GetDataPath() }, .wsvFileName { pDoc->GetFileName() },
		.wsvFriendlyName { pDoc->GetFriendlyName() }, .ullDataSize { pDoc->GetDataSize() },
		.dwPageSize { GetHexCtrl()->GetPageSize() }, .eOpenMode { pDoc->GetOpenMode() },
		.stDAC { pDoc->GetDataAccessMode() }, .eDataIOMode { pDoc->GetDataIOMode() } };
}

auto CHexerView::GetDlgProcMemory()const->HWND
{
	return m_dlgProcMem;
}

auto CHexerView::GetDocument()const->CHexerDoc*
{
	return reinterpret_cast<CHexerDoc*>(m_pDocument);
}

auto CHexerView::GetHexCtrl()const->HEXCTRL::IHexCtrl*
{
	return &*m_pHexCtrl;
}

auto CHexerView::GetHWNDForPane(UINT uPaneID)->HWND
{
	//If Pane with HexCtrl's dialog inside.
	if (const auto optDlg = ut::GetEHexWndFromPaneID(uPaneID); optDlg) {
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

bool CHexerView::OnBeforeClose()
{
	bool fClose { true };
	if (m_fIsHexCtrlDataModified) {
		const auto pDoc = GetDocument();
		const auto wstr = std::format(L"\"{}\" was modified.\nSave changes?", pDoc->GetDataPath());
		switch (MessageBoxW(wstr.data(), pDoc->GetFileName().data(), MB_YESNOCANCEL | MB_ICONQUESTION)) {
		case IDYES:
			SaveDataToDisk();
			fClose = true;
			break;
		case IDNO:
			fClose = true;
			break;
		case IDCANCEL:
			fClose = false;
			break;
		default:
			fClose = false;
		}
	}

	if (fClose) {
		HexCtrlSaveBkms();
	}

	return fClose;
}


//Private methods.

void CHexerView::ChangeDataAccessMode(ut::DATAACCESS stDAC)
{
	const auto pDoc = GetDocument();
	if (pDoc->GetDataAccessMode() == stDAC)
		return;

	pDoc->ChangeDataAccessMode(stDAC);
	HexCtrlSetData(true);
	GetMainFrame()->UpdatePaneFileInfo();
	const auto wstr = std::format(L"Data access changed: {} ({})", GetDocument()->GetFileName(),
		ut::GetWstrDATAACCESS(stDAC));
	ut::Log::AddLogEntryInfo(wstr);
}

void CHexerView::ChangeDataIOMode(ut::EDataIOMode eDataIOMode)
{
	const auto pDoc = GetDocument();
	if (pDoc->GetDataIOMode() == eDataIOMode)
		return;

	pDoc->ChangeDataIOMode(eDataIOMode);
	HexCtrlSetData(true);
	GetMainFrame()->UpdatePaneFileInfo();
	const auto wstr = std::format(L"Data IO mode changed: {} ({})", GetDocument()->GetFileName(),
		GetWstrEDataIOMode(pDoc->GetDataIOMode()));
	ut::Log::AddLogEntryInfo(wstr);
}

auto CHexerView::GetMainFrame()const->CMainFrame*
{
	return static_cast<CMainFrame*>(AfxGetMainWnd());
}

auto CHexerView::GetChildFrame()const->CChildFrame*
{
	return static_cast<CChildFrame*>(GetParentFrame());
}

void CHexerView::HexCtrlLoadSavedBkms()
{
	const auto pDoc = GetDocument();
	const auto pHex = GetHexCtrl();
	const auto vecBkm = theApp.GetAppSettings().GetSavedBkms(pDoc->GetDataPath());
	const auto pBkm = pHex->GetBookmarks();
	for (const auto& bkm : vecBkm) {
		pBkm->AddBkm(bkm);
	}
}

void CHexerView::HexCtrlSaveBkms()
{
	const auto pDoc = GetDocument();
	const auto pHex = GetHexCtrl();
	const auto pBkm = pHex->GetBookmarks();
	theApp.GetAppSettings().SaveBkms(pDoc->GetDataPath(), pBkm->GetAllAsArray());
}

void CHexerView::HexCtrlSetData(bool fAdjust)
{
	const auto pDoc = GetDocument();
	const auto fMutable = pDoc->IsDataAccessRW();
	GetHexCtrl()->SetData({ .spnData { pDoc->GetFileMMAPData(), pDoc->GetDataSize() },
		.pHexVirtData { pDoc->GetIHexVirtData() }, .ullMaxVirtOffset { pDoc->GetMaxVirtOffset() },
		.dwCacheSize { pDoc->GetCacheSize() }, .fMutable { fMutable } }, fAdjust);
}

void CHexerView::HexCtrlUpdateIcons()
{
	using enum HEXCTRL::EHexMenuItem;
	const auto pHex = GetHexCtrl();
	const auto& sett = theApp.GetAppSettings();

	MENUITEMINFOW mii { .cbSize { sizeof(MENUITEMINFOW) }, .fMask { MIIM_BITMAP } };
	mii.hbmpItem = sett.GetIconDataForCmd(IDM_FIND_SEARCH)->hBmp;
	pHex->SetMenuItem(IDM_SEARCH_POPUP, mii);
	pHex->SetMenuItem(IDM_SEARCH_SEARCH, mii);
	mii.hbmpItem = sett.GetIconDataForCmd(IDM_EDIT_COPYHEX)->hBmp;
	pHex->SetMenuItem(IDM_CLPBRD_POPUP, mii);
	pHex->SetMenuItem(IDM_CLPBRD_COPYHEX, mii);
	mii.hbmpItem = sett.GetIconDataForCmd(IDM_EDIT_PASTEHEX)->hBmp;
	pHex->SetMenuItem(IDM_CLPBRD_PASTEHEX, mii);
	mii.hbmpItem = sett.GetIconDataForCmd(IDM_VIEW_BKMMGR)->hBmp;
	pHex->SetMenuItem(IDM_BKM_POPUP, mii);
	pHex->SetMenuItem(IDM_BKM_ADD, mii);
	mii.hbmpItem = sett.GetIconDataForCmd(IDR_SVG_MODIFY)->hBmp;
	pHex->SetMenuItem(IDM_MODIFY_POPUP, mii);
	mii.hbmpItem = sett.GetIconDataForCmd(IDR_SVG_FONTCHOOSE)->hBmp;
	pHex->SetMenuItem(IDM_APPEAR_CHOOSEFONT, mii);
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

void CHexerView::OnDataAccessRO()
{
	ChangeDataAccessMode({ 0 });
}

void CHexerView::OnDataAccessRWSAFE()
{
	ChangeDataAccessMode({ 1 });
}

void CHexerView::OnDataAccessRWINPLACE()
{
	ChangeDataAccessMode({ 2 });
}

void CHexerView::OnDataIOMMAP()
{
	ChangeDataIOMode(DATA_MMAP);
}

void CHexerView::OnDataIOBuff()
{
	ChangeDataIOMode(DATA_IOBUFF);
}

void CHexerView::OnDataIOImmediate()
{
	ChangeDataIOMode(DATA_IOIMMEDIATE);
}

auto CHexerView::OnDPIChangedAfterParent(WPARAM /*wParam*/, LPARAM /*lParam*/)->LRESULT
{
	const auto ret = Default();
	HexCtrlUpdateIcons();
	return ret;
}

void CHexerView::OnDraw(CDC* /*pDC*/)
{ }

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

void CHexerView::OnFileSave()
{
	SaveDataToDisk();
}

void CHexerView::OnFindSearch()
{
	GetHexCtrl()->ExecuteCmd(HEXCTRL::EHexCmd::CMD_SEARCH_DLG);
}

void CHexerView::OnHexCtrlDLG(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	UINT uPaneID;
	switch (pNMHDR->code) {
	case HEXCTRL::HEXCTRL_MSG_DLGBKMMGR:
		uPaneID = IDC_PANE_BKMMGR;
		break;
	case HEXCTRL::HEXCTRL_MSG_DLGDATAINTERP:
		uPaneID = IDC_PANE_DATAINTERP;
		break;
	case HEXCTRL::HEXCTRL_MSG_DLGTEMPLMGR:
		uPaneID = IDC_PANE_TEMPLMGR;
		break;
	default:
		return;
	}

	GetMainFrame()->ShowPane(uPaneID, true, true);
}

void CHexerView::OnHexCtrlSetData(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	if (GetDocument()->IsDataAccessRWSAFE()) {
		m_fIsHexCtrlDataModified = true;
	}
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
	const auto& HexSett = theApp.GetAppSettings().GetHexCtrlSettings();
	pHex->Create({ .hWndParent { m_hWnd }, .pColors { &HexSett.stClrs }, .pLogFont { &HexSett.stLogFont },
		.uID { IDC_HEXCTRL_MAIN }, .dwStyle { WS_VISIBLE | WS_CHILD }, .dwCapacity { HexSett.dwCapacity },
		.dwGroupSize { HexSett.dwGroupSize }, .flScrollRatio { HexSett.flScrollRatio },
		.fScrollLines { HexSett.fScrollLines }, .fInfoBar { HexSett.fInfoBar }, .fOffsetHex { HexSett.fOffsetHex } });
	pHex->SetCharsExtraSpace(HexSett.dwCharsExtraSpace);
	pHex->SetDateInfo(HexSett.dwDateFormat, HexSett.wchDateSepar);
	pHex->SetPageSize(pDoc->IsProcess() ? pDoc->GetMemPageSize() : HexSett.dwPageSize);
	pHex->SetUnprintableChar(HexSett.wchUnprintable);
	pHex->SetHexCharsCase(HexSett.fHexCharsCaseUpper);

	for (const auto& p : theApp.GetAppSettings().GetHexCtrlTemplates()) {
		pHex->GetTemplates()->AddTemplate(*p);
	}

	HexCtrlSetData();
	HexCtrlUpdateIcons();
	HexCtrlLoadSavedBkms();
}

void CHexerView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if (GetHexCtrl()->IsCreated()) {
		GetHexCtrl()->SetWindowPos(m_hWnd, 0, 0, cx, cy);
	}
}

void CHexerView::OnUpdate(CView* /*pSender*/, LPARAM lHint, CObject* /*pHint*/)
{
	if (lHint == ut::WM_APP_SETTINGS_CHANGED) {
		const auto& HexSett = theApp.GetAppSettings().GetHexCtrlSettings();
		const auto pHex = GetHexCtrl();
		pHex->SetRedraw(false);
		pHex->SetCapacity(HexSett.dwCapacity);
		pHex->SetGroupSize(HexSett.dwGroupSize);
		pHex->SetPageSize(HexSett.dwPageSize);
		pHex->SetUnprintableChar(HexSett.wchUnprintable);
		pHex->SetDateInfo(HexSett.dwDateFormat, HexSett.wchDateSepar);
		pHex->SetScrollRatio(HexSett.flScrollRatio, HexSett.fScrollLines);
		pHex->ShowInfoBar(HexSett.fInfoBar);
		pHex->SetOffsetMode(HexSett.fOffsetHex);
		pHex->SetHexCharsCase(HexSett.fHexCharsCaseUpper);
		pHex->SetCharsExtraSpace(HexSett.dwCharsExtraSpace);
		pHex->SetFont(HexSett.stLogFont);
		pHex->SetColors(HexSett.stClrs);
		pHex->SetRedraw(true);
		pHex->Redraw();
	}
}

void CHexerView::OnUpdateDataAccessMode(CCmdUI* pCmdUI)
{
	const auto pDoc = GetDocument();
	const auto fRW = pDoc->IsDataWritable();
	const auto stDAC = pDoc->GetDataAccessMode();

	bool fEnable { false };
	bool fCheck { false };
	using enum ut::EDataAccessMode;
	switch (pCmdUI->m_nID) {
	case IDM_DA_RO:
		fEnable = true;
		fCheck = !stDAC.fMutable;
		break;
	case IDM_DA_RWSAFE:
		fEnable = pDoc->IsDataOKForDASAFE();
		fCheck = stDAC.fMutable && stDAC.eDataAccessMode == ACCESS_SAFE;
		break;
	case IDM_DA_RWINPLACE:
		fEnable = fRW;
		fCheck = stDAC.fMutable && stDAC.eDataAccessMode == ACCESS_INPLACE;
		break;
	default:
		break;
	}

	pCmdUI->Enable(fEnable);
	pCmdUI->SetCheck(fCheck);
}

void CHexerView::OnUpdateDataIOMode(CCmdUI* pCmdUI)
{
	const auto pDoc = GetDocument();
	const auto fIsFile = pDoc->IsFile();
	const auto fModeAllowed = pDoc->IsDataAccessRW() && !pDoc->IsDataAccessRWSAFE();
	const auto eDataIOMode = pDoc->GetDataIOMode();
	bool fEnable { false };
	bool fCheck { false };

	using enum ut::EDataIOMode;
	switch (pCmdUI->m_nID) {
	case IDM_DA_DATAIO_MMAP:
		fEnable = fModeAllowed && fIsFile;
		fCheck = fModeAllowed && eDataIOMode == DATA_MMAP;
		break;
	case IDM_DA_DATAIO_IOBUFF:
		fEnable = fModeAllowed && fIsFile;
		fCheck = fModeAllowed && eDataIOMode == DATA_IOBUFF;
		break;
	case IDM_DA_DATAIO_IOIMMEDIATE:
		fEnable = fModeAllowed;
		fCheck = fModeAllowed && eDataIOMode == DATA_IOIMMEDIATE;
		break;
	default:
		break;
	}

	pCmdUI->Enable(fEnable);
	pCmdUI->SetCheck(fCheck);
}

void CHexerView::OnUpdateFileSave(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_fIsHexCtrlDataModified);
}

void CHexerView::OnUpdateEditCopyHex(CCmdUI* pCmdUI)
{
	const auto fEnable = GetHexCtrl()->IsCmdAvail(HEXCTRL::EHexCmd::CMD_CLPBRD_COPY_HEX);
	pCmdUI->Enable(fEnable);
}

void CHexerView::OnUpdateEditPasteHex(CCmdUI* pCmdUI)
{
	const auto fEnable = GetHexCtrl()->IsCmdAvail(HEXCTRL::EHexCmd::CMD_CLPBRD_PASTE_HEX);
	pCmdUI->Enable(fEnable);
}

void CHexerView::OnUpdateEditUndo(CCmdUI* pCmdUI)
{
	const auto fEnable = GetHexCtrl()->IsCmdAvail(HEXCTRL::EHexCmd::CMD_MODIFY_UNDO);
	pCmdUI->Enable(fEnable);
}

void CHexerView::OnUpdateEditRedo(CCmdUI* pCmdUI)
{
	const auto fEnable = GetHexCtrl()->IsCmdAvail(HEXCTRL::EHexCmd::CMD_MODIFY_REDO);
	pCmdUI->Enable(fEnable);
}

void CHexerView::OnUpdateFindSearch(CCmdUI* pCmdUI)
{
	const auto fEnable = GetHexCtrl()->IsCmdAvail(HEXCTRL::EHexCmd::CMD_SEARCH_DLG);
	pCmdUI->Enable(fEnable);
}

void CHexerView::OnUpdateProcMemory(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetDocument()->IsProcess());
	if (::IsWindow(m_dlgProcMem)) {
		pCmdUI->SetCheck(m_dlgProcMem.IsWindowVisible());
	}
}

void CHexerView::OnViewProcMemory()
{
	if (!::IsWindow(m_dlgProcMem)) {
		m_dlgProcMem.Init(GetHexCtrl(), GetDocument()->GetVecProcMemory());
		m_dlgProcMem.Create(IDD_PROCMEMORY, AfxGetMainWnd());
	}
	m_dlgProcMem.ShowWindow(m_dlgProcMem.IsWindowVisible() ? SW_HIDE : SW_SHOW);
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

void CHexerView::SaveDataToDisk()
{
	if (!m_fIsHexCtrlDataModified)
		return;

	GetDocument()->SaveDataToDisk();
	m_fIsHexCtrlDataModified = false;
}

void CHexerView::UpdateDlgBkmMgr()const
{
	using enum HEXCTRL::EHexWnd;
	using enum HEXCTRL::EHexDlgItem;
	const auto pHex = GetHexCtrl();
	const auto u64Data = theApp.GetAppSettings().GetPaneData(IDC_PANE_BKMMGR);

	const auto hWnd = pHex->GetDlgItemHandle(BKMMGR_CHK_HEX);
	const auto pBtn = static_cast<CButton*>(CWnd::FromHandle(hWnd));
	if (pBtn->GetCheck() != (u64Data & ut::HEXCTRL_FLAG_BKMMGR_HEX) > 0) {
		pBtn->SendMessageW(BM_CLICK);
	}
}

void CHexerView::UpdateDlgDataInterp()const
{
	using enum HEXCTRL::EHexWnd;
	using enum HEXCTRL::EHexDlgItem;
	const auto pHex = GetHexCtrl();
	const auto u64Data = theApp.GetAppSettings().GetPaneData(IDC_PANE_DATAINTERP);

	const auto hWndHex = pHex->GetDlgItemHandle(DATAINTERP_CHK_HEX);
	const auto pBtnHex = static_cast<CButton*>(CWnd::FromHandle(hWndHex));
	if (pBtnHex->GetCheck() != (u64Data & ut::HEXCTRL_FLAG_DATAINTERP_HEX) > 0) {
		pBtnHex->SendMessageW(BM_CLICK);
	}

	const auto hWndBE = pHex->GetDlgItemHandle(DATAINTERP_CHK_BE);
	const auto pBtnBE = static_cast<CButton*>(CWnd::FromHandle(hWndBE));
	if (pBtnBE->GetCheck() != (u64Data & ut::HEXCTRL_FLAG_DATAINTERP_BE) > 0) {
		pBtnBE->SendMessageW(BM_CLICK);
	}
}

void CHexerView::UpdateDlgModify()const
{ }

void CHexerView::UpdateDlgSearch()const
{ }

void CHexerView::UpdateDlgTemplMgr()const
{
	using enum HEXCTRL::EHexWnd;
	using enum HEXCTRL::EHexDlgItem;
	const auto pHex = GetHexCtrl();
	const auto u64Data = theApp.GetAppSettings().GetPaneData(IDC_PANE_TEMPLMGR);

	const auto hWndMin = pHex->GetDlgItemHandle(TEMPLMGR_CHK_MIN);
	const auto pBtnMin = static_cast<CButton*>(CWnd::FromHandle(hWndMin));
	if (pBtnMin->GetCheck() != (u64Data & ut::HEXCTRL_FLAG_TEMPLMGR_MIN) > 0) {
		pBtnMin->SendMessageW(BM_CLICK);
	}

	const auto hWndHex = pHex->GetDlgItemHandle(TEMPLMGR_CHK_HEX);
	const auto pBtnHex = static_cast<CButton*>(CWnd::FromHandle(hWndHex));
	if (pBtnHex->GetCheck() != (u64Data & ut::HEXCTRL_FLAG_TEMPLMGR_HEX) > 0) {
		pBtnHex->SendMessageW(BM_CLICK);
	}

	const auto hWndTT = pHex->GetDlgItemHandle(TEMPLMGR_CHK_TT);
	const auto pBtnTT = static_cast<CButton*>(CWnd::FromHandle(hWndTT));
	if (pBtnTT->GetCheck() != (u64Data & ut::HEXCTRL_FLAG_TEMPLMGR_TT) > 0) {
		pBtnTT->SendMessageW(BM_CLICK);
	}

	const auto hWndHgl = pHex->GetDlgItemHandle(TEMPLMGR_CHK_HGL);
	const auto pBtnHgl = static_cast<CButton*>(CWnd::FromHandle(hWndHgl));
	if (pBtnHgl->GetCheck() != (u64Data & ut::HEXCTRL_FLAG_TEMPLMGR_HGL) > 0) {
		pBtnHgl->SendMessageW(BM_CLICK);
	}

	const auto hWndSwap = pHex->GetDlgItemHandle(TEMPLMGR_CHK_SWAP);
	const auto pBtnSwap = static_cast<CButton*>(CWnd::FromHandle(hWndSwap));
	if (pBtnSwap->GetCheck() != (u64Data & ut::HEXCTRL_FLAG_TEMPLMGR_SWAP) > 0) {
		pBtnSwap->SendMessageW(BM_CLICK);
	}
}

void CHexerView::UpdateHexCtrlDlgData(UINT uPaneID)const
{
	using enum HEXCTRL::EHexWnd;
	switch (*ut::GetEHexWndFromPaneID(uPaneID)) {
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