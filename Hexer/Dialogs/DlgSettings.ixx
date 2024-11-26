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
#include <utility>
#include <vector>
export module DlgSettings;

import HexerPropGridCtrl;
import AppSettings;
import Utility;

class IPropsHelper {
public:
	struct GRIDDATA {
		CMFCPropertyGridProperty* pProp { };
		std::uint8_t ui8Group { };
		std::uint8_t ui8Name { };
	};

	[[nodiscard]] auto GetGridData(std::uint8_t ui8Name)const->const GRIDDATA*;
	[[nodiscard]] auto GetProperty(std::uint8_t ui8Name)const->CMFCPropertyGridProperty*;
	[[nodiscard]] auto GetPropOptDataDWORD(std::uint8_t ui8Name)const->DWORD;
	[[nodiscard]] auto GetPropOptDataULL(std::uint8_t ui8Name)const->DWORD_PTR;
	[[nodiscard]] auto GetPropValueDWORD(std::uint8_t ui8Name)const->DWORD;
	[[nodiscard]] auto GetPropValueFLOAT(std::uint8_t ui8Name)const->float;
	[[nodiscard]] auto GetPropValuePLOGFONT(std::uint8_t ui8Name)const->LOGFONTW*;
	[[nodiscard]] auto GetPropValueRGB(std::uint8_t ui8Name)const->COLORREF;
	[[nodiscard]] auto GetPropValueWCHAR(std::uint8_t ui8Name)const->wchar_t;
	[[nodiscard]] auto GetPropValueWSTR(std::uint8_t ui8Name)const->std::wstring_view;
	void SetPropOptValueByData(std::uint8_t ui8Name, DWORD_PTR dwData)const; //Value according to inner user dwData.
	void SetPropValueDWORD(std::uint8_t ui8Name, DWORD dwValue)const;
	void SetPropValueFLOAT(std::uint8_t ui8Name, float flValue)const;
	void SetPropValueLOGFONT(std::uint8_t ui8Name, const LOGFONTW& lf)const;
	void SetPropValueRGB(std::uint8_t ui8Name, COLORREF clrValue)const;
	void SetPropValueWCHAR(std::uint8_t ui8Name, wchar_t wchValue)const;
	void SetPropValueWSTR(std::uint8_t ui8Name, LPCWSTR pwstr)const;
private:
	[[nodiscard]] virtual auto GetGridVec()const->const std::vector<GRIDDATA> & = 0;
};

auto IPropsHelper::GetGridData(std::uint8_t ui8Name)const->const GRIDDATA*
{
	const auto it = std::find_if(GetGridVec().begin(), GetGridVec().end(), [=](const GRIDDATA& refData) {
		return refData.ui8Name == ui8Name; });
	assert(it != GetGridVec().end());

	return it != GetGridVec().end() ? &*it : nullptr;
}

auto IPropsHelper::GetProperty(std::uint8_t ui8Name)const->CMFCPropertyGridProperty*
{
	return GetGridData(ui8Name)->pProp;
}

auto IPropsHelper::GetPropOptDataDWORD(std::uint8_t ui8Name)const->DWORD
{
	return static_cast<DWORD>(GetPropOptDataULL(ui8Name));
}

auto IPropsHelper::GetPropOptDataULL(std::uint8_t ui8Name) const -> DWORD_PTR
{
	return GetProperty(ui8Name)->GetData();
}

auto IPropsHelper::GetPropValueDWORD(std::uint8_t ui8Name)const->DWORD
{
	return GetProperty(ui8Name)->GetValue().uintVal;
}

auto IPropsHelper::GetPropValueFLOAT(std::uint8_t ui8Name)const->float
{
	return GetProperty(ui8Name)->GetValue().fltVal;
}

auto IPropsHelper::GetPropValuePLOGFONT(std::uint8_t ui8Name)const->LOGFONTW*
{
	return static_cast<CMFCPropertyGridFontProperty*>(GetProperty(ui8Name))->GetLogFont();
}

auto IPropsHelper::GetPropValueRGB(std::uint8_t ui8Name)const->COLORREF
{
	return static_cast<CMFCPropertyGridColorProperty*>(GetProperty(ui8Name))->GetColor();
}

auto IPropsHelper::GetPropValueWCHAR(std::uint8_t ui8Name)const->wchar_t
{
	const auto wsv = GetPropValueWSTR(ui8Name);
	return wsv.empty() ? L'\0' : wsv[0];
}

auto IPropsHelper::GetPropValueWSTR(std::uint8_t ui8Name)const->std::wstring_view
{
	return GetProperty(ui8Name)->GetValue().bstrVal;
}

