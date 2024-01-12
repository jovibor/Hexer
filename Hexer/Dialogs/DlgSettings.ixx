module;
/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
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
#include <vector>
export module DlgSettings;

import HexerPropGridCtrl;
import AppSettings;
import Utility;


//CDlgSettingsGeneral.
class CDlgSettingsGeneral final : public CDialogEx {
public:
	auto Create(UINT nIDTemplate, CWnd* pParentWnd, CAppSettings* pAppSettings) -> BOOL;
	void ResetToDefaults();
	void SaveSettings();
private:
	struct GRIDDATA; //Forward declarations.
	void DoDataExchange(CDataExchange* pDX)override;
	BOOL OnInitDialog()override;
	enum class EGroup : std::uint8_t;
	enum class EName : std::uint8_t;
	DECLARE_MESSAGE_MAP();
private:
	CAppSettings* m_pAppSettings { };
	CHexerPropGridCtrl m_grid;
	std::vector<GRIDDATA> m_vecGrid;
	CFont m_fntGrid;
};

enum class CDlgSettingsGeneral::EGroup : std::uint8_t { };

enum class CDlgSettingsGeneral::EName : std::uint8_t { };

struct CDlgSettingsGeneral::GRIDDATA {
	CMFCPropertyGridProperty* pProp { };
	EGroup eGroup { };
	EName eName { };
};

BEGIN_MESSAGE_MAP(CDlgSettingsGeneral, CDialogEx)
END_MESSAGE_MAP()

auto CDlgSettingsGeneral::Create(UINT nIDTemplate, CWnd* pParentWnd, CAppSettings* pAppSettings)->BOOL
{
	assert(pAppSettings != nullptr);
	m_pAppSettings = pAppSettings;

	return CDialogEx::Create(nIDTemplate, pParentWnd);
}

void CDlgSettingsGeneral::ResetToDefaults()
{
	using enum EName;
	m_grid.SetRedraw(FALSE);


	m_grid.SetRedraw(TRUE);
	m_grid.RedrawWindow();
}

void CDlgSettingsGeneral::SaveSettings()
{
	using enum EName;
}


//CDlgSettingsGeneral private methods.

void CDlgSettingsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SETTINGSGENERAL_GRID, m_grid);
}

BOOL CDlgSettingsGeneral::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_grid.MarkModifiedProperties(1, 0);
	m_grid.EnableHeaderCtrl(TRUE, L"Property", L"Value");
	HDITEMW hdPropGrid { .mask = HDI_WIDTH, .cxy = 150 };
	m_grid.GetHeaderCtrl().SetItem(0, &hdPropGrid); //Property grid left column width.

	//Set new bigger font to the property.
	const auto pFont = m_grid.GetFont();
	LOGFONTW lf;
	pFont->GetLogFont(&lf);
	const auto lFontSize = MulDiv(-lf.lfHeight, 72, Ut::GetHiDPIInfo().iLOGPIXELSY) + 1;
	lf.lfHeight = -MulDiv(lFontSize, Ut::GetHiDPIInfo().iLOGPIXELSY, 72);
	m_fntGrid.CreateFontIndirectW(&lf);
	m_grid.SetFont(&m_fntGrid);

	return TRUE;
}


