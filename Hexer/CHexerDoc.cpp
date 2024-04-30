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
	m_wstrDataPath = ResolveLNK(dos);
	m_wstrFileName = m_wstrDataPath.substr(m_wstrDataPath.find_last_of(L'\\') + 1); //Doc name with the .extension.

	if (!m_stDataLoader.Open(dos)) {
		const auto wstrLogError = std::wstring { Ut::GetNameFromEOpenMode(GetOpenMode()) } + L" open failed: " + GetFileName();
		theApp.GetAppSettings().RFLRemoveFromList(GetDataPath());
		Ut::Log::AddLogEntryError(wstrLogError);
		return false;
	}

	if (dos.eMode != Ut::EOpenMode::OPEN_PROC) {
		theApp.GetAppSettings().AddToLastOpened(GetDataPath());
		theApp.GetAppSettings().RFLAddToList(GetDataPath());
	}

	const auto wstrLogInfo = std::wstring { Ut::GetNameFromEOpenMode(GetOpenMode()) } + L" opened: "
		+ GetFileName() + std::wstring { IsFileMutable() ? L" (RW)" : L" (RO)" };
	Ut::Log::AddLogEntryInfo(wstrLogInfo);
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
	const auto wstrLogInfo = std::wstring { Ut::GetNameFromEOpenMode(GetOpenMode()) } + L" closed: " + GetFileName();
	Ut::Log::AddLogEntryInfo(wstrLogInfo);
	if (!static_cast<CMainFrame*>(AfxGetMainWnd())->IsAppClosing()) {
		theApp.GetAppSettings().RemoveFromLastOpened(GetDataPath());
	}

	CDocument::OnCloseDocument();
}

auto CHexerDoc::GetUniqueDocName(const Ut::DATAOPEN& dos)->std::wstring
{
	if (dos.eMode == Ut::EOpenMode::OPEN_PROC) {
		return std::format(L"Process: {} (ID: {})", dos.wstrDataPath, dos.dwProcID);
	}
	else {
		return ResolveLNK(dos);
	}
}

auto CHexerDoc::GetDocTitle(const Ut::DATAOPEN& dos)->std::wstring
{
	if (dos.eMode == Ut::EOpenMode::OPEN_PROC) {
		return GetUniqueDocName(dos);
	}
	else {
		if (dos.fResolveLNK) {
			auto wstrPath = Ut::ResolveLNK(dos);
			return wstrPath.substr(wstrPath.find_last_of(L'\\') + 1);
		}

		return dos.wstrDataPath.substr(dos.wstrDataPath.find_last_of(L'\\') + 1);
	}
}