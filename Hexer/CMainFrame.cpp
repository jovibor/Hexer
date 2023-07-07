/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "resource.h"
#include "CHexerApp.h"
#include "CMainFrame.h"
#include "CChildFrame.h"
#include "CHexerView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_COMMAND(IDM_TOOLBAR_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_COMMAND(IDM_VIEW_FILEPROPS, &CMainFrame::OnViewFileProps)
	ON_COMMAND(IDM_VIEW_DATAINTERP, &CMainFrame::OnViewDataInterp)
	ON_COMMAND(IDM_VIEW_TEMPLMGR, &CMainFrame::OnViewTemplMgr)
	ON_UPDATE_COMMAND_UI(IDM_VIEW_FILEPROPS, &CMainFrame::OnUpdateViewFileProps)
	ON_UPDATE_COMMAND_UI(IDM_VIEW_DATAINTERP, &CMainFrame::OnUpdateViewDataInterp)
	ON_UPDATE_COMMAND_UI(IDM_VIEW_TEMPLMGR, &CMainFrame::OnUpdateViewTemplMgr)
END_MESSAGE_MAP()

int& CMainFrame::GetChildFramesCount()
{
	return m_iChildFrames;
}

bool CMainFrame::IsPaneActive(UINT uPaneID)const
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		return m_paneFileProps.IsPaneVisible();
	case IDC_PANE_DATAINTERP:
		return m_paneDataInterp.IsPaneVisible();
	case IDC_PANE_TEMPLMGR:
		return m_paneTemplMgr.IsPaneVisible();
	default:
		return{ };
	}
}

bool CMainFrame::IsPaneVisible(UINT uPaneID)const
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		return m_paneFileProps.IsVisible();
	case IDC_PANE_DATAINTERP:
		return m_paneDataInterp.IsVisible();
	case IDC_PANE_TEMPLMGR:
		return m_paneTemplMgr.IsVisible();
	default:
		return{ };
	}
}

void CMainFrame::OnChildFrameActivate()
{
	//The ON_REGISTERED_MESSAGE(AFX_WM_CHANGE_ACTIVE_TAB,...) message does in fact work,
	//but this message is generated for ANY tab that is being activated in ANY tab-control,
	//not only in the main CMainFrame tab-control. 
	//Docking Panes that are combined in a tab group also send this message to the CMainFrame window.

	//Setting Panes' HWND according to the current active ChildFrame.
	if (HasChildFrame()) {
		for (auto uPaneID : Utility::g_arrPanes) {
			if (IsPaneVisible(uPaneID)) {
				ShowPane(uPaneID, true, IsPaneActive(uPaneID));
			}
		}
	}
}

void CMainFrame::OnChildFrameCloseLast()
{
	SavePanesSettings(); //It's called either here or in the OnClose.
	HideAllPanes(); //To disable panes from showing at the next app's start-up.
}

void CMainFrame::OnChildFrameFirstOpen()
{
	for (auto id : Utility::g_arrPanes) {
		if (const auto ps = theApp.GetAppSettings().GetPaneStatus(id); ps.fIsVisible) {
			ShowPane(id, true, ps.fIsActive);
		}
	}
}

void CMainFrame::ShowPane(UINT uPaneID, bool fShow, bool fActivate)
{
	CHexerDockablePane* pPane { };
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		pPane = &m_paneFileProps;
		break;
	case IDC_PANE_DATAINTERP:
		pPane = &m_paneDataInterp;
		break;
	case IDC_PANE_TEMPLMGR:
		pPane = &m_paneTemplMgr;
		break;
	default:
		return;
	}

	if (fShow) {
		const auto pView = GetHexerView();
		if (pView == nullptr) {
			return;
		}

		const auto hWndForPane = pView->GetHWNDForPane(uPaneID);
		const auto hWndCurr = pPane->GetNestedHWND();
		if (hWndForPane != nullptr && hWndForPane != hWndCurr) {
			pPane->SetNestedHWND(hWndForPane);
		}
	}

	pPane->ShowPane(fShow, FALSE, fActivate);
}


//Private methods.

auto CMainFrame::GetHexCtrl()->HEXCTRL::IHexCtrl*
{
	if (const auto pView = GetHexerView(); pView != nullptr) {
		return pView->GetHexCtrl();
	}

	return { };
}

auto CMainFrame::GetHexerView()->CHexerView*
{
	//If the MDI frame window has no active document, the implicit "this" pointer will be returned.
	//https://docs.microsoft.com/en-us/cpp/mfc/reference/cframewnd-class?view=msvc-160#getactiveframe
	if (const auto pFrame = GetActiveFrame(); pFrame != nullptr && pFrame != this) {
		return static_cast<CChildFrame*>(pFrame)->GetHexerView();
	}

	return { };
}