//CDlgSettingsHexCtrl.
class CDlgSettingsHexCtrl final : public CDialogEx {
public:
	auto Create(UINT nIDTemplate, CWnd* pParentWnd, CAppSettings* pAppSettings) -> BOOL;
	void ResetToDefaults();
	void SaveSettings();
private:
	struct GRIDDATA; //Forward declarations.
	enum class EGroup : std::uint8_t;
	enum class EName : std::uint8_t;
	void DoDataExchange(CDataExchange* pDX)override;
	[[nodiscard]] auto GetGridData(EName eName)const->const GRIDDATA*;
	[[nodiscard]] auto GetProperty(EName eName)const->CMFCPropertyGridProperty*;
	[[nodiscard]] auto GetPropOptDataDWORD(EName eName)const->DWORD;
	[[nodiscard]] auto GetPropOptDataULL(EName eName)const->DWORD_PTR;
	[[nodiscard]] auto GetPropValueDWORD(EName eName)const->DWORD;
	[[nodiscard]] auto GetPropValueFLOAT(EName eName)const->float;
	[[nodiscard]] auto GetPropValuePLOGFONT(EName eName)const->LOGFONTW*;
	[[nodiscard]] auto GetPropValueRGB(EName eName)const->COLORREF;
	[[nodiscard]] auto GetPropValueWCHAR(EName eName)const->wchar_t;
	[[nodiscard]] auto GetPropValueWSTR(EName eName)const->std::wstring_view;
	void OnCancel()override;
	BOOL OnInitDialog()override;
	void SetPropValueDWORD(EName eName, DWORD dwValue);
	void SetPropValueFLOAT(EName eName, float flValue);
	void SetPropValueByOptData(EName eName, DWORD_PTR dwData); //Value according to inner dwData.
	void SetPropValueLOGFONT(EName eName, const LOGFONTW& lf);
	void SetPropValueRGB(EName eName, COLORREF clrValue);
	void SetPropValueWCHAR(EName eName, wchar_t wchValue);
	void SetPropValueWSTR(EName eName, LPCWSTR pwstr);
	DECLARE_MESSAGE_MAP();
private:
	CAppSettings* m_pAppSettings { };
	CHexerPropGridCtrl m_grid;
	std::vector<GRIDDATA> m_vecGrid;
	CFont m_fntGrid;
};

enum class CDlgSettingsHexCtrl::EGroup : std::uint8_t {
	GROUP_GENERAL, GROUP_COLORS
};

enum class CDlgSettingsHexCtrl::EName : std::uint8_t {
	dwCapacity, dwGroupSize, dwPageSize, wchUnprintable, wstrDateFormat, wstrScrollLines, flScrollRatio,
	wstrInfoBar, wstrOffsetHex, dwCharsExtraSpace, stLogFont,
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

void CDlgSettingsHexCtrl::ResetToDefaults()
{
	using enum EName;
	m_grid.SetRedraw(FALSE);
	const auto& refDefs = CAppSettings::GetHexCtrlDefs();
	SetPropValueDWORD(dwCapacity, refDefs.dwCapacity);
	SetPropValueDWORD(dwGroupSize, refDefs.dwGroupSize);
	SetPropValueDWORD(dwPageSize, refDefs.dwPageSize);
	SetPropValueDWORD(dwCharsExtraSpace, refDefs.dwCharsExtraSpace);
	SetPropValueFLOAT(flScrollRatio, refDefs.flScrollRatio);
	SetPropValueWCHAR(wchUnprintable, refDefs.wchUnprintable);
	SetPropValueByOptData(wstrDateFormat, 0xFFFFFFFFUL); //User default.
	SetPropValueByOptData(wstrScrollLines, 1UL);
	SetPropValueByOptData(wstrInfoBar, 1UL);
	SetPropValueByOptData(wstrOffsetHex, 1UL);
	SetPropValueLOGFONT(stLogFont, refDefs.stLogFont);
	const auto& refClrs = refDefs.stClrs;
	SetPropValueRGB(clrFontHex, refClrs.clrFontHex);
	SetPropValueRGB(clrFontText, refClrs.clrFontText);
	SetPropValueRGB(clrFontSel, refClrs.clrFontSel);
	SetPropValueRGB(clrFontDataInterp, refClrs.clrFontDataInterp);
	SetPropValueRGB(clrFontCaption, refClrs.clrFontCaption);
	SetPropValueRGB(clrFontInfoParam, refClrs.clrFontInfoParam);
	SetPropValueRGB(clrFontInfoData, refClrs.clrFontInfoData);
	SetPropValueRGB(clrFontCaret, refClrs.clrFontCaret);
	SetPropValueRGB(clrBk, refClrs.clrBk);
	SetPropValueRGB(clrBkSel, refClrs.clrBkSel);
	SetPropValueRGB(clrBkDataInterp, refClrs.clrBkDataInterp);
	SetPropValueRGB(clrBkInfoBar, refClrs.clrBkInfoBar);
	SetPropValueRGB(clrBkCaret, refClrs.clrBkCaret);
	SetPropValueRGB(clrBkCaretSel, refClrs.clrBkCaretSel);
	m_grid.SetRedraw(TRUE);
	m_grid.RedrawWindow();
}

void CDlgSettingsHexCtrl::SaveSettings()
{
	using enum EName;
	auto& refSett = m_pAppSettings->GetHexCtrlSettings();
	refSett.stLogFont = *GetPropValuePLOGFONT(stLogFont);
	refSett.dwCapacity = GetPropValueDWORD(dwCapacity);
	refSett.dwGroupSize = GetPropValueDWORD(dwGroupSize);
	refSett.dwPageSize = GetPropValueDWORD(dwPageSize);
	refSett.dwCharsExtraSpace = GetPropValueDWORD(dwCharsExtraSpace);
	refSett.dwDateFormat = GetPropOptDataDWORD(wstrDateFormat);
	refSett.flScrollRatio = GetPropValueFLOAT(flScrollRatio);
	const auto wchUnprint = GetPropValueWCHAR(wchUnprintable);
	refSett.wchUnprintable = wchUnprint == 0 ? L' ' : wchUnprint;
	refSett.fScrollLines = GetPropOptDataDWORD(wstrScrollLines);
	refSett.fInfoBar = GetPropOptDataDWORD(wstrInfoBar);
	refSett.fOffsetHex = GetPropOptDataDWORD(wstrOffsetHex);
	auto& refClrs = refSett.stClrs;
	refClrs.clrFontHex = GetPropValueRGB(clrFontHex);
	refClrs.clrFontText = GetPropValueRGB(clrFontText);
	refClrs.clrFontSel = GetPropValueRGB(clrFontSel);
	refClrs.clrFontDataInterp = GetPropValueRGB(clrFontDataInterp);
	refClrs.clrFontCaption = GetPropValueRGB(clrFontCaption);
	refClrs.clrFontInfoParam = GetPropValueRGB(clrFontInfoParam);
	refClrs.clrFontInfoData = GetPropValueRGB(clrFontInfoData);
	refClrs.clrFontCaret = GetPropValueRGB(clrFontCaret);
	refClrs.clrBk = GetPropValueRGB(clrBk);
	refClrs.clrBkSel = GetPropValueRGB(clrBkSel);
	refClrs.clrBkDataInterp = GetPropValueRGB(clrBkDataInterp);
	refClrs.clrBkInfoBar = GetPropValueRGB(clrBkInfoBar);
	refClrs.clrBkCaret = GetPropValueRGB(clrBkCaret);
	refClrs.clrBkCaretSel = GetPropValueRGB(clrBkCaretSel);
}


//CDlgSettingsHexCtrl private methods.

void CDlgSettingsHexCtrl::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SETTINGSHEXCTRL_GRID, m_grid);
}

