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
import Utility;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHexerDoc, CDocument)

BEGIN_MESSAGE_MAP(CHexerDoc, CDocument)
END_MESSAGE_MAP()

auto CHexerDoc::GetCacheSize()const->DWORD
{
	return m_stFileLoader.GetCacheSize();
}

auto CHexerDoc::GetFileData()const->std::byte*
{
	return m_stFileLoader.GetFileData();
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
	return m_stFileLoader.GetFileSize();
}

auto CHexerDoc::GetVirtualInterface()->HEXCTRL::IHexVirtData*
{
	return m_stFileLoader.GetVirtualInterface();
}

bool CHexerDoc::IsFileMutable()const
{
	return m_stFileLoader.IsMutable();
}

bool CHexerDoc::OnOpenDocument(const Ut::FILEOPEN& fos)
{
	m_wstrFilePath = fos.wstrFilePath;
	m_wstrFileName = m_wstrFilePath.substr(m_wstrFilePath.find_last_of(L'\\') + 1); //Doc name with the .extension.

	if (!m_stFileLoader.OpenFile(fos)) {
		theApp.GetAppSettings().RFLRemoveFromList(m_wstrFilePath);
		Ut::Log::AddLogEntryError(L"File open failed: " + m_wstrFileName);
		return false;
	}

	theApp.GetAppSettings().AddToLastOpened(m_wstrFilePath);
	theApp.GetAppSettings().RFLAddToList(m_wstrFilePath);
	Ut::Log::AddLogEntryInfo(L"File opened: " + m_wstrFileName + std::wstring { IsFileMutable() ? L" (RW)" : L" (RO)" });

	return true;
}


//Private methods.

BOOL CHexerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	return OnOpenDocument(Ut::FILEOPEN { .wstrFilePath { lpszPathName }, .fNewFile { false } });
}

void CHexerDoc::OnCloseDocument()
{
	Ut::Log::AddLogEntryInfo(L"File closed: " + m_wstrFileName);
	if (!static_cast<CMainFrame*>(AfxGetMainWnd())->IsAppClosing()) {
		theApp.GetAppSettings().RemoveFromLastOpened(m_wstrFilePath);
	}

	CDocument::OnCloseDocument();
}

void CHexerDoc::SetPathName(LPCTSTR lpszPathName, BOOL /*bAddToMRU*/)
{
	//This code is copy-pasted from the original CDocument::SetPathName.
	//We need to override this method to remove calls to AfxFullPath and AfxGetFileTitle functions
	//from the original method, because these functions can't handle paths like "\\?\PhysicalDisk".
	//Also the AfxGetApp()->AddToRecentFileList call is removed since we manage the Recent File List manually.

	m_strPathName = lpszPathName;
	ASSERT(!m_strPathName.IsEmpty()); // must be set to something
	m_bEmbedded = FALSE;
	ASSERT_VALID(this);
	SetTitle(GetFileName().data());
	ASSERT_VALID(this);
}