module;
/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include <SDKDDKVer.h>
#include "HexCtrl.h"
#include "resource.h"
#include <afxdialogex.h>
#include <afxpropertygridctrl.h>
#include <comutil.h> //_variant_t.
#include <cassert>
#include <format>
#include <memory>
#include <ranges>
#include <vector>
export module DlgSettings;

import HexerPropGridCtrl;
import AppSettings;
import Utility;

//CDlgSettingsHexCtrl.
class CDlgSettingsHexCtrl final : public CDialogEx {
public:
	auto Create(UINT nIDTemplate, CWnd* pParentWnd, CAppSettings* pAppSettings) -> BOOL;
	void SaveSettings();
private:
	struct GRIDDATA; //Forward declarations.
	enum class EGroup : std::uint8_t;
	enum class EName : std::uint8_t;
	void DoDataExchange(CDataExchange* pDX)override;
	[[nodiscard]] auto GetGridData(EName eName)const->const GRIDDATA*;
	[[nodiscard]] auto GetPropValueDWORD(EName eName)const->DWORD;
	[[nodiscard]] auto GetPropValueFLOAT(EName eName)const->float;
	[[nodiscard]] auto GetPropValueFONT(EName eName)const->const LOGFONTW*;
	[[nodiscard]] auto GetPropValueRGB(EName eName)const->COLORREF;
	[[nodiscard]] auto GetPropValueWSTR(EName eName)const->std::wstring_view;
	void OnCancel()override;
	BOOL OnInitDialog()override;
	DECLARE_MESSAGE_MAP();
private:
	static constexpr const wchar_t* m_arrDateFmt[] { L"MM/DD/YYYY", L"DD/MM/YYYY", L"YYYY/MM/DD" };
	static constexpr const wchar_t* m_arrScrollType[] { L"Lines", L"Ratio" };
	static constexpr const wchar_t* m_arrShowHide[] { L"Show", L"Hide" };
	static constexpr const wchar_t* m_arrOffsetMode[] { L"Hex", L"Decimal" };
	CAppSettings* m_pAppSettings { };
	CHexerPropGridCtrl m_gridHexCtrl;
	std::vector<GRIDDATA> m_vecGrid;
	CFont m_fntGridHexCtrl;
};

enum class CDlgSettingsHexCtrl::EGroup : std::uint8_t {
	GROUP_GENERAL, GROUP_COLORS
};

enum class CDlgSettingsHexCtrl::EName : std::uint8_t {
	dwCapacity, dwGroupSize, dwPageSize, wchUnprintable, dwDateFormat, fScrollLines, flScrollRatio,
	fInfoBar, fOffsetHex, dwCharsExtraSpace, stLogFont,
	clrFontHex, clrFontText, clrFontSel, clrFontDataInterp, clrFontCaption, clrFontInfoParam, clrFontInfoData,
	clrFontCaret, clrBk, clrBkSel, clrBkDataInterp, clrBkInfoBar, clrBkCaret, clrBkCaretSel
};

struct CDlgSettingsHexCtrl::GRIDDATA {
	CMFCPropertyGridProperty* pProp { };
	EGroup eGroup { };
	EName eName { };
};

BEGIN_MESSAGE_MAP(CDlgSettingsHexCtrl, CDialogEx)
END_MESSAGE_MAP()

auto CDlgSettingsHexCtrl::Create(UINT nIDTemplate, CWnd* pParentWnd, CAppSettings* pAppSettings)->BOOL
{
	assert(pAppSettings != nullptr);
	m_pAppSettings = pAppSettings;

	return CDialogEx::Create(nIDTemplate, pParentWnd);
}