void IPropsHelper::SetPropOptValueByData(std::uint8_t ui8Name, DWORD_PTR dwData)const
{
	static_cast<CHexerPropGridProp*>(GetProperty(ui8Name))->SetValueFromData(dwData);
}

void IPropsHelper::SetPropValueDWORD(std::uint8_t ui8Name, DWORD dwValue)const
{
	GetProperty(ui8Name)->SetValue(static_cast<_variant_t>(dwValue));
}

void IPropsHelper::SetPropValueFLOAT(std::uint8_t ui8Name, float flValue)const
{
	GetProperty(ui8Name)->SetValue(static_cast<_variant_t>(flValue));
}

void IPropsHelper::SetPropValueLOGFONT(std::uint8_t ui8Name, const LOGFONTW& lf)const
{
	*GetPropValuePLOGFONT(ui8Name) = lf;
}

void IPropsHelper::SetPropValueRGB(std::uint8_t ui8Name, COLORREF clrValue)const
{
	static_cast<CMFCPropertyGridColorProperty*>(GetProperty(ui8Name))->SetColor(clrValue);
}

void IPropsHelper::SetPropValueWCHAR(std::uint8_t ui8Name, wchar_t wchValue)const
{
	GetProperty(ui8Name)->SetValue(static_cast<_variant_t>(std::format(L"{}", wchValue).data()));
}

void IPropsHelper::SetPropValueWSTR(std::uint8_t ui8Name, LPCWSTR pwstr)const
{
	GetProperty(ui8Name)->SetValue(pwstr);
}


//CDlgSettingsGeneral.
class CDlgSettingsGeneral final : public CDialogEx, IPropsHelper {
public:
	auto Create(UINT nIDTemplate, CWnd* pParentWnd, CAppSettings* pAppSettings) -> BOOL;
	void ResetToDefaults();
	void SaveSettings();
private:
	void DoDataExchange(CDataExchange* pDX)override;
	[[nodiscard]] auto GetGridVec()const->const std::vector<GRIDDATA> & override;
	[[nodiscard]] bool IsModified()const;
	void OnCancel()override;
	BOOL OnInitDialog()override;
	auto OnPropertyDataChanged(WPARAM wParam, LPARAM lParam) -> LRESULT;
	enum class EGroup : std::uint8_t;
	enum class EName : std::uint8_t;
	DECLARE_MESSAGE_MAP();
private:
	CAppSettings* m_pAppSettings { };
	CHexerPropGridCtrl m_grid;
	std::vector<GRIDDATA> m_vecGrid;
	CFont m_fntGrid;
	bool m_fModified { false }; //Some setting has been modified.
};

enum class CDlgSettingsGeneral::EGroup : std::uint8_t {
	GROUP_GENERAL, GROUP_IO
};

enum class CDlgSettingsGeneral::EName : std::uint8_t {
	fMultipleInst, dwRFLSize, eStartup, fWindowsMenu, eDataAccessMode, eDataIOMode
};

BEGIN_MESSAGE_MAP(CDlgSettingsGeneral, CDialogEx)
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CDlgSettingsGeneral::OnPropertyDataChanged)
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
	const auto& refDefs = CAppSettings::GetGeneralDefs();
	SetPropOptValueByData(std::to_underlying(fMultipleInst), refDefs.fMultipleInst);
	SetPropValueDWORD(std::to_underlying(dwRFLSize), refDefs.dwRFLSize);
	SetPropOptValueByData(std::to_underlying(eStartup), std::to_underlying(refDefs.eStartup));
	SetPropOptValueByData(std::to_underlying(fWindowsMenu), refDefs.fWindowsMenu);
	SetPropOptValueByData(std::to_underlying(eDataAccessMode), std::to_underlying(refDefs.eDataAccessMode));
	SetPropOptValueByData(std::to_underlying(eDataIOMode), std::to_underlying(refDefs.eDataIOMode));
	m_grid.SetRedraw(TRUE);
	m_grid.RedrawWindow();
	m_fModified = true;
}

void CDlgSettingsGeneral::SaveSettings()
{
	if (!IsModified())
		return;

	using enum EName;
	auto& refSett = m_pAppSettings->GetGeneralSettings();
	refSett.fMultipleInst = GetPropOptDataDWORD(std::to_underlying(fMultipleInst));
	refSett.dwRFLSize = GetPropValueDWORD(std::to_underlying(dwRFLSize));
	refSett.eStartup = static_cast<CAppSettings::EStartup>(GetPropOptDataDWORD(std::to_underlying(eStartup)));
	refSett.fWindowsMenu = GetPropOptDataDWORD(std::to_underlying(fWindowsMenu));
	refSett.eDataAccessMode = static_cast<Ut::EDataAccessMode>(GetPropOptDataDWORD(std::to_underlying(eDataAccessMode)));
	refSett.eDataIOMode = static_cast<Ut::EDataIOMode>(GetPropOptDataDWORD(std::to_underlying(eDataIOMode)));
	m_fModified = false;
}


