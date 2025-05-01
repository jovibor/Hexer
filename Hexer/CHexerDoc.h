/*******************************************************************************
* Copyright © 2023-present Jovibor https://github.com/jovibor/                 *
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
	void ChangeDataAccessMode(ut::DATAACCESS stDAC);
	void ChangeDataIOMode(ut::EDataIOMode eDataIOMode);
	[[nodiscard]] auto GetCacheSize()const->DWORD;
	[[nodiscard]] auto GetDataAccessMode()const->ut::DATAACCESS;
	[[nodiscard]] auto GetDataIOMode()const->ut::EDataIOMode;
	[[nodiscard]] auto GetDataPath()const->const std::wstring&;
	[[nodiscard]] auto GetDataSize()const->std::uint64_t;
	[[nodiscard]] auto GetDocIcon()const->HICON;
	[[nodiscard]] auto GetFileMMAPData()const->std::byte*;
	[[nodiscard]] auto GetFileName()const->const std::wstring&;
	[[nodiscard]] auto GetFriendlyName()const->const std::wstring&;
	[[nodiscard]] auto GetMaxVirtOffset()const->std::uint64_t;
	[[nodiscard]] auto GetMemPageSize()const->DWORD;
	[[nodiscard]] auto GetOpenMode()const->ut::EOpenMode;
	[[nodiscard]] auto GetProcID()const->DWORD;
	[[nodiscard]] auto GetVecProcMemory()const->const std::vector<MEMORY_BASIC_INFORMATION>&;
	[[nodiscard]] auto GetIHexVirtData() -> HEXCTRL::IHexVirtData*;
	[[nodiscard]] bool IsDataAccessRWINPLACE()const;
	[[nodiscard]] bool IsDataAccessRWSAFE()const;
	[[nodiscard]] bool IsDataAccessRO();
	[[nodiscard]] bool IsDataAccessRW(); //Is data access currently set to RW or RO?
	[[nodiscard]] bool IsDataOKForDASAFE()const;
	[[nodiscard]] bool IsDataWritable()const; //Is data opened writable or read only?
	[[nodiscard]] bool IsDevice()const;
	[[nodiscard]] bool IsFile()const;
	[[nodiscard]] bool IsProcess()const;
	[[nodiscard]] bool OnOpenDocument(const ut::DATAOPEN& dos);
	void SaveDataToDisk();
	static auto GetUniqueDocName(const ut::DATAOPEN& dos) -> std::wstring;
	static auto GetDocTitle(const ut::DATAOPEN& dos) -> std::wstring;
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
	std::wstring m_wstrFriendlyName;
	HICON m_hDocIcon { };     //Document icon.
	bool m_fOpened { false }; //Document was successfully opened or not.
};