void CDlgSettingsHexCtrl::SaveSettings()
{
	using enum EName;
	auto& refSett = m_pAppSettings->GetHexCtrlData();
	refSett.dwCapacity = GetPropValueDWORD(dwCapacity);
	refSett.dwGroupSize = GetPropValueDWORD(dwGroupSize);
	refSett.dwPageSize = GetPropValueDWORD(dwPageSize);
	const auto wsvUnprint = GetPropValueWSTR(wchUnprintable);
	refSett.wchUnprintable = wsvUnprint.empty() ? L' ' : wsvUnprint[0];

	const auto wsvFmt = GetPropValueWSTR(dwDateFormat);
	for (auto const [index, wsv] : std::views::enumerate(m_arrDateFmt)) {
		if (wsv == wsvFmt) {
			refSett.dwDateFormat = static_cast<DWORD>(index);
			break;
		}
	}

	refSett.fScrollLines = GetPropValueWSTR(fScrollLines) == m_arrScrollType[0];
	refSett.flScrollRatio = GetPropValueFLOAT(flScrollRatio);
	refSett.fInfoBar = GetPropValueWSTR(fInfoBar) == m_arrShowHide[0];
	refSett.fOffsetHex = GetPropValueWSTR(fOffsetHex) == m_arrOffsetMode[0];
	refSett.dwCharsExtraSpace = GetPropValueDWORD(dwCharsExtraSpace);
	refSett.stLogFont = *GetPropValueFONT(stLogFont);
	refSett.stClrs.clrFontHex = GetPropValueRGB(clrFontHex);
	refSett.stClrs.clrFontText = GetPropValueRGB(clrFontText);
	refSett.stClrs.clrFontSel = GetPropValueRGB(clrFontSel);
	refSett.stClrs.clrFontDataInterp = GetPropValueRGB(clrFontDataInterp);
	refSett.stClrs.clrFontCaption = GetPropValueRGB(clrFontCaption);
	refSett.stClrs.clrFontInfoParam = GetPropValueRGB(clrFontInfoParam);
	refSett.stClrs.clrFontInfoData = GetPropValueRGB(clrFontInfoData);
	refSett.stClrs.clrFontCaret = GetPropValueRGB(clrFontCaret);
	refSett.stClrs.clrBk = GetPropValueRGB(clrBk);
	refSett.stClrs.clrBkSel = GetPropValueRGB(clrBkSel);
	refSett.stClrs.clrBkDataInterp = GetPropValueRGB(clrBkDataInterp);
	refSett.stClrs.clrBkInfoBar = GetPropValueRGB(clrBkInfoBar);
	refSett.stClrs.clrBkCaret = GetPropValueRGB(clrBkCaret);
	refSett.stClrs.clrBkCaretSel = GetPropValueRGB(clrBkCaretSel);
}


//CDlgSettingsHexCtrl private methods.

void CDlgSettingsHexCtrl::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SETTINGSHEXCTRL_GRID, m_gridHexCtrl);
}

auto CDlgSettingsHexCtrl::GetGridData(EName eName)const->const GRIDDATA*
{
	return &*std::find_if(m_vecGrid.begin(), m_vecGrid.end(), [=](const GRIDDATA& refData) {
		return refData.eName == eName; });
}

auto CDlgSettingsHexCtrl::GetPropValueDWORD(EName eName)const->DWORD
{
	return GetGridData(eName)->pProp->GetValue().uintVal;
}

auto CDlgSettingsHexCtrl::GetPropValueFLOAT(EName eName)const->float
{
	return GetGridData(eName)->pProp->GetValue().fltVal;
}

auto CDlgSettingsHexCtrl::GetPropValueFONT(EName eName)const->const LOGFONTW*
{
	return static_cast<CMFCPropertyGridFontProperty*>(GetGridData(eName)->pProp)->GetLogFont();
}

auto CDlgSettingsHexCtrl::GetPropValueRGB(EName eName)const->COLORREF
{
	return static_cast<CMFCPropertyGridColorProperty*>(GetGridData(eName)->pProp)->GetColor();
}

auto CDlgSettingsHexCtrl::GetPropValueWSTR(EName eName)const->std::wstring_view
{
	return GetGridData(eName)->pProp->GetValue().bstrVal;
}

void CDlgSettingsHexCtrl::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

