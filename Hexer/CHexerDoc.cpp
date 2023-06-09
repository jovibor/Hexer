/*******************************************************************************
* Copyright © 2023 Jovibor https://github.com/jovibor/                         *
* Hexer is a Hexadecimal Editor for Windows platform.                          *
* Official git repository: https://github.com/jovibor/Hexer/                   *
* This software is available under "The Hexer License", see the LICENSE file.  *
*******************************************************************************/
#include "stdafx.h"
#include "CHexerDoc.h"
#include "CHexerApp.h"
#include "CMainFrame.h"
#include "CChildFrame.h"
#include <format>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CHexerDoc, CDocument)

BEGIN_MESSAGE_MAP(CHexerDoc, CDocument)
END_MESSAGE_MAP()

auto CHexerDoc::GetFileName()const->const std::wstring&
{
	return m_wstrFileName;
}

auto CHexerDoc::GetFilePath()const->const std::wstring&
{
	return m_wstrFilePath;
}

auto CHexerDoc::GetFileSize()const->std::size_t
{
	return static_cast<std::size_t>(m_stFileSize.QuadPart);
}

auto CHexerDoc::GetFileData()const->std::byte*
{
	return static_cast<std::byte*>(m_lpBase);
}

bool CHexerDoc::GetFileRW()const
{
	return m_fWritable;
}

BOOL CHexerDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}

BOOL CHexerDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	if (!OpenFile(lpszPathName)) {
		return FALSE;
	}

	m_wstrFilePath = lpszPathName;
	m_wstrFileName = m_wstrFilePath.substr(m_wstrFilePath.find_last_of(L'\\') + 1); //Doc name with the .extension.

	return TRUE;
}

void CHexerDoc::OnCloseDocument()
{
	CloseFile();

	CDocument::OnCloseDocument();
}

bool CHexerDoc::OpenFile(LPCWSTR lpszFileName)
{
	m_hFile = CreateFileW(lpszFileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (m_hFile == INVALID_HANDLE_VALUE) {
		m_hFile = CreateFileW(lpszFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (m_hFile == INVALID_HANDLE_VALUE) {
			const auto dwError = GetLastError();
			wchar_t buffErr[MAX_PATH];
			FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dwError,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffErr, MAX_PATH, NULL);
			const auto wstrMsg = std::format(L"CreateFileW failed: 0x{:08X}\r\n{}", dwError, buffErr);
			::MessageBoxW(nullptr, wstrMsg.data(), L"Error", MB_ICONERROR);
			return false;
		}
	}
	else {
		m_fWritable = true;
	}

	GetFileSizeEx(m_hFile, &m_stFileSize);
	if (m_stFileSize.QuadPart == 0) { //Zero size.
		MessageBoxW(nullptr, L"File is zero size.", L"Error", MB_ICONERROR);
		return false;
	}

	m_hMapObject = CreateFileMappingW(m_hFile, nullptr, m_fWritable ? PAGE_READWRITE : PAGE_READONLY, 0, 0, nullptr);
	if (!m_hMapObject) {
		CloseHandle(m_hFile);
		MessageBoxW(nullptr, L"CreateFileMappingW failed.", L"Error", MB_ICONERROR);
		return false;
	}

	m_lpBase = MapViewOfFile(m_hMapObject, m_fWritable ? FILE_MAP_WRITE : FILE_MAP_READ, 0, 0, 0);

	return true;
}

void CHexerDoc::CloseFile()
{
	FlushViewOfFile(m_lpBase, 0);
	UnmapViewOfFile(m_lpBase);
	CloseHandle(m_hMapObject);
	CloseHandle(m_hFile);

	m_lpBase = nullptr;
	m_hMapObject = nullptr;
	m_hFile = nullptr;
}