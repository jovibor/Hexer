/*******************************************************************************
* Copyright © 2023-present Jovibor https://github.com/jovibor/                 *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxcontrolbars.h>
#include "HexCtrl.h"

import Utility;
import AppSettings;

class CHexerApp final : public CWinAppEx {
public:
	void NotifyTabsOnSettingsChange();
	afx_msg void OnFileOpenFile();
	[[nodiscard]] auto GetAppSettings() -> CAppSettings&;
	[[nodiscard]] auto GetClassName()const->LPCWSTR;
	auto OpenDocumentCustom(LPCWSTR pwszPath, bool fDontLNK) -> CDocument*;
	auto OpenDocumentCustom(const ut::DATAOPEN& dos) -> CDocument*;
	auto OpenDocumentFile(LPCWSTR pwszPath) -> CDocument* override;
private:
	BOOL InitInstance()override;
	int ExitInstance()override;
	afx_msg void OnAppAbout();
	afx_msg void OnFileNewFile();
	afx_msg void OnFileOpenDevice();
	afx_msg void OnFileOpenProcess();
	afx_msg void OnToolsSettings();
	afx_msg void OnFileRFL(UINT uID);
	DECLARE_MESSAGE_MAP();
private:
	CAppSettings m_stAppSettings;
	bool m_fMainAppInSingleAppMode { false };
};

extern CHexerApp theApp;