//CDlgSettingsGeneral private methods.

void CDlgSettingsGeneral::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SETTINGSGENERAL_GRID, m_grid);
}

auto CDlgSettingsGeneral::GetGridVec()const->const std::vector<GRIDDATA>&
{
	return m_vecGrid;
}

bool CDlgSettingsGeneral::IsModified()const
{
	return m_fModified;
}

void CDlgSettingsGeneral::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

BOOL CDlgSettingsGeneral::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_grid.MarkModifiedProperties(TRUE, FALSE);
	m_grid.EnableHeaderCtrl(TRUE, L"Property", L"Value");
	HDITEMW hdPropGrid { .mask = HDI_WIDTH, .cxy = 190 };
	m_grid.GetHeaderCtrl().SetItem(0, &hdPropGrid); //Property grid left column width.

	//Set new bigger font to the property.
	const auto pFont = m_grid.GetFont();
	LOGFONTW lf;
	pFont->GetLogFont(&lf);
	const auto lFontSize = MulDiv(-lf.lfHeight, 72, Ut::GetHiDPIInfo().iLOGPIXELSY) + 1;
	lf.lfHeight = -MulDiv(lFontSize, Ut::GetHiDPIInfo().iLOGPIXELSY, 72);
	m_fntGrid.CreateFontIndirectW(&lf);
	m_grid.SetFont(&m_fntGrid);

	const auto& refSett = m_pAppSettings->GetGeneralSettings();

	using enum EGroup; using enum EName;
	const auto& refInst = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Allowed App Instances:", L""),
		std::to_underlying(GROUP_GENERAL), std::to_underlying(fMultipleInst));
	const auto pPropInst = static_cast<CHexerPropGridProp*>(refInst.pProp);
	pPropInst->AddOptionEx(L"Single", 0UL);
	pPropInst->AddOptionEx(L"Multiple", 1UL);
	pPropInst->SetValueFromData(refSett.fMultipleInst);
	pPropInst->AllowEdit(FALSE);

	const auto& refStartup = m_vecGrid.emplace_back(new CHexerPropGridProp(L"On Startup:", L""),
		std::to_underlying(GROUP_GENERAL), std::to_underlying(eStartup));
	const auto pPropStartup = static_cast<CHexerPropGridProp*>(refStartup.pProp);
	pPropStartup->AddOptionEx(L"Do Nothing", std::to_underlying(CAppSettings::EStartup::DO_NOTHING));
	pPropStartup->AddOptionEx(L"Restore Last Opened Files", std::to_underlying(CAppSettings::EStartup::RESTORE_LAST_OPENED));
	pPropStartup->AddOptionEx(L"Show File Open Dialog", std::to_underlying(CAppSettings::EStartup::SHOW_FOD));
	pPropStartup->SetValueFromData(std::to_underlying(refSett.eStartup));
	pPropStartup->AllowEdit(FALSE);

	const auto& refRFLSize = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Recent Files List Size:",
		static_cast<_variant_t>(refSett.dwRFLSize), 0, 0, 0, 0, L"0123456789"), std::to_underlying(GROUP_GENERAL),
		std::to_underlying(dwRFLSize));
	refRFLSize.pProp->EnableSpinControl(TRUE, 1, 20);
	refRFLSize.pProp->AllowEdit(TRUE);

	const auto& refWinMenu = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Show in Windows Context Menu:", L""),
		std::to_underlying(GROUP_GENERAL), std::to_underlying(fWindowsMenu));
	const auto pPropWindowsMenu = static_cast<CHexerPropGridProp*>(refWinMenu.pProp);
	pPropWindowsMenu->AddOptionEx(L"Yes", 1UL);
	pPropWindowsMenu->AddOptionEx(L"No", 0UL);
	pPropWindowsMenu->SetValueFromData(refSett.fWindowsMenu);
	pPropWindowsMenu->AllowEdit(FALSE);

	const auto pGeneral = new CMFCPropertyGridProperty(L"General:");
	for (const auto& it : m_vecGrid) {
		if (it.ui8Group == std::to_underlying(GROUP_GENERAL)) {
			pGeneral->AddSubItem(it.pProp);
		}
	}
	m_grid.AddProperty(pGeneral);

	const auto& refDataAccess = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Data Access Mode:", L""),
	std::to_underlying(GROUP_IO), std::to_underlying(eDataAccessMode));
	const auto pPropDataAccess = static_cast<CHexerPropGridProp*>(refDataAccess.pProp);
	pPropDataAccess->AddOptionEx(L"Read Only", std::to_underlying(Ut::EDataAccessMode::DA_RO));
	pPropDataAccess->AddOptionEx(L"Read/Write Default", std::to_underlying(Ut::EDataAccessMode::DA_RW_DEFAULT));
	pPropDataAccess->AddOptionEx(L"Read/Write In-Place", std::to_underlying(Ut::EDataAccessMode::DA_RW_INPLACE));
	pPropDataAccess->SetValueFromData(std::to_underlying(refSett.eDataAccessMode));
	pPropDataAccess->AllowEdit(FALSE);

	const auto& refDataIO = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Data IO Mode:", L""),
			std::to_underlying(GROUP_IO), std::to_underlying(eDataIOMode));
	const auto pPropDataIO = static_cast<CHexerPropGridProp*>(refDataIO.pProp);
	pPropDataIO->AddOptionEx(L"Memory Mapped File", std::to_underlying(Ut::EDataIOMode::DATA_MMAP));
	pPropDataIO->AddOptionEx(L"Data IO Buffered", std::to_underlying(Ut::EDataIOMode::DATA_IOBUFF));
	pPropDataIO->AddOptionEx(L"Data IO Immediate", std::to_underlying(Ut::EDataIOMode::DATA_IOIMMEDIATE));
	pPropDataIO->SetValueFromData(std::to_underlying(refSett.eDataIOMode));
	pPropDataIO->AllowEdit(FALSE);

	const auto pIO = new CMFCPropertyGridProperty(L"Data IO:");
	for (const auto& it : m_vecGrid) {
		if (it.ui8Group == std::to_underlying(GROUP_IO)) {
			pIO->AddSubItem(it.pProp);
		}
	}
	m_grid.AddProperty(pIO);

	return TRUE;
}

