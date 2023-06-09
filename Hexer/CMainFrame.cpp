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
	ON_COMMAND(IDM_TOOLBAR_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_COMMAND_RANGE(IDM_VIEW_FILEPROPS, IDM_VIEW_LOGINFO, &CMainFrame::OnViewRangePanes)
	ON_MESSAGE(Ut::WM_ADDLOGENTRY, OnAddLogEntry)
	ON_UPDATE_COMMAND_UI_RANGE(IDM_VIEW_FILEPROPS, IDM_VIEW_LOGINFO, &CMainFrame::OnUpdateRangePanes)
	ON_WM_CREATE()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

void CMainFrame::AddLogEntry(const Ut::Log::LOGDATA& stData)
{
	m_dlgLogInfo.AddLogEntry(stData);
}

int& CMainFrame::GetChildFramesCount()
{
	return m_iChildFrames;
}

bool CMainFrame::IsPaneActive(UINT uPaneID)
{
	return GetPtrFromPaneID(uPaneID)->IsPaneVisible();
}

bool CMainFrame::IsPaneVisible(UINT uPaneID)
{
	return GetPtrFromPaneID(uPaneID)->IsVisible();
}

void CMainFrame::OnChildFrameActivate()
{
	if (m_fClosing) {
		return;
	}

	//The ON_REGISTERED_MESSAGE(AFX_WM_CHANGE_ACTIVE_TAB,...) message does in fact work,
	//but this message is generated for ANY tab that is being activated in ANY tab-control,
	//not only in the main CMainFrame tab-control. 
	//Docking Panes that are combined in a tab group also send this message to the CMainFrame window.

	//Setting Panes' HWND according to the current active ChildFrame.
	if (HasChildFrame()) {
		for (auto uPaneID : Ut::g_arrPanes) {
			if (IsPaneVisible(uPaneID)) {
				ShowPane(uPaneID, true, IsPaneActive(uPaneID));
			}
		}
	}
}

void CMainFrame::OnChildFrameCloseLast()
{
	if (m_fClosing) {
		return;
	}

	SavePanesSettings(); //It's called either here or in the OnClose.
	HideAllPanes();      //To disable panes from showing at the next app's start-up.
}

void CMainFrame::OnChildFrameFirstOpen()
{
	if (m_fClosing) {
		return;
	}

	for (auto id : Ut::g_arrPanes) {
		if (const auto ps = theApp.GetAppSettings().GetPaneStatus(id); ps.fIsVisible) {
			ShowPane(id, true, ps.fIsActive);
		}
	}
}

void CMainFrame::ShowPane(UINT uPaneID, bool fShow, bool fActivate)
{
	const auto pPane = GetPtrFromPaneID(uPaneID);
	if (pPane == nullptr)
		return;

	if (fShow) {
		if (const auto hWndForPane = GetHWNDForPane(uPaneID); hWndForPane != nullptr) {
			if (const auto hWndCurr = pPane->GetNestedHWND(); hWndForPane != hWndCurr) {
				pPane->SetNestedHWND(hWndForPane);
			}
		}

		if (uPaneID == IDC_PANE_FILEINFO) {
			UpdatePaneFileInfo();
		}
	}

	pPane->ShowPane(fShow, FALSE, fActivate);
}

void CMainFrame::UpdatePaneFileInfo()
{
	m_dlgFileInfo.SetGridData(GetHexerView()->GetFileInfo());
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

auto CMainFrame::GetHWNDForPane(UINT uPaneID)->HWND
{
	switch (uPaneID) {
	case IDC_PANE_FILEINFO:
		if (!IsWindow(m_dlgFileInfo)) {
			m_dlgFileInfo.Create(IDD_FILEINFO, this);
		}
		return m_dlgFileInfo;
	case IDC_PANE_LOGINFO:
		if (!IsWindow(m_dlgLogInfo)) {
			m_dlgLogInfo.Create(IDD_LOGINFO, this);
		}
		return m_dlgLogInfo;
	default: //HWND for HexCtrl's Panes.
		if (const auto pView = GetHexerView(); pView != nullptr) {
			return pView->GetHWNDForPane(uPaneID);
		}
		return { };
	}
}

auto CMainFrame::GetPanesMap()->const std::unordered_map<UINT, CHexerDockablePane*>&
{
	//PaneID <-> CHexerDockablePane* correspondence.
	static const std::unordered_map<UINT, CHexerDockablePane*> umapPanes {
		{ IDC_PANE_FILEINFO, &m_paneFileInfo }, { IDC_PANE_BKMMGR, &m_paneBkmMgr },
		{ IDC_PANE_DATAINTERP, &m_paneDataInterp }, { IDC_PANE_TEMPLMGR, &m_paneTemplMgr },
		{ IDC_PANE_LOGINFO, &m_paneLogInfo }
	};

	return umapPanes;
}

auto CMainFrame::GetPtrFromPaneID(UINT uPaneID)->CHexerDockablePane*
{
	if (GetPanesMap().contains(uPaneID)) {
		return GetPanesMap().at(uPaneID);
	}

	return { };
}

bool CMainFrame::HasChildFrame()
{
	return GetChildFramesCount() > 0;
}

void CMainFrame::HideAllPanes()
{
	for (const auto [uKey, pPane] : GetPanesMap()) {
		pPane->ShowPane(FALSE, FALSE, FALSE);
	}
}

auto CMainFrame::OnAddLogEntry(WPARAM /*wParam*/, LPARAM lParam)->LRESULT
{
	AddLogEntry(*reinterpret_cast<Ut::Log::LOGDATA*>(lParam));

	return S_OK;
}

void CMainFrame::OnClose()
{
	m_fClosing = true;
	SavePanesSettings(); //It's called either here or in the OnChildFrameCloseLast.
	HideAllPanes();      //To disable panes from showing at the next app's start-up.

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
	const auto flScale = Ut::GetHiDPIInfo().flDPIScale; //Scale factor for HighDPI displays.
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
	strStr.LoadStringW(IDC_PANE_FILEINFO);
	m_paneFileInfo.Create(strStr, this, CRect(0, 0, 200, 400), TRUE, IDC_PANE_FILEINFO,
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI);
	m_paneFileInfo.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_paneFileInfo);

	//Pane "Bookmark Manager".
	strStr.LoadStringW(IDC_PANE_BKMMGR);
	m_paneBkmMgr.Create(strStr, this, CRect(0, 0, 200, 400), TRUE, IDC_PANE_BKMMGR,
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI);
	m_paneBkmMgr.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_paneBkmMgr);

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
	m_paneTemplMgr.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_paneTemplMgr);

	//Pane "Log Information".
	strStr.LoadStringW(IDC_PANE_LOGINFO);
	m_paneLogInfo.Create(strStr, this, CRect(0, 0, 400, 200), TRUE, IDC_PANE_LOGINFO,
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_BOTTOM | CBRS_FLOAT_MULTI);
	m_paneLogInfo.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_paneLogInfo);

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

