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
	afx_msg void OnFileOpen();
	afx_msg void OnFileOpenDevice();
	[[nodiscard]] auto GetAppSettings() -> CAppSettings&;
private:
	BOOL InitInstance()override;
	int ExitInstance()override;
	afx_msg void OnAppAbout();
	afx_msg void OnUpdateFileNew(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP();
private:
	CAppSettings m_stAppSettings;
};

extern CHexerApp theApp;