auto CDlgSettingsGeneral::OnPropertyDataChanged(WPARAM wParam, LPARAM lParam)->LRESULT
{
	m_fModified = true;
	GetParent()->SendMessageW(AFX_WM_PROPERTY_CHANGED, wParam, lParam);

	return TRUE;
}


//CDlgSettingsHexCtrl.
class CDlgSettingsHexCtrl final : public CDialogEx, IPropsHelper {
public:
	auto Create(UINT nIDTemplate, CWnd* pParentWnd, CAppSettings* pAppSettings) -> BOOL;
	void ResetToDefaults();
	void SaveSettings();
private:
	enum class EGroup : std::uint8_t;
	enum class EName : std::uint8_t;
	void DoDataExchange(CDataExchange* pDX)override;
	[[nodiscard]] auto GetGridVec()const->const std::vector<GRIDDATA> & override;
	[[nodiscard]] bool IsModified()const;
	void OnCancel()override;
	BOOL OnInitDialog()override;
	auto OnPropertyDataChanged(WPARAM wParam, LPARAM lParam) -> LRESULT;
	DECLARE_MESSAGE_MAP();
private:
	CAppSettings* m_pAppSettings { };
	CHexerPropGridCtrl m_grid;
	std::vector<GRIDDATA> m_vecGrid;
	CFont m_fntGrid;
	bool m_fModified { false };
};

enum class CDlgSettingsHexCtrl::EGroup : std::uint8_t {
	GROUP_GENERAL, GROUP_COLORS
};

enum class CDlgSettingsHexCtrl::EName : std::uint8_t {
	dwCapacity, dwGroupSize, dwPageSize, wchUnprintable, wchDateSepar, wstrDateFormat, fScrollLines,
	flScrollRatio, fInfoBar, fOffsetHex, dwCharsExtraSpace, stLogFont,
	clrFontHex, clrFontText, clrFontSel, clrFontDataInterp, clrFontCaption, clrFontInfoParam, clrFontInfoData,
	clrFontCaret, clrFontBkm, clrBk, clrBkSel, clrBkDataInterp, clrBkInfoBar, clrBkCaret, clrBkCaretSel, clrBkBkm
};

