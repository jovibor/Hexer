module;
/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "resource.h"
#include <afxcontrolbars.h>
#include <afxdialogex.h>
#include <algorithm>
#include <format>
#include <string>
#include <vector>
export module DlgFileInfo;

import Utility;
import HexerPropGridCtrl;

//CDlgFileInfo.
export class CDlgFileInfo final : public CDialogEx {
public:
	void SetGridData(const Ut::FILEINFO& fis);
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	void UpdateGridData();
	DECLARE_MESSAGE_MAP();
private:
	enum class EPropName : std::uint8_t {
		FILE_PATH = 0x1, FILE_NAME, FILE_SIZE, PAGE_SIZE, IS_MUTABLE
	};
	CHexerPropGridCtrl m_gridFileProps;
	std::vector<CMFCPropertyGridProperty*> m_vecPropsFileProps;
	CFont m_fntFilePropsGrid;
	Ut::FILEINFO m_fis { };
};

BEGIN_MESSAGE_MAP(CDlgFileInfo, CDialogEx)
END_MESSAGE_MAP()


//Private methods.

void CDlgFileInfo::SetGridData(const Ut::FILEINFO& fis)
{
	m_fis = fis;

	if (IsWindow(m_hWnd)) {
		UpdateGridData();
	}
}


//Private methods.

void CDlgFileInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILEINFO_GRID, m_gridFileProps);
}

BOOL CDlgFileInfo::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_gridFileProps.SetVSDotNetLook();
	m_gridFileProps.EnableHeaderCtrl(TRUE, L"Property", L"Value");
	HDITEMW hdPropGrid { .mask = HDI_WIDTH, .cxy = 80 };
	m_gridFileProps.GetHeaderCtrl().SetItem(0, &hdPropGrid); //Property grid left column width.

	//Set new bigger font to the property.
	const auto pFont = m_gridFileProps.GetFont();
	LOGFONTW lf { };
	pFont->GetLogFont(&lf);
	const auto lFontSize = MulDiv(-lf.lfHeight, 72, Ut::GetHiDPIInfo().iLOGPIXELSY) + 2;
	lf.lfHeight = -MulDiv(lFontSize, Ut::GetHiDPIInfo().iLOGPIXELSY, 72);
	m_fntFilePropsGrid.CreateFontIndirectW(&lf);
	m_gridFileProps.SetFont(&m_fntFilePropsGrid);

	using enum EPropName;
	const auto pFilePath = new CMFCPropertyGridProperty(L"File path:", L"");
	pFilePath->SetData(static_cast<DWORD_PTR>(FILE_PATH));
	pFilePath->AllowEdit(FALSE);
	m_vecPropsFileProps.emplace_back(pFilePath);
	m_gridFileProps.AddProperty(pFilePath);

	const auto pFileName = new CMFCPropertyGridProperty(L"File name:", L"");
	pFileName->SetData(static_cast<DWORD_PTR>(FILE_NAME));
	pFileName->AllowEdit(FALSE);
	m_vecPropsFileProps.emplace_back(pFileName);
	m_gridFileProps.AddProperty(pFileName);

	const auto pFileSize = new CMFCPropertyGridProperty(L"File size:", L"");
	pFileSize->SetData(static_cast<DWORD_PTR>(FILE_SIZE));
	pFileSize->AllowEdit(FALSE);
	m_vecPropsFileProps.emplace_back(pFileSize);
	m_gridFileProps.AddProperty(pFileSize);

	const auto pPageSize = new CMFCPropertyGridProperty(L"Page size:", L"");
	pPageSize->SetData(static_cast<DWORD_PTR>(PAGE_SIZE));
	pPageSize->AllowEdit(FALSE);
	m_vecPropsFileProps.emplace_back(pPageSize);
	m_gridFileProps.AddProperty(pPageSize);
	pPageSize->Show(FALSE);

	const auto pIsWritable = new CMFCPropertyGridProperty(L"Writable:", L"");
	pIsWritable->SetData(static_cast<DWORD_PTR>(IS_MUTABLE));
	pIsWritable->AllowEdit(FALSE);
	m_vecPropsFileProps.emplace_back(pIsWritable);
	m_gridFileProps.AddProperty(pIsWritable);

	UpdateGridData();

	if (const auto pLayout = GetDynamicLayout()) {
		pLayout->SetMinSize({ 0, 0 });
	}

	return TRUE;
}

void CDlgFileInfo::UpdateGridData()
{
	const auto lmbSetValue = [&](CMFCPropertyGridProperty* pProp) {
		using enum EPropName;
		switch (static_cast<EPropName>(pProp->GetData())) {
		case FILE_PATH:
			pProp->SetValue(m_fis.wsvFilePath.data());
			break;
		case FILE_NAME:
			pProp->SetValue(m_fis.wsvFileName.data());
			break;
		case FILE_SIZE:
			pProp->SetValue(std::format(std::locale("en_US.UTF-8"), L"{:L} bytes", m_fis.ullFileSize).data());
			break;
		case PAGE_SIZE:
			pProp->SetValue(std::format(L"{}", m_fis.dwPageSize).data());
			break;
		case IS_MUTABLE:
			pProp->SetValue(std::format(L"{}", m_fis.fMutable).data());
			break;
		default:
			break;
		}
		};

	m_gridFileProps.SetRedraw(false);
	std::ranges::for_each(m_vecPropsFileProps, lmbSetValue);
	m_gridFileProps.SetRedraw(true);
	m_gridFileProps.RedrawWindow();
}