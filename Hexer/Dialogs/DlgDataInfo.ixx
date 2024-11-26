module;
/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
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
export module DlgDataInfo;

import Utility;
import HexerPropGridCtrl;

//CDlgDataInfo.
export class CDlgDataInfo final : public CDialogEx {
public:
	void SetDataInfo(const Ut::DATAINFO& dis);
private:
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	void UpdateGridData();
	DECLARE_MESSAGE_MAP();
private:
	enum class EPropName : std::uint8_t {
		DATA_PATH = 0x1, FILE_NAME, DATA_SIZE, PAGE_SIZE, ACCESS_MODE, IO_MODE
	};
	CHexerPropGridCtrl m_gridDataInfo;
	std::vector<CMFCPropertyGridProperty*> m_vecPropsDataProps;
	CFont m_fntFilePropsGrid;
	Ut::DATAINFO m_dis { };
};

BEGIN_MESSAGE_MAP(CDlgDataInfo, CDialogEx)
END_MESSAGE_MAP()


//Private methods.

void CDlgDataInfo::SetDataInfo(const Ut::DATAINFO& dis)
{
	m_dis = dis;

	if (IsWindow(m_hWnd)) {
		UpdateGridData();
	}
}


//Private methods.

void CDlgDataInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FILEINFO_GRID, m_gridDataInfo);
}

BOOL CDlgDataInfo::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_gridDataInfo.SetVSDotNetLook();
	m_gridDataInfo.EnableHeaderCtrl(TRUE);
	HDITEMW hdPropGrid { .mask = HDI_WIDTH, .cxy = 100 };
	m_gridDataInfo.GetHeaderCtrl().SetItem(0, &hdPropGrid); //Property grid left column width.

	//Set new bigger font to the property.
	const auto pFont = m_gridDataInfo.GetFont();
	LOGFONTW lf { };
	pFont->GetLogFont(&lf);
	const auto lFontSize = MulDiv(-lf.lfHeight, 72, Ut::GetHiDPIInfo().iLOGPIXELSY) + 2;
	lf.lfHeight = -MulDiv(lFontSize, Ut::GetHiDPIInfo().iLOGPIXELSY, 72);
	m_fntFilePropsGrid.CreateFontIndirectW(&lf);
	m_gridDataInfo.SetFont(&m_fntFilePropsGrid);

	using enum EPropName;
	const auto pFilePath = new CMFCPropertyGridProperty(L"File Path:", L"");
	pFilePath->SetData(static_cast<DWORD_PTR>(DATA_PATH));
	pFilePath->AllowEdit(FALSE);
	m_vecPropsDataProps.emplace_back(pFilePath);
	m_gridDataInfo.AddProperty(pFilePath);

	const auto pFileName = new CMFCPropertyGridProperty(L"File Name:", L"");
	pFileName->SetData(static_cast<DWORD_PTR>(FILE_NAME));
	pFileName->AllowEdit(FALSE);
	m_vecPropsDataProps.emplace_back(pFileName);
	m_gridDataInfo.AddProperty(pFileName);

	const auto pFileSize = new CMFCPropertyGridProperty(L"File Size:", L"");
	pFileSize->SetData(static_cast<DWORD_PTR>(DATA_SIZE));
	pFileSize->AllowEdit(FALSE);
	m_vecPropsDataProps.emplace_back(pFileSize);
	m_gridDataInfo.AddProperty(pFileSize);

	const auto pPageSize = new CMFCPropertyGridProperty(L"Page Size:", L"");
	pPageSize->SetData(static_cast<DWORD_PTR>(PAGE_SIZE));
	pPageSize->AllowEdit(FALSE);
	m_vecPropsDataProps.emplace_back(pPageSize);
	m_gridDataInfo.AddProperty(pPageSize);
	pPageSize->Show(FALSE);

	const auto pAccessMode = new CMFCPropertyGridProperty(L"Access Mode:", L"");
	pAccessMode->SetData(static_cast<DWORD_PTR>(ACCESS_MODE));
	pAccessMode->AllowEdit(FALSE);
	m_vecPropsDataProps.emplace_back(pAccessMode);
	m_gridDataInfo.AddProperty(pAccessMode);

	const auto pDataIOMode = new CMFCPropertyGridProperty(L"Data IO Mode:", L"");
	pDataIOMode->SetData(static_cast<DWORD_PTR>(IO_MODE));
	pDataIOMode->AllowEdit(FALSE);
	m_vecPropsDataProps.emplace_back(pDataIOMode);
	m_gridDataInfo.AddProperty(pDataIOMode);

	UpdateGridData();

	if (const auto pLayout = GetDynamicLayout()) {
		pLayout->SetMinSize({ 0, 0 });
	}

	return TRUE;
}

void CDlgDataInfo::UpdateGridData()
{
	const auto lmbSetValue = [&](CMFCPropertyGridProperty* pProp) {
		using enum EPropName;
		auto wstr = std::wstring { Ut::GetWstrEOpenMode(m_dis.eOpenMode) };
		switch (static_cast<EPropName>(pProp->GetData())) {
		case DATA_PATH:
			wstr += L" path:";
			pProp->SetName(wstr.data(), FALSE);
			pProp->SetValue(m_dis.wsvDataPath.data());
			break;
		case FILE_NAME:
			wstr += L" name:";
			pProp->SetName(wstr.data(), FALSE);
			pProp->SetValue(m_dis.wsvFileName.data());
			break;
		case DATA_SIZE:
			wstr += L" size:";
			pProp->SetName(wstr.data(), FALSE);
			pProp->SetValue(std::format(std::locale("en_US.UTF-8"), L"{:L} bytes", m_dis.ullDataSize).data());
			break;
		case PAGE_SIZE:
			pProp->SetValue(std::format(L"{}", m_dis.dwPageSize).data());
			break;
		case ACCESS_MODE:
			pProp->SetValue(Ut::GetWstrEDataAccessMode(m_dis.eDataAccessMode).data());
			break;
		case IO_MODE:
			pProp->SetValue(Ut::GetWstrEDataIOMode(m_dis.eDataIOMode).data());
			break;
		default:
			break;
		}
		};

	m_gridDataInfo.SetRedraw(false);
	std::ranges::for_each(m_vecPropsDataProps, lmbSetValue);
	m_gridDataInfo.SetRedraw(true);
	m_gridDataInfo.RedrawWindow();
}