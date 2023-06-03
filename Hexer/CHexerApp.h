/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include "CAppSettings.h"

class CHexerApp final : public CWinAppEx
{
public:
	void OnFileOpen();
	[[nodiscard]] auto GetAppSettings() -> CAppSettings&;
private:
	BOOL InitInstance()override;
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP();
private:
	CAppSettings m_stAppSettings;
public:
	virtual int ExitInstance();
};

extern CHexerApp theApp;