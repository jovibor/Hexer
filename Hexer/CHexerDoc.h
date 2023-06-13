/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <cstddef>
#include <string>
#include "HexCtrl.h"
#include "CFileLoader.h"

class CHexerDoc final : public CDocument
{
public:
	[[nodiscard]] auto GetCacheSize()const->DWORD;
	[[nodiscard]] auto GetFileName()const->const std::wstring&;
	[[nodiscard]] auto GetFilePath()const->const std::wstring&;
	[[nodiscard]] auto GetFileSize()const->std::uint64_t;
	[[nodiscard]] auto GetFileData()const->std::byte*;
	[[nodiscard]] auto GetVirtualInterface() -> HEXCTRL::IHexVirtData*;
	[[nodiscard]] bool IsFileMutable()const;
	[[nodiscard]] bool IsOpenedVirtual()const;
private:
	void SetPathName(LPCTSTR lpszPathName, BOOL bAddToMRU = TRUE)override;
	BOOL OnNewDocument()override;
	BOOL OnOpenDocument(LPCTSTR lpszPathName)override;
	void OnCloseDocument()override;
	DECLARE_DYNCREATE(CHexerDoc);
	DECLARE_MESSAGE_MAP();
private:
	CFileLoader m_stFileLoader;
	std::wstring m_wstrFileName;
	std::wstring m_wstrFilePath;
};