BEGIN_MESSAGE_MAP(CDlgSettingsHexCtrl, CDialogEx)
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CDlgSettingsHexCtrl::OnPropertyDataChanged)
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
	SetPropValueDWORD(std::to_underlying(dwCapacity), refDefs.dwCapacity);
	SetPropValueDWORD(std::to_underlying(dwGroupSize), refDefs.dwGroupSize);
	SetPropValueDWORD(std::to_underlying(dwPageSize), refDefs.dwPageSize);
	SetPropValueDWORD(std::to_underlying(dwCharsExtraSpace), refDefs.dwCharsExtraSpace);
	SetPropValueFLOAT(std::to_underlying(flScrollRatio), refDefs.flScrollRatio);
	SetPropValueWCHAR(std::to_underlying(wchUnprintable), refDefs.wchUnprintable);
	SetPropOptValueByData(std::to_underlying(wchDateSepar), refDefs.wchDateSepar);
	SetPropOptValueByData(std::to_underlying(wstrDateFormat), refDefs.dwDateFormat);
	SetPropOptValueByData(std::to_underlying(fScrollLines), refDefs.fScrollLines);
	SetPropOptValueByData(std::to_underlying(fInfoBar), refDefs.fInfoBar);
	SetPropOptValueByData(std::to_underlying(fOffsetHex), refDefs.fOffsetHex);
	SetPropValueLOGFONT(std::to_underlying(stLogFont), refDefs.stLogFont);
	const auto& refClrs = refDefs.stClrs;
	SetPropValueRGB(std::to_underlying(clrFontHex), refClrs.clrFontHex);
	SetPropValueRGB(std::to_underlying(clrFontText), refClrs.clrFontText);
	SetPropValueRGB(std::to_underlying(clrFontSel), refClrs.clrFontSel);
	SetPropValueRGB(std::to_underlying(clrFontDataInterp), refClrs.clrFontDataInterp);
	SetPropValueRGB(std::to_underlying(clrFontCaption), refClrs.clrFontCaption);
	SetPropValueRGB(std::to_underlying(clrFontInfoParam), refClrs.clrFontInfoParam);
	SetPropValueRGB(std::to_underlying(clrFontInfoData), refClrs.clrFontInfoData);
	SetPropValueRGB(std::to_underlying(clrFontCaret), refClrs.clrFontCaret);
	SetPropValueRGB(std::to_underlying(clrFontBkm), refClrs.clrFontBkm);
	SetPropValueRGB(std::to_underlying(clrBk), refClrs.clrBk);
	SetPropValueRGB(std::to_underlying(clrBkSel), refClrs.clrBkSel);
	SetPropValueRGB(std::to_underlying(clrBkDataInterp), refClrs.clrBkDataInterp);
	SetPropValueRGB(std::to_underlying(clrBkInfoBar), refClrs.clrBkInfoBar);
	SetPropValueRGB(std::to_underlying(clrBkCaret), refClrs.clrBkCaret);
	SetPropValueRGB(std::to_underlying(clrBkCaretSel), refClrs.clrBkCaretSel);
	SetPropValueRGB(std::to_underlying(clrBkBkm), refClrs.clrBkBkm);
	m_grid.SetRedraw(TRUE);
	m_grid.RedrawWindow();
	m_fModified = true;
}

void CDlgSettingsHexCtrl::SaveSettings()
{
	if (!IsModified())
		return;

	using enum EName;
	auto& refSett = m_pAppSettings->GetHexCtrlSettings();
	refSett.stLogFont = *GetPropValuePLOGFONT(std::to_underlying(stLogFont));
	refSett.dwCapacity = GetPropValueDWORD(std::to_underlying(dwCapacity));
	refSett.dwGroupSize = GetPropValueDWORD(std::to_underlying(dwGroupSize));
	refSett.dwPageSize = GetPropValueDWORD(std::to_underlying(dwPageSize));
	refSett.dwCharsExtraSpace = GetPropValueDWORD(std::to_underlying(dwCharsExtraSpace));
	refSett.wchDateSepar = GetPropValueWCHAR(std::to_underlying(wchDateSepar));
	refSett.dwDateFormat = GetPropOptDataDWORD(std::to_underlying(wstrDateFormat));
	refSett.flScrollRatio = GetPropValueFLOAT(std::to_underlying(flScrollRatio));
	refSett.wchUnprintable = GetPropValueWCHAR(std::to_underlying(wchUnprintable));
	refSett.fScrollLines = GetPropOptDataDWORD(std::to_underlying(fScrollLines));
	refSett.fInfoBar = GetPropOptDataDWORD(std::to_underlying(fInfoBar));
	refSett.fOffsetHex = GetPropOptDataDWORD(std::to_underlying(fOffsetHex));
	auto& refClrs = refSett.stClrs;
	refClrs.clrFontHex = GetPropValueRGB(std::to_underlying(clrFontHex));
	refClrs.clrFontText = GetPropValueRGB(std::to_underlying(clrFontText));
	refClrs.clrFontSel = GetPropValueRGB(std::to_underlying(clrFontSel));
	refClrs.clrFontDataInterp = GetPropValueRGB(std::to_underlying(clrFontDataInterp));
	refClrs.clrFontCaption = GetPropValueRGB(std::to_underlying(clrFontCaption));
	refClrs.clrFontInfoParam = GetPropValueRGB(std::to_underlying(clrFontInfoParam));
	refClrs.clrFontInfoData = GetPropValueRGB(std::to_underlying(clrFontInfoData));
	refClrs.clrFontCaret = GetPropValueRGB(std::to_underlying(clrFontCaret));
	refClrs.clrFontBkm = GetPropValueRGB(std::to_underlying(clrFontBkm));
	refClrs.clrBk = GetPropValueRGB(std::to_underlying(clrBk));
	refClrs.clrBkSel = GetPropValueRGB(std::to_underlying(clrBkSel));
	refClrs.clrBkDataInterp = GetPropValueRGB(std::to_underlying(clrBkDataInterp));
	refClrs.clrBkInfoBar = GetPropValueRGB(std::to_underlying(clrBkInfoBar));
	refClrs.clrBkCaret = GetPropValueRGB(std::to_underlying(clrBkCaret));
	refClrs.clrBkCaretSel = GetPropValueRGB(std::to_underlying(clrBkCaretSel));
	refClrs.clrBkBkm = GetPropValueRGB(std::to_underlying(clrBkBkm));
	m_fModified = false;
}


