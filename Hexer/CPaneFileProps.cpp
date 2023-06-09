/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CPaneFileProps.h"
#include "CMainFrame.h"
#include "CHexerApp.h"
#include "resource.h"
#include <format>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CPaneFileProps, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

void CPaneFileProps::AdjustLayout()
{
	const auto pMainWnd = AfxGetMainWnd();
	if (GetSafeHwnd() == nullptr || (pMainWnd != nullptr && pMainWnd->IsIconic())) {
		return;
	}

	CRect rcClient;
	GetClientRect(rcClient);
	m_wndProperty.SetWindowPos(nullptr, rcClient.left, rcClient.top, rcClient.Width(),
		rcClient.Height(), SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW);
	HDITEMW hdPropGrid { .mask = HDI_WIDTH, .cxy = 80 };
	m_wndProperty.GetHeaderCtrl().SetItem(0, &hdPropGrid);
}

int CPaneFileProps::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_wndProperty.Create(WS_VISIBLE | WS_CHILD, { }, this, 1);
	m_wndProperty.SetVSDotNetLook();
	m_wndProperty.EnableHeaderCtrl(TRUE, L"Property", L"Value");

	//Set new bigger font to the property.
	const auto pFont = m_wndProperty.GetFont();
	LOGFONTW lf { };
	pFont->GetLogFont(&lf);
	const auto pDC = m_wndProperty.GetDC();
	const auto iLOGPIXELSY = GetDeviceCaps(pDC->m_hDC, LOGPIXELSY);
	ReleaseDC(pDC);
	const auto lFontSize = MulDiv(-lf.lfHeight, 72, iLOGPIXELSY) + 2;
	lf.lfHeight = -MulDiv(lFontSize, iLOGPIXELSY, 72);
	m_fntProperty.CreateFontIndirectW(&lf);
	m_wndProperty.SetFont(&m_fntProperty);

	using enum EPropName;
	const auto pFilePath = new CMFCPropertyGridProperty(L"File path:", L"");
	pFilePath->SetData(static_cast<DWORD_PTR>(FILE_PATH));
	pFilePath->AllowEdit(FALSE);
	m_vecProps.emplace_back(pFilePath);
	m_wndProperty.AddProperty(pFilePath);

	const auto pFileName = new CMFCPropertyGridProperty(L"File name:", L"");
	pFileName->SetData(static_cast<DWORD_PTR>(FILE_NAME));
	pFileName->AllowEdit(FALSE);
	m_vecProps.emplace_back(pFileName);
	m_wndProperty.AddProperty(pFileName);

	const auto pFileSize = new CMFCPropertyGridProperty(L"File size:", L"");
	pFileSize->SetData(static_cast<DWORD_PTR>(FILE_SIZE));
	pFileSize->AllowEdit(FALSE);
	m_vecProps.emplace_back(pFileSize);
	m_wndProperty.AddProperty(pFileSize);

	const auto pPageSize = new CMFCPropertyGridProperty(L"Page size:", L"");
	pPageSize->SetData(static_cast<DWORD_PTR>(PAGE_SIZE));
	pPageSize->AllowEdit(FALSE);
	m_vecProps.emplace_back(pPageSize);
	m_wndProperty.AddProperty(pPageSize);
	pPageSize->Show(FALSE);

	const auto pIsWritable = new CMFCPropertyGridProperty(L"Writable:", L"");
	pIsWritable->SetData(static_cast<DWORD_PTR>(IS_WRITABLE));
	pIsWritable->AllowEdit(FALSE);
	m_vecProps.emplace_back(pIsWritable);
	m_wndProperty.AddProperty(pIsWritable);

	AdjustLayout();

	return 0;
}

void CPaneFileProps::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CPaneFileProps::UpdatePaneFileProps(const Utility::FILEPROPS& stFP)
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