BOOL CDlgSettingsHexCtrl::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_gridHexCtrl.MarkModifiedProperties(1, 0);
	m_gridHexCtrl.EnableHeaderCtrl(TRUE, L"Property", L"Value");
	HDITEMW hdPropGrid { .mask = HDI_WIDTH, .cxy = 150 };
	m_gridHexCtrl.GetHeaderCtrl().SetItem(0, &hdPropGrid); //Property grid left column width.

	//Set new bigger font to the property.
	const auto pFont = m_gridHexCtrl.GetFont();
	LOGFONTW lf;
	pFont->GetLogFont(&lf);
	const auto lFontSize = MulDiv(-lf.lfHeight, 72, Ut::GetHiDPIInfo().iLOGPIXELSY) + 1;
	lf.lfHeight = -MulDiv(lFontSize, Ut::GetHiDPIInfo().iLOGPIXELSY, 72);
	m_fntGridHexCtrl.CreateFontIndirectW(&lf);
	m_gridHexCtrl.SetFont(&m_fntGridHexCtrl);

	auto& refSett = m_pAppSettings->GetHexCtrlData();

	using enum EGroup; using enum EName;
	const auto& refCapac = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Capacity:",
		static_cast<_variant_t>(refSett.dwCapacity), 0, 0, 0, 0, L"0123456789"), GROUP_GENERAL, dwCapacity);
	refCapac.pProp->EnableSpinControl(TRUE, 1, 100);
	refCapac.pProp->AllowEdit(TRUE);

	const auto& refGroup = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Group size:",
		static_cast<_variant_t>(refSett.dwGroupSize), 0, 0, 0, 0, L"0123456789"), GROUP_GENERAL, dwGroupSize);
	refGroup.pProp->EnableSpinControl(TRUE, 1, 64);
	refGroup.pProp->AllowEdit(TRUE);

	const auto& refPageSize = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Page size:",
		static_cast<_variant_t>(refSett.dwPageSize), 0, 0, 0, 0, L"0x123456789AaBbCcDdEeFfGg"), GROUP_GENERAL, dwPageSize);
	refPageSize.pProp->AllowEdit(TRUE);

	const auto& refUnprint = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Unprintable char:",
		static_cast<_variant_t>(std::format(L"{}", refSett.wchUnprintable).data()), 0, 0), GROUP_GENERAL, wchUnprintable);
	refUnprint.pProp->AllowEdit(TRUE);

	const auto& refDate = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Date format:",
		m_arrDateFmt[(std::min)(refSett.dwDateFormat, 2UL)], 0, 0), GROUP_GENERAL, dwDateFormat);
	for (const auto p : m_arrDateFmt) {
		refDate.pProp->AddOption(p);
	}
	refDate.pProp->AllowEdit(FALSE);

	const auto& refScroll = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Scroll lines or ratio:",
		m_arrScrollType[refSett.fScrollLines ? 0 : 1], 0, 0), GROUP_GENERAL, fScrollLines);
	for (const auto p : m_arrScrollType) {
		refScroll.pProp->AddOption(p);
	}
	refScroll.pProp->AllowEdit(FALSE);

	const auto& refScrollSize = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Scroll size:",
		static_cast<_variant_t>(refSett.flScrollRatio), 0, 0, 0, 0, L"0123456789."), GROUP_GENERAL, flScrollRatio);
	refScrollSize.pProp->AllowEdit(TRUE);

	const auto& refInfoBar = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Show Info bar:",
		m_arrShowHide[refSett.fInfoBar ? 0 : 1], 0, 0), GROUP_GENERAL, fInfoBar);
	for (const auto p : m_arrShowHide) {
		refInfoBar.pProp->AddOption(p);
	}
	refInfoBar.pProp->AllowEdit(FALSE);

	const auto& refOffset = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Offset mode:",
		m_arrOffsetMode[refSett.fOffsetHex ? 0 : 1], 0, 0), GROUP_GENERAL, fOffsetHex);
	for (const auto p : m_arrOffsetMode) {
		refOffset.pProp->AddOption(p);
	}
	refOffset.pProp->AllowEdit(FALSE);

	const auto& refExtraSpace = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Chars extra space:",
		static_cast<_variant_t>(refSett.dwCharsExtraSpace), 0, 0, 0, 0, L"0123456789"), GROUP_GENERAL, dwCharsExtraSpace);
	refExtraSpace.pProp->EnableSpinControl(TRUE, 1, 10);
	refExtraSpace.pProp->AllowEdit(TRUE);

	m_vecGrid.emplace_back(new CMFCPropertyGridFontProperty(L"Font:", refSett.stLogFont,
		CF_EFFECTS | CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_NOSIMULATIONS | CF_NOSCRIPTSEL),
		GROUP_GENERAL, stLogFont);

	const auto pAppear = new CMFCPropertyGridProperty(L"General:");
	for (const auto& it : m_vecGrid) {
		if (it.eGroup == GROUP_GENERAL) {
			pAppear->AddSubItem(it.pProp);
		}
	}
	m_gridHexCtrl.AddProperty(pAppear);

	//HexCtrl colors.
	const auto& refClrs = m_pAppSettings->GetHexCtrlData().stClrs;
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Hex", refClrs.clrFontHex), GROUP_COLORS, clrFontHex);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Text", refClrs.clrFontText), GROUP_COLORS, clrFontText);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Selection", refClrs.clrFontSel), GROUP_COLORS, clrFontSel);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Data Interpreter", refClrs.clrFontDataInterp), GROUP_COLORS, clrFontDataInterp);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Caption", refClrs.clrFontCaption), GROUP_COLORS, clrFontCaption);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Info parameters", refClrs.clrFontInfoParam), GROUP_COLORS, clrFontInfoParam);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Info data", refClrs.clrFontInfoData), GROUP_COLORS, clrFontInfoData);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Caret", refClrs.clrFontCaret), GROUP_COLORS, clrFontCaret);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Background", refClrs.clrBk), GROUP_COLORS, clrBk);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Selection bk", refClrs.clrBkSel), GROUP_COLORS, clrBkSel);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Data Interpreter bk", refClrs.clrBkDataInterp), GROUP_COLORS, clrBkDataInterp);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Info bar bk", refClrs.clrBkInfoBar), GROUP_COLORS, clrBkInfoBar);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Caret bk", refClrs.clrBkCaret), GROUP_COLORS, clrBkCaret);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Caret bk in selection", refClrs.clrBkCaretSel), GROUP_COLORS, clrBkCaretSel);

	const auto pColors = new CMFCPropertyGridProperty(L"Colors:");
	for (const auto& it : m_vecGrid) {
		if (it.eGroup == GROUP_COLORS) {
			pColors->AddSubItem(it.pProp);
			static_cast<CMFCPropertyGridColorProperty*>(it.pProp)->EnableOtherButton(L"Other");
		}
	}
	m_gridHexCtrl.AddProperty(pColors);

	return TRUE;
}


