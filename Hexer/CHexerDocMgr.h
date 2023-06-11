/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <afxwin.h>

class CHexerDocMgr final : public CDocManager
{
private:
	auto OpenDocumentFile(LPCTSTR lpszFileName, BOOL bAddToMRU) -> CDocument* override;
	DECLARE_DYNCREATE(CHexerDocMgr);
};