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
#include <format>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)
	ON_WM_CREATE()
	ON_COMMAND(IDM_TOOLBAR_CUSTOMIZE, &CMainFrame::OnViewCustomize)
	ON_COMMAND(IDM_VIEW_FILEPROPS, &CMainFrame::OnViewFileProps)
	ON_COMMAND(IDM_VIEW_DATAINTERP, &CMainFrame::OnViewDataInterp)
	ON_UPDATE_COMMAND_UI(IDM_VIEW_FILEPROPS, &CMainFrame::OnUpdateViewFileProps)
	ON_UPDATE_COMMAND_UI(IDM_VIEW_DATAINTERP, &CMainFrame::OnUpdateViewDataInterp)
END_MESSAGE_MAP()

int& CMainFrame::GetChildFramesCount()
{
	return m_iChildFrames;
}

void CMainFrame::HidePanes()
{
	m_paneFileProps.ShowPane(FALSE, FALSE, FALSE);
	m_paneDataInterp.ShowPane(FALSE, FALSE, FALSE);
}

void CMainFrame::OnOpenFirstTab()
{
	//Show "File Properties" pane according to app's settings.
	if (theApp.GetAppSettings().GetShowPaneFileProps()) {
		m_paneFileProps.ShowPane(TRUE, FALSE, TRUE);
	}

	//Show "Data Interpreter" pane according to app's settings.
	if (theApp.GetAppSettings().GetShowPaneDataInterp()) {
		m_paneDataInterp.ShowPane(TRUE, FALSE, TRUE);
	}
}

void CMainFrame::OnCloseLastTab()
{
	HidePanes();
}

void CMainFrame::UpdatePaneFileProps(const Utility::FILEPROPS& stFP)
{
	const auto lmbSetValue = [&](CMFCPropertyGridProperty* pProp) {
		using enum EPropName;
		switch (static_cast<EPropName>(pProp->GetData())) {
		case FILE_PATH:
			pProp->SetValue(stFP.wsvFilePath.data());
			break;
		case FILE_NAME:
			pProp->SetValue(stFP.wsvFileName.data());
			break;
		case FILE_SIZE:
			pProp->SetValue(std::format(std::locale("en_US.UTF-8"), L"{:L} bytes", stFP.ullFileSize).data());
			break;
		case PAGE_SIZE:
			pProp->SetValue(std::format(L"{}", stFP.dwPageSize).data());
			break;
		case IS_WRITABLE:
			pProp->SetValue(std::format(L"{}", stFP.fWritable).data());
			break;
		default:
			break;
		}
	};
	std::ranges::for_each(m_vecProps, lmbSetValue);
}

void CMainFrame::UpdatePaneDataInterp(HWND hWnd)
{
	m_paneDataInterp.SetAsNested(hWnd);
}

void CMainFrame::ShowPaneDataInterp()
{
	m_paneDataInterp.ShowPane(TRUE, FALSE, TRUE);
	theApp.GetAppSettings().SetShowPaneDataInterp(true);
}


