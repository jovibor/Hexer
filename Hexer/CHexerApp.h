/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include "HexCtrl.h"
#include <afxwin.h>
#include <string>
#include <vector>
import Utility;
import AppSettings;

class CHexerApp final : public CWinAppEx {
public:
	afx_msg void OnFileOpen();
	[[nodiscard]] auto GetAppSettings() -> CAppSettings&;
	void AddToRFL(std::wstring_view wsvPath); //Add path to the Recent File List.
	void RemoveFromRFL(std::wstring_view wsvPath); //Remove path from the RFL if any.
private:
	BOOL InitInstance()override;
	int ExitInstance()override;
	auto OpenDocumentFile(const Ut::FILEOPEN& fos) -> CDocument*;
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