auto CDlgSettingsHexCtrl::GetGridData(EName eName)const->const GRIDDATA*
{
	return &*std::find_if(m_vecGrid.begin(), m_vecGrid.end(), [=](const GRIDDATA& refData) {
		return refData.eName == eName; });
}

auto CDlgSettingsHexCtrl::GetProperty(EName eName)const->CMFCPropertyGridProperty*
{
	return GetGridData(eName)->pProp;
}

auto CDlgSettingsHexCtrl::GetPropOptDataDWORD(EName eName)const->DWORD
{
	return static_cast<DWORD>(GetPropOptDataULL(eName));
}

auto CDlgSettingsHexCtrl::GetPropOptDataULL(EName eName) const -> DWORD_PTR
{
	return GetProperty(eName)->GetData();
}

auto CDlgSettingsHexCtrl::GetPropValueDWORD(EName eName)const->DWORD
{
	return GetProperty(eName)->GetValue().uintVal;
}

auto CDlgSettingsHexCtrl::GetPropValueFLOAT(EName eName)const->float
{
	return GetProperty(eName)->GetValue().fltVal;
}

auto CDlgSettingsHexCtrl::GetPropValuePLOGFONT(EName eName)const->LOGFONTW*
{
	return static_cast<CMFCPropertyGridFontProperty*>(GetProperty(eName))->GetLogFont();
}

auto CDlgSettingsHexCtrl::GetPropValueRGB(EName eName)const->COLORREF
{
	return static_cast<CMFCPropertyGridColorProperty*>(GetProperty(eName))->GetColor();
}

