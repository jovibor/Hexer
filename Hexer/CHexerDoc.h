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
	[[nodiscard]] auto GetFileName()const->const std::wstring&;
	[[nodiscard]] auto GetDataPath()const->const std::wstring&;
	[[nodiscard]] auto GetDataSize()const->std::uint64_t;
	[[nodiscard]] auto GetDocIcon()const->HICON;
	[[nodiscard]] auto GetMaxVirtOffset()const->std::uint64_t;
	[[nodiscard]] auto GetMemPageSize()const->DWORD;
	[[nodiscard]] auto GetOpenMode()const->Ut::EOpenMode;
	[[nodiscard]] auto GetProcID()const->DWORD;
	[[nodiscard]] auto GetVecProcMemory()const->const std::vector<MEMORY_BASIC_INFORMATION>&;
	[[nodiscard]] auto GetVirtualInterface() -> HEXCTRL::IHexVirtData*;
	[[nodiscard]] bool IsFileMutable()const;
	[[nodiscard]] bool IsProcess()const;
	[[nodiscard]] bool OnOpenDocument(const Ut::DATAOPEN& dos);
	static auto GetUniqueDocName(const Ut::DATAOPEN& dos) -> std::wstring;
	static auto GetDocTitle(const Ut::DATAOPEN& dos) -> std::wstring;
private:
	[[nodiscard]] auto GetMainFrame()const->CMainFrame*;
	BOOL OnOpenDocument(LPCWSTR lpszPathName)override;
	void OnCloseDocument()override;
	DECLARE_DYNCREATE(CHexerDoc);
	DECLARE_MESSAGE_MAP();
private:
	CDataLoader m_stDataLoader;
	std::wstring m_wstrDataPath;
	std::wstring m_wstrFileName;
	Ut::EOpenMode m_eOpenMode { }; //Document open mode.
	HICON m_hDocIcon { };     //Document icon.
	bool m_fOpened { false }; //Document was successfully opened or not.
};