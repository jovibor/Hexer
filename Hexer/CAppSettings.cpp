/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CAppSettings.h"
#include <format>

void CAppSettings::LoadSettings(std::wstring_view wsvKeyName)
{
	//Settings.
	const std::wstring wstrKeySettings = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName } + L"\\Settings";
	if (CRegKey regSettings; regSettings.Open(HKEY_CURRENT_USER, wstrKeySettings.data()) == ERROR_SUCCESS) {
		//SetShowPaneFileProps.
		DWORD dwShowPaneFileProps { };
		regSettings.QueryDWORDValue(L"ShowPaneFileProps", dwShowPaneFileProps);
		SetShowPaneFileProps(dwShowPaneFileProps > 0);

		//SetShowPaneDataInterp.
		DWORD dwShowPaneDataInterp { };
		regSettings.QueryDWORDValue(L"ShowPaneDataInterp", dwShowPaneDataInterp);
		SetShowPaneDataInterp(dwShowPaneDataInterp > 0);

		//SetShowPaneTemplMgr.
		DWORD dwShowPaneTemplMgr { };
		regSettings.QueryDWORDValue(L"ShowPaneTemplMgr", dwShowPaneTemplMgr);
		SetShowPaneTemplMgr(dwShowPaneTemplMgr > 0);
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

	//SetShowPane*.
	regSettings.SetDWORDValue(L"ShowPaneFileProps", GetShowPaneFileProps());
	regSettings.SetDWORDValue(L"ShowPaneDataInterp", GetShowPaneDataInterp());
	regSettings.SetDWORDValue(L"ShowPaneTemplMgr", GetShowPaneTemplMgr());

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

bool CAppSettings::GetShowPaneFileProps()const
{
	return m_fShowPaneFileProps;
}

bool CAppSettings::GetShowPaneDataInterp()const
{
	return m_fShowPaneDataInterp;
}

bool CAppSettings::GetShowPaneTemplMgr()const
{
	return m_fShowPaneTemplMgr;
}

auto CAppSettings::GetRFL()->std::vector<std::wstring>&
{
	return m_vecRFL;
}

void CAppSettings::SetShowPaneFileProps(bool fShow)
{
	m_fShowPaneFileProps = fShow;
}

void CAppSettings::SetShowPaneDataInterp(bool fShow)
{
	m_fShowPaneDataInterp = fShow;
}

void CAppSettings::SetShowPaneTemplMgr(bool fShow)
{
	m_fShowPaneTemplMgr = fShow;
}