auto CDlgSettingsHexCtrl::GetPropValueWCHAR(EName eName)const->wchar_t
{
	const auto wsv = GetPropValueWSTR(eName);
	return wsv.empty() ? L'\0' : wsv[0];
}

auto CDlgSettingsHexCtrl::GetPropValueWSTR(EName eName)const->std::wstring_view
{
	return GetProperty(eName)->GetValue().bstrVal;
}

void CDlgSettingsHexCtrl::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

BOOL CDlgSettingsHexCtrl::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_grid.MarkModifiedProperties(1, 0);
	m_grid.EnableHeaderCtrl(TRUE, L"Property", L"Value");
	HDITEMW hdPropGrid { .mask = HDI_WIDTH, .cxy = 150 };
	m_grid.GetHeaderCtrl().SetItem(0, &hdPropGrid); //Property grid left column width.

	//Set new bigger font to the property.
	const auto pFont = m_grid.GetFont();
	LOGFONTW lf;
	pFont->GetLogFont(&lf);
	const auto lFontSize = MulDiv(-lf.lfHeight, 72, Ut::GetHiDPIInfo().iLOGPIXELSY) + 1;
	lf.lfHeight = -MulDiv(lFontSize, Ut::GetHiDPIInfo().iLOGPIXELSY, 72);
	m_fntGrid.CreateFontIndirectW(&lf);
	m_grid.SetFont(&m_fntGrid);

	auto& refSett = m_pAppSettings->GetHexCtrlSettings();

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

	const auto& refDate = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Date format:", L""), GROUP_GENERAL, wstrDateFormat);
	const auto pPropDate = static_cast<CHexerPropGridProp*>(refDate.pProp);
	pPropDate->AddOptionEx(L"User default", 0xFFFFFFFFUL);
	pPropDate->AddOptionEx(L"MM/DD/YYYY", 0UL);
	pPropDate->AddOptionEx(L"DD/MM/YYYY", 1UL);
	pPropDate->AddOptionEx(L"YYYY/MM/DD", 2UL);
	pPropDate->SetValueFromData(refSett.dwDateFormat);
	pPropDate->AllowEdit(FALSE);

	const auto& refScroll = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Scroll lines or ratio:", L""), GROUP_GENERAL, wstrScrollLines);
	const auto pPropScroll = static_cast<CHexerPropGridProp*>(refScroll.pProp);
	pPropScroll->AddOptionEx(L"Lines", 1UL);
	pPropScroll->AddOptionEx(L"Ratio", 0UL);
	pPropScroll->SetValueFromData(refSett.fScrollLines);
	pPropScroll->AllowEdit(FALSE);

	const auto& refScrollSize = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Scroll size:",
		static_cast<_variant_t>(refSett.flScrollRatio), 0, 0, 0, 0, L"0123456789."), GROUP_GENERAL, flScrollRatio);
	refScrollSize.pProp->AllowEdit(TRUE);

	const auto& refInfoBar = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Show Info bar:", L""), GROUP_GENERAL, wstrInfoBar);
	const auto pPropInfoBar = static_cast<CHexerPropGridProp*>(refInfoBar.pProp);
	pPropInfoBar->AddOptionEx(L"Show", 1UL);
	pPropInfoBar->AddOptionEx(L"Hide", 0UL);
	pPropInfoBar->SetValueFromData(refSett.fInfoBar);
	pPropInfoBar->AllowEdit(FALSE);

	const auto& refOffsetMode = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Offset mode:", L""), GROUP_GENERAL, wstrOffsetHex);
	const auto pPropOffsetMode = static_cast<CHexerPropGridProp*>(refOffsetMode.pProp);
	pPropOffsetMode->AddOptionEx(L"Hex", 1UL);
	pPropOffsetMode->AddOptionEx(L"Decimal", 0UL);
	pPropOffsetMode->SetValueFromData(refSett.fOffsetHex);
	pPropOffsetMode->AllowEdit(FALSE);

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
	m_grid.AddProperty(pAppear);

	//HexCtrl colors.
	const auto& refClrs = m_pAppSettings->GetHexCtrlSettings().stClrs;
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Hex", refClrs.clrFontHex), GROUP_COLORS, clrFontHex);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Text", refClrs.clrFontText), GROUP_COLORS, clrFontText);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Selection", refClrs.clrFontSel), GROUP_COLORS, clrFontSel);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Data Interpreter", refClrs.clrFontDataInterp), GROUP_COLORS, clrFontDataInterp);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Caption", refClrs.clrFontCaption), GROUP_COLORS, clrFontCaption);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Info parameters", refClrs.clrFontInfoParam), GROUP_COLORS, clrFontInfoParam);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Info data", refClrs.clrFontInfoData), GROUP_COLORS, clrFontInfoData);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Caret", refClrs.clrFontCaret), GROUP_COLORS, clrFontCaret);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Background", refClrs.clrBk), GROUP_COLORS, clrBk);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Bk Selection", refClrs.clrBkSel), GROUP_COLORS, clrBkSel);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Bk Data Interpreter", refClrs.clrBkDataInterp), GROUP_COLORS, clrBkDataInterp);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Bk Info bar", refClrs.clrBkInfoBar), GROUP_COLORS, clrBkInfoBar);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Bk Caret", refClrs.clrBkCaret), GROUP_COLORS, clrBkCaret);
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Bk Caret in selection", refClrs.clrBkCaretSel), GROUP_COLORS, clrBkCaretSel);

	const auto pColors = new CMFCPropertyGridProperty(L"Colors:");
	for (const auto& it : m_vecGrid) {
		if (it.eGroup == GROUP_COLORS) {
			pColors->AddSubItem(it.pProp);
			static_cast<CMFCPropertyGridColorProperty*>(it.pProp)->EnableOtherButton(L"Other");
		}
	}
	m_grid.AddProperty(pColors);

	return TRUE;
}