//CDlgSettingsHexCtrl private methods.

void CDlgSettingsHexCtrl::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SETTINGSHEXCTRL_GRID, m_grid);
}

auto CDlgSettingsHexCtrl::GetGridVec()const->const std::vector<GRIDDATA>&
{
	return m_vecGrid;
}

bool CDlgSettingsHexCtrl::IsModified()const
{
	return m_fModified;
}

void CDlgSettingsHexCtrl::OnCancel()
{
	static_cast<CDialogEx*>(GetParentOwner())->EndDialog(IDCANCEL);
}

BOOL CDlgSettingsHexCtrl::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_grid.MarkModifiedProperties(TRUE, FALSE);
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
		static_cast<_variant_t>(refSett.dwCapacity), 0, 0, 0, 0, L"0123456789"), std::to_underlying(GROUP_GENERAL),
		std::to_underlying(dwCapacity));
	refCapac.pProp->EnableSpinControl(TRUE, 1, 100);
	refCapac.pProp->AllowEdit(TRUE);

	const auto& refGroup = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Group Size:",
		static_cast<_variant_t>(refSett.dwGroupSize), 0, 0, 0, 0, L"0123456789"), std::to_underlying(GROUP_GENERAL),
		std::to_underlying(dwGroupSize));
	refGroup.pProp->EnableSpinControl(TRUE, 1, 64);
	refGroup.pProp->AllowEdit(TRUE);

	const auto& refPageSize = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Page Size:",
		static_cast<_variant_t>(refSett.dwPageSize), 0, 0, 0, 0, L"0x123456789AaBbCcDdEeFfGg"), std::to_underlying(GROUP_GENERAL),
		std::to_underlying(dwPageSize));
	refPageSize.pProp->AllowEdit(TRUE);

	const auto& refUnprint = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Unprintable Char:",
		static_cast<_variant_t>(std::format(L"{}", refSett.wchUnprintable).data()), nullptr, 0, L"*", L"_"),
		std::to_underlying(GROUP_GENERAL), std::to_underlying(wchUnprintable));
	refUnprint.pProp->AllowEdit(TRUE);

	const auto& refDateSepar = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Date Separator:",
		static_cast<_variant_t>(std::format(L"{}", refSett.wchDateSepar).data()), nullptr, 0, L"*", L"_"),
		std::to_underlying(GROUP_GENERAL), std::to_underlying(wchDateSepar));
	const auto pPropDateSepar = static_cast<CHexerPropGridProp*>(refDateSepar.pProp);
	pPropDateSepar->AllowEdit(TRUE);

	const auto& refDate = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Date Format:", L""), std::to_underlying(GROUP_GENERAL),
		std::to_underlying(wstrDateFormat));
	const auto pPropDate = static_cast<CHexerPropGridProp*>(refDate.pProp);
	pPropDate->AddOptionEx(L"User default", 0xFFFFFFFFUL);
	pPropDate->AddOptionEx(std::format(L"MM{0}DD{0}YYYY", refSett.wchDateSepar).data(), 0UL);
	pPropDate->AddOptionEx(std::format(L"DD{0}MM{0}YYYY", refSett.wchDateSepar).data(), 1UL);
	pPropDate->AddOptionEx(std::format(L"YYYY{0}MM{0}DD", refSett.wchDateSepar).data(), 2UL);
	pPropDate->SetValueFromData(refSett.dwDateFormat);
	pPropDate->AllowEdit(FALSE);

	const auto& refScroll = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Scroll Lines or Ratio:", L""),
		std::to_underlying(GROUP_GENERAL), std::to_underlying(fScrollLines));
	const auto pPropScroll = static_cast<CHexerPropGridProp*>(refScroll.pProp);
	pPropScroll->AddOptionEx(L"Lines", 1UL);
	pPropScroll->AddOptionEx(L"Ratio", 0UL);
	pPropScroll->SetValueFromData(refSett.fScrollLines);
	pPropScroll->AllowEdit(FALSE);

	const auto& refScrollSize = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Scroll Size:",
		static_cast<_variant_t>(refSett.flScrollRatio), 0, 0, 0, 0, L"0123456789."), std::to_underlying(GROUP_GENERAL),
		std::to_underlying(flScrollRatio));
	refScrollSize.pProp->AllowEdit(TRUE);

	const auto& refInfoBar = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Show Info Bar:", L""), std::to_underlying(GROUP_GENERAL),
		std::to_underlying(fInfoBar));
	const auto pPropInfoBar = static_cast<CHexerPropGridProp*>(refInfoBar.pProp);
	pPropInfoBar->AddOptionEx(L"Show", 1UL);
	pPropInfoBar->AddOptionEx(L"Hide", 0UL);
	pPropInfoBar->SetValueFromData(refSett.fInfoBar);
	pPropInfoBar->AllowEdit(FALSE);

	const auto& refOffsetMode = m_vecGrid.emplace_back(new CHexerPropGridProp(L"Offset Mode:", L""), std::to_underlying(GROUP_GENERAL),
		std::to_underlying(fOffsetHex));
	const auto pPropOffsetMode = static_cast<CHexerPropGridProp*>(refOffsetMode.pProp);
	pPropOffsetMode->AddOptionEx(L"Hex", 1UL);
	pPropOffsetMode->AddOptionEx(L"Decimal", 0UL);
	pPropOffsetMode->SetValueFromData(refSett.fOffsetHex);
	pPropOffsetMode->AllowEdit(FALSE);

	const auto& refExtraSpace = m_vecGrid.emplace_back(new CMFCPropertyGridProperty(L"Chars Extra Space:",
		static_cast<_variant_t>(refSett.dwCharsExtraSpace), 0, 0, 0, 0, L"0123456789"), std::to_underlying(GROUP_GENERAL),
		std::to_underlying(dwCharsExtraSpace));
	refExtraSpace.pProp->EnableSpinControl(TRUE, 1, 10);
	refExtraSpace.pProp->AllowEdit(TRUE);

	m_vecGrid.emplace_back(new CMFCPropertyGridFontProperty(L"Font:", refSett.stLogFont,
		CF_EFFECTS | CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST | CF_INITTOLOGFONTSTRUCT | CF_NOSIMULATIONS | CF_NOSCRIPTSEL),
		std::to_underlying(GROUP_GENERAL), std::to_underlying(stLogFont));

	const auto pGeneral = new CMFCPropertyGridProperty(L"General:");
	for (const auto& it : m_vecGrid) {
		if (it.ui8Group == std::to_underlying(GROUP_GENERAL)) {
			pGeneral->AddSubItem(it.pProp);
		}
	}
	m_grid.AddProperty(pGeneral);

	//HexCtrl colors.
	const auto& refClrs = m_pAppSettings->GetHexCtrlSettings().stClrs;
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Hex", refClrs.clrFontHex),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrFontHex));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Text", refClrs.clrFontText),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrFontText));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Selection", refClrs.clrFontSel),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrFontSel));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Data Interpreter", refClrs.clrFontDataInterp),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrFontDataInterp));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Caption", refClrs.clrFontCaption),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrFontCaption));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Info Parameters", refClrs.clrFontInfoParam),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrFontInfoParam));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Info Data", refClrs.clrFontInfoData),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrFontInfoData));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Caret", refClrs.clrFontCaret),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrFontCaret));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Font Bookmarks", refClrs.clrFontBkm),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrFontBkm));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Background", refClrs.clrBk),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrBk));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Bk Selection", refClrs.clrBkSel),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrBkSel));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Bk Data Interpreter", refClrs.clrBkDataInterp),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrBkDataInterp));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Bk Info Bar", refClrs.clrBkInfoBar),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrBkInfoBar));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Bk Caret", refClrs.clrBkCaret),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrBkCaret));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Bk Caret in Selection", refClrs.clrBkCaretSel),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrBkCaretSel));
	m_vecGrid.emplace_back(new CMFCPropertyGridColorProperty(L"Bk Bookmarks", refClrs.clrBkBkm),
		std::to_underlying(GROUP_COLORS), std::to_underlying(clrBkBkm));

	const auto pColors = new CMFCPropertyGridProperty(L"Colors:");
	for (const auto& it : m_vecGrid) {
		if (it.ui8Group == std::to_underlying(GROUP_COLORS)) {
			pColors->AddSubItem(it.pProp);
			static_cast<CMFCPropertyGridColorProperty*>(it.pProp)->EnableOtherButton(L"Other");
		}
	}
	m_grid.AddProperty(pColors);

	return TRUE;
}

