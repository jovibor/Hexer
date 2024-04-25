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

auto CHexerDoc::GetFileData()const->std::byte*
{
	return m_stDataLoader.GetFileData();
}

auto CHexerDoc::GetFileName()const->const std::wstring&
{
	return m_wstrFileName;
}

auto CHexerDoc::GetFilePath()const->const std::wstring&
{
	return m_wstrFilePath;
}

auto CHexerDoc::GetFileSize()const->std::uint64_t
{
	return m_stDataLoader.GetDataSize();
}

auto CHexerDoc::GetMemPageSize()const->DWORD
{
	return m_stDataLoader.GetMemPageSize();
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

bool CHexerDoc::OnOpenDocument(const Ut::FILEOPEN& fos)
{
	m_wstrFilePath = fos.wstrFilePath;
	m_wstrFileName = m_wstrFilePath.substr(m_wstrFilePath.find_last_of(L'\\') + 1); //Doc name with the .extension.
	std::wstring wstrUniqueDocName = GetUniqueDocName(fos);
	std::wstring wstrTitle;

	if (fos.eMode == Ut::EOpenMode::OPEN_PROC) {
		if (!m_stDataLoader.Open(fos)) {
			Ut::Log::AddLogEntryError(L"Process open failed: " + GetFileName());
			return false;
		}

		wstrTitle = wstrUniqueDocName;
		Ut::Log::AddLogEntryInfo(L"Process opened: " + GetFileName() + std::wstring { IsFileMutable() ? L" (RW)" : L" (RO)" });
	}
	else {
		if (!m_stDataLoader.Open(fos)) {
			theApp.GetAppSettings().RFLRemoveFromList(GetFilePath());
			Ut::Log::AddLogEntryError(L"File open failed: " + GetFileName());
			return false;
		}

		wstrTitle = GetFileName();
		theApp.GetAppSettings().AddToLastOpened(GetFilePath());
		theApp.GetAppSettings().RFLAddToList(GetFilePath());
		Ut::Log::AddLogEntryInfo(L"File opened: " + GetFileName() + std::wstring { IsFileMutable() ? L" (RW)" : L" (RO)" });
	}

	m_strPathName = wstrUniqueDocName.data();
	m_bEmbedded = FALSE;
	SetTitle(wstrTitle.data());

	return true;
}


//Private methods.

BOOL CHexerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	return OnOpenDocument(Ut::FILEOPEN { .eMode { Ut::EOpenMode::OPEN_FILE }, .wstrFilePath { lpszPathName } });
}

void CHexerDoc::OnCloseDocument()
{
	Ut::Log::AddLogEntryInfo(L"File closed: " + GetFileName());
	if (!static_cast<CMainFrame*>(AfxGetMainWnd())->IsAppClosing()) {
		theApp.GetAppSettings().RemoveFromLastOpened(GetFilePath());
	}

	CDocument::OnCloseDocument();
}

auto CHexerDoc::GetUniqueDocName(const Ut::FILEOPEN& fos)->std::wstring
{
	if (fos.eMode == Ut::EOpenMode::OPEN_PROC) {
		return std::format(L"Process: {} (ID: {})", fos.wstrFilePath, fos.dwProcID);
	}
	else {
		return fos.wstrFilePath;
	}
}