void CMainFrame::OnViewCustomize()
{
	const auto pDlgCust = new CMFCToolBarsCustomizeDialog(this, TRUE /* scan menus */);
	pDlgCust->EnableUserDefinedToolbars();
	pDlgCust->Create();
}

void CMainFrame::OnViewRangePanes(UINT uMenuID)
{
	const auto uPaneID = Ut::GetPaneIDFromMenuID(uMenuID);
	const auto fVisible = !IsPaneVisible(uPaneID);
	ShowPane(uPaneID, fVisible, fVisible);
}

void CMainFrame::OnUpdateRangePanes(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(HasChildFrame());
	pCmdUI->SetCheck(HasChildFrame() ? IsPaneVisible(Ut::GetPaneIDFromMenuID(pCmdUI->m_nID)) : FALSE);
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
		for (auto id : Ut::g_arrPanes) {
			refSett.SetPaneStatus(id, IsPaneVisible(id), IsPaneActive(id));
			if (const auto optDlg = Ut::GetEHexWndFromPaneID(id); optDlg && IsPaneVisible(id)) {
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
		pDC->DrawTextW(Ut::GetAppName().data(), static_cast<int>(Ut::GetAppName().size()), rcShadow, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		pDC->SetTextColor(RGB(203, 203, 203)); //Text color.
		pDC->DrawTextW(Ut::GetAppName().data(), static_cast<int>(Ut::GetAppName().size()), rcText, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
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
	LOGFONTW lf { .lfHeight { -MulDiv(iFontSizeMin, Ut::GetHiDPIInfo().iLOGPIXELSY, 72) }, .lfPitchAndFamily { FIXED_PITCH },
		.lfFaceName { L"Consolas" } };

	m_fontMDIClient.DeleteObject();
	m_fontMDIClient.CreateFontIndirectW(&lf);
	CSize stSizeText { };
	while (stSizeText.cx < (iWidthNew - 150)) { //Until the text size is not big enough to fill the window's width.
		m_fontMDIClient.DeleteObject();
		iFontSizeMin += 4;
		lf.lfHeight = -MulDiv(iFontSizeMin, Ut::GetHiDPIInfo().iLOGPIXELSY, 72);
		m_fontMDIClient.CreateFontIndirectW(&lf);
		pDC->SelectObject(m_fontMDIClient);
		stSizeText = pDC->GetTextExtent(Ut::GetAppName().data(), static_cast<int>(Ut::GetAppName().size()));
	}
	::ReleaseDC(hWnd, pDC->m_hDC);
	::RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}