//CDlgSettings.
export class CDlgSettings final : public CDialogEx {
public:
	CDlgSettings(CWnd* pParent = nullptr) : CDialogEx(IDD_SETTINGS, pParent) {}
	auto DoModal(CAppSettings& refSettings) -> INT_PTR;
private:
	void DoDataExchange(CDataExchange* pDX)override;
	void OnCancel()override;
	BOOL OnInitDialog()override;
	void OnOK()override;
	afx_msg void OnTabSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	void SetCurrentTab(int iTab);
	DECLARE_MESSAGE_MAP();
private:
	CTabCtrl m_tabMain;
	std::unique_ptr<CDlgSettingsHexCtrl> m_pDlgSettingsHexCtrl { std::make_unique<CDlgSettingsHexCtrl>() };
	CAppSettings* m_pAppSettings { };
};

BEGIN_MESSAGE_MAP(CDlgSettings, CDialogEx)
	ON_NOTIFY(TCN_SELCHANGE, IDC_SETTINGS_TAB, &CDlgSettings::OnTabSelChanged)
END_MESSAGE_MAP()

auto CDlgSettings::DoModal(CAppSettings& refSettings)->INT_PTR
{
	m_pAppSettings = &refSettings;

	return CDialogEx::DoModal();
}


//CDlgSettings private methods.

void CDlgSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SETTINGS_TAB, m_tabMain);
}

void CDlgSettings::OnCancel()
{
	CDialogEx::OnCancel();
}

BOOL CDlgSettings::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_tabMain.InsertItem(0, L"HexCtrl");

	CRect rcTab;
	m_tabMain.GetItemRect(0, rcTab);
	CRect rcClient;
	GetClientRect(rcClient);
	CRect rcOK;
	GetDlgItem(IDOK)->GetWindowRect(rcOK);
	ScreenToClient(rcOK);

	//Child dialogs coordinates.
	const auto iX = rcTab.left;
	const auto iY = rcTab.bottom;
	const auto iWidth = rcClient.Width();
	const auto iHeight = rcOK.top - rcTab.Height() - (rcClient.bottom - rcOK.bottom);

	m_pDlgSettingsHexCtrl->Create(IDD_SETTINGSHEXCTRL, this, m_pAppSettings);
	m_pDlgSettingsHexCtrl->SetWindowPos(nullptr, iX, iY, iWidth, iHeight, SWP_NOZORDER | SWP_HIDEWINDOW);

	EnableDynamicLayout(TRUE);
	const auto pLayout = GetDynamicLayout();
	pLayout->Create(this);
	pLayout->AddItem(m_pDlgSettingsHexCtrl->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
	pLayout->AddItem(IDOK, CMFCDynamicLayout::MoveHorizontalAndVertical(100, 100), CMFCDynamicLayout::SizeNone());
	pLayout->AddItem(IDCANCEL, CMFCDynamicLayout::MoveHorizontalAndVertical(100, 100), CMFCDynamicLayout::SizeNone());

	SetCurrentTab(0); //Show HexCtrl settings.

	return TRUE;
}

void CDlgSettings::OnOK()
{
	m_pDlgSettingsHexCtrl->SaveSettings();

	CDialogEx::OnOK();
}

void CDlgSettings::OnTabSelChanged(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	SetCurrentTab(m_tabMain.GetCurSel());
}

void CDlgSettings::SetCurrentTab(int iTab)
{
	m_tabMain.SetCurSel(iTab);
	switch (iTab) {
	case 0:
		m_pDlgSettingsHexCtrl->ShowWindow(SW_SHOW);
		break;
	case 1:
		m_pDlgSettingsHexCtrl->ShowWindow(SW_HIDE);
		break;
	default:
		std::unreachable();
	}
}