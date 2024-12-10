module;
/*******************************************************************************
* Copyright © 2023-present Jovibor https://github.com/jovibor/                 *
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
		DATA_PATH = 0x1, DATA_NAME, DATA_SIZE, PAGE_SIZE, ACCESS_MODE, IO_MODE
	};
	CHexerPropGridCtrl m_gridDataInfo;
	std::vector<CHexerPropGridProp*> m_vecPropsDataProps;
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
	constexpr wchar_t pwszFont[] { L"Microsoft Sans Serif" };
	std::copy_n(pwszFont, std::size(pwszFont), lf.lfFaceName);
	const auto lFontSize = MulDiv(-lf.lfHeight, 72, Ut::GetHiDPIInfo().iLOGPIXELSY) + 1; //Convert font Height to point size.
	lf.lfHeight = -MulDiv(lFontSize, Ut::GetHiDPIInfo().iLOGPIXELSY, 72); //Convert point size to font Height.
	m_fntFilePropsGrid.CreateFontIndirectW(&lf);
	m_gridDataInfo.SetFont(&m_fntFilePropsGrid);

	using enum EPropName;
	const auto pDataPath = new CHexerPropGridProp(L"", L"");
	pDataPath->SetData(static_cast<DWORD_PTR>(DATA_PATH));
	pDataPath->SetValueColor(RGB(0, 0, 250));
	pDataPath->AllowEdit(FALSE);
	m_vecPropsDataProps.emplace_back(pDataPath);
	m_gridDataInfo.AddProperty(pDataPath);

	const auto pDataName = new CHexerPropGridProp(L"", L"");
	pDataName->SetData(static_cast<DWORD_PTR>(DATA_NAME));
	pDataName->SetValueColor(RGB(0, 0, 250));
	pDataName->AllowEdit(FALSE);
	m_vecPropsDataProps.emplace_back(pDataName);
	m_gridDataInfo.AddProperty(pDataName);

	const auto pDataSize = new CHexerPropGridProp(L"", L"");
	pDataSize->SetData(static_cast<DWORD_PTR>(DATA_SIZE));
	pDataSize->SetValueColor(RGB(0, 0, 250));
	pDataSize->AllowEdit(FALSE);
	m_vecPropsDataProps.emplace_back(pDataSize);
	m_gridDataInfo.AddProperty(pDataSize);

	const auto pPageSize = new CHexerPropGridProp(L"Page Size:", L"");
	pPageSize->SetData(static_cast<DWORD_PTR>(PAGE_SIZE));
	pPageSize->SetValueColor(RGB(0, 0, 250));
	pPageSize->AllowEdit(FALSE);
	m_vecPropsDataProps.emplace_back(pPageSize);
	m_gridDataInfo.AddProperty(pPageSize);

	const auto pAccessMode = new CHexerPropGridProp(L"Access Mode:", L"");
	pAccessMode->SetData(static_cast<DWORD_PTR>(ACCESS_MODE));
	pAccessMode->AllowEdit(FALSE);
	m_vecPropsDataProps.emplace_back(pAccessMode);
	m_gridDataInfo.AddProperty(pAccessMode);

	const auto pDataIOMode = new CHexerPropGridProp(L"Data IO Mode:", L"");
	pDataIOMode->SetData(static_cast<DWORD_PTR>(IO_MODE));
	pDataIOMode->SetValueColor(RGB(0, 0, 250));
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
	const auto lmbSetValue = [&](CHexerPropGridProp* pProp) {
		using enum EPropName;
		auto wstr = std::wstring { Ut::GetWstrEOpenMode(m_dis.eOpenMode) };
		switch (static_cast<EPropName>(pProp->GetData())) {
		case DATA_PATH:
			pProp->SetName((wstr += L" path:").data(), FALSE);
			pProp->SetValue(m_dis.wsvDataPath.data());
			break;
		case DATA_NAME:
			pProp->SetName((wstr += L" name:").data(), FALSE);
			pProp->SetValue(m_dis.wsvFileName.data());
			break;
		case DATA_SIZE:
			pProp->SetName((wstr += L" size:").data(), FALSE);
			pProp->SetValue(std::format(std::locale("en_US.UTF-8"), L"{:L} bytes", m_dis.ullDataSize).data());
			break;
		case PAGE_SIZE:
			pProp->SetValue(std::format(L"{}", m_dis.dwPageSize).data());
			break;
		case ACCESS_MODE:
			pProp->SetValue(Ut::GetWstrDATAACCESS(m_dis.stDAC));
			COLORREF clr;
			if (!m_dis.stDAC.fMutable) {
				clr = RGB(128, 128, 128);
			}
			else {
				switch (m_dis.stDAC.eDataAccessMode) {
				case ACCESS_SAFE:
					clr = RGB(20, 200, 20);
					break;
				case ACCESS_INPLACE:
					clr = RGB(200, 20, 20);
					break;
				default:
					clr = 0xFFFFFFFFUL;
					break;
				}
			}
			pProp->SetValueColor(clr);
			break;
		case IO_MODE:
			if (m_dis.stDAC.fMutable && m_dis.stDAC.eDataAccessMode == Ut::EDataAccessMode::ACCESS_INPLACE) {
				pProp->SetValue(Ut::GetWstrEDataIOMode(m_dis.eDataIOMode));
			}
			else {
				pProp->SetValue(L"—");
			}
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