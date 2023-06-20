/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include "CAppSettings.h"
#include "CHexerRFL.h"
import Utility;

class CHexerApp final : public CWinAppEx
{
public:
	afx_msg void OnFileOpen();
	[[nodiscard]] auto GetAppSettings() -> CAppSettings&;
	void AddToRFL(std::wstring_view wsvPath); //Add path to Recent File List.
private:
	BOOL InitInstance()override;
	int ExitInstance()override;
	auto OpenDocumentFile(const Utility::FILEOPEN& fos) -> CDocument*;
	afx_msg void OnAppAbout();
	afx_msg void OnFileNew();
	afx_msg void OnFileOpenDevice();
	afx_msg void OnFileRFL(UINT uID);
	afx_msg void OnUpdateFileNew(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFileRFL(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP();
private:
	CAppSettings m_stAppSettings;
	CHexerRFL    m_stRFL;
};

extern CHexerApp theApp;