void CDlgSettingsHexCtrl::SetPropValueDWORD(EName eName, DWORD dwValue)
{
	GetProperty(eName)->SetValue(static_cast<_variant_t>(dwValue));
}

void CDlgSettingsHexCtrl::SetPropValueFLOAT(EName eName, float flValue)
{
	GetProperty(eName)->SetValue(static_cast<_variant_t>(flValue));
}

void CDlgSettingsHexCtrl::SetPropValueByOptData(EName eName, DWORD_PTR dwData)
{
	static_cast<CHexerPropGridProp*>(GetProperty(eName))->SetValueFromData(dwData);
}

void CDlgSettingsHexCtrl::SetPropValueLOGFONT(EName eName, const LOGFONTW& lf)
{
	*GetPropValuePLOGFONT(eName) = lf;
}

void CDlgSettingsHexCtrl::SetPropValueRGB(EName eName, COLORREF clrValue)
{
	static_cast<CMFCPropertyGridColorProperty*>(GetProperty(eName))->SetColor(clrValue);
}

void CDlgSettingsHexCtrl::SetPropValueWCHAR(EName eName, wchar_t wchValue)
{
	GetProperty(eName)->SetValue(static_cast<_variant_t>(std::format(L"{}", wchValue).data()));
}

void CDlgSettingsHexCtrl::SetPropValueWSTR(EName eName, LPCWSTR pwstr)
{
	GetProperty(eName)->SetValue(pwstr);
}


//CDlgSettings.
export class CDlgSettings final : public CDialogEx {
public:
	CDlgSettings(CWnd* pParent = nullptr) : CDialogEx(IDD_SETTINGS, pParent) {}
	auto DoModal(CAppSettings& refSettings) -> INT_PTR;
private:
	enum class ETabs : std::uint8_t; //All the tabs.
	void DoDataExchange(CDataExchange* pDX)override;
	void OnCancel()override;
	afx_msg void OnClickedDefaults();
	BOOL OnInitDialog()override;
	void OnOK()override;
	afx_msg void OnTabSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	void SetCurrentTab(ETabs eTab);
	[[nodiscard]] auto TabIDToName(int iTab)const->ETabs;
	[[nodiscard]] auto TabNameToID(ETabs eTab)const->int;
	DECLARE_MESSAGE_MAP();
private:
	CTabCtrl m_tabMain;
	std::unique_ptr<CDlgSettingsGeneral> m_pDlgSettingsGeneral { std::make_unique<CDlgSettingsGeneral>() };
	std::unique_ptr<CDlgSettingsHexCtrl> m_pDlgSettingsHexCtrl { std::make_unique<CDlgSettingsHexCtrl>() };
	CAppSettings* m_pAppSettings { };
};