auto CDlgSettingsHexCtrl::OnPropertyDataChanged(WPARAM wParam, LPARAM lParam)->LRESULT
{
	m_fModified = true;
	GetParent()->SendMessageW(AFX_WM_PROPERTY_CHANGED, wParam, lParam);

	return TRUE;
}


//CDlgSettings.
export class CDlgSettings final : public CDialogEx {
public:
	CDlgSettings(CWnd* pParent = nullptr) : CDialogEx(IDD_SETTINGS, pParent) { }
	auto DoModal(CAppSettings& refSettings) -> INT_PTR;
private:
	enum class ETabs : std::uint8_t; //All the tabs.
	void DoDataExchange(CDataExchange* pDX)override;
	void OnApply();
	void OnCancel()override;
	afx_msg void OnDefaults();
	BOOL OnInitDialog()override;
	void OnOK()override;
	auto OnAnyPropertyDataChanged(WPARAM wParam, LPARAM lParam) -> LRESULT;
	afx_msg void OnTabSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	void SaveSettings();
	void SetCurrentTab(ETabs eTab);
	[[nodiscard]] auto TabIDToName(int iTab)const->ETabs;
	[[nodiscard]] auto TabNameToID(ETabs eTab)const->int;
	DECLARE_MESSAGE_MAP();
private:
	CTabCtrl m_tabMain;
	std::unique_ptr<CDlgSettingsGeneral> m_pDlgSettingsGeneral { std::make_unique<CDlgSettingsGeneral>() };
	std::unique_ptr<CDlgSettingsHexCtrl> m_pDlgSettingsHexCtrl { std::make_unique<CDlgSettingsHexCtrl>() };
	CAppSettings* m_pAppSettings { };
	bool m_fModified { false }; //Some setting has been modified.
};

