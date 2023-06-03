/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CAppSettings.h"

void CAppSettings::LoadSettings(std::wstring_view wsvKeyName)
{
	const std::wstring wstrKeyName = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName } + L"\\Settings";
	CRegKey regSettings;
	if (regSettings.Open(HKEY_CURRENT_USER, wstrKeyName.data()) != ERROR_SUCCESS)
		return;

	DWORD dwShowPaneFileProps { };
	regSettings.QueryDWORDValue(L"ShowPaneFileProps", dwShowPaneFileProps);
	SetShowPaneFileProps(dwShowPaneFileProps > 0);
}

void CAppSettings::SaveSettings(std::wstring_view wsvKeyName)
{
	CRegKey regSettings;
	const std::wstring wstrKeyName = std::wstring { L"SOFTWARE\\" } + std::wstring { wsvKeyName } + L"\\Settings";
	if (regSettings.Open(HKEY_CURRENT_USER, wstrKeyName.data()) != ERROR_SUCCESS) {
		regSettings.Create(HKEY_CURRENT_USER, wstrKeyName.data());
	}

	regSettings.SetDWORDValue(L"ShowPaneFileProps", GetShowPaneFileProps());
}

bool CAppSettings::GetShowPaneFileProps() const
{
	return m_fShowPaneFileProps;
}

void CAppSettings::SetShowPaneFileProps(bool fShow)
{
	m_fShowPaneFileProps = fShow;
}