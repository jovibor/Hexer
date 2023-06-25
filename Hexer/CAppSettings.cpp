/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CAppSettings.h"
#include "resource.h"
#include <format>

void CAppSettings::LoadSettings(std::wstring_view wsvKeyName)
{
	//Settings.
	const std::wstring wstrKeySettings = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName } + L"\\Settings";
	if (CRegKey regSettings; regSettings.Open(HKEY_CURRENT_USER, wstrKeySettings.data()) == ERROR_SUCCESS) {

		//SetShowPane.
		DWORD dwShowPaneFileProps { };
		regSettings.QueryDWORDValue(L"ShowPaneFileProps", dwShowPaneFileProps);
		SetShowPane(IDC_PANE_FILEPROPS, dwShowPaneFileProps > 0);

		//SetShowPane.
		DWORD dwShowPaneDataInterp { };
		regSettings.QueryDWORDValue(L"ShowPaneDataInterp", dwShowPaneDataInterp);
		SetShowPane(IDC_PANE_DATAINTERP, dwShowPaneDataInterp > 0);

		//SetShowPane.
		DWORD dwShowPaneTemplMgr { };
		regSettings.QueryDWORDValue(L"ShowPaneTemplMgr", dwShowPaneTemplMgr);
		SetShowPane(IDC_PANE_TEMPLMGR, dwShowPaneTemplMgr > 0);

		//SetPaneActive.
		DWORD dwIsPaneActiveFileProps { };
		regSettings.QueryDWORDValue(L"IsPaneActiveFileProps", dwIsPaneActiveFileProps);
		SetPaneActive(IDC_PANE_FILEPROPS, dwIsPaneActiveFileProps > 0);

		//SetPaneActive.
		DWORD dwIsPaneActiveDataInterp { };
		regSettings.QueryDWORDValue(L"IsPaneActiveDataInterp", dwIsPaneActiveDataInterp);
		SetPaneActive(IDC_PANE_DATAINTERP, dwIsPaneActiveDataInterp > 0);

		//SetPaneActive.
		DWORD dwIsPaneActiveTemplMgr { };
		regSettings.QueryDWORDValue(L"IsPaneActiveTemplMgr", dwIsPaneActiveTemplMgr);
		SetPaneActive(IDC_PANE_TEMPLMGR, dwIsPaneActiveTemplMgr > 0);

		//SetPaneData.
		QWORD ullPaneDataTemplMgr { };
		regSettings.QueryQWORDValue(L"PaneDataTemplMgr", ullPaneDataTemplMgr);
		SetPaneData(IDC_PANE_TEMPLMGR, ullPaneDataTemplMgr);

		//SetPaneData.
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

	//SetShowPane.
	regSettings.SetDWORDValue(L"ShowPaneFileProps", GetShowPane(IDC_PANE_FILEPROPS));
	regSettings.SetDWORDValue(L"ShowPaneDataInterp", GetShowPane(IDC_PANE_DATAINTERP));
	regSettings.SetDWORDValue(L"ShowPaneTemplMgr", GetShowPane(IDC_PANE_TEMPLMGR));

	//SetPaneActive.
	regSettings.SetDWORDValue(L"IsPaneActiveFileProps", GetPaneActive(IDC_PANE_FILEPROPS));
	regSettings.SetDWORDValue(L"IsPaneActiveDataInterp", GetPaneActive(IDC_PANE_DATAINTERP));
	regSettings.SetDWORDValue(L"IsPaneActiveTemplMgr", GetPaneActive(IDC_PANE_TEMPLMGR));

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

bool CAppSettings::GetPaneActive(UINT uPaneID)const
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		return m_fPaneActiveFileProps;
	case IDC_PANE_DATAINTERP:
		return m_fPaneActiveDataInterp;
	case IDC_PANE_TEMPLMGR:
		return m_fPaneActiveTemplMgr;
	default:
		return { };
	}
}

bool CAppSettings::GetShowPane(UINT uPaneID)const
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		return m_fShowPaneFileProps;
	case IDC_PANE_DATAINTERP:
		return m_fShowPaneDataInterp;
	case IDC_PANE_TEMPLMGR:
		return m_fShowPaneTemplMgr;
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

void CAppSettings::SetPaneActive(UINT uPaneID, bool fActive)
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		m_fPaneActiveFileProps = fActive;
	case IDC_PANE_DATAINTERP:
		m_fPaneActiveDataInterp = fActive;
	case IDC_PANE_TEMPLMGR:
		m_fPaneActiveTemplMgr = fActive;
	default:
		return;
	}
}

void CAppSettings::SetShowPane(UINT uPaneID, bool fShow)
{
	switch (uPaneID) {
	case IDC_PANE_FILEPROPS:
		m_fShowPaneFileProps = fShow;
		break;
	case IDC_PANE_DATAINTERP:
		m_fShowPaneDataInterp = fShow;
		break;
	case IDC_PANE_TEMPLMGR:
		m_fShowPaneTemplMgr = fShow;
		break;
	default:
		return;
	}
}