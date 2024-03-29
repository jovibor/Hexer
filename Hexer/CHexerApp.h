/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include "HexCtrl.h"
#include <afxwin.h>
import Utility;
import AppSettings;

class CHexerApp final : public CWinAppEx {
public:
	afx_msg void OnFileOpen();
	[[nodiscard]] auto GetAppSettings() -> CAppSettings&;
	[[nodiscard]] auto GetClassName()const->LPCWSTR;
	auto OpenDocumentFile(Ut::FILEOPEN& fos) -> CDocument*;
private:
	BOOL InitInstance()override;
	int ExitInstance()override;
	afx_msg void OnAppAbout();
	afx_msg void OnFileNew();
	afx_msg void OnFileOpenDevice();
	afx_msg void OnToolsSettings();
	afx_msg void OnFileRFL(UINT uID);
	afx_msg void OnUpdateFileNew(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileRFL(CCmdUI* pCmdUI);
	afx_msg void OnUpdateToolsSettings(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP();
private:
	CAppSettings m_stAppSettings;
};

extern CHexerApp theApp;