/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#pragma once
#include <cstddef>
#include <string>
#include "HexCtrl.h"
import DataLoader;
import Utility;

class CHexerDoc final : public CDocument {
public:
	[[nodiscard]] auto GetCacheSize()const->DWORD;
	[[nodiscard]] auto GetFileData()const->std::byte*;
	[[nodiscard]] auto GetFileName()const->const std::wstring&;
	[[nodiscard]] auto GetFilePath()const->const std::wstring&;
	[[nodiscard]] auto GetFileSize()const->std::uint64_t;
	[[nodiscard]] auto GetMemPageSize()const->DWORD;
	[[nodiscard]] auto GetVirtualInterface() -> HEXCTRL::IHexVirtData*;
	[[nodiscard]] bool IsFileMutable()const;
	[[nodiscard]] bool IsProcess()const;
	[[nodiscard]] bool OnOpenDocument(const Ut::FILEOPEN& fos);
	static auto GetUniqueDocName(const Ut::FILEOPEN& fos) -> std::wstring;
private:
	BOOL OnOpenDocument(LPCTSTR lpszPathName)override;
	void OnCloseDocument()override;
	DECLARE_DYNCREATE(CHexerDoc);
	DECLARE_MESSAGE_MAP();
private:
	CDataLoader m_stDataLoader;
	std::wstring m_wstrFilePath;
	std::wstring m_wstrFileName;
};