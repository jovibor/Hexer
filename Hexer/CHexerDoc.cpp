/*******************************************************************************
* Copyright © 2023-2024 Jovibor https://github.com/jovibor/                    *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CHexerApp.h"
#include "CMainFrame.h"
#include "CChildFrame.h"
#include "CHexerDoc.h"
#include <cassert>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHexerDoc, CDocument)

BEGIN_MESSAGE_MAP(CHexerDoc, CDocument)
END_MESSAGE_MAP()

auto CHexerDoc::GetCacheSize()const->DWORD
{
	return m_stDataLoader.GetCacheSize();
}

auto CHexerDoc::GetFileMapData()const->std::byte*
{
	return m_stDataLoader.GetFileMapData();
}

auto CHexerDoc::GetFileName()const->const std::wstring&
{
	return m_wstrFileName;
}

auto CHexerDoc::GetDataPath()const->const std::wstring&
{
	return m_wstrDataPath;
}

auto CHexerDoc::GetDataSize()const->std::uint64_t
{
	return m_stDataLoader.GetDataSize();
}

auto CHexerDoc::GetMemPageSize()const->DWORD
{
	return m_stDataLoader.GetMemPageSize();
}

auto CHexerDoc::GetOpenMode()const->Ut::EOpenMode
{
	return m_eOpenMode;
}

auto CHexerDoc::GetProcID()const->DWORD
{
	return m_stDataLoader.GetProcID();
}

auto CHexerDoc::GetVirtualInterface()->HEXCTRL::IHexVirtData*
{
	return m_stDataLoader.GetVirtualInterface();
}

bool CHexerDoc::IsFileMutable()const
{
	return m_stDataLoader.IsMutable();
}

bool CHexerDoc::IsProcess()const
{
	return m_stDataLoader.IsProcess();
}

bool CHexerDoc::OnOpenDocument(const Ut::DATAOPEN& dos)
{
	m_eOpenMode = dos.eMode;
	m_wstrDataPath = dos.wstrDataPath;
	m_wstrFileName = m_wstrDataPath.substr(m_wstrDataPath.find_last_of(L'\\') + 1); //Doc name with the .extension.

	using enum Ut::EOpenMode;
	if (!m_stDataLoader.Open(dos)) {
		std::wstring wstrErr;
		if (IsProcess()) {
			wstrErr = std::format(L"{} open failed: {} (ID: {})",
				Ut::GetNameFromEOpenMode(GetOpenMode()), GetFileName(), dos.dwProcID);
		}
		else {
			wstrErr = std::wstring { Ut::GetNameFromEOpenMode(GetOpenMode()) } + L" open failed: " + GetFileName();
		}
		Ut::Log::AddLogEntryError(wstrErr);
		theApp.GetAppSettings().RFLRemoveFromList(dos);
		return false;
	}

	theApp.GetAppSettings().RFLAddToList(dos);
	theApp.GetAppSettings().LOLAddToList(dos);

	std::wstring_view wsvRW = IsFileMutable() ? L"(RW)" : L"(RO)";
	std::wstring wstrInfo;
	if (IsProcess()) {
		wstrInfo = std::format(L"{} opened: {} (ID: {}) {}",
			Ut::GetNameFromEOpenMode(GetOpenMode()), GetFileName(), GetProcID(), wsvRW);
	}
	else {
		wstrInfo = std::format(L"{} opened: {} {}",
			Ut::GetNameFromEOpenMode(GetOpenMode()), GetFileName(), wsvRW);
	}
	Ut::Log::AddLogEntryInfo(wstrInfo);
	m_strPathName = GetUniqueDocName(dos).data();
	m_bEmbedded = FALSE;
	SetTitle(GetDocTitle(dos).data());

	return true;
}


//Private methods.

BOOL CHexerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	return OnOpenDocument({ .wstrDataPath { lpszPathName }, .eMode { Ut::EOpenMode::OPEN_FILE } });
}

void CHexerDoc::OnCloseDocument()
{
	std::wstring wstrInfo;
	if (IsProcess()) {
		wstrInfo = std::format(L"{} closed: {} (ID: {})",
			Ut::GetNameFromEOpenMode(GetOpenMode()), GetFileName(), GetProcID());
	}
	else {
		(((wstrInfo += Ut::GetNameFromEOpenMode(GetOpenMode())) += L" closed: ") += GetFileName());
	}
	Ut::Log::AddLogEntryInfo(wstrInfo);

	if (!static_cast<CMainFrame*>(AfxGetMainWnd())->IsAppClosing()) {
		theApp.GetAppSettings().LOLRemoveFromList({ .wstrDataPath { GetDataPath() }, .dwProcID { GetProcID() } });
	}

	CDocument::OnCloseDocument();
}

auto CHexerDoc::GetUniqueDocName(const Ut::DATAOPEN& dos)->std::wstring
{
	if (dos.eMode == Ut::EOpenMode::OPEN_PROC) {
		return std::format(L"Process: {} (ID: {})", dos.wstrDataPath, dos.dwProcID);
	}
	else {
		return dos.wstrDataPath;
	}
}

auto CHexerDoc::GetDocTitle(const Ut::DATAOPEN& dos)->std::wstring
{
	using enum Ut::EOpenMode;
	if (dos.eMode == OPEN_PROC) {
		return GetUniqueDocName(dos);
	}

	const auto nName = dos.wstrDataPath.find_last_of(L'\\');
	assert(nName != std::wstring::npos);
	if (nName == std::wstring::npos) {
		return { };
	}

	switch (dos.eMode) {
	case OPEN_DEVICE:
		return std::format(L"{}: {}", Ut::GetNameFromEOpenMode(OPEN_DEVICE), dos.wstrDataPath.substr(nName + 1));
	default:
		return dos.wstrDataPath.substr(nName + 1);
	}
}