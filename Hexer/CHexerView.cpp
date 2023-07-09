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
#include <algorithm>
#include <format>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//CHexerPropGridCtrl.
BEGIN_MESSAGE_MAP(CHexerPropGridCtrl, CMFCPropertyGridCtrl)
	ON_WM_SIZE()
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(CHexerView, CView)

BEGIN_MESSAGE_MAP(CHexerView, CView)
	ON_WM_SIZE()
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CHexerView::OnFilePrintPreview)
	ON_COMMAND(IDM_EDIT_EDITMODE, &CHexerView::OnEditEditMode)
	ON_UPDATE_COMMAND_UI(IDM_EDIT_EDITMODE, &CHexerView::OnUpdateEditEditMode)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_DLGDATAINTERP, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlDLG)
	ON_NOTIFY(HEXCTRL::HEXCTRL_MSG_DLGTEMPLMGR, IDC_HEXCTRL_MAIN, &CHexerView::OnHexCtrlDLG)
END_MESSAGE_MAP()

auto CHexerView::GetHexCtrl()const->HEXCTRL::IHexCtrl*
{
	return &*m_pHexCtrl;
}

auto CHexerView::GetHWNDForPane(UINT uPaneID)->HWND
{
	//If Pane with HexCtrl's dialog inside.
	if (const auto optDlg = Ut::PaneIDToEHexWnd(uPaneID); optDlg) {
		if (!IsPaneAlreadyLaunch(uPaneID)) {
			SetPaneAlreadyLaunch(uPaneID);
			return GetHexCtrl()->SetDlgData(*optDlg, theApp.GetAppSettings().GetPaneData(uPaneID));
		}
		return GetHexCtrl()->GetWindowHandle(*optDlg);
	}

	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		if (!IsPaneAlreadyLaunch(uPaneID)) {
			SetPaneAlreadyLaunch(uPaneID);
			return CreateGridFileProps();
		}
		return m_stGridFileProps;
	default:
		return { };
	}
}


//Private methods.

auto CHexerView::CreateGridFileProps()->HWND
{
	m_stGridFileProps.Create(WS_VISIBLE | WS_CHILD, RECT { 0, 0, 100, 100 }, this, IDC_GRID_FILEPROPS);
	m_stGridFileProps.SetVSDotNetLook();
	m_stGridFileProps.EnableHeaderCtrl(TRUE, L"Property", L"Value");
	HDITEMW hdPropGrid { .mask = HDI_WIDTH, .cxy = 80 };
	m_stGridFileProps.GetHeaderCtrl().SetItem(0, &hdPropGrid); //Property grid left column width.

	//Set new bigger font to the property.
	const auto pFont = m_stGridFileProps.GetFont();
	LOGFONTW lf { };
	pFont->GetLogFont(&lf);
	const auto lFontSize = MulDiv(-lf.lfHeight, 72, Ut::GetHiDPIInfo().iLOGPIXELSY) + 2;
	lf.lfHeight = -MulDiv(lFontSize, Ut::GetHiDPIInfo().iLOGPIXELSY, 72);
	m_fntFilePropsGrid.CreateFontIndirectW(&lf);
	m_stGridFileProps.SetFont(&m_fntFilePropsGrid);

	using enum EPropName;
	const auto pFilePath = new CMFCPropertyGridProperty(L"File path:", L"");
	pFilePath->SetData(static_cast<DWORD_PTR>(FILE_PATH));
	pFilePath->AllowEdit(FALSE);
	m_vecPropsFileProps.emplace_back(pFilePath);
	m_stGridFileProps.AddProperty(pFilePath);

	const auto pFileName = new CMFCPropertyGridProperty(L"File name:", L"");
	pFileName->SetData(static_cast<DWORD_PTR>(FILE_NAME));
	pFileName->AllowEdit(FALSE);
	m_vecPropsFileProps.emplace_back(pFileName);
	m_stGridFileProps.AddProperty(pFileName);

	const auto pFileSize = new CMFCPropertyGridProperty(L"File size:", L"");
	pFileSize->SetData(static_cast<DWORD_PTR>(FILE_SIZE));
	pFileSize->AllowEdit(FALSE);
	m_vecPropsFileProps.emplace_back(pFileSize);
	m_stGridFileProps.AddProperty(pFileSize);

	const auto pPageSize = new CMFCPropertyGridProperty(L"Page size:", L"");
	pPageSize->SetData(static_cast<DWORD_PTR>(PAGE_SIZE));
	pPageSize->AllowEdit(FALSE);
	m_vecPropsFileProps.emplace_back(pPageSize);
	m_stGridFileProps.AddProperty(pPageSize);
	pPageSize->Show(FALSE);

	const auto pIsWritable = new CMFCPropertyGridProperty(L"Writable:", L"");
	pIsWritable->SetData(static_cast<DWORD_PTR>(IS_MUTABLE));
	pIsWritable->AllowEdit(FALSE);
	m_vecPropsFileProps.emplace_back(pIsWritable);
	m_stGridFileProps.AddProperty(pIsWritable);

	UpdateGridFileProps(); //Set initial values.

	return m_stGridFileProps;
}

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
	case IDC_PANE_FILEPROPS:
		return m_fIsAlreadyLaunchGridFileProps;
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