bool CMainFrame::HasChildFrame()
{
	return GetChildFramesCount() > 0;
}

void CMainFrame::HideAllPanes()
{
	m_paneFileProps.ShowPane(FALSE, FALSE, FALSE);
	m_paneDataInterp.ShowPane(FALSE, FALSE, FALSE);
	m_paneTemplMgr.ShowPane(FALSE, FALSE, FALSE);
}

void CMainFrame::OnClose()
{
	m_fClosing = true;
	SavePanesSettings(); //It's called either here or in the OnChildFrameCloseLast.
	HideAllPanes(); //To disable panes from showing at the next app's start-up.

	CMDIFrameWndEx::OnClose();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpcs)
{
	if (CMDIFrameWndEx::OnCreate(lpcs) == -1)
		return -1;

	CMDITabInfo mdiTabParams { };
	mdiTabParams.m_style = CMFCTabCtrl::STYLE_3D_ONENOTE; // other styles available...
	mdiTabParams.m_bActiveTabCloseButton = TRUE;          // set to FALSE to place close button at right of tab area
	mdiTabParams.m_bTabIcons = FALSE;     //set to TRUE to enable document icons on MDI taba
	mdiTabParams.m_bAutoColor = FALSE;    //set to FALSE to disable auto-coloring of MDI tabs
	mdiTabParams.m_bDocumentMenu = FALSE; //enable the document menu at the right edge of the tab area
	mdiTabParams.m_bFlatFrame = TRUE;
	mdiTabParams.m_bEnableTabSwap = TRUE;
	mdiTabParams.m_nTabBorderSize = 0;
	EnableMDITabbedGroups(TRUE, mdiTabParams);
	EnableDocking(CBRS_ALIGN_ANY);
	CPaneDivider::m_pSliderRTC = RUNTIME_CLASS(CHexerPaneDivider);

	m_wndToolBar.CreateEx(this, TBSTYLE_FLAT,
		WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndToolBar.LoadToolBar(IDR_TOOLBAR_MAIN);

	CMFCToolBar::m_bDontScaleImages = TRUE;
	const auto imgTB = CMFCToolBar::GetImages();    //Toolbar image.
	const auto sizeImgCurr = imgTB->GetImageSize(); //One button's dimensions.
	const auto flToolbarScaledFactor = sizeImgCurr.cx / 16.0; //How many times our toolbar is bigger than the standard one.
	const auto flScale = Utility::GetHiDPIInfo().flDPIScale; //Scale factor for HighDPI displays.
	const auto flScaleTB = flScale / flToolbarScaledFactor;
	const SIZE sizeBtn { static_cast<int>(sizeImgCurr.cx * flScaleTB) + 7,
		static_cast<int>(sizeImgCurr.cy * flScaleTB) + 7 }; //Size of the toolbar's button.
	imgTB->SmoothResize(flScaleTB); //Resize image according to the current DPI.
	CMFCToolBar::SetSizes(sizeBtn, imgTB->GetImageSize());
	CMFCToolBar::SetMenuSizes(sizeBtn, imgTB->GetImageSize());

	m_wndToolBar.SetWindowTextW(L"Standard"); //This text is in fact menu name.
	m_wndToolBar.EnableCustomizeButton(TRUE, IDM_TOOLBAR_CUSTOMIZE, L"Customize...");
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_wndToolBar);

	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerVS2008));
	CDockingManager::SetDockingMode(DT_SMART); //enable Visual Studio 2005 style docking window behavior
	EnableAutoHidePanes(CBRS_ALIGN_ANY);	   //enable Visual Studio 2005 style docking window auto-hide behavior

	//Pane "File Properties".
	CStringW strStr;
	strStr.LoadStringW(IDC_PANE_FILEPROPS);
	m_paneFileProps.Create(strStr, this, CRect(0, 0, 200, 400), TRUE, IDC_PANE_FILEPROPS,
			WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI);
	m_paneFileProps.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_paneFileProps);

	//Pane "Data Interpreter".
	strStr.LoadStringW(IDC_PANE_DATAINTERP);
	m_paneDataInterp.Create(strStr, this, CRect(0, 0, 200, 400), TRUE, IDC_PANE_DATAINTERP,
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI);
	m_paneDataInterp.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_paneDataInterp);

	//Pane "Template Manager".
	strStr.LoadStringW(IDC_PANE_TEMPLMGR);
	m_paneTemplMgr.Create(strStr, this, CRect(0, 0, 400, 200), TRUE, IDC_PANE_TEMPLMGR,
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI);
	m_paneTemplMgr.EnableDocking(CBRS_ALIGN_TOP | CBRS_ALIGN_BOTTOM);
	DockPane(&m_paneTemplMgr);

	UpdateMDITabbedBarsIcons();

	//EnablePaneMenu(TRUE, IDM_TOOLBAR_CUSTOMIZE, L"Customize...", ID_VIEW_TOOLBAR); //MainFrame and Pane context menu.
	CMFCToolBar::EnableQuickCustomization(); // enable quick (Alt+drag) toolbar customization
	ModifyStyle(0, FWS_PREFIXTITLE);

	return 0;
}

BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
	//Default "MdiClient" class doesn't have CS_DBLCLKS flag set.
	//We acquire class info, unregister it, set the CS_DBLCLKS flag and then register again.
	//Now we will recieve WM_LBUTTONDBLCLK messages for m_hWndMDIClient window.
	WNDCLASSEXW wndClass { };
	if (!::GetClassInfoExW(AfxGetInstanceHandle(), L"MdiClient", &wndClass)) {
		MessageBoxW(L"GetClassInfo(MdiClient) failed");
		return FALSE;
	}

	UnregisterClassW(L"MdiClient", AfxGetInstanceHandle());
	wndClass.cbSize = sizeof(WNDCLASSEXW);
	wndClass.style |= CS_DBLCLKS;
	RegisterClassExW(&wndClass);

	if (CMDIFrameWndEx::OnCreateClient(lpcs, pContext) == FALSE)
		return FALSE;

	SetWindowSubclass(m_hWndMDIClient, MDIClientProc, 1, reinterpret_cast<DWORD_PTR>(this));

	return TRUE;
}

BOOL CMainFrame::OnEraseMDIClientBackground(CDC* /*pDC*/)
{
	return TRUE;
}

void CMainFrame::OnUpdateViewFileProps(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(HasChildFrame());
	pCmdUI->SetCheck(HasChildFrame() ? IsPaneVisible(IDC_PANE_FILEPROPS) : FALSE);
}

void CMainFrame::OnUpdateViewDataInterp(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(HasChildFrame());
	pCmdUI->SetCheck(HasChildFrame() ? IsPaneVisible(IDC_PANE_DATAINTERP) : FALSE);
}

void CMainFrame::OnUpdateViewTemplMgr(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(HasChildFrame());
	pCmdUI->SetCheck(HasChildFrame() ? IsPaneVisible(IDC_PANE_TEMPLMGR) : FALSE);
}