//Private methods.

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWndEx::OnCreate(lpCreateStruct) == -1)
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

	m_wndToolBar.CreateEx(this, TBSTYLE_FLAT,
		WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndToolBar.LoadToolBar(IDR_TOOLBAR_MAIN);

	CMFCToolBar::m_bDontScaleImages = TRUE;
	const auto imgTB = CMFCToolBar::GetImages();    //Toolbar image.
	const auto sizeImgCurr = imgTB->GetImageSize(); //One button's dimensions.
	const auto flToolbarScaledFactor = sizeImgCurr.cx / 16.0; //How many times our toolbar is bigger than the standard one.
	const auto flScale = m_HiDPIInfo.flDPIScale; //Scale factor for HighDPI displays.
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
	CreateGridFileProps();
	m_paneFileProps.SetAsNested(m_wndGridFileProps.m_hWnd);
	DockPane(&m_paneFileProps);

	//Pane "Data Interpreter".
	strStr.LoadStringW(IDC_PANE_DATAINTERP);
	m_paneDataInterp.Create(strStr, this, CRect(0, 0, 200, 400), TRUE, IDC_PANE_DATAINTERP,
		WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | CBRS_RIGHT | CBRS_FLOAT_MULTI);
	m_paneDataInterp.EnableDocking(CBRS_ALIGN_ANY);
	DockPane(&m_paneDataInterp);
	m_paneFileProps.AttachToTabWnd(&m_paneDataInterp, DM_SHOW, FALSE);

	UpdateMDITabbedBarsIcons();

	//	EnablePaneMenu(TRUE, IDM_TOOLBAR_CUSTOMIZE, L"Customize...", ID_VIEW_TOOLBAR); //MainFrame and Pane context menu.
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

BOOL CMainFrame::OnCloseDockingPane(CDockablePane* pWnd)
{
	CStringW str;
	pWnd->GetWindowTextW(str);
	CStringW strRes;
	if (strRes.LoadStringW(IDC_PANE_FILEPROPS); strRes == str) {
		theApp.GetAppSettings().SetShowPaneFileProps(false);
	}
	else if (strRes.LoadStringW(IDC_PANE_DATAINTERP); strRes == str) {
		theApp.GetAppSettings().SetShowPaneDataInterp(false);
	}

	return CMDIFrameWndEx::OnCloseDockingPane(pWnd);
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

void CMainFrame::OnViewFileProps()
{
	const auto fShow = !m_paneFileProps.IsVisible();
	m_paneFileProps.ShowPane(fShow, FALSE, TRUE);
	theApp.GetAppSettings().SetShowPaneFileProps(fShow);
}

void CMainFrame::OnViewDataInterp()
{
	const auto fShow = !m_paneDataInterp.IsVisible();
	m_paneDataInterp.ShowPane(fShow, FALSE, TRUE);
	theApp.GetAppSettings().SetShowPaneDataInterp(fShow);
}

void CMainFrame::OnUpdateViewFileProps(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(m_paneFileProps.IsVisible());
}

void CMainFrame::OnUpdateViewDataInterp(CCmdUI * pCmdUI)
{
	pCmdUI->Enable(TRUE);
	pCmdUI->SetCheck(m_paneDataInterp.IsVisible());
}

void CMainFrame::CreateGridFileProps()
{
	m_wndGridFileProps.Create(WS_VISIBLE | WS_CHILD, { }, this, 1);
	m_wndGridFileProps.SetVSDotNetLook();
	m_wndGridFileProps.EnableHeaderCtrl(TRUE, L"Property", L"Value");

	//Set new bigger font to the property.
	const auto pFont = m_wndGridFileProps.GetFont();
	LOGFONTW lf { };
	pFont->GetLogFont(&lf);
	const auto lFontSize = MulDiv(-lf.lfHeight, 72, Utility::GetHiDPIInfo().iLOGPIXELSY) + 2;
	lf.lfHeight = -MulDiv(lFontSize, Utility::GetHiDPIInfo().iLOGPIXELSY, 72);
	m_fntProperty.CreateFontIndirectW(&lf);
	m_wndGridFileProps.SetFont(&m_fntProperty);

	using enum EPropName;
	const auto pFilePath = new CMFCPropertyGridProperty(L"File path:", L"");
	pFilePath->SetData(static_cast<DWORD_PTR>(FILE_PATH));
	pFilePath->AllowEdit(FALSE);
	m_vecProps.emplace_back(pFilePath);
	m_wndGridFileProps.AddProperty(pFilePath);

	const auto pFileName = new CMFCPropertyGridProperty(L"File name:", L"");
	pFileName->SetData(static_cast<DWORD_PTR>(FILE_NAME));
	pFileName->AllowEdit(FALSE);
	m_vecProps.emplace_back(pFileName);
	m_wndGridFileProps.AddProperty(pFileName);

	const auto pFileSize = new CMFCPropertyGridProperty(L"File size:", L"");
	pFileSize->SetData(static_cast<DWORD_PTR>(FILE_SIZE));
	pFileSize->AllowEdit(FALSE);
	m_vecProps.emplace_back(pFileSize);
	m_wndGridFileProps.AddProperty(pFileSize);

	const auto pPageSize = new CMFCPropertyGridProperty(L"Page size:", L"");
	pPageSize->SetData(static_cast<DWORD_PTR>(PAGE_SIZE));
	pPageSize->AllowEdit(FALSE);
	m_vecProps.emplace_back(pPageSize);
	m_wndGridFileProps.AddProperty(pPageSize);
	pPageSize->Show(FALSE);

	const auto pIsWritable = new CMFCPropertyGridProperty(L"Writable:", L"");
	pIsWritable->SetData(static_cast<DWORD_PTR>(IS_WRITABLE));
	pIsWritable->AllowEdit(FALSE);
	m_vecProps.emplace_back(pIsWritable);
	m_wndGridFileProps.AddProperty(pIsWritable);

	//HDITEMW hdPropGrid { .mask = HDI_WIDTH, .cxy = 80 };
	//m_wndProperty.GetHeaderCtrl().SetItem(0, &hdPropGrid);
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

auto CMainFrame::MDIClientProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwData)->LRESULT
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
		pDC->DrawTextW(Utility::g_wstrAppName, static_cast<int>(std::size(Utility::g_wstrAppName) - 1), rcShadow, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		pDC->SetTextColor(RGB(203, 203, 203)); //Text color.
		pDC->DrawTextW(Utility::g_wstrAppName, static_cast<int>(std::size(Utility::g_wstrAppName) - 1), rcText, DT_CENTER | DT_SINGLELINE | DT_VCENTER);
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
	LOGFONTW lf { .lfHeight { -MulDiv(iFontSizeMin, m_HiDPIInfo.iLOGPIXELSY, 72) }, .lfPitchAndFamily { FIXED_PITCH },
		.lfFaceName { L"Consolas" } };

	m_fontMDIClient.DeleteObject();
	m_fontMDIClient.CreateFontIndirectW(&lf);
	CSize stSizeText { };
	while (stSizeText.cx < (iWidthNew - 150)) { //Until the text size is not big enough to fill the window's width.
		m_fontMDIClient.DeleteObject();
		iFontSizeMin += 4;
		lf.lfHeight = -MulDiv(iFontSizeMin, m_HiDPIInfo.iLOGPIXELSY, 72);
		m_fontMDIClient.CreateFontIndirectW(&lf);
		pDC->SelectObject(m_fontMDIClient);
		stSizeText = pDC->GetTextExtent(Utility::g_wstrAppName, static_cast<int>(std::size(Utility::g_wstrAppName) - 1));
	}
	::ReleaseDC(hWnd, pDC->m_hDC);
	::RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}