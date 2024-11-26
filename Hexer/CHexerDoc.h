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
	void ChangeDataAccessMode(Ut::EDataAccessMode eDataAccessMode);
	void ChangeDataIOMode(Ut::EDataIOMode eDataIOMode);
	[[nodiscard]] auto GetCacheSize()const->DWORD;
	[[nodiscard]] auto GetDataAccessMode()const->Ut::EDataAccessMode;
	[[nodiscard]] auto GetDataIOMode()const->Ut::EDataIOMode;
	[[nodiscard]] auto GetDataPath()const->const std::wstring&;
	[[nodiscard]] auto GetDataSize()const->std::uint64_t;
	[[nodiscard]] auto GetDocIcon()const->HICON;
	[[nodiscard]] auto GetFileMMAPData()const->std::byte*;
	[[nodiscard]] auto GetFileName()const->const std::wstring&;
	[[nodiscard]] auto GetMaxVirtOffset()const->std::uint64_t;
	[[nodiscard]] auto GetMemPageSize()const->DWORD;
	[[nodiscard]] auto GetOpenMode()const->Ut::EOpenMode;
	[[nodiscard]] auto GetProcID()const->DWORD;
	[[nodiscard]] auto GetVecProcMemory()const->const std::vector<MEMORY_BASIC_INFORMATION>&;
	[[nodiscard]] auto GetIHexVirtData() -> HEXCTRL::IHexVirtData*;
	[[nodiscard]] bool IsDataAccessRO();
	[[nodiscard]] bool IsDataAccessRW(); //Is data access currently set to RW or RO?
	[[nodiscard]] bool IsDataWritable()const; //Is data opened writable or read only?
	[[nodiscard]] bool IsDevice()const;
	[[nodiscard]] bool IsFile()const;
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
	HICON m_hDocIcon { };     //Document icon.
	bool m_fOpened { false }; //Document was successfully opened or not.
};