void CMainFrame::OnViewCustomize()
{
	const auto pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

void CMainFrame::OnViewFileProps()
{
	ShowPane(IDC_PANE_FILEPROPS, !IsPaneVisible(IDC_PANE_FILEPROPS), !IsPaneVisible(IDC_PANE_FILEPROPS));
}

void CMainFrame::OnViewDataInterp()
{
	ShowPane(IDC_PANE_DATAINTERP, !IsPaneVisible(IDC_PANE_DATAINTERP), !IsPaneVisible(IDC_PANE_DATAINTERP));
}

void CMainFrame::OnViewTemplMgr()
{
	ShowPane(IDC_PANE_TEMPLMGR, !IsPaneVisible(IDC_PANE_TEMPLMGR), !IsPaneVisible(IDC_PANE_TEMPLMGR));
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	switch (pMsg->message) {
	case WM_MBUTTONDOWN: //Closing tabs with the middle mouse button.
	{
		m_pWndMBtnCurrDown = nullptr;
		CPoint pt = pMsg->pt;
		const auto pWnd = WindowFromPoint(pt);
		if (pWnd == nullptr)
			break;

		const auto& tabGroups = GetMDITabGroups();
		if (tabGroups.GetCount() <= 0)
			break;

		auto pos = tabGroups.GetHeadPosition();
		while (pos != nullptr) {
			if (const auto pTabCtrl = DYNAMIC_DOWNCAST(CMFCTabCtrl, tabGroups.GetNext(pos)); pTabCtrl == pWnd) { //Click on TabCtrl.
				pTabCtrl->ScreenToClient(&pt);
				if (const auto iTab = pTabCtrl->GetTabFromPoint(pt); iTab != -1) {
					if (auto pWndTab = pTabCtrl->GetTabWnd(iTab); pWndTab != nullptr) {
						m_pWndMBtnCurrDown = pWndTab;
					}
					break;
				}
			}
		}
	}
	break;
	case WM_MBUTTONUP:
	{
		CPoint pt = pMsg->pt;
		auto pWnd = WindowFromPoint(pt);
		if (pWnd == nullptr)
			break;

		const auto& tabGroups = m_wndClientArea.GetMDITabGroups();
		if (tabGroups.GetCount() <= 0)
			break;

		auto pos = tabGroups.GetHeadPosition();
		while (pos != nullptr) {
			if (auto pTabCtrl = DYNAMIC_DOWNCAST(CMFCTabCtrl, tabGroups.GetNext(pos)); pTabCtrl == pWnd) //Click on TabCtrl.
			{
				pTabCtrl->ScreenToClient(&pt);
				if (const auto iTab = pTabCtrl->GetTabFromPoint(pt); iTab != -1) {
					if (const auto pTab = pTabCtrl->GetTabWnd(iTab); pTab != nullptr && pTab == m_pWndMBtnCurrDown) {
						pTab->SendMessageW(WM_CLOSE);
					}
				}
			}
		}
	}
	break;
	default:
		break;
	}

	return CMDIFrameWndEx::PreTranslateMessage(pMsg);
}

void CMainFrame::SavePanesSettings()
{
	if (const auto pView = GetHexerView(); pView != nullptr) {
		auto& refSett = theApp.GetAppSettings();
		for (auto id : Utility::g_arrPanes) {
			refSett.SetPaneStatus(id, IsPaneVisible(id), IsPaneActive(id));
			if (const auto optDlg = Utility::PaneIDToEHexWnd(id); optDlg && IsPaneVisible(id)) {
				refSett.SetPaneData(id, GetHexCtrl()->GetDlgData(*optDlg));
			}
		}
	}
}

auto CMainFrame::MDIClientProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR /*uID*/, DWORD_PTR dwData)->LRESULT
{
	const auto pMainFrame = reinterpret_cast<CMainFrame*>(dwData);
	if (pMainFrame->GetChildFramesCount() != 0) {
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
	case WM_PAINT:
	{
		CPaintDC dc(FromHandle(hWnd));
		CRect rcText;
		::GetClientRect(hWnd, rcText);
		CRect rcShadow { rcText };
		rcShadow.OffsetRect(2, -2);
		CMemDC dcMem(dc, rcText);
		auto pDC = &dcMem.GetDC();

		pDC->FillSolidRect(rcText, RGB(200, 200, 200)); //Bk color.
		pDC->SelectObject(m_fontMDIClient);
		pDC->SetBkMode(TRANSPARENT);
		pDC->SetTextColor(RGB(220, 220, 220)); //Shadow color.
		pDC->DrawTextW(Utility::GetAppName().data(), static_cast<int>(Utility::GetAppName().size()), rcShadow, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		pDC->SetTextColor(RGB(203, 203, 203)); //Text color.
		pDC->DrawTextW(Utility::GetAppName().data(), static_cast<int>(Utility::GetAppName().size()), rcText, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
	}
	break;
	case WM_SIZE:
		MDIClientSize(hWnd, wParam, lParam);
		break;
	case WM_LBUTTONDBLCLK:
		theApp.OnFileOpen();
		break;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

void CMainFrame::MDIClientSize(HWND hWnd, WPARAM /*wParam*/, LPARAM lParam)
{
	const auto pDC = CDC::FromHandle(::GetDC(hWnd));
	const auto iWidthNew = LOWORD(lParam);
	auto iFontSizeMin = 10;
	LOGFONTW lf { .lfHeight { -MulDiv(iFontSizeMin, Utility::GetHiDPIInfo().iLOGPIXELSY, 72) }, .lfPitchAndFamily { FIXED_PITCH },
		.lfFaceName { L"Consolas" } };

	m_fontMDIClient.DeleteObject();
	m_fontMDIClient.CreateFontIndirectW(&lf);
	CSize stSizeText { };
	while (stSizeText.cx < (iWidthNew - 150)) { //Until the text size is not big enough to fill the window's width.
		m_fontMDIClient.DeleteObject();
		iFontSizeMin += 4;
		lf.lfHeight = -MulDiv(iFontSizeMin, Utility::GetHiDPIInfo().iLOGPIXELSY, 72);
		m_fontMDIClient.CreateFontIndirectW(&lf);
		pDC->SelectObject(m_fontMDIClient);
		stSizeText = pDC->GetTextExtent(Utility::GetAppName().data(), static_cast<int>(Utility::GetAppName().size()));
	}
	::ReleaseDC(hWnd, pDC->m_hDC);
	::RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}