void CHexerView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	GetChildFrame()->SetHexerView(this);
	const auto pDoc = GetDocument();
	GetHexCtrl()->Create({ .hWndParent { m_hWnd }, .uID { IDC_HEXCTRL_MAIN }, .dwStyle { WS_VISIBLE | WS_CHILD } });
	GetHexCtrl()->SetData({ .spnData{ std::span<std::byte>{ pDoc->GetFileData(), pDoc->GetFileSize() } },
		 .pHexVirtData { pDoc->GetVirtualInterface() }, .dwCacheSize { pDoc->GetCacheSize() }, .fMutable { pDoc->IsFileMutable() } });
}

void CHexerView::OnUpdate(CView* /*pSender*/, LPARAM /*lHint*/, CObject* /*pHint*/)
{
}

void CHexerView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);

	if (!GetHexCtrl()->IsCreated()) {
		return;
	}

	::SetWindowPos(GetHexCtrl()->GetWindowHandle(HEXCTRL::EHexWnd::WND_MAIN), m_hWnd, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CHexerView::OnDraw(CDC* /*pDC*/)
{
}

void CHexerView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

void CHexerView::OnEditEditMode()
{
	GetHexCtrl()->SetMutable(!GetHexCtrl()->IsMutable());
	UpdateGridFileProps();
}

void CHexerView::OnHexCtrlDLG(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	UINT uPaineID;
	switch (pNMHDR->code) {
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
	case IDC_PANE_FILEPROPS:
		m_fIsAlreadyLaunchGridFileProps = true;
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

void CHexerView::UpdateGridFileProps()
{
	const auto pDoc = GetDocument();
	const auto lmbSetValue = [&](CMFCPropertyGridProperty* pProp) {
		using enum EPropName;
		switch (static_cast<EPropName>(pProp->GetData())) {
		case FILE_PATH:
			pProp->SetValue(pDoc->GetFilePath().data());
			break;
		case FILE_NAME:
			pProp->SetValue(pDoc->GetFileName().data());
			break;
		case FILE_SIZE:
			pProp->SetValue(std::format(std::locale("en_US.UTF-8"), L"{:L} bytes", pDoc->GetFileSize()).data());
			break;
		case PAGE_SIZE:
			pProp->SetValue(std::format(L"{}", GetHexCtrl()->GetPageSize()).data());
			break;
		case IS_MUTABLE:
			pProp->SetValue(std::format(L"{}", GetHexCtrl()->IsMutable()).data());
			break;
		default:
			break;
		}
	};
	std::ranges::for_each(m_vecPropsFileProps, lmbSetValue);
}