enum class CDlgSettings::ETabs : std::uint8_t {
	TAB_GENERAL = 1, TAB_HEXCTRL
};

BEGIN_MESSAGE_MAP(CDlgSettings, CDialogEx)
	ON_BN_CLICKED(IDC_SETTINGS_DEFS, &CDlgSettings::OnClickedDefaults)
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

void CDlgSettings::OnClickedDefaults()
{
	if (MessageBoxW(L"Reset all settings to their defaults?", L"Defaults", MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;

	m_pDlgSettingsHexCtrl->ResetToDefaults();
}

BOOL CDlgSettings::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_tabMain.InsertItem(TCIF_TEXT | TCIF_PARAM, 0, L"General", 0, static_cast<int>(ETabs::TAB_GENERAL));
	m_tabMain.InsertItem(TCIF_TEXT | TCIF_PARAM, 1, L"HexCtrl", 0, static_cast<int>(ETabs::TAB_HEXCTRL));

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

	m_pDlgSettingsGeneral->Create(IDD_SETTINGSGENERAL, this, m_pAppSettings);
	m_pDlgSettingsGeneral->SetWindowPos(nullptr, iX, iY, iWidth, iHeight, SWP_NOZORDER | SWP_HIDEWINDOW);
	m_pDlgSettingsHexCtrl->Create(IDD_SETTINGSHEXCTRL, this, m_pAppSettings);
	m_pDlgSettingsHexCtrl->SetWindowPos(nullptr, iX, iY, iWidth, iHeight, SWP_NOZORDER | SWP_HIDEWINDOW);

	EnableDynamicLayout(TRUE);
	const auto pLayout = GetDynamicLayout();
	pLayout->Create(this);
	pLayout->AddItem(m_pDlgSettingsHexCtrl->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
	pLayout->AddItem(m_pDlgSettingsGeneral->m_hWnd, CMFCDynamicLayout::MoveNone(), CMFCDynamicLayout::SizeHorizontalAndVertical(100, 100));
	pLayout->AddItem(IDOK, CMFCDynamicLayout::MoveHorizontalAndVertical(100, 100), CMFCDynamicLayout::SizeNone());
	pLayout->AddItem(IDCANCEL, CMFCDynamicLayout::MoveHorizontalAndVertical(100, 100), CMFCDynamicLayout::SizeNone());
	pLayout->AddItem(IDC_SETTINGS_DEFS, CMFCDynamicLayout::MoveVertical(100), CMFCDynamicLayout::SizeNone());

	SetCurrentTab(ETabs::TAB_HEXCTRL); //Setting startup tab.

	return TRUE;
}

void CDlgSettings::OnOK()
{
	m_pDlgSettingsHexCtrl->SaveSettings();

	CDialogEx::OnOK();
}

void CDlgSettings::OnTabSelChanged(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	SetCurrentTab(TabIDToName(m_tabMain.GetCurSel()));
}

void CDlgSettings::SetCurrentTab(ETabs eTab)
{
	m_tabMain.SetCurSel(TabNameToID(eTab));
	using enum ETabs;
	switch (eTab) {
	case TAB_GENERAL:
		m_pDlgSettingsGeneral->ShowWindow(SW_SHOW);
		m_pDlgSettingsHexCtrl->ShowWindow(SW_HIDE);
		break;
	case TAB_HEXCTRL:
		m_pDlgSettingsGeneral->ShowWindow(SW_HIDE);
		m_pDlgSettingsHexCtrl->ShowWindow(SW_SHOW);
		break;
	default:
		std::unreachable();
	}
}

auto CDlgSettings::TabIDToName(int iTab)const->ETabs
{
	TCITEMW tci { .mask { TCIF_PARAM } };
	m_tabMain.GetItem(iTab, &tci);

	return static_cast<ETabs>(tci.lParam);
}

auto CDlgSettings::TabNameToID(ETabs eTab)const->int {
	for (auto i { 0 }; i < m_tabMain.GetItemCount(); ++i) {
		TCITEMW tci { .mask { TCIF_PARAM } };
		m_tabMain.GetItem(i, &tci);
		if (tci.lParam == static_cast<int>(eTab)) {
			return i;
		}
	}

	return -1;
}