/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CAppSettings.h"
#include "resource.h"
#include <bitset>
#include <format>

void CAppSettings::LoadSettings(std::wstring_view wsvKeyName)
{
	//Settings.
	const std::wstring wstrKeySettings = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName } + L"\\Settings";
	if (CRegKey regSettings; regSettings.Open(HKEY_CURRENT_USER, wstrKeySettings.data()) == ERROR_SUCCESS) {

		//PaneStatus.
		DWORD dwPaneStatusFileProps { };
		regSettings.QueryDWORDValue(L"PaneStatusFileProps", dwPaneStatusFileProps);
		m_stPSFileProps = DWORD2PaneStatus(dwPaneStatusFileProps);
		DWORD dwPaneStatusDataInterp { };
		regSettings.QueryDWORDValue(L"PaneStatusDataInterp", dwPaneStatusDataInterp);
		m_stPSDataInterp = DWORD2PaneStatus(dwPaneStatusDataInterp);
		DWORD dwPaneStatusTemplMgr { };
		regSettings.QueryDWORDValue(L"PaneStatusTemplMgr", dwPaneStatusTemplMgr);
		m_stPSTemplMgr = DWORD2PaneStatus(dwPaneStatusTemplMgr);

		//PaneData.
		QWORD ullPaneDataTemplMgr { };
		regSettings.QueryQWORDValue(L"PaneDataTemplMgr", ullPaneDataTemplMgr);
		SetPaneData(IDC_PANE_TEMPLMGR, ullPaneDataTemplMgr);
		QWORD ullPaneDataDataInterp { };
		regSettings.QueryQWORDValue(L"PaneDataDataInterp", ullPaneDataDataInterp);
		SetPaneData(IDC_PANE_DATAINTERP, ullPaneDataDataInterp);
	}

	//Recent File List.
	const std::wstring wstrKeyRFL = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName } + L"\\Recent File List";
	if (CRegKey regRFL; regRFL.Open(HKEY_CURRENT_USER, wstrKeyRFL.data()) == ERROR_SUCCESS) {
		int iCode { };
		wchar_t buffName[32];
		wchar_t buffData[MAX_PATH];
		DWORD dwIndex = 0;
		auto& vecRFL = GetRFL();
		vecRFL.clear();
		while (iCode != ERROR_NO_MORE_ITEMS) {
			DWORD dwNameSize { 32 };
			DWORD dwDataType { };
			DWORD dwDataSize { MAX_PATH * sizeof(wchar_t) };
			iCode = RegEnumValueW(regRFL.m_hKey, dwIndex, buffName, &dwNameSize, nullptr, &dwDataType,
				reinterpret_cast<LPBYTE>(&buffData), &dwDataSize);
			if (iCode == ERROR_SUCCESS && dwDataType == REG_SZ) {
				vecRFL.emplace_back(buffData);
			}
			++dwIndex;
		}
	}
}

void CAppSettings::SaveSettings(std::wstring_view wsvKeyName)
{
	const std::wstring wstrAppKey = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName };

	//Settings.
	CRegKey regSettings;
	const std::wstring wstrKeySettings = wstrAppKey + L"\\Settings";
	if (regSettings.Open(HKEY_CURRENT_USER, wstrKeySettings.data()) != ERROR_SUCCESS) {
		regSettings.Create(HKEY_CURRENT_USER, wstrKeySettings.data());
	}

	//PaneStatus.
	regSettings.SetDWORDValue(L"PaneStatusFileProps", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_FILEPROPS)));
	regSettings.SetDWORDValue(L"PaneStatusDataInterp", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_DATAINTERP)));
	regSettings.SetDWORDValue(L"PaneStatusTemplMgr", PaneStatus2DWORD(GetPaneStatus(IDC_PANE_TEMPLMGR)));

	//SetPaneData
	regSettings.SetQWORDValue(L"PaneDataDataInterp", GetPaneData(IDC_PANE_DATAINTERP));
	regSettings.SetQWORDValue(L"PaneDataTemplMgr", GetPaneData(IDC_PANE_TEMPLMGR));

	//Recent File List.
	CRegKey regRFL;
	regRFL.Open(HKEY_CURRENT_USER, wstrAppKey.data());
	regRFL.RecurseDeleteKey(L"Recent File List"); //Remove all data to set it below.
	const std::wstring wstrKeyRFL = wstrAppKey + L"\\Recent File List";
	regRFL.Create(HKEY_CURRENT_USER, wstrKeyRFL.data());
	int iIndex { 1 };
	for (const auto& wstr : GetRFL()) {
		regRFL.SetStringValue(std::format(L"File{:02d}", iIndex++).data(), wstr.data());
	}
}

auto CAppSettings::GetPaneData(UINT uPaneID)const->std::uint64_t
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		return m_ullPaneDataFileProps;
	case IDC_PANE_DATAINTERP:
		return m_ullPaneDataDataInterp;
	case IDC_PANE_TEMPLMGR:
		return m_ullPaneDataTemplMgr;
	default:
		return { };
	}
}

auto CAppSettings::GetPaneStatus(UINT uPaneID)const->PANESTATUS
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		return m_stPSFileProps;
	case IDC_PANE_DATAINTERP:
		return m_stPSDataInterp;
	case IDC_PANE_TEMPLMGR:
		return m_stPSTemplMgr;
	default:
		return { };
	}
}

auto CAppSettings::GetRFL()->std::vector<std::wstring>&
{
	return m_vecRFL;
}

void CAppSettings::SetPaneData(UINT uPaneID, std::uint64_t ullData)
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		m_ullPaneDataFileProps = ullData;
		break;
	case IDC_PANE_DATAINTERP:
		m_ullPaneDataDataInterp = ullData;
		break;
	case IDC_PANE_TEMPLMGR:
		m_ullPaneDataTemplMgr = ullData;
		break;
	default:
		return;
	}
}

void CAppSettings::SetPaneStatus(UINT uPaneID, bool fShow, bool fActive)
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		m_stPSFileProps = { fShow, fActive };
	case IDC_PANE_DATAINTERP:
		m_stPSDataInterp = { fShow, fActive };
	case IDC_PANE_TEMPLMGR:
		m_stPSTemplMgr = { fShow, fActive };
	default:
		return;
	}
}

constexpr auto CAppSettings::PaneStatus2DWORD(PANESTATUS ps)->DWORD
{
	std::bitset<32> dw;
	dw[0] = ps.fIsVisible;
	dw[1] = ps.fIsActive;

	return dw.to_ulong();
}

constexpr auto CAppSettings::DWORD2PaneStatus(DWORD dw)->PANESTATUS
{
	std::bitset<32> dwPS(dw);

	return { dwPS[0] == true, dwPS[1] == true };
}