enum class CDlgSettings::ETabs : std::uint8_t {
	TAB_GENERAL = 1, TAB_HEXCTRL
};

BEGIN_MESSAGE_MAP(CDlgSettings, CDialogEx)
	ON_BN_CLICKED(IDC_SETTINGS_APPLY, &CDlgSettings::OnApply)
	ON_BN_CLICKED(IDC_SETTINGS_DEFS, &CDlgSettings::OnDefaults)
	ON_NOTIFY(TCN_SELCHANGE, IDC_SETTINGS_TAB, &CDlgSettings::OnTabSelChanged)
	ON_REGISTERED_MESSAGE(AFX_WM_PROPERTY_CHANGED, &CDlgSettings::OnAnyPropertyDataChanged)
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

void CDlgSettings::OnApply()
{
	SaveSettings();
	GetDlgItem(IDC_SETTINGS_APPLY)->EnableWindow(FALSE);
}

void CDlgSettings::OnCancel()
{
	CDialogEx::OnCancel();
}

void CDlgSettings::OnDefaults()
{
	if (MessageBoxW(L"Reset all settings to their defaults?", L"Defaults", MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;

	m_pDlgSettingsGeneral->ResetToDefaults();
	m_pDlgSettingsHexCtrl->ResetToDefaults();
	GetDlgItem(IDC_SETTINGS_APPLY)->EnableWindow(TRUE);
	m_fModified = true;
}

BOOL CDlgSettings::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_tabMain.InsertItem(TCIF_TEXT | TCIF_PARAM, 0, L"General", 0, std::to_underlying(ETabs::TAB_GENERAL));
	m_tabMain.InsertItem(TCIF_TEXT | TCIF_PARAM, 1, L"HexCtrl", 0, std::to_underlying(ETabs::TAB_HEXCTRL));

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

	SetCurrentTab(ETabs::TAB_GENERAL); //Setting startup tab.

	return TRUE;
}

void CDlgSettings::OnOK()
{
	SaveSettings();
	CDialogEx::OnOK();
}

auto CDlgSettings::OnAnyPropertyDataChanged(WPARAM /*wParam*/, LPARAM /*lParam*/)->LRESULT
{
	GetDlgItem(IDC_SETTINGS_APPLY)->EnableWindow(TRUE);
	m_fModified = true;

	return TRUE;
}

void CDlgSettings::OnTabSelChanged(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	SetCurrentTab(TabIDToName(m_tabMain.GetCurSel()));
}

void CDlgSettings::SaveSettings()
{
	if (!m_fModified)
		return;

	m_pDlgSettingsGeneral->SaveSettings();
	m_pDlgSettingsHexCtrl->SaveSettings();
	m_pAppSettings->OnSettingsChanged();
	::SendMessageW(Ut::GetMainWnd(), Ut::WM_APP_SETTINGS_CHANGED, 0, 0);
	m_fModified = false;
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
		if (tci.lParam == std::to_underlying(eTab)) {
			return i;
		}
	}

	return -1;
}